# Known Issues — LinShot 1.4.0 Beta

> LinShot is beta software. The keybinding system works reliably on **Linux Mint (Cinnamon)** but has known limitations on other desktop environments. This document tracks those issues and their severity.

---

## Keybinding Registration — Cross-DE Reliability

**Filed:** 2026-03-29
**Severity:** High
**Affects:** GNOME, XFCE, MATE, KDE — all Debian-based distros running these DEs
**Component:** `src/keybinding_manager.c`, `src/main_window.c`

### Summary

LinShot's "Set as default screenshot app" feature registers a system-wide keybinding (default: PrintScreen) to launch `linshot --capture`. This registration uses DE-specific APIs (gsettings, dconf, xfconf-query, XGrabKey). While the Cinnamon path has been hardened through multiple bug fixes, the other DE paths have unresolved issues that may cause silent failures, data loss, or conflicts.

### Per-DE Status

| Desktop Environment | Distros | Status | Key Issues |
|---|---|---|---|
| **Cinnamon** | Linux Mint | **Working** | Fully hardened — stale binding purge, D-Bus reload, cross-schema cleanup |
| **GNOME** | Ubuntu, Pop!_OS, Debian | **Bugs** | Overwrites user's custom keybindings; missing GNOME 42+ support |
| **XFCE** | Xubuntu, MX Linux | **Partial** | Does not disable xfce4-screenshooter; may not override default bindings |
| **MATE** | Ubuntu MATE | **Partial** | Does not work with Compiz; doesn't disable mate-screenshot |
| **KDE Plasma** | Kubuntu, KDE Neon | **Limited** | X11-only fallback; no kglobalaccel integration; fails on Wayland |
| **Other/Unknown** | i3, Openbox, Sway, etc. | **Fragile** | Raw XGrabKey; fails if compositor grabs key first; no Wayland |

---

### Issue 1: GNOME — Custom Keybindings List Overwrite (Critical)

**Severity:** Critical — data loss
**Affects:** Ubuntu 20.04+, Pop!_OS, Debian GNOME

When LinShot registers its keybinding on GNOME, it **replaces the entire `custom-keybindings` list** with only its own entry. Any custom keyboard shortcuts the user previously configured in GNOME Settings are silently removed from the active list.

**Root cause:** `gnome_register()` sets:
```
org.gnome.settings-daemon.plugins.media-keys custom-keybindings = ['/…/linshot/']
```
instead of appending to the existing list.

**Workaround:** Do not enable "Set as default screenshot app" on GNOME if you have custom keyboard shortcuts. Instead, manually add the keybinding in GNOME Settings → Keyboard → Custom Shortcuts.

---

### Issue 2: GNOME 42+ — Incomplete Screenshot Disabling

**Severity:** Medium
**Affects:** Ubuntu 22.04+, Fedora 36+, Debian 12+

GNOME 42 replaced `gnome-screenshot` with a built-in screenshot UI (`org.gnome.shell.keybindings show-screenshot-ui`). LinShot disables the old `screenshot` key but not `show-screenshot-ui`, so GNOME's built-in tool may still intercept PrintScreen.

**Workaround:** Manually disable GNOME's screenshot UI:
```bash
gsettings set org.gnome.shell.keybindings show-screenshot-ui '[]'
```

---

### Issue 3: XFCE — Default Binding Not Overridden

**Severity:** Medium
**Affects:** Xubuntu, MX Linux, Linux Lite

LinShot adds a custom XFCE shortcut but does not remove `xfce4-screenshooter`'s default PrintScreen binding. Both may fire simultaneously, or XFCE's default may take precedence.

**Workaround:** Manually remove the default in XFCE Settings → Keyboard → Application Shortcuts.

---

### Issue 4: MATE — Compiz Incompatibility

**Severity:** Medium
**Affects:** Ubuntu MATE with Compiz window manager

LinShot writes keybindings to `org.mate.Marco.global-keybindings`, but users running Compiz (selectable via MATE Tweak) use a different keybinding system. The registration succeeds silently but has no effect.

Additionally, `mate-screenshot`'s own binding is not disabled, so it may intercept the key.

**Workaround:** Use Marco (not Compiz) as your window manager, or bind manually via CompizConfig Settings Manager.

---

### Issue 5: KDE Plasma — No Native Integration

**Severity:** High
**Affects:** Kubuntu, KDE Neon, any Plasma desktop

LinShot falls back to raw X11 `XGrabKey` on KDE, which:
- Cannot override Spectacle's PrintScreen binding (owned by `kglobalaccel5` at the compositor level)
- Does not work at all on Wayland sessions (KDE defaults to Wayland on many distros)
- Provides no error feedback if the grab fails

**Workaround:** Manually unbind PrintScreen from Spectacle in KDE System Settings → Shortcuts → Spectacle, then add a custom shortcut for `linshot --capture`.

---

### Issue 6: No Error Reporting

**Severity:** Medium
**Affects:** All DEs

