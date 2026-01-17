cmake_minimum_required(VERSION 3.10)
project(idac-rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(idac-rpc SHARED src/cpp/main.cpp)

set_target_properties(idac-rpc PROPERTIES PREFIX "")
set_target_properties(idac-rpc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(WIN32)
    target_link_libraries(idac-rpc PRIVATE 
        user32 
        gdi32 
        wsock32
        ws2_32
    )
    
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_link_options(idac-rpc PRIVATE -static)
    endif()
endif()