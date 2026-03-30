#ifdef _WIN32

// Eski MinGW için tanımla
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// ─── İç değişkenler ───────────────────────────────────────────────────────────
static HANDLE hStdin;
static HANDLE hStdout;
static DWORD  orig_in_mode;
static DWORD  orig_out_mode;

// UTF-8 çok baytlı karakter kuyruğu (Türkçe/Unicode desteği)
static unsigned char utf8_queue[4];
static int           utf8_queue_len = 0;
static int           utf8_queue_pos = 0;

// ─── Raw mode ─────────────────────────────────────────────────────────────────

void term_enable_raw_mode() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    hStdin  = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(hStdin,  &orig_in_mode);
    GetConsoleMode(hStdout, &orig_out_mode);

    DWORD new_in = orig_in_mode;
    new_in &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    new_in |=  ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hStdin, new_in);

    DWORD new_out = orig_out_mode;
    new_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
    SetConsoleMode(hStdout, new_out);
}

void term_disable_raw_mode() {
    SetConsoleMode(hStdin,  orig_in_mode);
    SetConsoleMode(hStdout, orig_out_mode);
}

// ─── Klavye okuma — ReadConsoleInput ile ──────────────────────────────────────
// _getch() Shift/Ctrl modifier'larını kaybeder. ReadConsoleInput bunları
// dwControlKeyState aracılığıyla tam olarak iletir.

