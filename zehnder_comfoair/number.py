import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    ICON_FAN,
    UNIT_CELSIUS,
)

from . import CONF_ZEHNDER_COMFOAIR_ID, zehnder_comfoair_ns, ZehnderComfoAirComponent

DEPENDENCIES = ["zehnder_comfoair"]

CONF_COMFORT_TEMPERATURE = "comfort_temperature"
CONF_LEVEL = "level"

ICON_THERMOMETER_AUTO = "mdi:thermometer-auto"

ZehnderComfoAirNumber = zehnder_comfoair_ns.class_(
    "ZehnderComfoAirNumber", number.Number
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ZEHNDER_COMFOAIR_ID): cv.use_id(ZehnderComfoAirComponent),
            cv.Optional(CONF_LEVEL): number.number_schema(
                ZehnderComfoAirNumber,
                icon=ICON_FAN,
            ),
            cv.Optional(CONF_COMFORT_TEMPERATURE): number.number_schema(
                ZehnderComfoAirNumber,
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER_AUTO,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
        }
    )
)

async def to_code(config):
    var = await cg.get_variable(config[CONF_ZEHNDER_COMFOAIR_ID])

    if CONF_LEVEL in config:
        level = await number.new_number(config[CONF_LEVEL], min_value=1, max_value=3, step=1)
        cg.add(getattr(var, "set_level_number")(level))
    if CONF_COMFORT_TEMPERATURE in config:
        comfort_temperature = await number.new_number(config[CONF_COMFORT_TEMPERATURE], min_value=12, max_value=28, step=0.5)
        cg.add(getattr(var, "set_comfort_temperature_number")(comfort_temperature))
