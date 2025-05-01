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

# Find all source files in the src directory
file(GLOB SOURCE_FILES "src/*.cpp")

# Add an executable target
# This creates an executable named after your project
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Add the include directory AFTER the target is created
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Optional: Add libraries if needed
# find_package(Boost REQUIRED)
# target_link_libraries(${PROJECT_NAME} Boost::Boost)