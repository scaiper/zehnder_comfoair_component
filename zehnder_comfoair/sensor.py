import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from . import CONF_ZEHNDER_COMFOAIR_ID, zehnder_comfoair_ns, ZehnderComfoAirComponent

DEPENDENCIES = ["zehnder_comfoair"]

CONF_BYPASS_STATUS = "bypass_status"
CONF_OUTSIDE_TEMPERATURE = "outside_temperature"
CONF_SUPPLY_TEMPERATURE = "supply_temperature"
CONF_EXTRACT_TEMPERATURE = "extract_temperature"
CONF_EXHAUST_TEMPERATURE = "exhaust_temperature"

ICON_CALL_SPLIT = "mdi:call-split"
ICON_HOME_EXPORT_OUTLINE = "mdi:home-export-outline"
ICON_HOME_IMPORT_OUTLINE = "mdi:home-import-outline"
ICON_HOME_LOCATION_ENTER = "mdi:location-enter"
ICON_HOME_LOCATION_EXIT = "mdi:location-exit"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ZEHNDER_COMFOAIR_ID): cv.use_id(ZehnderComfoAirComponent),
            cv.Optional(CONF_BYPASS_STATUS): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_CALL_SPLIT,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_OUTSIDE_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_HOME_IMPORT_OUTLINE,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SUPPLY_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_HOME_LOCATION_EXIT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_EXTRACT_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_HOME_LOCATION_ENTER,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_EXHAUST_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_HOME_EXPORT_OUTLINE,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    )
)

SENSOR_MAP = {
    CONF_BYPASS_STATUS: "set_bypass_status_sensor",
    CONF_OUTSIDE_TEMPERATURE: "set_outside_temperature_sensor",
    CONF_SUPPLY_TEMPERATURE: "set_supply_temperature_sensor",
    CONF_EXTRACT_TEMPERATURE: "set_extract_temperature_sensor",
    CONF_EXHAUST_TEMPERATURE: "set_exhaust_temperature_sensor",
}

async def to_code(config):
    var = await cg.get_variable(config[CONF_ZEHNDER_COMFOAIR_ID])

    for key, funcName in SENSOR_MAP.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, funcName)(sens))