int term_read_key() {
    // Önceki çok baytlı UTF-8 karakterin kalan baytları varsa önce onları ver
    if (utf8_queue_pos < utf8_queue_len)
        return (int)utf8_queue[utf8_queue_pos++];
    utf8_queue_len = 0;
    utf8_queue_pos = 0;

    INPUT_RECORD ir;
    DWORD read;

    for (;;) {
        if (!ReadConsoleInputW(hStdin, &ir, 1, &read)) return -1;
        if (read == 0) continue;

        // Sadece KEY_EVENT ve basma (keydown) işle
        if (ir.EventType != KEY_EVENT) continue;
        if (!ir.Event.KeyEvent.bKeyDown) continue;

        WORD  vk  = ir.Event.KeyEvent.wVirtualKeyCode;
        DWORD ctl = ir.Event.KeyEvent.dwControlKeyState;
        WCHAR wch = ir.Event.KeyEvent.uChar.UnicodeChar;

        int shift = (ctl & SHIFT_PRESSED)                            != 0;
        int ctrl  = (ctl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        int alt   = (ctl & (LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED))  != 0;

        // ── CTRL + harf kombinasyonları ──────────────────────────────────────
        if (ctrl && !shift) {
            switch (vk) {
                case 'C': return KEY_CTRL_C;
                case 'F': return KEY_CTRL_F;
                case 'G': return KEY_CTRL_G;
                case 'O': return KEY_CTRL_O;
                case 'R': return KEY_CTRL_R;
                case 'S': return KEY_CTRL_S;
                case 'V': return KEY_CTRL_V;
                case 'W': return KEY_CTRL_W;
                case 'X': return KEY_CTRL_X;
                case 'Z': return KEY_CTRL_Z;
                case VK_BACK: return KEY_CTRL_BACKSPACE;
                default: break;
            }
        }

        // ── Özel / yön tuşları ───────────────────────────────────────────────
        switch (vk) {
            case VK_UP:
                if (ctrl && shift) return KEY_CTRL_SHIFT_RIGHT;
                if (ctrl)          return KEY_CTRL_UP;
                if (shift)         return KEY_SHIFT_UP;
                return KEY_UP;

            case VK_DOWN:
                if (ctrl)  return KEY_CTRL_DOWN;
                if (shift) return KEY_SHIFT_DOWN;
                return KEY_DOWN;

            case VK_RIGHT:
                if (ctrl && shift) return KEY_CTRL_SHIFT_RIGHT;
                if (ctrl)          return KEY_CTRL_RIGHT;
                if (shift)         return KEY_SHIFT_RIGHT;
                return KEY_RIGHT;

            case VK_LEFT:
                if (ctrl && shift) return KEY_CTRL_SHIFT_LEFT;
                if (ctrl)          return KEY_CTRL_LEFT;
                if (shift)         return KEY_SHIFT_LEFT;
                return KEY_LEFT;

            case VK_DELETE:  return KEY_DELETE;
            case VK_BACK:
                if (alt)  return KEY_ALT_BACKSPACE;
                if (ctrl) return KEY_CTRL_BACKSPACE;
                return KEY_BACKSPACE;
            case VK_RETURN:  return KEY_ENTER;
            case VK_ESCAPE:  return KEY_ESC;
            case VK_TAB:     return KEY_TAB;
            default: break;
        }

        // ── Modifier-only tuşları yoksay ─────────────────────────────────────
        if (vk == VK_SHIFT    || vk == VK_CONTROL  || vk == VK_MENU ||
            vk == VK_LSHIFT   || vk == VK_RSHIFT   ||
            vk == VK_LCONTROL || vk == VK_RCONTROL ||
            vk == VK_LMENU    || vk == VK_RMENU)
            continue;

        // ── Yazdırılabilir karakter (ASCII + Türkçe/Unicode) ─────────────────
        if (wch >= 32 && wch != 127) {
            if (wch < 0x80) {
                // ASCII: doğrudan döndür
                return (int)wch;
            } else {
                // Türkçe/Latin extended karakterler: UTF-16 → UTF-8 dönüşümü
                // Örn: ş(U+015F) → 0xC5 0x9F (2 bayt)
                char mb[4] = {0};
                WCHAR tmp = wch;
                int n = WideCharToMultiByte(CP_UTF8, 0, &tmp, 1, mb, 4, NULL, NULL);
                if (n >= 1) {
                    // Kalan baytları kuyruğa ekle
                    for (int i = 1; i < n && i < 4; i++)
                        utf8_queue[utf8_queue_len++] = (unsigned char)mb[i];
                    return (unsigned char)mb[0]; // İlk baytı hemen döndür
                }
            }
        }
    }
}

// ─── Ekran işlemleri ──────────────────────────────────────────────────────────

void term_clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void term_clear_line() {
    printf("\033[2K");
    fflush(stdout);
}

void term_set_cursor(int row, int col) {
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
    printf("\033[%d;%dm", fg, bg);
    fflush(stdout);
}

void term_reset_color() {
    printf("\033[0m");
    fflush(stdout);
}

// ─── İmleç efekti ─────────────────────────────────────────────────────────────

void term_cursor_blink_on() {
    printf("\033[?12h");
    printf("\033[6 q");
    fflush(stdout);
}

void term_cursor_blink_off() {
    printf("\033[?12l");
    fflush(stdout);
}

// ─── Windows Sistem Panosu ────────────────────────────────────────────────────

/* İç buffer'ı Windows panosuna gönder (Ctrl+C / Ctrl+X sonrası çağrılır) */
void term_clipboard_set(const char *text, int len) {
    if (!text || len <= 0) return;

    // UTF-8 → UTF-16 dönüşümü
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    if (wlen <= 0) return;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wlen + 1) * sizeof(WCHAR));
    if (!hMem) return;

    WCHAR *pMem = (WCHAR *)GlobalLock(hMem);
    if (!pMem) { GlobalFree(hMem); return; }
    MultiByteToWideChar(CP_UTF8, 0, text, len, pMem, wlen);
    pMem[wlen] = L'\0';
    GlobalUnlock(hMem);

    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    } else {
        GlobalFree(hMem);
    }
}

/* Windows panosundan oku (Ctrl+V sonrası çağrılır), byte sayısını döndürür */
int term_clipboard_get(char *buf, int max_len) {
    if (!buf || max_len <= 1) return 0;
    if (!OpenClipboard(NULL)) return 0;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return 0; }

    WCHAR *wtext = (WCHAR *)GlobalLock(hData);
    if (!wtext) { CloseClipboard(); return 0; }

    // UTF-16 → UTF-8, geçici tampon
    char tmp[65536];
    int n = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, tmp, sizeof(tmp) - 1, NULL, NULL);
    GlobalUnlock(hData);
    CloseClipboard();

    if (n <= 0) return 0;
    n--; // null terminator'ı say dışı bırak

    // \r\n → \n dönüşümü (Windows panosu CRLF kullanır)
    int pos = 0;
    for (int i = 0; i < n && pos < max_len - 1; i++) {
        if (tmp[i] == '\r') continue; // \r'ı atla
        buf[pos++] = tmp[i];
    }
    buf[pos] = '\0';
    return pos;
}

#endif // _WIN32