#include "port.hh"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Brother QL-800 Basic Test" << std::endl;

    // Open the printer
    libusb_device_handle* handle = open_printer_port();
    if (!handle) {
        std::cerr << "Failed to open printer" << std::endl;
        return 1;
    }

    // Try to send some basic commands

    // 1. Initialize printer (ESC @)
    std::vector<uint8_t> init = {0x1B, 0x40};
    if (!send_command(handle, init)) {
        close_printer_port(handle);
        return 1;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 2. Switch to raster mode (ESC i a 01)
    std::vector<uint8_t> raster_mode = {0x1B, 0x69, 0x61, 0x01};
    if (!send_command(handle, raster_mode)) {
        close_printer_port(handle);
        return 1;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 3. Set media type - Try a few different common ones
    // Standard die-cut labels
    std::vector<uint8_t> media_info = {
        0x1B, 0x69, 0x7A,  // ESC i z
        0x0A,              // Media type: Die-cut label
        0x3E,              // Width: 62mm
        0x00,              // Reserved
        0xD0, 0x01,        // Length: 464 dots (58mm height)
        0x01,              // Number of copies
        0x00,              // Reserved
        0x40,              // Cut setting: auto cut
        0x00               // Compression mode: none
    };

    if (!send_command(handle, media_info)) {
        close_printer_port(handle);
        return 1;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. Create a simple test pattern (black rectangle)
    const int width_bytes = 90;  // 720 pixels (90 bytes * 8 bits)
    const int height = 50;      // 50 lines tall

    // Create a test pattern - a black line at the top, middle, and bottom
    for (int y = 0; y < height; y++) {
        std::vector<uint8_t> raster_line;
        raster_line.push_back(0x67);  // 'g' command
        raster_line.push_back(0x00);  // m = 0 (no compression)
        raster_line.push_back(width_bytes);  // n = width in bytes

        // Create the bitmap data
        std::vector<uint8_t> bitmap(width_bytes, 0);

        // Make top 5 lines a solid black bar
        if (y < 5) {
            std::fill(bitmap.begin(), bitmap.end(), 0xFF);
        }
        // Make middle 5 lines a solid black bar
        else if (y >= height/2 - 2 && y <= height/2 + 2) {
            std::fill(bitmap.begin(), bitmap.end(), 0xFF);
        }
        // Make bottom 5 lines a solid black bar
        else if (y >= height - 5) {
            std::fill(bitmap.begin(), bitmap.end(), 0xFF);
        }
        // For other lines, make left and right edges black
        else {
            bitmap[0] = 0xFF;
            bitmap[width_bytes-1] = 0xFF;
        }

        // Add the bitmap data to the raster line
        raster_line.insert(raster_line.end(), bitmap.begin(), bitmap.end());

        // Send the raster line
        if (!send_command(handle, raster_line)) {
            std::cerr << "Failed to send raster line " << y << std::endl;
            close_printer_port(handle);
            return 1;
        }

        // Wait a tiny bit between lines
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 5. Send print command
    std::vector<uint8_t> print_cmd1 = {0x1A};  // Form feed
    if (!send_command(handle, print_cmd1)) {
        close_printer_port(handle);
        return 1;
    }

    // Wait for printing to start
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Try another form feed
    std::vector<uint8_t> print_cmd2 = {0x0C};  // Form feed
    if (!send_command(handle, print_cmd2)) {
        close_printer_port(handle);
        return 1;
    }

    // Wait a bit longer
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // 6. Close the printer
    close_printer_port(handle);

    std::cout << "Test pattern sent to printer." << std::endl;
    std::cout << "If no printing occurred, check printer status and try:" << std::endl;
    std::cout << "1. Ensure media is properly loaded" << std::endl;
    std::cout << "2. Press feed button on printer" << std::endl;
    std::cout << "3. Reset the printer by turning it off and on" << std::endl;

    return 0;
}
