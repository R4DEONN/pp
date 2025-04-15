project(wh)

set(CMAKE_CXX_STANDARD 23)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")

include(FindPkgConfig)
pkg_check_modules(GLFW REQUIRED glfw3)

# Для поиска OpenGL
find_package(OpenGL REQUIRED)

find_package(GLUT REQUIRED)

add_executable(wh main.cpp miniaudio.h
        PianoView.h)

target_include_directories(wh PRIVATE
        ${GLFW_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}

        ${GLUT_INCLUDE_DIR}
)

target_link_libraries(wh PRIVATE
        ${GLFW_LIBRARIES}
        ${OPENGL_LIBRARIES}

        ${GLUT_LIBRARIES}
        glu32
        gdi32
        opengl32
        user32
        shell32
        dwmapi
)

if(MINGW)
    target_link_options(wh PRIVATE
            "-static-libgcc"
            "-static-libstdc++"
    )
endif()