All `gsettings`, `dconf`, and `xfconf-query` commands run via `system()` with errors redirected to `/dev/null`. If registration fails (missing tools, permission denied, schema not found), the user receives no feedback — the checkbox appears to work but the keybinding is not active.

---

### Issue 7: Cinnamon — custom0 Collision

**Severity:** Low
**Affects:** Linux Mint (Cinnamon)

LinShot always uses the `custom0` slot for its keybinding. If the user already has a custom keybinding named `custom0` configured in Cinnamon's Keyboard Settings, LinShot will overwrite it.

**Workaround:** If you have existing custom keybindings, rename them to avoid the `custom0` slot before enabling LinShot's keybinding.

---

### Issue 8: Wayland — No Support

**Severity:** Medium (increasing over time)
**Affects:** All DEs on Wayland sessions

LinShot's keybinding system is entirely X11-based. The XGrabKey fallback and GDK event filter do nothing on Wayland. DE-native registration (gsettings/dconf) still works for Cinnamon/GNOME/XFCE/MATE to launch the command, but LinShot's own X11 screen capture will need porting to the XDG Desktop Portal screenshot API for Wayland.

Linux Mint currently defaults to X11 but is actively developing Wayland support for Cinnamon. Ubuntu has defaulted to Wayland since 21.04.

---

## Recommendations for Users

1. **Linux Mint (Cinnamon):** Use "Set as default screenshot app" — it works reliably
2. **All other DEs:** Configure the keybinding manually through your DE's keyboard settings, pointing to `linshot --capture`
3. **KDE/Wayland users:** Manual configuration is required; the automatic setup will not work
4. **GNOME users:** Do NOT use the automatic setup if you have existing custom keyboard shortcuts — it will erase them

## For Developers

These issues are tracked for resolution in future releases. The Cinnamon path (`cinnamon_register`/`cinnamon_unregister`) serves as the reference implementation for how each DE path should work:
- Clear stale entries before registering
- Disable the DE's built-in screenshot handler
- Force the DE to reload (not just write to dconf)
- Clean up fully on unregister (restore defaults)
- Report errors to the user

---

## Clipboard — Image Not Pastable Into Terminal / Non-GTK Applications

**Filed:** 2026-08-24
**Status:** Resolved (packaging fix — no application code change)
**Severity:** High — a core feature appears broken
**Affects:** Any Debian/Ubuntu/Mint install that does not already have `xclip`
**Component:** `install.sh`, `debian/control`, `packaging/build-deb.sh`

### Symptom

After a capture, LinShot reports "Image with annotations copied to clipboard" and pasting into
another GTK application (GIMP, Firefox) works. But pasting into a terminal-based or non-GTK
consumer — a CLI agent, an Electron app, a terminal emulator's image paste — yields nothing. The
clipboard looks like it was only partially populated.

### Root cause

**Not a defect in `copy_to_clipboard()`.** LinShot publishes the image correctly:

- `gtk_clipboard_set_image()` takes ownership of the X11 `CLIPBOARD` selection.
- `gtk_clipboard_store()` hands the payload to the session clipboard manager (`csd-clipboard` on
  Cinnamon), so the image survives LinShot exiting.
- GTK advertises the full image target set: `image/png`, `image/bmp`, `image/jpeg`, `image/tiff`,
  `image/webp`, alongside `TARGETS` / `SAVE_TARGETS` / `MULTIPLE` / `TIMESTAMP`.

The gap is on the **reading** side. GTK applications retrieve the selection through GDK directly.
Non-GTK consumers almost universally shell out to `xclip` (X11) or `wl-paste` (Wayland) to pull
`image/png` off the clipboard. Neither ships in a default Debian/Ubuntu/Mint desktop install, and
LinShot did not declare either — so on a clean system those consumers had no way to read the
selection, and failed silently.

### Verification

Measured on Linux Mint (Cinnamon, X11) against a live capture:

| Check | Result |
|---|---|
| Targets advertised after capture | `image/png`, `image/bmp`, `image/jpeg`, `image/tiff`, `image/webp` |
| `xclip -selection clipboard -t image/png -o` | valid PNG, 606x306, RGBA |
| Saved file `LinShot_20260824_155332.png` | 606x306, RGB |
| Pixel diff, clipboard vs saved file | **0 mismatches across all 185,436 pixels** |
| Image survives LinShot exiting | yes — clipboard manager retains it |

The clipboard payload is pixel-for-pixel the same screenshot that was written to disk.

### Fix

Declare the clipboard bridge so it is present on every install:

- `install.sh` — added `xclip` to the `apt install` line
- `debian/control` — `Recommends: xdg-utils, xclip`
- `packaging/build-deb.sh` — same `Recommends` in the generated control file

### Fix for an existing install

```bash
sudo apt install xclip
```

No rebuild needed — this is a runtime dependency, not a code change. Take a new screenshot and the
paste will work.

### Wayland note

The equivalent bridge on Wayland is `wl-clipboard` (`wl-paste`). LinShot's capture path is X11-only
today (see Issue 8 above), so this only becomes relevant once Wayland capture lands.
