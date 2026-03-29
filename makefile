# ─── Derleyici ────────────────────────────────────────────────────────────────
CC     = gcc
TARGET = main.exe

# ─── Kaynak dosyalar ──────────────────────────────────────────────────────────
SRCS = src/main.c \
       src/terminal/terminal_linux.c \
       src/terminal/terminal_windows.c \
       src/editor/buffer.c \
       src/editor/editor.c \
       src/editor/cursor.c \
       src/editor/selection.c \
       src/search/search.c \
       src/ui/toolbox.c \
       src/ui/renderer.c \
       src/io/file_manager.c \
       src/undo/undo_stack.c

# ─── Platform tespiti ─────────────────────────────────────────────────────────
ifeq ($(OS), Windows_NT)
    CFLAGS = -Wall -Wextra -g -D_WIN32 -I src
    LIBS   =
    RM     = del /Q
else
    CFLAGS = -Wall -Wextra -g -I src
    LIBS   =
    RM     = rm -f
endif

# ─── Hedefler ─────────────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	$(RM) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild