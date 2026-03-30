#ifndef _WIN32   // Sadece Linux'ta derlenir

#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <locale.h>
#include <sys/select.h>

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

    // Kontrol bayrakları
    raw.c_cflag |= (CS8);
    // Yerel bayraklar: ECHO kapalı, ICANON kapalı (raw), sinyaller kapalı
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // read() hemen dönsün (timeout yok)
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_active = 1;
    /* Bracketed paste mode: terminal yapıştırmayı ESC[200~ ... ESC[201~ ile sarar */
    write(STDOUT_FILENO, "\033[?2004h", 8);
}

void term_disable_raw_mode() {
    if (raw_mode_active) {
        write(STDOUT_FILENO, "\033[?2004l", 8); /* bracketed paste kapat */
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_active = 0;
    }
}

// ─── Klavye okuma ─────────────────────────────────────────────────────────────

int term_read_key() {
    unsigned char c;

    if (read(STDIN_FILENO, &c, 1) != 1) return -1;

    // CTRL kombinasyonlarını ESC sequence'den ÖNCE yakala
    if (c == 18)  return KEY_CTRL_R;
    if (c == 6)  return KEY_CTRL_F;
    if (c == 7)  return KEY_CTRL_G;
    if (c == 3)  return KEY_CTRL_C;
    if (c == 24) return KEY_CTRL_X;
    if (c == 22) return KEY_CTRL_V;
    if (c == 26) return KEY_CTRL_Z;
    if (c == 23) return KEY_CTRL_W;

    // Özel tuş kontrolü: ESC ile başlayan sequence
    if (c == 27) {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);          // ← EKSİKTİ, bu şart
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        unsigned char seq[4] = {0};
        int n = read(STDIN_FILENO, &seq[0], 1);

        t.c_cc[VMIN]  = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        if (n <= 0) return KEY_ESC;

        // Alt+Backspace: ESC + 0x7F veya ESC + 0x08
        if (seq[0] == 0x7F || seq[0] == 0x08) return KEY_ALT_BACKSPACE;

        // seq[1] sadece bir kez okunuyor
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
            if (seq[1] >= '0' && seq[1] <= '9') {
                unsigned char seq2;
                read(STDIN_FILENO, &seq2, 1);
                if (seq[1] == '3' && seq2 == '~') return KEY_DELETE;

                /* Bracketed paste: ESC[200~ başlangıç, ESC[201~ bitiş */
                if ((seq[1] == '2') && (seq2 == '0')) {
                    unsigned char seq3, seq4;
                    read(STDIN_FILENO, &seq3, 1);
                    read(STDIN_FILENO, &seq4, 1);
                    /* seq3='0' seq4='~' → paste start; seq3='1' seq4='~' → paste end */
                    if (seq4 == '~') {
                        if (seq3 == '0') return KEY_PASTE_START;
                        if (seq3 == '1') return KEY_PASTE_END;
                    }
                }

                if (seq[1] == '1' && seq2 == ';') {
                    unsigned char mod, dir;
                    read(STDIN_FILENO, &mod, 1);
                    read(STDIN_FILENO, &dir, 1);
                    if (mod == '5') {
                        if (dir == 'A') return KEY_CTRL_UP;
                        if (dir == 'B') return KEY_CTRL_DOWN;
                        if (dir == 'C') return KEY_CTRL_RIGHT;
                        if (dir == 'D') return KEY_CTRL_LEFT;
                    }
                    if (mod == '2') {
                        if (dir == 'A') return KEY_SHIFT_UP;
                        if (dir == 'B') return KEY_SHIFT_DOWN;
                        if (dir == 'C') return KEY_SHIFT_RIGHT;
                        if (dir == 'D') return KEY_SHIFT_LEFT;
                    }
                    if (mod == '6') {
                        if (dir == 'C') return KEY_CTRL_SHIFT_RIGHT;
                        if (dir == 'D') return KEY_CTRL_SHIFT_LEFT;
                    }
                }
            }
        }
        return KEY_ESC;
    }

    // CTRL + Backspace (genellikle 127 veya 8 gelir)
    if (c == 127) return KEY_BACKSPACE;
    if (c == 8)   return KEY_BACKSPACE;

    return (int)c;
}

// ─── Ekran işlemleri ──────────────────────────────────────────────────────────

void term_clear_screen() {
    write(STDOUT_FILENO, "\033[5 q", 5);
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
    write(STDOUT_FILENO, "\033[5 q", 5);  // yanıp sönen dikey çizgi
}

void term_cursor_blink_off() {
    write(STDOUT_FILENO, "\033[?12l", 6);
}

// Linux: sistem panosu entegrasyonu yok (X11/Wayland gerektirir)
// İç buffer (clipboard_buf edilebilir.c) yeterlidir
void term_clipboard_set(const char *text, int len) { (void)text; (void)len; }
int  term_clipboard_get(char *buf, int max_len)    { (void)buf; (void)max_len; return 0; }

#endif // !_WIN32