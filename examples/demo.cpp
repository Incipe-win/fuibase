// ============================================================
// fuibase demo — showcases all widgets
// Build: g++ -std=c++20 -o demo examples/demo.cpp -Iinclude
// ============================================================
#include <fuibase/fuibase.hpp>
#include <cstdio>

int main() {
    using namespace fb;

    App app;

    // ---- App state ----
    bool show_advanced = false;
    std::string name = "";
    std::string email = "";
    int theme_choice = 0;
    int selected_tool = 0;
    std::vector<std::string> tools = {
        "nvim   — hyperextensible text editor",
        "rg     — recursively search directories",
        "fd     — fast and user-friendly find",
        "bat    — cat with syntax highlighting",
        "delta  — syntax-highlighting pager",
        "lazygit— simple terminal UI for git",
        "fzf    — fuzzy finder",
        "zoxide — smarter cd command",
        "eza    — modern replacement for ls",
        "tmux   — terminal multiplexer",
    };

    // Focus management: 0=name, 1=email, 2=ok button
    int focus = 0;
    bool done = false;

    app.on_render([&](Screen& g, Key key, Theme& theme) {
        int w = g.width();
        int h = g.height();

        // ---- Title bar ----
        {
            Style title_s; title_s.fg = theme.accent; title_s.bold = true;
            g.text(0, (w - 20) / 2, "✦ fuibase demo ✦", title_s);
        }

        // ---- Main panel ----
        Rect panel_r = rect_at(2, 2, w - 4, h - 6);
        auto inner = panel(g, "Setup", panel_r, theme);
        int r = inner.row, c = inner.col, iw = inner.w;

        // Section: Info
        text(g, "User Information", r, c, style::heading({theme.text}));
        r += 2;

        // Name input
        text(g, "Name", r, c, {theme.text_dim});
        input(g, "", name, r, c + 5, std::min(40, iw - 6),
              focus == 0 ? key : Key_none, theme);
        r += 2;

        // Email input
        text(g, "Email", r, c, {theme.text_dim});
        input(g, "", email, r, c + 6, std::min(40, iw - 7),
              focus == 1 ? key : Key_none, theme);
        r += 2;

        // Toggle advanced
        checkbox(g, "Show advanced options", show_advanced, r, c,
                 focus == 3 ? key : Key_none, theme);
        r += 2;

        if (show_advanced) {
            separator(g, r, c, iw, theme);
            r++;

            text(g, "Theme", r, c, style::heading({theme.text}));
            r++;

            // Radio group
            std::vector<std::string_view> themes = {"Tokyo Night", "Catppuccin", "Nord", "Gruvbox"};
            radio_group(g, themes, theme_choice, r, c,
                       focus == 4 ? key : Key_none, theme);
            r += (int)themes.size() + 1;

            // Spinner demo
            text(g, "Loading state demo: ", r, c, {theme.text_dim});
            spinner(g, "Processing...", r, c + 20, theme);
            r += 2;

            // Progress bar demo
            text(g, "Progress: ", r, c, {theme.text_dim});
            // Animate progress based on frame count
            float pct = (float)((detail::tick_ms() / 500) % 100) / 100.0f;
            progress_bar(g, pct, r, c + 10, 30, theme);
            r += 2;
        }

        r += 1;
        separator(g, r, c, iw, theme);
        r += 1;

        // Tool list
        text(g, "Available Tools", r, c, style::heading({theme.text}));
        r++;
        select_list(g, tools, selected_tool, r, c, iw - 2, 6,
                    focus == 5 ? key : Key_none, theme);
        r += 8;

        // Buttons row
        r += 1;
        bool ok_focus = (focus == 6);
        if (button(g, "  OK  ", r, c,
                   ok_focus ? key : Key_none, theme, ok_focus))
            done = true;
        if (button(g, " Cancel ", r, c + 10,
                   focus == 7 ? key : Key_none, theme, focus == 7))
            app.quit();

        // ---- Key hints ----
        key_hints(g, h - 2, 2, w - 4, {
            {"↑↓", "navigate"},
            {"Tab", "next field"},
            {"Enter", "confirm"},
            {"q/Esc", "quit"},
        }, theme);

        // ---- Navigation (Tab/Shift-Tab) ----
        if (key == Key_tab)       focus = (focus + 1) % 8;
        if (key == Key_escape || key == Key_q) app.quit();
        if (done) app.quit();
    });

    int ret = app.run();

    // Print results
    std::printf("\n  Name:  %s\n  Email: %s\n  Theme: %d\n  Tool:  %s\n\n",
                name.c_str(), email.c_str(), theme_choice,
                selected_tool < (int)tools.size() ? tools[selected_tool].c_str() : "");

    return ret;
}
