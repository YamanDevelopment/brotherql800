#include "image.hh"
#include "port.hh"
#include <iostream>

// Include stb_image.h with implementation
#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb_image.h"

// Load an image and convert it to a 1-bit bitmap
std::vector<std::vector<uint8_t>> load_image_as_bitmap(const std::string& filepath, int width, int height) {
    std::vector<std::vector<uint8_t>> bitmap;

    int img_width, img_height, channels;
    unsigned char* img = stbi_load(filepath.c_str(), &img_width, &img_height, &channels, 1);

    if (!img) {
        std::cerr << "Failed to load image: " << filepath << std::endl;
        return bitmap;
    }

    std::cout << "Loaded image: " << filepath << " (" << img_width << "x" << img_height
              << ", " << channels << " channel(s))" << std::endl;

    // Resize or scale the image to the desired dimensions
    // For simplicity, we're just going to use the original size for now
    // In a real implementation, you'd want to resize/crop as needed

    // Convert to 1-bit bitmap (90 bytes per line = 720 pixels)
    // Each bit represents one pixel, 0 = white, 1 = black
    int bytes_per_line = (width + 7) / 8;  // Round up to nearest byte

    for (int y = 0; y < height && y < img_height; y++) {
        std::vector<uint8_t> line(bytes_per_line, 0);

        for (int x = 0; x < width && x < img_width; x++) {
            // Get pixel value (0-255)
            unsigned char pixel = img[y * img_width + x];

            // Convert to binary (threshold at 128)
            if (pixel < 128) {
                // Set the corresponding bit (black pixel)
                int byte_idx = x / 8;
                int bit_idx = 7 - (x % 8);  // MSB first
                line[byte_idx] |= (1 << bit_idx);
            }
        }

        bitmap.push_back(line);
    }

    stbi_image_free(img);
    std::cout << "Converted to " << bitmap.size() << " lines of bitmap data" << std::endl;

    return bitmap;
}

// Generate raster data from bitmap
std::vector<std::vector<uint8_t>> generate_raster_data(const std::vector<std::vector<uint8_t>>& bitmap) {
    std::vector<std::vector<uint8_t>> raster_lines;

    for (const auto& line : bitmap) {
        // Each raster line starts with the 'g' command (0x67) followed by width bytes
        std::vector<uint8_t> raster_line;
        raster_line.push_back(0x67);  // 'g' command
        raster_line.push_back(0x00);  // m = 0 (no compression)
        raster_line.push_back(static_cast<uint8_t>(line.size()));  // n = width in bytes

        // Add the bitmap data
        raster_line.insert(raster_line.end(), line.begin(), line.end());

        raster_lines.push_back(raster_line);
    }

    return raster_lines;
}

// Send raster image to printer
bool send_raster_image(libusb_device_handle* handle, const std::vector<std::vector<uint8_t>>& raster_lines) {
    if (!handle) return false;

    for (const auto& line : raster_lines) {
        if (!send_command(handle, line)) {
            std::cerr << "Failed to send raster line" << std::endl;
            return false;
        }
    }

    // Send final print command (0x1A - Control-Z)
    std::vector<uint8_t> print_cmd = {0x1A};
    if (!send_command(handle, print_cmd)) {
        std::cerr << "Failed to send print command" << std::endl;
        return false;
    }

    std::cout << "Sent " << raster_lines.size() << " raster lines to printer" << std::endl;
    return true;
}

// High-level function to print a label from an image file
bool print_label(libusb_device_handle* handle, const std::string& filepath, int copies) {
    if (!handle) return false;

    // Initialize the printer
    if (!send_initialization_sequence(handle)) {
        std::cerr << "Failed to initialize printer" << std::endl;
        return false;
    }

    // Load and convert the image (assuming 720 pixels wide, adjust height as needed)
    int width = 720;  // 90 bytes * 8 bits
    int height = 300;  // Adjust based on your label size
    auto bitmap = load_image_as_bitmap(filepath, width, height);
    if (bitmap.empty()) {
        std::cerr << "Failed to convert image to bitmap" << std::endl;
        return false;
    }

    // Generate raster data
    auto raster_lines = generate_raster_data(bitmap);

    // Print the image
    for (int i = 0; i < copies; i++) {
        if (i > 0) {
            // Re-initialize for each copy after the first
            if (!send_initialization_sequence(handle)) {
                std::cerr << "Failed to initialize printer for copy " << i+1 << std::endl;
                return false;
            }
        }

        std::cout << "Printing copy " << i+1 << " of " << copies << std::endl;
        if (!send_raster_image(handle, raster_lines)) {
            std::cerr << "Failed to print copy " << i+1 << std::endl;
            return false;
        }
    }

    return true;
}
