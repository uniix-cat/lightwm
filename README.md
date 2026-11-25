# LightWM

An "out of the box" Wayland desktop environment built on top of **River (0.5+)**, **C++20**, and **Qt 6 Quick (QML)**.

## Architecture

- **`lightwm-session` / `lightwm-init`**: Wayland session entrypoint for Display Managers (`/usr/share/wayland-sessions/lightwm.desktop`). Launches `river` and initializes D-Bus, portals, WM, and shell.
- **`lightwm_core`**: Shared config (`~/.config/lightwm/config.json`), themes (`cream` and `pine`), and Unix socket IPC schema.
- **`lightwm-wm`**: Pure C++20 `river-window-management-v1` client with Hyprland-style Dwindle BSP tiling, floating mode, `Super + LMB/RMB` interactive move/resize, per-output workspace model, XKB keybindings, and libinput/xkb configuration.
- **`lightwm-shell`**: Unified Qt6 Quick + `LayerShellQt` shell (top bar, workspaces, SNI tray, app launcher, Wi-Fi/Bluetooth/audio/brightness popovers, notifications, OSD, hybrid QML + `grim` screenshots, native `ext-session-lock-v1` lockscreen).
- **`lightwm-settings`**: Pure Qt6 Quick settings application (no `QtWidgets`) sharing the live config and theme engine.

## Built-in Themes

1. **Cream (Light)**: Warm cream `#FFFDD0` (`rgb(255, 253, 208)`) with soft yellow accents `#FFEC8B` (`rgb(255, 236, 139)`).
2. **Pine (Dark)**: Slate gray surfaces with pine-green accents.

## Build

### Arch Linux
```bash
# Вариант 1: собрать и установить пакет
cd packaging && makepkg -si

# Вариант 2: зависимости вручную
sudo pacman -S --needed cmake ninja river qt6-base qt6-declarative qt6-wayland layer-shell-qt wayland wayland-protocols libxkbcommon libpulse pam libinput grim wl-clipboard
```

### Void Linux
```bash
sudo xbps-install -S cmake ninja pkg-config river qt6-base-devel qt6-declarative-devel qt6-wayland-devel layer-shell-qt-devel wayland-devel wayland-protocols libxkbcommon-devel pulseaudio-devel pam-devel libinput-devel eudev-libudev-devel grim wl-clipboard
```

### Fedora
```bash
sudo dnf install cmake ninja-build gcc-c++ pkgconf-pkg-config qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtdeclarative-devel qt6-qtwayland-devel layer-shell-qt-devel wayland-devel wayland-protocols-devel libxkbcommon-devel pulseaudio-libs-devel pam-devel libinput-devel systemd-devel grim wl-clipboard
# Требуется River 0.5+ и river-protocols (из COPR или из исходников)
```

### Сборка и установка
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

## License

GNU General Public License v3.0 ([LICENSE](LICENSE)).
