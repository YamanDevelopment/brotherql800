#pragma once
#include <cstdio>
#include <vector>
#include <string>
#include <libusb-1.0/libusb.h>

// Open the printer USB port
libusb_device_handle* open_printer_port();

// Close the printer USB port
void close_printer_port(libusb_device_handle* handle);

// Send raw data to the printer
bool send_command(libusb_device_handle* handle, const std::vector<uint8_t>& data);

// Get printer status
std::vector<uint8_t> get_printer_status(libusb_device_handle* handle);

// Send initialization sequence
bool send_initialization_sequence(libusb_device_handle* handle);
