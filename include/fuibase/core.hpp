#pragma once
// ============================================================
// fuibase/core.hpp — Terminal, Screen, Color, Style, Key, Theme
// ============================================================
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <csignal>

namespace fb {

// ============================================================
// Key codes — printable ASCII matches char value; specials start at 0x100
// ============================================================
enum Key : uint32_t {
    Key_none = 0,
    Key_enter     = '\n',
    Key_tab       = '\t',
    Key_escape    = 27,
    Key_backspace = 127,
    Key_space     = ' ',
    // Common letter shortcuts — cast from char preserves the value
    Key_a = 'a', Key_j = 'j', Key_k = 'k', Key_n = 'n', Key_q = 'q',

    // Special keys (outside ASCII)
    Key_up        = 0x100,
    Key_down,
    Key_left,
    Key_right,
    Key_home,
    Key_end,
    Key_page_up,
    Key_page_down,
    Key_delete,
    Key_insert,
    Key_f1,  Key_f2,  Key_f3,  Key_f4,
    Key_f5,  Key_f6,  Key_f7,  Key_f8,
    Key_f9,  Key_f10, Key_f11, Key_f12,
    Key_resize,
};

constexpr bool key_is_printable(Key k) { return k < 0x80 && k >= 0x20 && k != 127; }

// ============================================================
// Color — 24-bit truecolor + terminal-default sentinel
// ============================================================
struct Color {
    uint8_t r = 0, g = 0, b = 0;
    bool is_default = true;  // use terminal default fg/bg

    static constexpr Color none()        { return {0,0,0,true}; }
    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) { return {r,g,b,false}; }
    static constexpr Color hex(uint32_t h) {
        return rgb((h>>16)&0xFF, (h>>8)&0xFF, h&0xFF);
    }

    bool operator==(const Color& o) const {
        return is_default == o.is_default && (!is_default || (r==o.r && g==o.g && b==o.b));
    }
};

// ---- Vibrant dark palette (higher contrast than Tokyo Night) ------------------
namespace palette {
    inline constexpr auto bg       = Color::hex(0x0f0f17);  // deep ink
    inline constexpr auto surface  = Color::hex(0x252536);  // rich dark
    inline constexpr auto primary  = Color::hex(0x6db3ff);  // bright blue
    inline constexpr auto accent   = Color::hex(0xc792ff);  // vivid violet
    inline constexpr auto success  = Color::hex(0x5ce0a2);  // mint green
    inline constexpr auto warning  = Color::hex(0xf5b042);  // warm amber
    inline constexpr auto error    = Color::hex(0xff5e7a);  // coral red
    inline constexpr auto text     = Color::hex(0xe6eaf0);  // near white
    inline constexpr auto text_dim = Color::hex(0x8a8faa);  // readable gray
    inline constexpr auto border   = Color::hex(0x4a4d6b);  // visible border
} // namespace palette

// ============================================================
// Style
// ============================================================
struct Style {
    Color fg = Color::none();
    Color bg = Color::none();
    bool bold      : 1 = false;
    bool dim       : 1 = false;
    bool italic    : 1 = false;
    bool underline : 1 = false;
    bool reverse   : 1 = false;

    static constexpr Style none() { return {}; }

    Style& fg_color(Color c)   { fg = c; return *this; }
    Style& bg_color(Color c)   { bg = c; return *this; }
    Style& with_bold(bool b=true)    { bold = b; return *this; }
    Style& with_dim(bool d=true)     { dim = d; return *this; }

    bool operator==(const Style& o) const {
        return fg == o.fg && bg == o.bg
            && bold == o.bold && dim == o.dim
            && italic == o.italic && underline == o.underline
            && reverse == o.reverse;
    }
};

// Pre-built styles
namespace style {
    inline Style heading(Style s = {}) { s.bold = true; return s; }
    inline Style muted(Style s = {})   { s.dim = true; return s; }
} // namespace style

// ============================================================
// Theme — a complete color set
// ============================================================
struct Theme {
    Color bg, surface, primary, accent, success, warning, error, text, text_dim, border;

    static Theme tokyo_night() {
        return {palette::bg,  palette::surface, palette::primary, palette::accent,
                palette::success, palette::warning,  palette::error,
                palette::text, palette::text_dim, palette::border};
    }
};

// ============================================================
// Point, Size & Rect
// ============================================================
struct Point { int row, col; };
struct Size  { int w, h; };
struct Rect  { int row, col, w, h; };

constexpr Rect rect_at(int row, int col, int w, int h) { return {row,col,w,h}; }

