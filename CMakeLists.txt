cmake_minimum_required(VERSION 3.10)
project(prolab_2_proje_1 C)

set(CMAKE_C_STANDARD 11)

set(SOURCES
        src/main.c
        src/undo/undo_stack.c
        src/editor/buffer.c
        src/editor/cursor.c
        src/editor/search.c
        src/editor/selection.c
        src/editor/editor.c
        src/ui/toolbox.c
        src/ui/renderer.c
        src/io/file_io.c
        src/io/file_manager.c
)

if(WIN32)
    list(APPEND SOURCES src/terminal/terminal_windows.c)
else()
    list(APPEND SOURCES src/terminal/terminal_linux.c)
endif()

add_executable(prolab_2_proje_1 ${SOURCES})

target_include_directories(prolab_2_proje_1 PRIVATE src)

if(WIN32)
    target_link_libraries(prolab_2_proje_1 kernel32)
endif()