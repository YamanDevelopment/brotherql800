#!/usr/bin/env python3

import sys
import traceback

# Attempt to import necessary modules from brother_ql
try:
    from brother_ql.conversion import convert
    from brother_ql.raster import BrotherQLRaster
    # Import the send function directly
    from brother_ql.backends.helpers import send
    from brother_ql.devicedependent import models, label_type_specs, label_sizes
    # Import the base error class for the library
    from brother_ql import BrotherQLError
except ImportError as e:
    print(f"Error importing the 'brother_ql' library. Is it installed?")
    print(f"You can install it using: pip install brother_ql")
    print(f"Details: {e}")
    sys.exit(1)

# Attempt to import PyUSB (required for the pyusb backend)
try:
    import usb.core
    import usb.util
except ImportError as e:
    print(f"Error importing the 'pyusb' library. Is it installed?")
    print(f"You can install it using: pip install pyusb")
    print(f"Also ensure 'libusb' is installed on your system (see brother_ql docs).")
    print(f"Details: {e}")
    sys.exit(1)

# --- Configuration ---
# --> CHANGE THESE VALUES TO MATCH YOUR SETUP <--

# 1. Printer Connection Details (USB)
#    Use the 'brother_ql discover' command in your terminal to find this.
#    Copy the full 'usb://...' string for your printer here.
#    Examples:
#      'usb://0x04f9:0x209c'  (Vendor ID 0x04f9, Product ID for QL-810W)
#      'usb://0x04f9:0x209d/000MABC123XYZ' (Includes serial number if needed)
# !! IMPORTANT: Replace the example below with YOUR printer's identifier !! 
PRINTER_IDENTIFIER = 'usb://0x04f9:0x209b' # <-- Updated with correct QL-800 identifier # <-- *** REPLACE THIS WITH YOUR PRINTER'S IDENTIFIER ***

# 2. Printer Model
#    Choose your specific model.
PRINTER_MODEL = 'QL-800' # Options: 'QL-800', 'QL-810W', 'QL-820NWB'

# 3. Label Type
#    Check 'brother_ql info labels' for available sizes.
LABEL_TYPE = '62' # Example: 62mm endless tape

# 4. Image File Path
#    The image you want to print.
IMAGE_PATH = './testforprinter.png' # Example: Replace with your image file path

# 5. Optional Settings
ROTATE = '0'          # '0', '90', '180', '270', 'auto'
COMPRESS = False      # True for data compression (if model supports it)
USE_RED = False       # SET TO TRUE *only* if using DK-22251 tape (even for black-only printing on it)
CUT_TAPE = True       # Cut the tape after printing?
DITHER_IMAGE = False  # Use dithering algorithm for B/W conversion?
THRESHOLD_BW = 70.0   # Black/White threshold (0-100, higher = more black), ignored if dither=True
HIGH_QUALITY = True   # Use high quality print mode (slower) vs low quality (faster)
# --- End Configuration ---

# --- Input Validation (Basic) ---
if PRINTER_MODEL not in models:
    print(f"Error: Model '{PRINTER_MODEL}' not recognized by the library.")
    print(f"Available models: {list(models.keys())}")
    sys.exit(1)

if LABEL_TYPE not in label_sizes:
    print(f"Error: Label type '{LABEL_TYPE}' not recognized by the library.")
    print(f"Run 'brother_ql info labels' for available label names.")
    sys.exit(1)

if USE_RED and LABEL_TYPE != '62red' and not LABEL_TYPE.startswith('dk-22251'):
     print(f"Warning: 'USE_RED = True' is set, but label type is '{LABEL_TYPE}'.")
     print("Red printing requires DK-22251 tape (label type '62red').")

