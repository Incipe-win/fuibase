#pragma once
// ============================================================
// fuibase/widgets.hpp — immediate-mode TUI widgets
// ============================================================
#include "core.hpp"
#include <string_view>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace fb {

// ============================================================
// Helpers
// ============================================================
namespace detail {
    // Simple string hash for widget identity
    inline uint64_t hash_ptr(const void* p, uint32_t salt = 0) {
        auto x = (uint64_t)(uintptr_t)p;
        x ^= (uint64_t)salt << 32;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    inline int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

    // Widget state storage — keyed by widget identity (pointer hash)
    struct WidgetState {
        int cursor = 0;      // cursor position in text
        int scroll = 0;      // scroll offset
        int frame  = 0;      // animation frame counter
        int active = -1;     // active item index
    };
    inline auto& state_map() {
        static std::unordered_map<uint64_t, WidgetState> m;
        return m;
    }
    inline WidgetState& get_state(uint64_t id) { return state_map()[id]; }

    // Clock for animations
    inline int64_t tick_ms() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    // Truncate/ellipsize string to fit width
    inline std::string ellipsize(std::string_view s, int max_w) {
        if ((int)s.size() <= max_w) return std::string(s);
        if (max_w <= 3) return std::string(s.substr(0, max_w));
        return std::string(s.substr(0, max_w - 1)) + "…";
    }
} // namespace detail

// ============================================================
// Panel — bordered container. Returns inner Rect for children.
// ============================================================
inline Rect panel(Screen& g, std::string_view title, Rect bounds, Theme& theme) {
    Style border_s; border_s.fg = theme.border;
    Style title_s; title_s.fg = theme.accent; title_s.bold = true;

    // Top border with title
    g.panel_header(bounds.row, bounds.col, bounds.w, title, border_s, title_s);
    // Sides & bottom
    g.vline(bounds.row+1, bounds.col, bounds.h-2, U'│', border_s);
    g.vline(bounds.row+1, bounds.col+bounds.w-1, bounds.h-2, U'│', border_s);
    g.put(bounds.row+bounds.h-1, bounds.col, U'└', border_s);
    g.put(bounds.row+bounds.h-1, bounds.col+bounds.w-1, U'┘', border_s);
    g.hline(bounds.row+bounds.h-1, bounds.col+1, bounds.w-2, U'─', border_s);

    return {bounds.row+1, bounds.col+1, bounds.w-2, bounds.h-2};  // inner area
}

// ============================================================
// Text — styled text at position
// ============================================================
inline void text(Screen& g, std::string_view s, int row, int col, Style st = {}) {
    g.text(row, col, s, st);
}

// ============================================================
// Separator — horizontal line
// ============================================================
inline void separator(Screen& g, int row, int col, int w, Theme& theme) {
    Style s; s.fg = theme.border; s.dim = true;
    g.hline(row, col, w, U'─', s);
}

// ============================================================
// Key hints — footer bar
// ============================================================
inline void key_hints(Screen& g, int row, int col, int /*w*/,
                      const std::vector<std::pair<std::string, std::string>>& hints,
                      Theme& theme) {
    Style key_s; key_s.fg = theme.accent; key_s.bold = true;
    Style sep_s; sep_s.fg = theme.border; sep_s.dim = true;
    Style desc_s; desc_s.fg = theme.text_dim;

    int x = col;
    for (auto& [key, desc] : hints) {
        if (x > col) { g.text(row, x, " │ ", sep_s); x += 3; }
        g.text(row, x, key, key_s);  x += (int)key.size();
        g.text(row, x, " ", desc_s); x += 1;
        g.text(row, x, desc, desc_s); x += (int)desc.size();
    }
}

// ============================================================
// Spinner — animated braille dots
// ============================================================
inline void spinner(Screen& g, std::string_view label, int row, int col, Theme& theme) {
    static const char* frames[] = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
    auto ms = detail::tick_ms();
    int frame = (ms / 80) % 10;  // 80ms per frame

    Style spin_s; spin_s.fg = theme.accent; spin_s.bold = true;
    Style text_s; text_s.fg = theme.text_dim;

    g.text(row, col, frames[frame], spin_s);
    g.text(row, col + 2, label, text_s);
}

// ============================================================
// ProgressBar — ████████░░░░ 75%
// ============================================================
inline void progress_bar(Screen& g, float pct, int row, int col, int width, Theme& theme) {
    pct = std::clamp(pct, 0.0f, 1.0f);
    int filled = (int)std::round(pct * width);

    Style fill_s; fill_s.fg = theme.success; fill_s.bold = true;
    Style empty_s; empty_s.fg = theme.text_dim;
    Style pct_s; pct_s.fg = theme.text;

    for (int i = 0; i < width; i++) {
        g.put(row, col+i, i < filled ? U'█' : U'░', i < filled ? fill_s : empty_s);
    }
    char buf[16];
    std::snprintf(buf, sizeof(buf), " %d%%", (int)std::round(pct * 100));
    g.text(row, col + width + 1, buf, pct_s);
}

// ============================================================
// Checkbox — [x] / [ ] toggle. Returns true if toggled this frame.
// ============================================================
inline bool checkbox(Screen& g, std::string_view label, bool& value,
                     int row, int col, Key key, Theme& theme) {
    Style box_s;  box_s.fg = value ? theme.success : theme.text_dim;
    Style text_s; text_s.fg = theme.text;
    Style focus; focus.fg = theme.primary;

    // Draw
    std::string box = value ? "[✓]" : "[ ]";
    g.text(row, col, box, box_s);
    if (label.data()) g.text(row, col + 4, label, text_s);

    // Handle
    if (key == Key_space || key == Key_enter) {
        value = !value;
        return true;
    }
    return false;
}

// ============================================================
// RadioGroup — single-select from list. Returns index of newly selected, or -1.
// ============================================================
inline void radio_group(Screen& g,
                         const std::vector<std::string_view>& options,
                         int& selected,
                         int row, int col, Key key, Theme& theme) {
    Style sel_s;   sel_s.fg = theme.accent; sel_s.bold = true;
    Style unsel_s; unsel_s.fg = theme.text_dim;
    Style text_s;  text_s.fg = theme.text;

    int r = row;
    for (int i = 0; i < (int)options.size(); i++, r++) {
        bool is_sel = (i == selected);
        g.text(r, col, is_sel ? " ● " : " ○ ", is_sel ? sel_s : unsel_s);
        g.text(r, col + 4, options[i], is_sel ? text_s : unsel_s);
    }

    if (key == Key_up   && selected > 0)                     selected--;
    if (key == Key_down && selected < (int)options.size()-1)  selected++;
}

// ============================================================
// Input — single-line text input. Returns true if Enter pressed.
// ============================================================
inline bool input(Screen& g,
                  std::string_view label,
                  std::string& value,
                  int row, int col, int width, Key key, Theme& theme,
                  bool focused = true) {
    auto id = detail::hash_ptr(&value);
    auto& st = detail::get_state(id);

    if (!focused) { key = Key_none; }
    st.cursor = detail::clamp(st.cursor, 0, (int)value.size());

    // Handle key
    if (key == Key_left)        st.cursor = std::max(0, st.cursor - 1);
    else if (key == Key_right)  st.cursor = std::min((int)value.size(), st.cursor + 1);
    else if (key == Key_home)   st.cursor = 0;
    else if (key == Key_end)    st.cursor = (int)value.size();
    else if (key == Key_backspace || key == Key_delete) {
        if (key == Key_backspace && st.cursor > 0) {
            value.erase(st.cursor - 1, 1);
            st.cursor--;
        } else if (key == Key_delete && st.cursor < (int)value.size()) {
            value.erase(st.cursor, 1);
        }
    } else if (key_is_printable((Key)key)) {
        value.insert(st.cursor, 1, (char)key);
        st.cursor++;
    }

    // Scroll to keep cursor visible
    int field_w = width - (int)label.size() - 2;  // 2 for spacing
    if (field_w < 4) field_w = 4;
    int visible_start = st.scroll;
    if (st.cursor < visible_start) visible_start = st.cursor;
    if (st.cursor >= visible_start + field_w) visible_start = st.cursor - field_w + 1;
    st.scroll = visible_start;

    // Draw
    Style label_s; label_s.fg = theme.text_dim;
    Style text_s;  text_s.fg = theme.text;
    Style cursor_s; cursor_s.reverse = true; cursor_s.bg = theme.primary; cursor_s.fg = Color::hex(0x1a1b26);

    g.text(row, col, label, label_s);
    int field_col = col + (int)label.size() + 1;

    // Visible text slice
    std::string_view visible = std::string_view(value).substr(visible_start, field_w);
    g.text(row, field_col, visible, text_s);
    // Fill rest
    for (int i = (int)visible.size(); i < field_w; i++)
        g.put(row, field_col + i, U' ', text_s);

    // Cursor (only shown when focused)
    if (focused) {
        int cursor_vis = st.cursor - visible_start;
        if (cursor_vis >= 0 && cursor_vis < field_w) {
            char32_t ch = (cursor_vis < (int)visible.size()) ? (unsigned char)visible[cursor_vis] : U' ';
            g.put(row, field_col + cursor_vis, ch, cursor_s);
        }
    }

    return key == Key_enter;
}

// ============================================================
// SelectList — scrollable list with search
// ============================================================
inline int select_list(Screen& g,
                       const std::vector<std::string>& items,
                       int& selected,
                       int row, int col, int width, int height,
                       Key key, Theme& theme) {
    if (items.empty()) return -1;

    selected = detail::clamp(selected, 0, (int)items.size() - 1);
    auto id = detail::hash_ptr(&selected, 1);
    auto& st = detail::get_state(id);
    int& scroll = st.cursor;  // reuse cursor field for scroll

    // Scroll to keep selected visible
    if (selected < scroll) scroll = selected;
    if (selected >= scroll + height) scroll = selected - height + 1;
    scroll = std::max(0, std::min(scroll, (int)items.size() - height));

    // Draw items
    Style norm_s;   norm_s.fg = theme.text_dim;
    Style sel_s;    sel_s.fg = theme.text; sel_s.bold = true;
    Style mark_s;   mark_s.fg = theme.accent; mark_s.bold = true;
    Style row_hl;   row_hl.bg = theme.surface;

    for (int i = 0; i < height && scroll + i < (int)items.size(); i++) {
        int idx = scroll + i;
        bool is_sel = (idx == selected);
        if (is_sel)
            for (int x = 0; x < width; x++) g.put(row + i, col + x, U' ', row_hl);
        g.text(row + i, col, is_sel ? "❯" : " ", mark_s);
        auto label = detail::ellipsize(items[idx], width - 4);
        g.text(row + i, col + 2, label, is_sel ? sel_s : norm_s);
    }

    // Navigation
    if (key == Key_up)        selected--;
    if (key == Key_down)      selected++;
    if (key == Key_page_up)   selected -= height;
    if (key == Key_page_down) selected += height;
    if (key == Key_home)      selected = 0;
    if (key == Key_end)       selected = (int)items.size() - 1;
    selected = detail::clamp(selected, 0, (int)items.size() - 1);

    // Enter = confirm selection
    if (key == Key_enter) return selected;
    return -1;  // no confirmation
}

// ============================================================
// Button — clickable text. Returns true when activated.
// ============================================================
inline bool button(Screen& g, std::string_view label, int row, int col,
                   Key key, Theme& theme, bool is_focused = false) {
    Style btn_s;
    if (is_focused) { btn_s.fg = theme.bg; btn_s.bg = theme.primary; btn_s.bold = true; }
    else            { btn_s.fg = theme.primary; btn_s.bold = true; }

    int w = (int)label.size() + 4;  // padding: [ Label ]
    g.text(row, col, "[ ", btn_s);
    g.text(row, col+2, label, btn_s);
    g.text(row, col+w-2, " ]", btn_s);

    return is_focused && key == Key_enter;
}

// ============================================================
// Marquee — indeterminate scanning progress bar
// ============================================================
inline void marquee(Screen& g, std::string_view label, int row, int col, int width, Theme& theme) {
    auto ms = detail::tick_ms();
    int pos = (ms / 100) % (width * 2);  // scan back and forth

    Style fill_s; fill_s.fg = theme.primary;
    Style empty_s; empty_s.fg = theme.text_dim;
    Style label_s; label_s.fg = theme.text_dim;

    // 3-char wide block that scans across
    int bar_start, bar_end;
    if (pos < width) {
        bar_start = pos;
        bar_end = std::min(pos + 6, width);
    } else {
        bar_start = std::max(0, width * 2 - pos - 6);
        bar_end = std::min(bar_start + 6, width);
    }

    for (int i = 0; i < width; i++) {
        bool in = (i >= bar_start && i < bar_end);
        float alpha = in ? 1.0f - (float)std::abs(i - bar_start - 3) / 6.0f : 0.0f;
        if (alpha > 0.3f) g.put(row, col+i, U'█', fill_s);
        else              g.put(row, col+i, U'░', empty_s);
    }

    if (!label.empty()) g.text(row, col + width + 2, label, label_s);
}

// ============================================================
// TaskList — show a list of items with status icons
// ============================================================
enum class TaskState : uint8_t { Pending, Running, Done, Error };

struct TaskItem {
    std::string name;
    TaskState state = TaskState::Pending;
};

inline void task_list(Screen& g,
                      const std::vector<TaskItem>& tasks,
                      int row, int col, int max_h, Theme& theme) {
    for (int i = 0; i < (int)tasks.size() && i < max_h; i++) {
        auto& t = tasks[i];
        Style icon_s, text_s;
        std::string icon;
        switch (t.state) {
            case TaskState::Done:    icon = " ✓ "; icon_s.fg = theme.success; break;
            case TaskState::Running: icon = " ⠋ "; icon_s.fg = theme.accent; icon_s.bold = true; break;
            case TaskState::Error:   icon = " ✗ "; icon_s.fg = theme.error; break;
            default:                 icon = " ○ "; icon_s.fg = theme.text_dim; break;
        }
        text_s.fg = (t.state == TaskState::Pending) ? theme.text_dim : theme.text;

        g.text(row + i, col, icon, icon_s);
        g.text(row + i, col + 4, t.name, text_s);
    }
}

// ============================================================
// StatusBar — bottom-of-screen status line
// ============================================================
inline void status_bar(Screen& g, std::string_view left, std::string_view right,
                       int row, int w, Theme& theme) {
    Style bar_s; bar_s.reverse = true; bar_s.bg = theme.surface;
    Style text_s; text_s.fg = theme.text; text_s.reverse = true; text_s.bg = theme.surface;
    Style dim_s;  dim_s.fg = theme.text_dim; dim_s.reverse = true; dim_s.bg = theme.surface;

    // Fill entire row
    for (int c = 0; c < w; c++) g.put(row, c, U' ', bar_s);
    g.text(row, 2, left, text_s);
    if (!right.empty()) {
        int rc = w - 2 - (int)right.size();
        if (rc > 0) g.text(row, rc, right, dim_s);
    }
}

} // namespace fb
