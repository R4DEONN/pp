cmake_minimum_required(VERSION 3.21)
project(webcam)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

find_package(Boost REQUIRED COMPONENTS system endian)
find_package(OpenCV REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        Server.h
        Constants.h
        Client.h
        PacketHeader.h
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${Boost_LIBS}
        sfml-graphics
        sfml-window
        sfml-system
        ws2_32
        ${SDL2_LIBRARIES}
        ${OpenCV_LIBS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIR}
        ${SDL2_INCLUDE_DIR}
)