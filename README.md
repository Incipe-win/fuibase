# fuibase

> A modern C++20 immediate-mode TUI framework — header-only, zero dependencies, zero bloat.

Build terminal interfaces that don't look like they're from 1985. Checkboxes, input fields, select lists, spinners, progress bars — all with a polished Tokyo Night aesthetic out of the box.

```cpp
#include <fuibase/fuibase.hpp>
using namespace fb;

int main() {
    App app;
    bool opt = false;
    std::string name;

    app.on_render([&](Screen& g, Key key, Theme& theme) {
        g.clear();
        auto inner = panel(g, "Example", rect_at(1, 1, 50, 12), theme);
        checkbox(g, "Enable feature", opt, inner.row, inner.col, key, theme);
        input(g, "Name", name, inner.row+2, inner.col, 30, key, theme);
        if (button(g, "OK", inner.row+5, inner.col, key, theme, true))
            app.quit();
    });
    return app.run();
}
```

## Design

**Immediate mode** — no widget tree, no retained state, no virtual dispatch. Every frame, you call widget functions that render themselves and handle input. State lives in your variables. Think Dear ImGui, but for the terminal.

**Double-buffered** — the Screen renders to an off-screen cell buffer and diffs against the previous frame. Only changed cells reach the terminal. Flicker-free, minimal output.

**Tokyo Night palette** — default theme designed for dark terminals. Swap themes with a struct literal.

## Widgets

| Widget | What it does |
|--------|-------------|
| `panel()` | Bordered container with title |
| `text()` | Styled text |
| `checkbox()` | `[✓]` / `[ ]` toggle |
| `radio_group()` | ` ● ` / ` ○ ` single-select |
| `input()` | Single-line text entry with cursor |
| `select_list()` | Scrollable list with keyboard nav |
| `spinner()` | Animated braille-dot spinner |
| `progress_bar()` | `█████░░░░░ 75%` |
| `button()` | Clickable `[ Label ]` |
| `key_hints()` | Footer keybinding bar |
| `separator()` | Horizontal rule |
| `marquee()` | Animated scanning progress bar |
| `task_list()` | List of items with status icons |
| `status_bar()` | Full-width status line |

## Using as a dependency

```bash
# Clone alongside your project
git clone https://github.com/you/fuibase.git ../fuibase

# Or as a git submodule
git submodule add https://github.com/you/fuibase.git external/fuibase

# Then in your build:
g++ -std=c++20 -O2 -o myapp src/*.cpp -Iexternal/fuibase/include
```

Or with xmake:
```lua
add_includedirs("external/fuibase/include")
```

Or with CMake:
```cmake
add_subdirectory(external/fuibase)
target_link_libraries(myapp fuibase)
```

## Building

```bash
# Option 1: Single compile (no build system needed)
g++ -std=c++20 -O2 -o myapp myapp.cpp -Iinclude

# Option 2: xmake
xmake

# Option 3: CMake
mkdir build && cd build && cmake .. && make
```

Requires: **C++20** (g++ 10+ or clang 14+), Linux/macOS.

## Why immediate mode?

Retained-mode TUI libraries make you subclass Widget, manage ownership, and think about layout engines. Immediate mode flips this: you write a single lambda that redraws the entire UI every frame. The framework handles diffing and output. Less code, fewer concepts, faster iteration.

## License

MIT
