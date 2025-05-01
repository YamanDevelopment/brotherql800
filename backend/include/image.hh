#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <libusb.h>

// Load an image and convert it to a 1-bit bitmap
std::vector<std::vector<uint8_t>> load_image_as_bitmap(const std::string& filepath, int width, int height);

// Generate raster data from bitmap
std::vector<std::vector<uint8_t>> generate_raster_data(const std::vector<std::vector<uint8_t>>& bitmap);

// Send raster image to printer
bool send_raster_image(libusb_device_handle* handle, const std::vector<std::vector<uint8_t>>& raster_lines);

// High-level function to print a label from an image file
bool print_label(libusb_device_handle* handle, const std::string& filepath, int copies = 1);
