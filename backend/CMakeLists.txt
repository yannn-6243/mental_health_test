cmake_minimum_required(VERSION 3.10)
project(mental_health_api)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Add executable
add_executable(server main.cpp)

# Link libraries
target_link_libraries(server 
    sqlite3 
    Threads::Threads
)

# Enable CORS in Crow
target_compile_definitions(server PRIVATE CROW_ENABLE_CORS)
