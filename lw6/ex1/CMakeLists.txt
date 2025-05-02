cmake_minimum_required(VERSION 3.21)
project(ticket_office)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(${PROJECT_NAME}
        main.cpp
        TicketOffice.h
)


add_executable(test
    TicketOffice.h
    TicketOfficeTest.cpp
)

set_target_properties(test PROPERTIES WIN32_EXECUTABLE FALSE)
find_package(Catch2 REQUIRED)
target_link_libraries(test PRIVATE Catch2::Catch2)