project(bank)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(bank main.cpp
        Bank.cpp
        Bank.h)

find_package(Catch2 REQUIRED)
target_link_libraries(${PROJECT_NAME} Catch2::Catch2)

add_executable(tests BankTest.cpp Bank.cpp Bank.h)