# --- Important Check for USB ---
print("\n--- USB Printing Notes ---")
print(f"Attempting to print to: {PRINTER_IDENTIFIER}")
print(f"Using Model: {PRINTER_MODEL}, Label: {LABEL_TYPE}")
print("1. Ensure 'Editor Lite' mode is OFF on your printer (the 'Lite' LED should be unlit).")
print("2. Ensure 'libusb' is installed (see brother_ql/pyusb documentation).")
print("3. On Windows, ensure the libusb-win32/libusbK filter driver is installed for the printer using Zadig or the libusb-win32 filter installer.")
print("4. On Linux, you might need permission to access the USB device (check udev rules or try running with 'sudo').")
print("--- Starting Print Job ---\n")

try:
    # --- Core Logic ---

    # 1. Initialize the Raster object for the specific model
    print(f"Initializing raster for model: {PRINTER_MODEL}")
    qlr = BrotherQLRaster(PRINTER_MODEL)
    # Optional: Raise exceptions for warnings like image/label size mismatches
    # qlr.exception_on_warning = True

    # 2. Convert the image file to raster instructions
    print(f"Converting image '{IMAGE_PATH}' for label '{LABEL_TYPE}'...")
    # Note: 'images' parameter expects a list of file paths
    instructions = convert(
        qlr=qlr,
        images=[IMAGE_PATH],
        label=LABEL_TYPE,
        rotate=ROTATE,
        cut=CUT_TAPE,
        compress=COMPRESS,
        red=USE_RED,
        dither=DITHER_IMAGE,
        threshold=THRESHOLD_BW,
        hq=HIGH_QUALITY,
    )

    # 3. Send the instructions to the printer via the USB backend (pyusb)
    print(f"Sending {len(instructions)} bytes to printer: {PRINTER_IDENTIFIER} using pyusb backend...")
    # The 'send' helper function automatically selects the 'pyusb' backend
    # based on the 'usb://' identifier string.
    # Explicitly specifying 'pyusb' is good practice.
    send(
        instructions=instructions,
        printer_identifier=PRINTER_IDENTIFIER,
        backend_identifier='pyusb', # Explicitly use pyusb
        blocking=True # Wait for the backend to finish sending
    )

    print("\nPrinting command sent successfully via USB.")

# --- Error Handling ---
except FileNotFoundError:
    print(f"Error: Image file not found at '{IMAGE_PATH}'")
    sys.exit(1)

# Catch the base BrotherQL library error
except BrotherQLError as e:
     # Check if the error message indicates a backend/connection problem
     # Keywords to look for might include 'backend', 'device not found', 'access denied', etc.
     error_str = str(e).lower()
     if 'backend' in error_str or 'no device found' in error_str or 'access denied' in error_str or 'could not claim' in error_str or 'pipe error' in error_str:
         print(f"\nError: Backend/Connection issue. Could not find or access printer: {PRINTER_IDENTIFIER}")
         print(f"Troubleshooting:")
         print(f" - Is the printer connected via USB and turned ON?")
         print(f" - Is the PRINTER_IDENTIFIER string '{PRINTER_IDENTIFIER}' correct? (Run 'brother_ql discover' to verify)")
         print(f" - Is 'libusb' installed correctly for your OS?")
         print(f" - On Windows, is the correct libusb-win32/libusbK filter driver associated with the printer using Zadig?")
         print(f" - On Linux, do you have permissions for the USB device? (Check /etc/udev/rules.d/, or try 'sudo python your_script.py')")
         print(f" - Is 'Editor Lite' mode definitely OFF (Lite LED unlit)?")
         print(f" - Is another program potentially using the printer?")
         print(f"Error Details: {e}")
     else:
         # Handle other potential BrotherQLErrors (e.g., image conversion issues)
         print(f"\nError specific to brother_ql library: {e}")
     sys.exit(1)

# Catch generic Python OS errors which might indicate deeper USB issues
except OSError as e:
    print(f"\nOS Error during USB communication: {e}")
    print("This might indicate a low-level USB issue or driver problem.")
    print("Check system USB device recognition and libusb installation.")
    sys.exit(1)

# Catch any other unexpected errors
except Exception as e:
    print(f"\nAn unexpected error occurred:")
    # Print the full traceback for detailed debugging
    traceback.print_exc()
    sys.exit(1)