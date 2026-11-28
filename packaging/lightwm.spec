Name:           lightwm
Version:        0.1.0
Release:        1%{?dist}
Summary:        Out-of-the-box Wayland desktop environment built on River and Qt6 Quick
License:        GPL-3.0-or-later
URL:            https://github.com/uniix-cat/lightwm
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.24
BuildRequires:  gcc-c++
BuildRequires:  ninja-build
BuildRequires:  pkgconf-pkg-config
BuildRequires:  layer-shell-qt-devel
BuildRequires:  libinput-devel
BuildRequires:  libxkbcommon-devel
BuildRequires:  pam-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtbase-private-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtwayland-devel
BuildRequires:  river-protocols-devel >= 0.5.0
BuildRequires:  systemd-devel
BuildRequires:  wayland-devel
BuildRequires:  wayland-protocols-devel

Requires:       river >= 0.5.0
Requires:       grim
Requires:       wl-clipboard
Requires:       qt6-qtwayland
Requires:       layer-shell-qt
Requires:       NetworkManager
Requires:       bluez
Recommends:     foot

%description
LightWM is an out-of-the-box Wayland desktop environment built on top of
River (0.5+), C++20, and Qt 6 Quick (QML), featuring Dwindle BSP tiling,
a unified LayerShellQt desktop shell, native ext-session-lock lockscreen,
fractional scaling via wlr-output-management, and a Qt Quick settings app.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/lightwm-wm
%{_bindir}/lightwm-shell
%{_bindir}/lightwm-settings
%{_bindir}/lightwm-session
%{_bindir}/lightwm-init
%{_datadir}/wayland-sessions/lightwm.desktop
%{_datadir}/applications/lightwm-settings.desktop
%{_prefix}/lib/udev/rules.d/71-lightwm-touchpad.rules

%changelog
* Sat Nov 28 2026 uniix-cat <uniix-cat@users.noreply.github.com> - 0.1.0-1
- Initial Fedora RPM / COPR package with fractional scaling support
