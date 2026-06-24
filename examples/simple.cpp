// ============================================================
// fuibase minimal example — checkbox list (like dotrix setup)
// Build: g++ -std=c++20 -o simple examples/simple.cpp -Iinclude
// ============================================================
#include <fuibase/fuibase.hpp>

int main() {
    using namespace fb;

    struct Tool { std::string name, desc; bool checked = false, installed = false; };
    std::vector<Tool> tools = {
        {"nvim",    "neovim editor",           false, true},
        {"rg",      "ripgrep",                 false, true},
        {"fd",      "fast find",               false, false},
        {"bat",     "cat with highlighting",   true,  false},
        {"lazygit", "terminal git UI",         false, false},
        {"fzf",     "fuzzy finder",            true,  true},
        {"eza",     "modern ls",               false, false},
        {"zoxide",  "smarter cd",              false, false},
        {"delta",   "better git diff",         false, false},
        {"tmux",    "terminal multiplexer",    false, true},
    };

    int cursor = 0, scroll = 0;
    bool done = false;

    App app;
    app.on_render([&](Screen& g, Key key, Theme& theme) {
        int w = g.width(), h = g.height();
        auto in = panel(g, "Select tools to install", rect_at(1, 1, w-2, h-2), theme);

        // Scroll
        int vis_h = in.h - 1;
        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + vis_h) scroll = cursor - vis_h + 1;
        scroll = std::max(0, std::min(scroll, (int)tools.size() - vis_h));

        // Keys
        if (key == Key_up || key == Key_k)    cursor--;
        if (key == Key_down || key == Key_j)  cursor++;
        if (key == Key_space)                 tools[cursor].checked ^= 1;
        if (key == Key_a) for (auto& t : tools) t.checked = true;
        if (key == Key_n) for (auto& t : tools) if (!t.installed) t.checked = false;
        if (key == Key_enter)                 done = true;
        if (key == Key_escape || key == Key_q) app.quit();
        cursor = std::clamp(cursor, 0, (int)tools.size() - 1);

        // Render
        Style sel_bg; sel_bg.reverse = true;
        for (int i = 0; i < vis_h && scroll + i < (int)tools.size(); i++) {
            int idx = scroll + i;
            auto& t = tools[idx];

            if (idx == cursor)
                for (int x = 0; x < in.w; x++) g.put(in.row + i, in.col + x, U' ', sel_bg);

            Style box_s; box_s.fg = t.checked ? theme.success : theme.text_dim;
            if (t.installed) box_s.dim = true;
            g.text(in.row + i, in.col + 1, t.checked ? "[x]" : "[ ]", box_s);
            g.text(in.row + i, in.col + 5, t.name, t.installed ? Style{theme.text_dim, {}, false, true} : Style{theme.text});
            g.text(in.row + i, in.col + (int)t.name.size() + 7, t.desc, Style{theme.text_dim, {}, false, true});
            if (t.installed) g.text(in.row + i, in.col + in.w - 14, "(installed)", Style{theme.text_dim, {}, false, true});
        }

        if (done) app.quit();
    });

    app.run();

    std::printf("\nSelected:\n");
    for (auto& t : tools)
        if (t.checked && !t.installed)
            std::printf("  - %s\n", t.name.c_str());
}
