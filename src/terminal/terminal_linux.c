#ifndef _WIN32   // Sadece Linux'ta derlenir

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <locale.h>

// ─── İç değişkenler ───────────────────────────────────────────────────────────
static struct termios orig_termios;
static int raw_mode_active = 0;

// ─── Raw mode ─────────────────────────────────────────────────────────────────

void term_enable_raw_mode() {
    setlocale(LC_ALL, "");  // Türkçe karakter desteği

    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;

    // Giriş bayrakları
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // Çıkış bayrakları
    raw.c_oflag &= ~(OPOST);
    // Kontrol bayrakları
    raw.c_cflag |= (CS8);
    // Yerel bayraklar: ECHO kapalı, ICANON kapalı (raw), sinyaller kapalı
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // read() hemen dönsün (timeout yok)
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_active = 1;
}

void term_disable_raw_mode() {
    if (raw_mode_active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_active = 0;
    }
}

// ─── Klavye okuma ─────────────────────────────────────────────────────────────

int term_read_key() {
    unsigned char c;

    if (read(STDIN_FILENO, &c, 1) != 1) return -1;

    // CTRL + harf kombinasyonları (ASCII 1-26)
    // Bunlar zaten doğru değerle geliyor, direkt döndür
    // Özel tuş kontrolü: ESC ile başlayan sequence
    if (c == 27) {
        unsigned char seq[4] = {0};

        // Sequence geldi mi kontrol et
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

        if (seq[0] == '[') {
            // Sayısal sequence (örn: [1;5C = CTRL+Sağ)
            if (seq[1] >= '0' && seq[1] <= '9') {
                read(STDIN_FILENO, &seq[2], 1);

                if (seq[1] == '3' && seq[2] == '~') return KEY_DELETE;

                // CTRL + Yön: ESC[1;5A/B/C/D
                if (seq[1] == '1' && seq[2] == ';') {
                    unsigned char mod, dir;
                    read(STDIN_FILENO, &mod,  1);
                    read(STDIN_FILENO, &dir,  1);

                    if (mod == '5') { // CTRL
                        if (dir == 'A') return KEY_CTRL_UP;
                        if (dir == 'B') return KEY_CTRL_DOWN;
                        if (dir == 'C') return KEY_CTRL_RIGHT;
                        if (dir == 'D') return KEY_CTRL_LEFT;
                    }
                    if (mod == '2') { // SHIFT
                        if (dir == 'A') return KEY_SHIFT_UP;
                        if (dir == 'B') return KEY_SHIFT_DOWN;
                        if (dir == 'C') return KEY_SHIFT_RIGHT;
                        if (dir == 'D') return KEY_SHIFT_LEFT;
                    }
                    if (mod == '6') { // CTRL + SHIFT
                        if (dir == 'C') return KEY_CTRL_SHIFT_RIGHT;
                        if (dir == 'D') return KEY_CTRL_SHIFT_LEFT;
                    }
                }
            }

            // Normal yön tuşları: ESC[A/B/C/D
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
        return KEY_ESC;
    }

    // CTRL + Backspace (genellikle 127 veya 8 gelir)
    if (c == 127) return KEY_BACKSPACE;
    if (c == 31)  return KEY_CTRL_BACKSPACE; // CTRL+Backspace bazı terminallerde

    return (int)c;
}

// ─── Ekran işlemleri ──────────────────────────────────────────────────────────

void term_clear_screen() {
    write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

void term_clear_line() {
    write(STDOUT_FILENO, "\033[2K", 4);
}

void term_set_cursor(int row, int col) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
    write(STDOUT_FILENO, buf, len);
}

void term_hide_cursor() {
    write(STDOUT_FILENO, "\033[?25l", 6);
}

void term_show_cursor() {
    write(STDOUT_FILENO, "\033[?25h", 6);
}

void term_get_size(int *rows, int *cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

// ─── Renk işlemleri ───────────────────────────────────────────────────────────

void term_set_color(int fg, int bg) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\033[%d;%dm", fg, bg);
    write(STDOUT_FILENO, buf, len);
}

void term_reset_color() {
    write(STDOUT_FILENO, "\033[0m", 4);
}

// ─── İmleç efekti ─────────────────────────────────────────────────────────────

void term_cursor_blink_on() {
    write(STDOUT_FILENO, "\033[?12h", 6);  // Yanıp sönen dikey çizgi
    write(STDOUT_FILENO, "\033[6 q",  5);  // Bar imleci
}

void term_cursor_blink_off() {
    write(STDOUT_FILENO, "\033[?12l", 6);
}

#endif // !_WIN32