cmake_minimum_required(VERSION 4.1)
project(prolab_2_proje_1 C)

set(CMAKE_C_STANDARD 23)

add_executable(prolab_2_proje_1 src/main.c
        src/terminal/terminal_linux.c
        src/terminal/terminal_windows.c
        src/terminal/terminal.h
        src/undo/undo_stack.c
        src/undo/undo_stack.h
        src/editor/buffer.c
        src/editor/buffer.h
        src/editor/cursor.c
        src/editor/cursor.h
        src/editor/search.h
        src/ui/toolbox.c
        src/ui/toolbox.h
        src/ui/renderer.h
        src/ui/renderer.c
        src/io/file_io.h
        src/io/file_io.c
        src/io/file_manager.h
        src/io/file_manager.c)
