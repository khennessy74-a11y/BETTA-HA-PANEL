<!-- SPDX-License-Identifier: LicenseRef-FNCL-1.1 | Copyright (c) 2026 Cpt_Kirk | Copyright (c) 2026 khennessy74-a11y -->
<img src="images/BETTAOS.jpg" alt="BETTA OS Logo" width="10%" />

# BETTA HA Panel

A runtime-configurable Home Assistant wall panel for ESP32-P4 and ESP32-S3 touchscreen devices. Build your dashboard directly on the device — no YAML edits, no firmware rebuilds.

<p float="left">
  <img width="32%" alt="Page settings" src="https://github.com/user-attachments/assets/9caf6e2b-6ea9-4b76-b404-1b58da822712" />
  <img width="32%" alt="Media Player & Roborock" src="https://github.com/user-attachments/assets/c31df7b8-f7e7-461a-98af-80070eac0392" />
  <img width="32%" alt="Widget inspector" src="https://github.com/user-attachments/assets/97be77c3-0716-4641-994c-efe90c929953" />
</p>
---

## Supported hardware

BETTA HA Panel ships as **three firmware variants**, one per supported device:

| Variant    | Device                                                  | Resolution | v0.9 factory image |
|------------|---------------------------------------------------------|------------|--------------------|
| `panel4`   | Waveshare **ESP32-P4-WIFI6-Touch-LCD-4B** (4")       | 720 × 720  | `betta-ha-panel-v0.9-panel4.factory.bin` |
| `panel10`  | Waveshare **ESP32-P4 Module Nano + 10.1" DSI panel** | 1280 × 800 | `betta-ha-panel-v0.9-panel10.factory.bin` |
| `panels3`  | Guition **ESP32-S3-4848S040** (4")                   | 480 × 480  | `betta-ha-panel-v0.9-panels3.factory.bin` |

All variants share the same dashboard engine, web editor, and Home Assistant integration. Pick the image that matches your board.

---

## Main features

- **Live Home Assistant link** — WebSocket connection with REST fallback for forecasts and long-poll states.
- **On-device editor** — BETTA Editor in the browser at `http://<panel-ip>`; drag-and-drop widgets, multi-page layouts, room-grouped entity picker.
- **Widget library** — sensor, button, slider, graph, light, heating, weather with adaptive multi-day forecasting, media player, todo list, Roborock, energy dashboard, empty tile.
- **Advanced light control** — brightness, color temperature, RGB — exposed only when Home Assistant reports the capability.
- **Energy dashboard** — automatic grid / solar / battery / gas / water flow visualization driven by the Home Assistant energy model.
- **Graphs** — line, smoothed line, or bar-chart modes; event-rate sampling up to 4096 points with progressive decimation.
- **First-run provisioning** — `BETTA-Setup` Wi-Fi AP, guided Wi-Fi + Home Assistant setup, Quick Setup flow for a starter dashboard.
- **OTA updates** — upload an `.ota.bin` or point to an OTA URL from the web editor; no reflash required after v0.7.1.
- **Multilingual** — built-in English, German, Spanish, French; custom translation JSON upload/download.
- **Touch-friendly UX** — auto-dimming backlight after idle, scheduled Night Mode, pointer-capture drag/resize, stable GT911 touch startup.
- **On-panel System & Diagnostics** — open diagnostics from the status area to inspect firmware, Wi-Fi and Home Assistant connection state/logs, restart safely, and adjust display settings without a browser.
- **On-panel display controls** — brightness, Night Mode schedules and Auto Dim settings with explicit Save behaviour and automatic return to Home after inactivity.
- **Resilient configured boot** — saved Wi-Fi/Home Assistant configuration is retained when the network or Home Assistant is unavailable during startup; the panel continues to the dashboard and retries recovery instead of dropping into provisioning.

---

## Getting started

1. **Download** the factory image for your board. During v0.9 release-candidate validation, use the matching artifact from the [Build BETTA HA Panel](../../actions/workflows/build-panels3.yml) workflow. Once v0.9 is published, the same filenames will be attached to the GitHub Release.
2. **Flash** it with any ESP32 flasher — for example the browser-based [esptool-js](https://espressif.github.io/esptool-js/):
   - Use the outer USB-C port.
   - Baud rate `115200`, flash offset `0x0`.
   - The factory image includes the ESP32-C6 network coprocessor firmware.
3. **Reboot** the device. It opens a Wi-Fi AP called `BETTA-Setup`.
4. Connect to `BETTA-Setup`, open `http://192.168.4.1`, pick your country, scan for your network and save.
5. After reboot the panel joins your LAN. Open its IP in a browser, link Home Assistant via long-lived access token, and build your first page with **Quick Setup**.

Future updates install via OTA from the editor — no cable needed.

<img width="1080" alt="image" src="https://github.com/user-attachments/assets/cda02fa3-8270-48c2-bde8-987ebf069c36" />
<img width="1080" alt="image" src="https://github.com/user-attachments/assets/5d72053d-2889-413f-bc8a-f2170ea1e889" />
<img width="1080" alt="image" src="https://github.com/user-attachments/assets/9cbd3880-07da-4e14-b81c-d3e52cfd8dad" />



---

## What's new in v0.9 (Release Candidate)

- **Home Assistant actions** — dashboard buttons can run scripts directly, with expanded action/button modes.
- **Night Mode + idle dimming** — configurable day/night brightness, automatic schedules, wake-time refresh, and coordinated idle dimming.
- **Richer widget controls** — custom MDI icons, ON/OFF icon colours, sensor precision, cover controls, graph options, heating styles, and responsive compact controls.
- **Editor improvements** — dynamic page sizing, broader domain-aware entity discovery/search, and better inspector/runtime parity.
- **Reliability and diagnostics** — stronger WebSocket recovery, HA connection diagnostics, weather correctness fixes, and improved Save & Reboot handling.
- **Three-variant release CI** — `panel4`, `panel10`, and `panels3` now build and package versioned factory + OTA images in the same validation workflow.

The hardware table lists this fork's v0.9 release filenames. Until the first v0.9 GitHub Release is published, release-candidate binaries are available from the build workflow artifacts. Hardware validation is still in progress, including extended observation for intermittent display twitching and final reconnect/weather persistence checks.

Full history: [release-notes.md](release-notes.md).

---

## New features & fixes in this fork

The v0.9 development line extends the v0.8.2 baseline with Home Assistant, editor, layout, display, weather, and reliability improvements.

### New features

- **Home Assistant script execution** — dashboard buttons can run Home Assistant scripts directly from the panel.
- **Expanded button modes** — button widgets can be configured for the appropriate Home Assistant action/entity behaviour.
- **Dynamic page sizing** — dashboard pages adapt more naturally to their content and available panel space.
- **Improved entity picker** — domain-aware discovery and search for lights, sensors, switches, weather, climate, todo, media players, vacuums, images, and other supported entities.
- **Enhanced widget configuration** — additional slider, graph, heating, button, and accent/style options in the web editor.
- **Graph improvements** — configurable line colour, time window, display mode, bar buckets, and point counts.
- **Heating tile variants** — additional heating display styles including semi/open arc layouts.
- **Weather improvements** — current conditions plus adaptive multi-day forecasts backed by a normalized weather model, with provider-neutral high/low handling, observed-temperature range extension, and date-keyed persistence of authoritative Today extrema.
- **HA connection diagnostics** — the Settings page now exposes a rolling firmware-side Home Assistant connection history to make startup and reconnect problems easier to diagnose.
- **Improved save/reboot handling** — network and runtime state are cleaned up more carefully before rebooting after settings changes.

### Recent development progress

- **Custom MDI icon parity** — custom icons now work across the supported entity-backed tiles, including lights, fans, updates, calendars, selects, input helpers, alarms, graphs, todo lists, media players, Roborock, sensors, timers, sliders, heating, and runnable script/scene/automation buttons. Weather keeps its dedicated condition-driven icon system.
- **Configurable state icon colours** — supported tiles can use separate ON and OFF icon colours while preserving theme defaults and Home Assistant light RGB behaviour where appropriate.
- **Sensor precision control** — sensor tiles can configure the number of decimal places shown on the panel.
- **Cover controls** — cover sliders support position and tilt where Home Assistant exposes those capabilities, with open/stop/close actions and movement-aware control states.
- **Responsive compact tiles** — button geometry and controls for Select, Input Number, Fan, Update, Input Text, Input Date/Time, and Heating tiles have been tightened so smaller dashboard cards remain usable without changing normal-size layouts.
- **Night Mode and idle dimming** — configurable day/night brightness, automatic scheduled transitions, wake-time schedule refresh, and inactivity dimming now work together without allowing idle mode to brighten an already-dimmer night display.
- **Editor parity cleanup** — widget options now better match runtime capabilities; unsupported generic custom-icon controls are hidden for Weather while its dynamic weather icons remain intact.
- **Hardware validation** — the current development line has been repeatedly built, flashed, and exercised on-panel, including physical-touch inactivity return, protected Admin entry, themed restart confirmation, repeated software reboots, weather model validation, and display/connection recovery testing. Extended intermittent-twitch observation remains open before final release.

### Reliability fixes

- **WebSocket connection-state recovery** — BETTA now uses the ESP WebSocket client's native connection state as the source of truth, preventing a stale local state from trapping the panel in repeated failed authentication attempts.
- **Dead authentication-session recovery** — a failed native WebSocket is no longer protected as though an authentication handshake were still active; BETTA can tear it down and reconnect automatically.
- **Authentication TX diagnostics** — connection logs can report raw WebSocket send results, native connection state, free heap, and largest free memory block when diagnosing transport failures.
- **Safer boot settings loading** — runtime settings are no longer destructively reinitialised during normal startup retries.
- **Weather data correctness** — missing forecast highs/lows are no longer fabricated from the current temperature, and current-temperature handling has been hardened.
- **HTTP route and handler hardening** — improved robustness around web/API handlers used by the editor and diagnostics.

### Final RC validation

- **Cold-start recovery test** — the configured-panel boot path has been changed so a temporary Wi-Fi/Home Assistant outage cannot force an already-configured panel back into provisioning. Final RC validation includes powering up with the network unavailable, restoring it, and confirming automatic recovery with settings intact.
- **Global inactivity return** — secondary dashboard/System/Display screens return Home after 60 seconds of inactivity, with the timer reset from actual touch events across nested controls.
- **Restart confirmation UX** — the on-panel restart action uses a themed, centred modal with vertically stacked actions, touch-through protection, and a dimmed background.

---

## Building from source

Prerequisites: **ESP-IDF v5.5.2**, Python 3.11+, the Smart86 / Waveshare BSP components (pulled automatically via the component manager).

```powershell
# Pick a variant preset
idf.py -B build-panel4   -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.panel4"                              build
idf.py -B build-panel10  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.panel10"                             build
idf.py -B build-panels3  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.s3;sdkconfig.defaults.panels3" -DSDKCONFIG=sdkconfig.panels3  build

# Package release images (factory + OTA) for the supported variants
pwsh tools/make_factory_bin.ps1 -Variant both
```

Build artifacts land in `release/` and `release/ota/`. Previous versions are moved to `release/archive/`.

---

## Editor preview


<p>
  <img width="49%" alt="Energy dashboard" src="https://github.com/user-attachments/assets/96d51c1d-743a-4c7a-b2c6-29f2cff1e41f" />
  <img width="49%" alt="languages" src="https://github.com/user-attachments/assets/93702c1a-994c-4390-bfbc-6b4eb6b9d779" />
</p>
<p>
  <img width="49%" alt="theme editor" src="https://github.com/user-attachments/assets/2c016e94-bd7e-4c4e-a4e5-2826e6ba6269" />
  <img width="49%" alt="OTA firmware" alt="image" src="https://github.com/user-attachments/assets/f655f102-47dc-4881-aed1-3e6a8cd451a4" />

</p>

---

## Support development of this fork

If the additions and ongoing development in this fork are useful to you, you can support Kieran's work:

[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Support%20this%20fork-FFDD00?logo=buy-me-a-coffee&logoColor=000000)](https://buymeacoffee.com/khennessy74)


## License

Source released under [LicenseRef-FNCL-1.1](LICENSE). See [release-notes.md](release-notes.md) for per-version changes.
