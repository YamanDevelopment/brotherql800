#include "main.hh"
#include "port.hh"
#include "image.hh"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Brother QL-800 Printer Utility" << std::endl;

    // Check command line arguments
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <image_file> [copies]" << std::endl;
        return 1;
    }

    std::string image_path = argv[1];
    int copies = 1;

    if (argc >= 3) {
        copies = std::stoi(argv[2]);
        if (copies < 1) copies = 1;
    }

    // Open printer connection
    libusb_device_handle* handle = open_printer_port();
    if (!handle) {
        std::cerr << "Failed to open printer connection" << std::endl;
        return 1;
    }

    // Check printer status
    auto status = get_printer_status(handle);
    if (status.empty()) {
        std::cerr << "Failed to get printer status" << std::endl;
        close_printer_port(handle);
        return 1;
    }

    // Print the label
    bool success = print_label(handle, image_path, copies);

    // Close printer connection
    close_printer_port(handle);

    if (!success) {
        std::cerr << "Failed to print label" << std::endl;
        return 1;
    }

    std::cout << "Label printed successfully" << std::endl;
    return 0;
}
