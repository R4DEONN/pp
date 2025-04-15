cmake_minimum_required(VERSION 3.21)
project(webcam)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

find_package(Boost REQUIRED COMPONENTS system endian)
find_package(SFML REQUIRED COMPONENTS graphics window system)
find_package(OpenCV REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        UDPSocket.h
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${Boost_LIBS}
        ${SFML_LIBS}
        sfml-graphics
        sfml-window
        sfml-system
        ws2_32

        ${OpenCV_LIBS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${SFML_INCLUDE_DIR}
        ${OpenCV_INCLUDE_DIR}
)