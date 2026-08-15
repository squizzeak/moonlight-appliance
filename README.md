# Moonlight Appliance

Turn a small EndeavourOS PC into a controller-first Moonlight client while
retaining a usable KDE Plasma desktop for local maintenance.

## Initial project iteration

This repository currently represents the project's **initial packaging
commit**. Installation is deliberately source-based and manual so every system
change remains visible and reviewable while the supported configuration is
still being established.

Future iterations will automate package checks, bridge compilation, file and
service installation, configuration validation, upgrades, and clean removal.
Until that automation is implemented and tested, follow the manual steps below
and review each command before running it. Progress toward the automated setup
is tracked in
[improvement issue #1](https://github.com/squizzeak/moonlight-appliance/issues/1).

For task-oriented documentation, see the
[project Wiki](https://github.com/squizzeak/moonlight-appliance/wiki). The
README remains authoritative for installation and safety guidance.

> [!WARNING]
> **Development provenance and disclaimer:** Everything in this project was
> vibe-coded using Codex with GPT-5.6 Sol. The controller daemon, custom
> keyboard, native bridge, system integration, documentation, and packaging of
> this GitHub repository were all produced through that same AI-assisted
> workflow. The project has been iteratively tested on one appliance, but it
> has not received an independent security audit or broad hardware testing.
> Review the source and commands before running them, keep a recovery path, and
> use the project at your own risk.

## Features

- Controller-driven local mouse when Moonlight is not running
- Per-controller evdev axis calibration, deadzones, reconnect support, and
  simultaneous-controller safety
- Home/Guide launches or focuses Moonlight on a short **release**
- Long Home holds and disconnects never accidentally launch Moonlight
- No physical controller grab: Moonlight continues receiving the real gamepad
- All synthetic local input stops whenever Moonlight exists
- Full custom QWERTY on-screen keyboard with function, navigation, arrow, and
  modifier keys
- Sticky and multi-modifier Ctrl, Alt, Shift, Super, and AltGr support
- Resolution- and scale-aware keyboard occupying exactly the bottom third of
  the display
- Wayland layer-shell placement: always on top, no focus stealing, and an
  exclusive work area above the keyboard
- Controller application switcher, focused-window close gesture, and KDE
  Overview-safe Moonlight launching
- Bluetooth gamepad disconnect on system sleep

## Tested platform

- EndeavourOS / Arch Linux
- KDE Plasma 6 on Wayland
- Native `moonlight-qt`
- Xbox-style Bluetooth and USB controllers exposing Linux evdev gamepad codes
- 4K display with Plasma scaling

Other Arch-based Plasma systems may work, but the current instructions and
service behavior are developed and tested on EndeavourOS. The controller must
expose `BTN_MODE` through evdev to be discovered.

## 1. Install EndeavourOS with KDE Plasma

1. Download the EndeavourOS ISO from <https://endeavouros.com/latest-release/>.
2. Write it to a USB drive and boot the installer.
3. Choose the online installation and select **KDE Plasma** as the desktop.
4. Create a normal user account with administrator (`sudo`) access.
5. Boot the installed system and log into a **Plasma (Wayland)** session.

The project assumes a normal graphical user session and installs user services;
do not run the controller programs as root.

## 2. Update the system and install packages

First update official repository packages:

```bash
sudo pacman -Syu
```

EndeavourOS normally includes `yay`. Update installed AUR packages as well:

```bash
yay -Sua
```

Install the official repository dependencies:

```bash
sudo pacman -S --needed \
  base-devel bluez-utils cmake dbus gcc git layer-shell-qt \
  moonlight-qt python-evdev python-pyqt6 qt6-base qt6-tools
```

Install `kdotool` from the AUR:

```bash
yay -S --needed kdotool
```

What these provide:

| Package | Purpose |
| --- | --- |
| `moonlight-qt` | Native Moonlight client |
| `python-evdev` | Passive controller input and virtual uinput devices |
| `python-pyqt6` | Custom on-screen keyboard UI |
| `layer-shell-qt`, `qt6-base` | Bottom-layer keyboard surface and build headers |
| `qt6-tools` | `qdbus6`, used to close KDE Overview safely |
| `kdotool` | Find, focus, and close KWin windows on Wayland |
| `bluez-utils` | `bluetoothctl` for sleep-time Bluetooth disconnect |
| `dbus` | Monitor logind's `PrepareForSleep` signal |
| `cmake`, `gcc`, `base-devel` | Build the keyboard layer-shell bridge |
| `git` | Obtain and update this project |

Reboot after a full system upgrade if the kernel, graphics stack, Qt, or Plasma
was updated.

## 3. Configure the appliance desktop

### Automatic login

In **System Settings → Colors & Themes → Login Screen (SDDM) → Behavior**, enable
automatic login for the appliance user and choose the Plasma session.

Automatic login reduces physical security. Use it only on a dedicated appliance
where that tradeoff is acceptable.

### Disable screen locking and unwanted display sleep

In **System Settings → Security & Privacy → Screen Locking**:

- disable automatic locking;
- disable locking after waking from sleep.

In **System Settings → Power Management**, configure inactivity and display
power behavior appropriate for the television/appliance. In particular, disable
automatic display sleep if the client must remain continuously visible.

Disabling the lock screen means anyone with physical access can use the logged-in
session.

### Auto-hide the Plasma panel

Right-click the panel, enter **Edit Mode**, open **More Options**, and set panel
visibility to **Auto Hide**.

### Optional Moonlight autostart

The Home button can launch Moonlight, so autostart is optional. To start it at
every login:

```bash
mkdir -p ~/.config/autostart
cp /usr/share/applications/com.moonlight_stream.Moonlight.desktop \
  ~/.config/autostart/
```

## 4. Clone the project

```bash
mkdir -p ~/Documents/git
git clone https://github.com/squizzeak/moonlight-appliance.git \
  ~/Documents/git/moonlight-appliance
cd ~/Documents/git/moonlight-appliance
```

The complete keyboard implementation is included under `src/` and `keyboard/`.
It is not `wvkbd`, and no external keyboard source or patch series is required.

## 5. Build the keyboard layer-shell bridge

The Python keyboard uses a small C++ shared library to configure its Qt window
as a native Wayland layer-shell surface.

```bash
cmake -S keyboard -B build/keyboard -DCMAKE_BUILD_TYPE=Release
cmake --build build/keyboard --parallel
```

Install the resulting library for the current user:

```bash
install -Dm755 \
  build/keyboard/libmoonlight-keyboard-layer.so \
  ~/.local/lib/libmoonlight-keyboard-layer.so
```

Rebuild this bridge after incompatible Qt or `layer-shell-qt` ABI upgrades.

## 6. Install the controller programs

Install both complete source programs into the location expected by the units:

```bash
install -Dm755 src/moonlight-home-button \
  ~/.local/bin/moonlight-home-button
install -Dm755 src/moonlight-controller-keyboard \
  ~/.local/bin/moonlight-controller-keyboard
```

The main daemon passively reads controllers and creates a virtual mouse. The
keyboard program creates a separate virtual keyboard. Neither program should be
run with `sudo`.

### Verify uinput access

Load the kernel module and check access:

```bash
sudo modprobe uinput
test -w /dev/uinput && echo "uinput is writable"
```

Plasma installations commonly grant the active local user access through a
`uaccess` udev rule. If the test prints nothing, create one:

```bash
sudo install -Dm644 /dev/stdin /etc/udev/rules.d/70-moonlight-uinput.rules <<'EOF'
KERNEL=="uinput", SUBSYSTEM=="misc", TAG+="uaccess", OPTIONS+="static_node=uinput"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger --name-match=uinput
```

Log out and back in, then repeat the writability test. Do not make `/dev/uinput`
world-writable.

## 7. Install and enable the user services

```bash
install -Dm644 systemd/user/moonlight-controller-keyboard.service \
  ~/.config/systemd/user/moonlight-controller-keyboard.service
install -Dm644 systemd/user/moonlight-home-button.service \
  ~/.config/systemd/user/moonlight-home-button.service

systemctl --user daemon-reload
systemctl --user enable --now moonlight-controller-keyboard.service
systemctl --user enable --now moonlight-home-button.service
```

Verify both services:

```bash
systemctl --user status moonlight-controller-keyboard.service
systemctl --user status moonlight-home-button.service
```

Follow logs with:

```bash
journalctl --user -u moonlight-controller-keyboard.service -f
journalctl --user -u moonlight-home-button.service -f
```

## Controller reference

These mappings apply only while Moonlight is absent. Once the launcher or a
stream exists, the daemon stops producing synthetic local input.

### Desktop controls

| Control | Action |
| --- | --- |
| Left stick | Move pointer |
| Right stick | Vertical and horizontal scroll |
| A | Left click |
| Short B | Right click |
| Hold B for 1 second | Close the focused window |
| Right shoulder | Left click, including while the keyboard is visible |
| Left shoulder | Right click, including while the keyboard is visible |
| D-pad | Arrow keys |
| Select | Show or hide the keyboard |
| Short Home release | Launch Moonlight or focus its launcher |
| Home held for 2+ seconds | No local action; safe for controller power-off |

### Application switcher

- Hold Y for 350 ms to open KDE's Alt-Tab switcher.
- Use D-pad Left, Right, or Up to change the highlighted application.
- Press D-pad Down to close the highlighted application. The daemon briefly
  selects and closes it, then reopens the switcher if Y remains held.
- Release Y to select the highlighted application.
- A quick Y tap has no local action.

### On-screen keyboard

| Control | Action |
| --- | --- |
| D-pad | Move highlighted key |
| Hold D-pad | Repeated navigation |
| A | Type highlighted key |
| X | Backspace |
| Short B | Hide keyboard |
| L3 / left-stick press | Toggle sticky Shift |
| Select | Hide keyboard |

Shift, Ctrl, Alt, Super, and AltGr keys on the keyboard are sticky. Multiple
modifiers can be selected together, such as Ctrl+Shift. Hiding the keyboard
clears all sticky modifiers.

The keyboard hides automatically when Moonlight appears, the last controller
disconnects, no controller is connected at service startup, or the system
begins sleeping.

## Bluetooth sleep behavior

The main service watches logind's `PrepareForSleep` signal and runs
`bluetoothctl disconnect` for each currently tracked Bluetooth controller.
This powered off the tested Bluetooth Xbox controller.

USB controllers and dongles are deliberately untouched. USB authorization does
not necessarily remove VBUS power, and many hubs cannot switch port power.

To disable Bluetooth disconnects while retaining keyboard hide-on-sleep, edit
`~/.local/bin/moonlight-home-button`:

```python
DISCONNECT_BLUETOOTH_ON_SLEEP = False
```

Then restart the service:

```bash
systemctl --user restart moonlight-home-button.service
```

## Updating a manual installation

```bash
cd ~/Documents/git/moonlight-appliance
git pull --ff-only
cmake --build build/keyboard --parallel

install -Dm755 build/keyboard/libmoonlight-keyboard-layer.so \
  ~/.local/lib/libmoonlight-keyboard-layer.so
install -Dm755 src/moonlight-home-button ~/.local/bin/moonlight-home-button
install -Dm755 src/moonlight-controller-keyboard \
  ~/.local/bin/moonlight-controller-keyboard
install -Dm644 systemd/user/*.service ~/.config/systemd/user/

systemctl --user daemon-reload
systemctl --user restart moonlight-controller-keyboard.service
systemctl --user restart moonlight-home-button.service
```

## Architecture and safety notes

- Physical controllers are never grabbed with `EVIOCGRAB`.
- Synthetic input is enabled only when no Moonlight-class KWin window exists.
- Unknown Moonlight captions fail safely by disabling local synthetic input.
- The keyboard does not accept focus; uinput keystrokes go to the application
  that already has focus.
- KWin window detection uses class `com.moonlight_stream.Moonlight`; active
  streams are recognized by captions ending in ` - Moonlight`.
- The keyboard's exclusive bottom-third layer surface reduces the usable
  desktop work area while visible.

## Repository layout

```text
src/
  moonlight-home-button             controller daemon
  moonlight-controller-keyboard     complete custom keyboard
keyboard/
  CMakeLists.txt                    bridge build
  layer_shell_bridge.cpp            Wayland layer-shell integration
systemd/user/
  moonlight-home-button.service
  moonlight-controller-keyboard.service
```

## License

This project is licensed under the GNU Affero General Public License v3.0; see
`LICENSE`.
