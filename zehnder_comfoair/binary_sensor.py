import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    DEVICE_CLASS_PROBLEM,
)

from . import CONF_ZEHNDER_COMFOAIR_ID, zehnder_comfoair_ns, ZehnderComfoAirComponent

DEPENDENCIES = ["zehnder_comfoair"]

CONF_FILTER_FULL = "filter_full"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ZEHNDER_COMFOAIR_ID): cv.use_id(ZehnderComfoAirComponent),
            cv.Optional(CONF_FILTER_FULL): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
            ),
        }
    )
)

SENSOR_MAP = {
    CONF_FILTER_FULL: "set_filter_full_binary_sensor",
}

async def to_code(config):
    var = await cg.get_variable(config[CONF_ZEHNDER_COMFOAIR_ID])

    for key, funcName in SENSOR_MAP.items():
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(var, funcName)(sens))
