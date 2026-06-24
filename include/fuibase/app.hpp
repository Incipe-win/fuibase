#pragma once
// ============================================================
// fuibase/app.hpp — App event loop
// ============================================================
#include "core.hpp"
#include <functional>
#include <string>

namespace fb {

// ============================================================
// App — owns the event loop, Screen, and Terminal lifecycle
// ============================================================
class App {
public:
    using RenderFn = std::function<void(Screen& g, Key key, Theme& theme)>;

    App()  { Terminal::raw_mode(); Terminal::hide_cursor(); resize_screen(); }
    ~App() { Terminal::show_cursor(); Terminal::restore(); Terminal::clear(); }

    // Set the render callback
    void on_render(RenderFn fn) { render_ = std::move(fn); }

    // Quit the event loop
    void quit() { running_ = false; }

    // Run the main loop (blocks until quit)
    int run() {
        if (!render_) return 1;

        // Install SIGWINCH handler
        auto old_winch = std::signal(SIGWINCH, sigwinch_handler);
        auto old_int   = std::signal(SIGINT,  [](int){ Terminal::show_cursor(); Terminal::restore(); ::_exit(130); });

        // Clear any previous terminal content before first frame
        Terminal::clear();
        running_ = true;
        while (running_) {
            // Handle resize
            if (g_resize_flag) {
                g_resize_flag = 0;
                resize_screen();
                render_(*screen_, Key_resize, theme_);
                screen_->present();
                // Read a second time to flush the resize event
                Key k;
                do { k = read_key(); } while (k != Key_none);
                continue;
            }

            // Read input with timeout (for animation ticks)
            Key key = read_key_with_timeout(50);  // 50ms = 20fps for animations

            // Render
            screen_->clear();
            render_(*screen_, key, theme_);
            screen_->present();
        }

        std::signal(SIGWINCH, old_winch);
        std::signal(SIGINT, old_int);
        return 0;
    }

    Screen& screen() { return *screen_; }
    Theme& theme()   { return theme_; }

private:
    Theme theme_ = Theme::tokyo_night();
    std::unique_ptr<Screen> screen_;
    RenderFn render_;
    bool running_ = false;

    void resize_screen() {
        auto sz = Terminal::size();
        if (screen_) screen_->resize(sz.w, sz.h);
        else screen_ = std::make_unique<Screen>(sz.w, sz.h);
    }

    // select()-based timeout read — returns Key_none on timeout
    static Key read_key_with_timeout(int ms) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        timeval tv{ms / 1000, (ms % 1000) * 1000};
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0)
            return read_key();
        return Key_none;
    }
};

} // namespace fb
