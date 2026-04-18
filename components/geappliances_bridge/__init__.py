"""ESPHome component for GE Appliances Bridge."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import esp32, uart, mqtt
from esphome.const import (
    CONF_ID,
)
from esphome.core import CORE
import json
import re
import logging
from pathlib import Path

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@joshualongenecker"]
DEPENDENCIES = ["uart", "mqtt"]
AUTO_LOAD = []

# UART configuration keys
CONF_GEA3_UART_ID = "gea3_uart_id"
CONF_GEA2_UART_ID = "gea2_uart_id"

# Bridge (MQTT) configuration keys
CONF_ADAPTER_ADDRESS = "adapter_address"
CONF_DEVICE_ID = "device_id"
CONF_MODE = "mode"
CONF_POLLING_INTERVAL = "polling_interval"
CONF_POLLING_ONLY_PUBLISH_ON_CHANGE = "polling_onlypublish_onchange"
CONF_APPLIANCE_API_PARSING = "appliance_api_parsing"
CONF_CUSTOM_ERDS = "custom_erds"
CONF_GENERATE_DEVICE_CONFIG = "generate_device_config"
CONF_HA_DISCOVERY_BASE_URL = "ha_discovery_base_url"

# Default base URL for the per-category JSONL files used by runtime HA discovery.
HA_DISCOVERY_DEFAULT_BASE_URL = (
    "https://raw.githubusercontent.com/joshualongenecker/"
    "home-assistant-bridge-esphome/copilot/implement-goal-2-autodiscovery/ha_discovery"
)

# Bridge mode options (polling vs subscriptions)
MODE_POLL = "poll"
MODE_SUBSCRIBE = "subscribe"
MODE_AUTO = "auto"

# Mode enum values (must match BridgeMode enum in C++)
MODE_POLL_VALUE = 0
MODE_SUBSCRIBE_VALUE = 1
MODE_AUTO_VALUE = 2

geappliances_bridge_ns = cg.esphome_ns.namespace("geappliances_bridge")
GeappliancesBridge = geappliances_bridge_ns.class_(
    "GeappliancesBridge", cg.Component
)


def sanitize_appliance_name(name):
    """Sanitize appliance type name for use in C++ identifiers.
    
    Replaces special characters with readable equivalents and removes
    any non-alphanumeric characters to create valid C++ identifiers.
    
    Args:
        name: The appliance type name to sanitize
        
    Returns:
        A sanitized string suitable for use as a C++ identifier
    """
    # Replace special characters with more readable equivalents
    replacements = {
        ' ': '',
        '/': '',
        '&': 'And',
        '-': '',
        '(': '',
        ')': '',
    }
    
    result = name
    for old, new in replacements.items():
        result = result.replace(old, new)
    
    # Remove any remaining non-alphanumeric characters
    result = re.sub(r'[^a-zA-Z0-9]', '', result)
    return result


def load_appliance_types():
    """Load appliance type mappings from the lib/public-appliance-api-documentation submodule.

    Resolves the submodule path relative to this file, following symlinks.
    Compilation fails with a clear error if the file is not found.

    Returns:
        Dictionary mapping appliance type IDs (int) to names (str)
    """
    json_filename = "appliance_api_erd_definitions.json"

    # Resolve __file__ to its real location so symlinks don't produce a wrong
    # relative path, then navigate to the submodule two directories up.
    component_dir = Path(__file__).resolve().parent
    json_path = (
        component_dir / ".." / ".." / "lib" / "public-appliance-api-documentation" / json_filename
    ).resolve()

    if not json_path.exists():
        raise RuntimeError(
            f"Required library file not found: {json_path}\n"
            "Ensure the 'lib/public-appliance-api-documentation' submodule is initialised:\n"
            "  git submodule update --init --recursive"
        )

    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
        _LOGGER.info("Loaded appliance types from local submodule: %s", json_path)
    except Exception as e:
        raise RuntimeError(f"Failed to read '{json_path}': {e}") from e

    try:
        for erd in data.get("erds", []):
            if erd.get("id") == "0x0008":
                erd_data = erd.get("data", [])
                if erd_data and erd_data[0].get("type") == "enum":
                    values = erd_data[0].get("values", {})
                    mapping = {
                        int(key): sanitize_appliance_name(value)
                        for key, value in values.items()
                    }
                    _LOGGER.info("Loaded %d appliance type mappings", len(mapping))
                    return mapping
    except Exception as e:
        raise RuntimeError(f"Failed to parse appliance types from '{json_path}': {e}") from e

    raise RuntimeError(
        f"ERD 0x0008 (Appliance Type) not found in '{json_path}'. "
        "The submodule may be outdated or corrupt."
    )


def generate_appliance_type_function(appliance_types):
    """Generate C++ code for the appliance type to string function."""
    # Generate switch cases with consistent indentation
    cases = []
    for type_id, type_name in sorted(appliance_types.items()):
        cases.append(f'    case {type_id}: return "{type_name}";')
    
    cases_str = "\n".join(cases)
    
    # Generate the function with consistent 2-space indentation
    function_code = f'''
std::string appliance_type_to_string(uint8_t appliance_type) {{
  // Auto-generated from public-appliance-api-documentation
  // ERD 0x0008 - Appliance Type enum mapping
  switch (appliance_type) {{
{cases_str}
    default: return "Unknown";
  }}
}}
'''
    return function_code

def validate_at_least_one_uart(config):
    if CONF_GEA3_UART_ID not in config and CONF_GEA2_UART_ID not in config:
        raise cv.Invalid("At least one of gea3_uart_id or gea2_uart_id must be specified")
    return config


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(GeappliancesBridge),
        cv.Optional(CONF_GEA3_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Optional(CONF_GEA2_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Optional(CONF_ADAPTER_ADDRESS, default=0xE4): cv.int_range(min=0x00, max=0xFF),
        cv.Optional(CONF_DEVICE_ID): cv.string,
        cv.Optional(CONF_MODE, default=MODE_AUTO): cv.enum(
            {
                MODE_POLL: MODE_POLL_VALUE,
                MODE_SUBSCRIBE: MODE_SUBSCRIBE_VALUE,
                MODE_AUTO: MODE_AUTO_VALUE,
            },
            upper=False
        ),
        cv.Optional(CONF_POLLING_INTERVAL, default=10000): cv.positive_int,
        cv.Optional(CONF_POLLING_ONLY_PUBLISH_ON_CHANGE, default=False): cv.boolean,
        cv.Optional(CONF_APPLIANCE_API_PARSING, default=False): cv.boolean,
        cv.Optional(CONF_GENERATE_DEVICE_CONFIG, default=True): cv.boolean,
        cv.Optional(CONF_CUSTOM_ERDS, default=[]): cv.ensure_list(
            cv.int_range(min=0, max=0xFFFF)
        ),
        cv.Optional(CONF_HA_DISCOVERY_BASE_URL,
                    default=HA_DISCOVERY_DEFAULT_BASE_URL): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)
CONFIG_SCHEMA = cv.All(CONFIG_SCHEMA, validate_at_least_one_uart)


async def to_code(config):
    """Generate C++ code for the component."""
    # Add library dependencies
    cg.add_library("https://github.com/ryanplusplus/tiny", None)
    cg.add_library("https://github.com/geappliances/tiny-gea-api#develop", None)
    # Add public-appliance-api-documentation as a library dependency
    # This allows users to control the version by updating the library reference
    cg.add_library("https://github.com/joshualongenecker/public-appliance-api-documentation", None)
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # The HA-discovery HTTPS fetch uses esp_http_client, which ESPHome excludes
    # from all builds by default.  Re-enable it here for ESP32 targets so that
    # esp_http_client.h (and its transitive dependencies like esp_crt_bundle.h)
    # are on the include path — the same technique used by ESPHome's built-in
    # http_request component.
    if CORE.is_esp32:
        if hasattr(esp32, 'include_builtin_idf_component'):
            esp32.include_builtin_idf_component("esp_http_client")

    # Get optional GEA3 UART component reference
    if CONF_GEA3_UART_ID in config:
        gea3_uart_component = await cg.get_variable(config[CONF_GEA3_UART_ID])
        cg.add(var.set_gea3_uart(gea3_uart_component))

    # Get optional GEA2 UART component reference
    if CONF_GEA2_UART_ID in config:
        gea2_uart_component = await cg.get_variable(config[CONF_GEA2_UART_ID])
        cg.add(var.set_gea2_uart(gea2_uart_component))

    # Set device ID if provided, otherwise it will be auto-generated
    if CONF_DEVICE_ID in config:
        cg.add(var.set_device_id(config[CONF_DEVICE_ID]))

    # Set adapter address (defaults to 0xE4)
    cg.add(var.set_client_address(config[CONF_ADAPTER_ADDRESS]))

    # Set bridge mode configuration (config[CONF_MODE] is now an integer from cv.enum)
    cg.add(var.set_mode(config[CONF_MODE]))
    cg.add(var.set_polling_interval(config[CONF_POLLING_INTERVAL]))
    cg.add(var.set_polling_only_publish_on_change(config[CONF_POLLING_ONLY_PUBLISH_ON_CHANGE]))
    cg.add(var.set_appliance_api_parsing(config[CONF_APPLIANCE_API_PARSING]))
    cg.add(var.set_generate_device_config(config[CONF_GENERATE_DEVICE_CONFIG]))

    # Set the base URL for runtime HA-discovery JSONL download
    cg.add(var.set_ha_discovery_base_url(config[CONF_HA_DISCOVERY_BASE_URL]))

    # Register any user-configured custom ERDs
    for erd in config[CONF_CUSTOM_ERDS]:
        cg.add(var.add_custom_erd(erd))
    
    # Load appliance types from JSON and generate C++ mapping function
    appliance_types = load_appliance_types()
    function_code = generate_appliance_type_function(appliance_types)
    
    # Add the generated function to the global namespace
    cg.add_global(cg.RawStatement(function_code))
