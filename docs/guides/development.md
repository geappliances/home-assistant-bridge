# Development Guide

Build, test, and debug the GE Appliances Bridge.

## Prerequisites

### Build Tools

- **ESP-IDF** (v5.3+) — for firmware compilation. Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).
- **ESPHome** — for the build and flash workflow: `pip install esphome`.
- **Python 3.10+** — for pipeline scripts and tests.
- **CppUTest** — for unit tests:
  - Ubuntu/Debian: `sudo apt-get install cpputest libcpputest-dev`
  - macOS: `brew install cpputest`

### Clone

```bash
git clone --recursive https://github.com/joshualongenecker/home-assistant-bridge-esphome.git
cd home-assistant-bridge-esphome
```

The `--recursive` flag is required — the GEA protocol stack lives in `lib/tiny` and `lib/tiny-gea-api` submodules.

## Build

### Firmware (ESPHome)

```bash
esphome run gea-bridge.yaml
```

This compiles the component with ESP-IDF and flashes it to the connected adapter.

### Unit Tests (Host)

```bash
make test
```

Compiles and runs the CppUTest suite. Tests run on the host (not the ESP32) using stubs for ESP-IDF APIs.

### Python Tests

```bash
make pytest
```

Runs `test_pipeline.py`, `test_ha_discovery.py`, and `test_compress_ha_discovery.py`.

## Debug

### ESPHome Logs

```bash
esphome logs gea-bridge.yaml
```

Shows real-time serial output from the adapter.

### Enable Debug Logging

Add to your YAML:

```yaml
logger:
  level: DEBUG
  logs:
    geappliances_bridge: DEBUG
```

### GDB on ESP-IDF

```bash
idf.py monitor
# In the monitor: Ctrl+] to access serial port, then:
# monitor reset
# monitor gdb
```

In another terminal:

```bash
xtensa-esp32c3-elf-gdb build/gea-bridge.elf
(target remote) :3333
```

## Test Infrastructure

### Unit Tests (`test/`)

CppUTest-based tests for individual modules. Each module has a corresponding test file in `test/tests/`.

### Simulation Tests (`test/simulation/`)

Application-level integration tests that simulate complete workflows without physical hardware. Uses test doubles for the GEA3 ERD client, MQTT client, and timer system.

See [test/simulation/README.md](../../test/simulation/README.md) for details.

### Pipeline Tests (`scripts/`)

Python tests for the HA discovery pipeline:
- `test_pipeline.py` — validates pipeline stages produce correct output.
- `test_ha_discovery.py` — validates discovery payload generation.
- `test_compress_ha_discovery.py` — validates compression/decompression roundtrip.

## HA Discovery Pipeline

Any change to ERD definitions, overrides, or pipeline scripts requires a full pipeline rerun:

```bash
python3 scripts/ha_discovery/run_pipeline.py
```

This regenerates:
- `scripts/ha_discovery/appliance_api_erd_definitions_processed.json`
- `ha_discovery/*.jsonl`
- `components/geappliances_bridge/ha_discovery_data.h`

**Commit all generated files together with your changes.**

See [Pipeline Guide](./pipeline.md) for details on adding overrides and debugging pipeline issues.

## Code Structure

```
components/geappliances_bridge/   # Main component source
lib/tiny/                          # GEA protocol stack (submodule)
lib/tiny-gea-api/                  # GEA API abstractions (submodule)
test/                              # Unit tests
scripts/ha_discovery/              # HA discovery pipeline
docs/                              # Documentation
```

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for the PR process and code style.