# Contributing

## Development Workflow

### Prerequisites

- **ESP-IDF** (for firmware builds)
- **Python 3.10+** (for pipeline scripts and tests)
- **CppUTest** (for unit tests):
  - Ubuntu/Debian: `sudo apt-get install cpputest libcpputest-dev`
  - macOS: `brew install cpputest`

### Clone with Submodules

```bash
git clone --recursive https://github.com/joshualongenecker/home-assistant-bridge-esphome.git
cd home-assistant-bridge-esphome
```

### Build and Test

```bash
# Run unit tests
make test

# Run Python tests
make pytest
```

See [Development Guide](./docs/guides/development.md) for the full workflow including ESPHome compilation and simulation testing.

## Pull Request Process

1. **Fork and branch** from `develop`.
2. **Make your changes.** Follow existing patterns — grep the codebase before inventing a new convention.
3. **Run tests.** `make test` and `make pytest` must pass.
4. **Run the pipeline** if you modified ERD definitions, overrides, or pipeline scripts:
   ```bash
   python3 scripts/ha_discovery/run_pipeline.py
   ```
   Commit all generated files (`ha_discovery/*.jsonl`, `components/geappliances_bridge/ha_discovery_data.h`).
5. **Update documentation** if your change affects behavior, configuration, or architecture. See [docs/style-guide.md](./docs/style-guide.md).
6. **Open a PR** against `develop` with a clear description of what changed and why.

## Code Style

- **C++**: Follow the existing codebase style (4-space indent, braces on same line, `camelCase` for functions, `snake_case` for variables).
- **C**: Same as C++ but use `snake_case` for all identifiers.
- **Python**: PEP 8.
- **No trailing whitespace.** No lines > 120 characters in prose.

## Documentation

All documentation lives in `docs/`. The [style guide](./docs/style-guide.md) defines formatting conventions and the spec template.

**Before committing any behavioral change, update the relevant spec.** Specs are the single source of truth for module contracts.

## Adding Overrides to the HA Discovery Pipeline

If an ERD needs a manual override (wrong domain, missing unit, incorrect scaling), add it to `OVERRIDES` in `scripts/ha_discovery/pipeline/post_process.py`. See [.github/copilot-instructions.md](./.github/copilot-instructions.md) for override format and valid keys.

After adding an override, run the pipeline and commit all generated files.