// ============================================================
// Terminal RAII guard — raw mode, cursor, signal handlers
// ============================================================
class Terminal {
    static inline termios orig_{};
    static inline bool    raw_ = false;
public:
    static void raw_mode() {
        if (raw_) return;
        tcgetattr(STDIN_FILENO, &orig_);
        termios raw = orig_;
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        raw_ = true;
    }
    static void restore() {
        if (!raw_) return;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_);
        raw_ = false;
    }
    static void hide_cursor()   { write_str("\033[?25l"); }
    static void show_cursor()   { write_str("\033[?25h"); }
    static void clear()         { write_str("\033[2J\033[H"); }
    static void move(int r, int c) { write_str("\033["+std::to_string(r)+";"+std::to_string(c)+"H"); }
    static void flush()         { std::fflush(stdout); }

    static Size size() {
        winsize w{};
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return {w.ws_col > 0 ? (int)w.ws_col : 80,
                w.ws_row > 0 ? (int)w.ws_row : 24};
    }
    static void write_str(const std::string& s) {
        [[maybe_unused]] auto _ = ::write(STDOUT_FILENO, s.data(), s.size());
    }
}; // terminal: this exists as a namespace of static helpers, not a class with instances

// ============================================================
// Screen — double-buffered cell grid with diff-based present
// ============================================================
class Screen {
public:
    Screen(int w, int h) : w_(w), h_(h), buf_(w*h), prev_(w*h) { clear(); }

    int width()  const { return w_; }
    int height() const { return h_; }

    void resize(int w, int h) {
        w_ = w; h_ = h;
        buf_.assign(w_*h_, Cell{});
        prev_.assign(w_*h_, Cell{});
        clear();
    }

    void clear() {
        for (auto& c : buf_) c = {U' ', Style::none()};
    }

    // ---- drawing primitives ----

    void put(int row, int col, char32_t ch, Style s = Style::none()) {
        if (row < 0 || row >= h_ || col < 0 || col >= w_) return;
        buf_[row * w_ + col] = {ch, s};
    }

    void text(int row, int col, std::string_view sv, Style s = Style::none()) {
        int c = col;
        size_t i = 0;
        while (i < sv.size() && c < w_) {
            if (row < 0 || row >= h_) return;
            char32_t cp;
            unsigned char b = (unsigned char)sv[i];
            // Decode UTF-8 lead byte
            int extra = 0;
            if      ((b & 0x80) == 0x00) { cp = b; extra = 0; }
            else if ((b & 0xE0) == 0xC0) { cp = b & 0x1F; extra = 1; }
            else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; extra = 2; }
            else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; extra = 3; }
            else { cp = b; extra = 0; }  // continuation byte alone → keep as-is
            i++;
            // Read continuation bytes
            for (int k = 0; k < extra && i < sv.size(); k++, i++) {
                cp = (cp << 6) | ((unsigned char)sv[i] & 0x3F);
            }
            put(row, c, cp, s);
            c++;
        }
    }

    void hline(int row, int col, int len, char32_t ch = U'─', Style s = Style::none()) {
        for (int i = 0; i < len; i++) put(row, col + i, ch, s);
    }
    void vline(int row, int col, int len, char32_t ch = U'│', Style s = Style::none()) {
        for (int i = 0; i < len; i++) put(row + i, col, ch, s);
    }
    void fill(int row, int col, int w, int h, char32_t ch, Style s = Style::none()) {
        for (int r = row; r < row + h && r < h_; r++)
            for (int c = col; c < col + w && c < w_; c++)
                put(r, c, ch, s);
    }

    // Draw a bordered box: ┌──┐ │  │ └──┘
    void border_rect(int row, int col, int w, int h, Style s = Style::none()) {
        if (w < 2 || h < 2) return;
        put(row, col, U'┌', s);  put(row, col+w-1, U'┐', s);
        put(row+h-1, col, U'└', s);  put(row+h-1, col+w-1, U'┘', s);
        hline(row, col+1, w-2, U'─', s);
        hline(row+h-1, col+1, w-2, U'─', s);
        vline(row+1, col, h-2, U'│', s);
        vline(row+1, col+w-1, h-2, U'│', s);
    }

    // Border with title embedded: ┌─ Title ──────┐
    void panel_header(int row, int col, int w, std::string_view title, Style border_s, Style title_s) {
        put(row, col, U'┌', border_s);
        put(row, col+w-1, U'┐', border_s);
        int pad = (w - 2 - (int)title.size()) / 2;
        if (pad < 0) pad = 0;
        hline(row, col+1, pad, U'─', border_s);
        if (col+1+pad < w_-1) text(row, col+1+pad, title, title_s);
        int rem = w - 2 - pad - (int)title.size();
        if (rem > 0) hline(row, col+1+pad+(int)title.size(), rem, U'─', border_s);
    }

    // ---- diff-based present ----
    void present() {
        std::string out;
        out.reserve(4096);
        int last_r = -1, last_c = -1;
        Style cur_style{};

        for (int r = 0; r < h_; r++) {
            for (int c = 0; c < w_; c++) {
                auto& cell = buf_[r * w_ + c];
                auto& prev = prev_[r * w_ + c];
                if (cell == prev) continue;

                // Move cursor if needed
                if (r != last_r || c != last_c) {
                    out += "\033[" + std::to_string(r+1) + ";" + std::to_string(c+1) + "H";
                    last_r = r; last_c = c;
                }
                // Apply style if changed
                if (!(cell.style == cur_style)) {
                    out += style_escape(cell.style);
                    cur_style = cell.style;
                }
                // Output the character (utf8)
                char32_to_utf8(cell.ch, out);
                last_c++;
            }
        }

        // Reset style at end
        if (!(cur_style == Style::none())) {
            out += "\033[0m";
        }

        if (!out.empty()) {
            [[maybe_unused]] auto _w = ::write(STDOUT_FILENO, out.data(), out.size());
        }

        prev_ = buf_;  // copy current to previous
    }

