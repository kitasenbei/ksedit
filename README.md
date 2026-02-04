# ksedit

A fast, minimal text editor for Linux using raw X11. No GTK, no Qt, no SDL - just pure Xlib.

## Features

- Syntax highlighting (C/C++)
- Undo/redo
- Find and goto line
- Mouse selection with scroll support
- Position memory (jump back/forward)
- ~50KB binary, ~16MB RAM

## Building

```bash
make
```

Requires: `libX11-devel` (Fedora) or `libx11-dev` (Debian/Ubuntu)

## Keybindings

### File
| Key | Action |
|-----|--------|
| Ctrl+S | Save |
| Ctrl+Q | Quit |

### Navigation
| Key | Action |
|-----|--------|
| Ctrl+G | Goto line |
| Ctrl+F | Find |
| Ctrl+Home/End | Jump to file start/end |
| Ctrl+Left/Right | Jump by word |
| Alt+Left/Right | Jump back/forward (position history) |

### Editing
| Key | Action |
|-----|--------|
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+D | Duplicate line |
| Ctrl+K | Delete line |
| Alt+Up/Down | Move line up/down |
| Ctrl+Backspace | Delete word backward |
| Ctrl+Delete | Delete word forward |

### Selection
| Key | Action |
|-----|--------|
| Shift+Arrows | Select |
| Ctrl+Shift+Left/Right | Select by word |
| Ctrl+A | Select all |
| Ctrl+C | Copy |
| Ctrl+X | Cut |
| Ctrl+V | Paste |
| Double-click | Select word |
| Triple-click | Select line |

### View
| Key | Action |
|-----|--------|
| Ctrl+Scroll | Zoom in/out |

## License

MIT
