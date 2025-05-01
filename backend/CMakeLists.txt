# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(brotherql800 VERSION 1.0)

# Set the C++ standard to use
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler extensions

# Enable the generation of compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find libusb using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Find all source files in the src directory
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.cc")

# Add an executable target
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Add the include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps
        ${LIBUSB_INCLUDE_DIRS}
)

# Link against libusb
target_link_libraries(${PROJECT_NAME}
    ${LIBUSB_LIBRARIES}
)

# Add compile options from pkg-config if any
target_compile_options(${PROJECT_NAME} PRIVATE ${LIBUSB_CFLAGS_OTHER})
