<!-- SPDX-License-Identifier: LicenseRef-FNCL-1.1 | Copyright (c) 2026 Cpt_Kirk -->
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

| Variant    | Device                                                  | Resolution | Factory image                                                                     |
|------------|---------------------------------------------------------|------------|-----------------------------------------------------------------------------------|
| `panel4`   | Waveshare **ESP32-P4-WIFI6-Touch-LCD-4B** (4")          | 720 × 720  | [betta86-ha-panel-v0.8.2-panel4.factory.bin](release/betta86-ha-panel-v0.8.2-panel4.factory.bin)   |
| `panel10`  | Waveshare **ESP32-P4 Module Nano + 10.1" DSI panel**    | 1280 × 800 | [betta86-ha-panel-v0.8.2-panel10.factory.bin](release/betta86-ha-panel-v0.8.2-panel10.factory.bin) |
| `panels3`  | Guition **ESP32-S3-4848S040** (4")                      | 480 × 480  | [betta86-ha-panel-v0.8.2-panels3.factory.bin](release/betta86-ha-panel-v0.8.2-panels3.factory.bin) |

All variants share the same dashboard engine, web editor, and Home Assistant integration. Pick the image that matches your board.

---

## Main features

- **Live Home Assistant link** — WebSocket connection with REST fallback for forecasts and long-poll states.
- **On-device editor** — BETTA Editor in the browser at `http://<panel-ip>`; drag-and-drop widgets, multi-page layouts, room-grouped entity picker.
- **Widget library** — sensor, button, slider, graph, light, heating, weather, up to 5 day weather forecast, media player, todo list, Roborock, energy dashboard, empty tile.
- **Advanced light control** — brightness, color temperature, RGB — exposed only when Home Assistant reports the capability.
- **Energy dashboard** — automatic grid / solar / battery / gas / water flow visualization driven by the Home Assistant energy model.
- **Graphs** — line, smoothed line, or bar-chart modes; event-rate sampling up to 4096 points with progressive decimation.
- **First-run provisioning** — `BETTA-Setup` Wi-Fi AP, guided Wi-Fi + Home Assistant setup, Quick Setup flow for a starter dashboard.
- **OTA updates** — upload an `.ota.bin` or point to an OTA URL from the web editor; no reflash required after v0.7.1.
- **Multilingual** — built-in English, German, Spanish, French; custom translation JSON upload/download.
- **Touch-friendly UX** — auto-dimming backlight after idle, pointer-capture drag/resize, stable GT911 touch startup.

---

## Getting started

1. **Download** the factory image for your board from the table above.
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

## What's new in v0.8.2

- **ESP32-S3 support** — new `panels3` variant for the Guition ESP32-S3-4848S040 (4.8" 480×480 RGB panel, 16 MB flash, 8 MB PSRAM).
- **Three-variant release** — factory and OTA images now ship for `panel4`, `panel10`, and `panels3`.
- **MDI weather icons on S3** — clean Material Design Icon weather display on the S3 panel.
- **Release tooling** — `make_factory_bin.ps1` extended; `-Variant both` now packages all three variants in one run.

Full history: [release-notes.md](release-notes.md).

---

## New features & fixes in this fork

Development on this fork extends the v0.8.2 baseline with a growing set of Home Assistant, editor, layout, weather, and reliability improvements.

### New features

- **Home Assistant script execution** — dashboard buttons can run Home Assistant scripts directly from the panel.
- **Expanded button modes** — button widgets can be configured for the appropriate Home Assistant action/entity behaviour.
- **Dynamic page sizing** — dashboard pages adapt more naturally to their content and available panel space.
- **Improved entity picker** — domain-aware discovery and search for lights, sensors, switches, weather, climate, todo, media players, vacuums, images, and other supported entities.
- **Enhanced widget configuration** — additional slider, graph, heating, button, and accent/style options in the web editor.
- **Graph improvements** — configurable line colour, time window, display mode, bar buckets, and point counts.
- **Heating tile variants** — additional heating display styles including semi/open arc layouts.
- **Weather improvements** — current conditions plus multi-day forecasts using Home Assistant's weather data, with provider-neutral handling of available high/low temperatures.
- **HA connection diagnostics** — the Settings page now exposes a rolling firmware-side Home Assistant connection history to make startup and reconnect problems easier to diagnose.
- **Improved save/reboot handling** — network and runtime state are cleaned up more carefully before rebooting after settings changes.

### Reliability fixes

- **WebSocket connection-state recovery** — BETTA now uses the ESP WebSocket client's native connection state as the source of truth, preventing a stale local state from trapping the panel in repeated failed authentication attempts.
- **Dead authentication-session recovery** — a failed native WebSocket is no longer protected as though an authentication handshake were still active; BETTA can tear it down and reconnect automatically.
- **Authentication TX diagnostics** — connection logs can report raw WebSocket send results, native connection state, free heap, and largest free memory block when diagnosing transport failures.
- **Safer boot settings loading** — runtime settings are no longer destructively reinitialised during normal startup retries.
- **Weather data correctness** — missing forecast highs/lows are no longer fabricated from the current temperature, and current-temperature handling has been hardened.
- **HTTP route and handler hardening** — improved robustness around web/API handlers used by the editor and diagnostics.

### Currently under investigation

- **Cold power-cycle HA startup delay on ESP32-P4/C6** — HA now self-recovers and connects, but a true cold boot can require several WebSocket reconnect attempts before the ESP-Hosted network path becomes fully usable. Warm starts connect immediately. Additional native transport-state diagnostics are being used to isolate the remaining startup delay.

---

## Building from source

Prerequisites: **ESP-IDF v5.5.2**, Python 3.11+, the Smart86 / Waveshare BSP components (pulled automatically via the component manager).

```powershell
# Pick a variant preset
idf.py -B build-panel4   -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.panel4"                              build
idf.py -B build-panel10  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.panel10"                             build
idf.py -B build-panels3  -DSDKCONFIG_DEFAULTS="sdkconfig.defaults.s3;sdkconfig.defaults.panels3" -DSDKCONFIG=sdkconfig.panels3  build

# Package release images (factory + OTA) for one or both variants
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