private:
    struct Cell {
        char32_t ch = U' ';
        Style style;
        bool operator==(const Cell& o) const { return ch == o.ch && style == o.style; }
    };

    int w_, h_;
    std::vector<Cell> buf_, prev_;

    static void char32_to_utf8(char32_t cp, std::string& out) {
        if (cp < 0x80) {
            out += (char)cp;
        } else if (cp < 0x800) {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    }

    static std::string style_escape(const Style& s) {
        // Reset then build
        if (s == Style::none()) return "\033[0m";
        std::string e = "\033[0";
        if (s.bold)      e += ";1";
        if (s.dim)       e += ";2";
        if (s.italic)    e += ";3";
        if (s.underline) e += ";4";
        if (s.reverse)   e += ";7";
        if (!s.fg.is_default) { e += ";38;2;" + std::to_string(s.fg.r) + ";" + std::to_string(s.fg.g) + ";" + std::to_string(s.fg.b); }
        if (!s.bg.is_default) { e += ";48;2;" + std::to_string(s.bg.r) + ";" + std::to_string(s.bg.g) + ";" + std::to_string(s.bg.b); }
        e += "m";
        return e;
    }
};

// ============================================================
// Key parser — read stdin, parse escape sequences
// ============================================================
inline Key read_key() {
    unsigned char c;
    int n = ::read(STDIN_FILENO, &c, 1);
    if (n <= 0) return Key_none;

    if (c == '\033') {
        // Might be escape sequence or bare Escape
        unsigned char seq[8];
        int m = ::read(STDIN_FILENO, seq, sizeof(seq));
        if (m <= 0) return Key_escape;  // bare ESC

        if (seq[0] == '[') {
            if (m >= 1) {
                switch (seq[1]) {
                    case 'A': return Key_up;
                    case 'B': return Key_down;
                    case 'C': return Key_right;
                    case 'D': return Key_left;
                    case 'H': return Key_home;
                    case 'F': return Key_end;
                    case 'Z': return Key_tab;  // shift-tab
                    case '3': return (m >= 2 && seq[2] == '~') ? Key_delete : Key_escape;
                    case '5': return (m >= 2 && seq[2] == '~') ? Key_page_up : Key_escape;
                    case '6': return (m >= 2 && seq[2] == '~') ? Key_page_down : Key_escape;
                    case '2': return (m >= 2 && seq[2] == '~') ? Key_insert : Key_escape;
                    case '1':
                        if (m >= 2 && seq[2] == '~') return Key_home;
                        if (m >= 2 && seq[2] == ';' && m >= 4) {
                            // F1-F4: ESC[1;2P etc.
                            if (seq[3] >= '0' && seq[3] <= '9')
                                return (Key)(Key_f1 + (seq[3] - '0') - 1);
                        }
                        return Key_home;  // ESC[1~ = home
                    case '4': return (m >= 2 && seq[2] == '~') ? Key_end : Key_escape;
                }
            }
        } else if (seq[0] == 'O') {
            if (m >= 1) {
                switch (seq[1]) {
                    case 'P': return Key_f1;  case 'Q': return Key_f2;
                    case 'R': return Key_f3;  case 'S': return Key_f4;
                }
            }
        }
        return Key_escape;  // unrecognized
    }

    // Printable or control
    if (c == '\n' || c == '\r') return Key_enter;
    if (c == '\t') return Key_tab;
    if (c == 127 || c == 8) return Key_backspace;  // DEL or BS

    return (Key)c;
}

// ---- Signal handling -------------------------------------------------------
inline volatile sig_atomic_t g_resize_flag = 0;
inline void sigwinch_handler(int) { g_resize_flag = 1; }

} // namespace fb
