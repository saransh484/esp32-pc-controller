# ESP32 Outbound-Poll OTA & Remote Host Controller

A lightweight, automated system for remote infrastructure management using an ESP32. This project implements a pull-based HTTPS firmware update architecture alongside an MQTT-driven host management interface.

## System Architecture

- **Hardware Interface:** ESP32 platform utilizing GPIO 4 (Status indicator toggle), GPIO 12 (Host Power), and GPIO 13 (Host Reset).
- **Firmware Management:** Client-driven HTTP polling loop utilizing the Arduino `HTTPUpdate` and `ArduinoJson` libraries. Updates are validated via metadata payloads and pulled down.
- **Telemetry & Control:** Asynchronous MQTT state machine for low-latency command execution and real-time operational status loops.
- **Alerting Engine:** Event-driven notification handling routing critical telemetry vectors directly to mobile endpoints via self-hosted `ntfy` infrastructure.

---

## Security & Secrets Decoupling

To prevent credential leakage in version control, all deployment targets, network criteria, and authorization keys are completely abstracted from the codebase using an SCons pre-build script (`read_env.py`).

### 1. Local Configuration

Environment variables are maintained in a local, untracked `.env` file at the root of the project workspace:

```ini
WIFI_SSID="Network_SSID"
WIFI_PASS="Network_WPA_Key"
MQTT_SERVER="broker.domain.tld"
MQTT_USER="system_user"
MQTT_PASS="system_password"
```

### 2. Compilation Matrix

The Python compilation script reads the `.env` context during execution and injects the parameters directly into the GCC compiler workspace as preprocessor macros (CPPDEFINES), ensuring string literals are never written to disk within raw source files.

To trigger a manual compilation profile:

```bash
pio run --target clean
pio run
```
