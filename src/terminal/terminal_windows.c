#ifdef _WIN32

// Eski MinGW için tanımla
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>

// ─── İç değişkenler ───────────────────────────────────────────────────────────
static HANDLE hStdin;
static HANDLE hStdout;
static DWORD  orig_console_mode;

// ─── Raw mode ─────────────────────────────────────────────────────────────────

void term_enable_raw_mode() {
    // UTF-8 / Türkçe karakter desteği
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    hStdin  = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    // Orijinal modu sakla
    GetConsoleMode(hStdin, &orig_console_mode);

    // Raw mode: ENABLE_ECHO_INPUT ve ENABLE_LINE_INPUT kapalı
    DWORD new_mode = orig_console_mode;
    new_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(hStdin, new_mode);

    // ANSI escape kodlarını aktif et (Windows 10+)
    DWORD out_mode;
    GetConsoleMode(hStdout, &out_mode);
    out_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdout, out_mode);
}

void term_disable_raw_mode() {
    SetConsoleMode(hStdin, orig_console_mode);
}

// ─── Klavye okuma ─────────────────────────────────────────────────────────────
// Windows'ta _getch() ile özel tuşlar:
//   İlk _getch() → 0 veya 224 döner (özel tuş sinyali)
//   İkinci _getch() → tuşun scan kodu gelir

int term_read_key() {
    int c = _getch();

    // Özel tuş sinyali
    if (c == 0 || c == 224) {
        int scan = _getch();

        switch (scan) {
            // Yön tuşları
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 77: return KEY_RIGHT;
            case 75: return KEY_LEFT;

            // CTRL + Yön
            case 141: return KEY_CTRL_UP;
            case 145: return KEY_CTRL_DOWN;
            case 116: return KEY_CTRL_RIGHT;
            case 115: return KEY_CTRL_LEFT;

            // SHIFT + Yön
            case 152: return KEY_SHIFT_UP;
            case 160: return KEY_SHIFT_DOWN;
            case 157: return KEY_SHIFT_RIGHT;
            case 155: return KEY_SHIFT_LEFT;

            // CTRL + SHIFT + Yön
            case 148: return KEY_CTRL_SHIFT_RIGHT;
            case 147: return KEY_CTRL_SHIFT_LEFT;

            // Delete
            case 83:  return KEY_DELETE;

            default:  return -1; // Bilinmeyen tuş
        }
    }

    // Backspace (Windows'ta 8 gelir)
    if (c == 8) {
        // CTRL+Backspace da 127 değil, farklı bir değerle gelebilir
        // _getch() ile CTRL+Backspace genellikle 127 döner
        return KEY_BACKSPACE;
    }
    if (c == 127) return KEY_CTRL_BACKSPACE;

    // ESC
    if (c == 27) return KEY_ESC;

    // CTRL + harf kombinasyonları (ASCII 1-26 arası gelir)
    // Örn: CTRL+C = 3, CTRL+F = 6, CTRL+Z = 26
    // terminal.h'daki sabitlerle zaten eşleşiyor, direkt döndür

    return c;
}

// ─── Ekran işlemleri ──────────────────────────────────────────────────────────

void term_clear_screen() {
    // ANSI aktifse bu çalışır (Windows 10+)
    printf("\033[2J\033[H");
    fflush(stdout);
}

void term_clear_line() {
    printf("\033[2K");
    fflush(stdout);
}

void term_set_cursor(int row, int col) {
    // ANSI yöntemi (Windows 10+)
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

void term_hide_cursor() {
    printf("\033[?25l");
    fflush(stdout);
}

void term_show_cursor() {
    printf("\033[?25h");
    fflush(stdout);
}

void term_get_size(int *rows, int *cols) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    *cols = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
}

// ─── Renk işlemleri ───────────────────────────────────────────────────────────

void term_set_color(int fg, int bg) {
    // ANSI renk kodları (Windows 10+ ANSI desteğiyle çalışır)
    printf("\033[%d;%dm", fg, bg);
    fflush(stdout);
}

void term_reset_color() {
    printf("\033[0m");
    fflush(stdout);
}

// ─── İmleç efekti ─────────────────────────────────────────────────────────────

void term_cursor_blink_on() {
    printf("\033[?12h");  // Yanıp sönen
    printf("\033[6 q");   // Bar (dikey çizgi) imleci
    fflush(stdout);
}

void term_cursor_blink_off() {
    printf("\033[?12l");
    fflush(stdout);
}

#endif // _WIN32