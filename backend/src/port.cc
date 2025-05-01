#include "port.hh"
#include <iostream>

void print_device_info(libusb_device_handle* handle) {
    if (!handle) return;

    libusb_device* dev = libusb_get_device(handle);
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);

    std::cout << "Device information:" << std::endl;
    std::cout << "  VID:PID = " << std::hex << desc.idVendor << ":" << desc.idProduct << std::dec << std::endl;
    std::cout << "  Class:SubClass:Protocol = " <<
        static_cast<int>(desc.bDeviceClass) << ":" <<
        static_cast<int>(desc.bDeviceSubClass) << ":" <<
        static_cast<int>(desc.bDeviceProtocol) << std::endl;

    // Get configuration
    struct libusb_config_descriptor* config;
    libusb_get_active_config_descriptor(dev, &config);

    std::cout << "  Number of interfaces: " << static_cast<int>(config->bNumInterfaces) << std::endl;

    // Print interface and endpoint information
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface* interface = &config->interface[i];

        for (int j = 0; j < interface->num_altsetting; j++) {
            const struct libusb_interface_descriptor* iface_desc = &interface->altsetting[j];

            std::cout << "  Interface " << i << ", Alt Setting " << j << ":" << std::endl;
            std::cout << "    Number of endpoints: " << static_cast<int>(iface_desc->bNumEndpoints) << std::endl;

            for (int k = 0; k < iface_desc->bNumEndpoints; k++) {
                const struct libusb_endpoint_descriptor* ep_desc = &iface_desc->endpoint[k];

                std::cout << "      Endpoint " << k << ":" << std::endl;
                std::cout << "        Address: 0x" << std::hex << static_cast<int>(ep_desc->bEndpointAddress) << std::dec << std::endl;
                std::cout << "        Type: " << (ep_desc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) << std::endl;
                std::cout << "        Max packet size: " << ep_desc->wMaxPacketSize << std::endl;
                std::cout << "        Interval: " << static_cast<int>(ep_desc->bInterval) << std::endl;
            }
        }
    }

    libusb_free_config_descriptor(config);
}

// Open the printer USB port
libusb_device_handle* open_printer_port() {
    libusb_context* ctx = nullptr;
    int r = libusb_init(&ctx);
    if (r < 0) {
        std::cerr << "Failed to initialize libusb: " << libusb_error_name(r) << std::endl;
        return nullptr;
    }

    const uint16_t VENDOR_ID = 0x04F9;   // Brother Industries
    const uint16_t PRODUCT_ID = 0x209B;  // QL-800

    std::cout << "Looking for Brother QL-800 (VID:PID = " << std::hex << VENDOR_ID
              << ":" << PRODUCT_ID << std::dec << ")" << std::endl;

    libusb_device_handle* handle = libusb_open_device_with_vid_pid(ctx, VENDOR_ID, PRODUCT_ID);
    if (!handle) {
        std::cerr << "Could not find/open Brother QL-800" << std::endl;
        return nullptr;
    }

    std::cout << "Found Brother QL-800" << std::endl;
    print_device_info(handle);

    // Detach kernel driver if necessary
    if (libusb_kernel_driver_active(handle, 0) == 1) {
        std::cout << "Detaching kernel driver" << std::endl;
        r = libusb_detach_kernel_driver(handle, 0);
        if (r != 0) {
            std::cerr << "Failed to detach kernel driver: " << libusb_error_name(r) << std::endl;
        }
    }

    r = libusb_claim_interface(handle, 0);
    if (r != 0) {
        std::cerr << "Failed to claim interface: " << libusb_error_name(r) << std::endl;
        libusb_close(handle);
        return nullptr;
    }

    std::cout << "Successfully claimed interface" << std::endl;
    return handle;
}

// Close the printer USB port
void close_printer_port(libusb_device_handle* handle) {
    if (handle) {
        libusb_release_interface(handle, 0);
        libusb_close(handle);
        libusb_exit(nullptr);  // Clean up the libusb context
        std::cout << "Printer connection closed" << std::endl;
    }
}

// Send raw data to the printer
bool send_command(libusb_device_handle* handle, const std::vector<uint8_t>& data) {
    if (!handle) {
        std::cerr << "Invalid handle" << std::endl;
        return false;
    }

    int actual_length = 0;
    // Use endpoint 0x02 for output based on device info
    unsigned char endpoint = 0x02;  // Changed from 0x01 to 0x02

    // Increase timeout to 10 seconds
    int timeout = 10000;

    std::cout << "Sending " << data.size() << " bytes to endpoint 0x" << std::hex << static_cast<int>(endpoint) << std::dec << std::endl;

    // Debug: print first few bytes
    if (!data.empty()) {
        std::cout << "Data starts with: ";
        for (size_t i = 0; i < std::min(data.size(), size_t(10)); i++) {
            std::cout << std::hex << static_cast<int>(data[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    }

    int r = libusb_bulk_transfer(handle, endpoint, const_cast<unsigned char*>(data.data()),
                                data.size(), &actual_length, timeout);

    if (r != 0) {
        std::cerr << "Failed to send data: " << libusb_error_name(r);
        switch (r) {
            case LIBUSB_ERROR_TIMEOUT:
                std::cerr << " (timeout)";
                break;
            case LIBUSB_ERROR_PIPE:
                std::cerr << " (endpoint halted)";
                break;
            case LIBUSB_ERROR_NO_DEVICE:
                std::cerr << " (device disconnected)";
                break;
        }
        std::cerr << " (sent " << actual_length << " of " << data.size() << " bytes)" << std::endl;
        return false;
    }

    if (static_cast<size_t>(actual_length) != data.size()) {
        std::cerr << "Warning: Only sent " << actual_length << " of " << data.size() << " bytes" << std::endl;
    } else {
        std::cout << "Successfully sent " << actual_length << " bytes" << std::endl;
    }

    return true;
}

// Get printer status
std::vector<uint8_t> get_printer_status(libusb_device_handle* handle) {
    std::vector<uint8_t> result;
    if (!handle) {
        std::cerr << "Invalid handle" << std::endl;
        return result;
    }

    // Send status request command (ESC i S)
    std::vector<uint8_t> status_cmd = {0x1B, 0x69, 0x53};
    if (!send_command(handle, status_cmd)) {
        std::cerr << "Failed to send status request" << std::endl;
        return result;
    }

    // Use endpoint 0x81 for input based on device info
    unsigned char endpoint = 0x81;  // Changed from 0x82 to 0x81
    int timeout = 10000;  // 10 seconds

    std::cout << "Reading status data from endpoint 0x" << std::hex << static_cast<int>(endpoint) << std::dec << std::endl;

    // Read the 32-byte response
    result.resize(32);
    int actual_length;
    int r = libusb_bulk_transfer(handle, endpoint, result.data(), result.size(), &actual_length, timeout);

    if (r != 0) {
        std::cerr << "Failed to read printer status: " << libusb_error_name(r);
        switch (r) {
            case LIBUSB_ERROR_TIMEOUT:
                std::cerr << " (timeout)";
                break;
            case LIBUSB_ERROR_PIPE:
                std::cerr << " (endpoint halted)";
                break;
            case LIBUSB_ERROR_NO_DEVICE:
                std::cerr << " (device disconnected)";
                break;
        }
        std::cerr << std::endl;
        result.clear();
        return result;
    }

    result.resize(actual_length);  // Resize to actual data received
    std::cout << "Read " << actual_length << " bytes of status data" << std::endl;

    // Debug: print status bytes
    std::cout << "Status bytes: ";
    for (auto byte : result) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;

    return result;
}

// Send initialization sequence
bool send_initialization_sequence(libusb_device_handle* handle) {
    if (!handle) return false;

    // 1. Initialize printer (ESC @)
    std::vector<uint8_t> init = {0x1B, 0x40};
    if (!send_command(handle, init)) return false;

    // 2. Switch to raster mode (ESC i a 01)
    std::vector<uint8_t> raster_mode = {0x1B, 0x69, 0x61, 0x01};
    if (!send_command(handle, raster_mode)) return false;

    // 3. Set print information
    // ESC i z - print info command for 62mm continuous label
    std::vector<uint8_t> print_info = {
        0x1B, 0x69, 0x7A,  // ESC i z
        0x84,              // Media type: continuous and length specified
        0x26,              // Width: 62mm (0x26 = 38 * 2)
        0x00,              // Reserved
        0x00, 0x00,        // Length (will be set properly later)
        0x01,              // Number of copies
        0x00,              // Reserved
        0x40,              // Cut setting: auto cut
        0x00               // Compression mode: none
    };
    if (!send_command(handle, print_info)) return false;

    std::cout << "Printer initialization complete" << std::endl;
    return true;
}
