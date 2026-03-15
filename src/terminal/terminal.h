#ifndef TERMINAL_H
#define TERMINAL_H

// ─── Özel tuş kodları (platform bağımsız) ────────────────────────────────────
// Yön tuşları
#define KEY_UP        1001
#define KEY_DOWN      1002
#define KEY_RIGHT     1003
#define KEY_LEFT      1004

// CTRL + Yön
#define KEY_CTRL_RIGHT  1005
#define KEY_CTRL_LEFT   1006
#define KEY_CTRL_UP     1007
#define KEY_CTRL_DOWN   1008

// SHIFT + Yön (seçim)
#define KEY_SHIFT_RIGHT  1009
#define KEY_SHIFT_LEFT   1010
#define KEY_SHIFT_UP     1011
#define KEY_SHIFT_DOWN   1012

// CTRL + SHIFT + Yön (kelime bazlı seçim)
#define KEY_CTRL_SHIFT_RIGHT  1013
#define KEY_CTRL_SHIFT_LEFT   1014

// Fonksiyon tuşları
#define KEY_BACKSPACE   127
#define KEY_ENTER       13
#define KEY_ESC         27
#define KEY_TAB         9
#define KEY_DELETE      1020

// CTRL kısayolları
#define KEY_CTRL_C      3
#define KEY_CTRL_F      6
#define KEY_CTRL_G      7
#define KEY_CTRL_H      8   // Not: bazı sistemlerde backspace ile çakışır, dikkat!
#define KEY_CTRL_S      19
#define KEY_CTRL_V      22
#define KEY_CTRL_X      24
#define KEY_CTRL_Z      26
#define KEY_CTRL_BACKSPACE 1030

// ─── Renk kodları ─────────────────────────────────────────────────────────────
#define COLOR_RESET     0
#define COLOR_BLACK     30
#define COLOR_RED       31
#define COLOR_GREEN     32
#define COLOR_YELLOW    33
#define COLOR_BLUE      34
#define COLOR_MAGENTA   35
#define COLOR_CYAN      36
#define COLOR_WHITE     37

#define BG_BLACK        40
#define BG_RED          41
#define BG_GREEN        42
#define BG_YELLOW       43  // Seçili metin
#define BG_BLUE         44  // Arama sonucu
#define BG_MAGENTA      45
#define BG_CYAN         46
#define BG_WHITE        47

// ─── Fonksiyon bildirimleri ───────────────────────────────────────────────────

// Terminal başlatma & kapatma
void term_enable_raw_mode();
void term_disable_raw_mode();

// Klavye
int  term_read_key();

// Ekran
void term_clear_screen();
void term_clear_line();
void term_set_cursor(int row, int col);   // 1'den başlar
void term_hide_cursor();
void term_show_cursor();
void term_get_size(int *rows, int *cols); // Terminal boyutu

// Renk
void term_set_color(int fg, int bg);
void term_reset_color();

// İmleç efekti (yanıp sönen dikey çizgi)
void term_cursor_blink_on();
void term_cursor_blink_off();

#endif // TERMINAL_H