project(life)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(life main.cpp)

find_package(SFML 2 COMPONENTS audio window graphics system REQUIRED)
target_include_directories(life PRIVATE ${SFML_INCLUDE_DIR})
target_link_libraries(life PRIVATE sfml-graphics sfml-window sfml-system)