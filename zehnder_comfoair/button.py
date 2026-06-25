import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

from . import CONF_ZEHNDER_COMFOAIR_ID, zehnder_comfoair_ns, ZehnderComfoAirComponent

DEPENDENCIES = ["zehnder_comfoair"]

CONF_OPEN_BYPASS = "open_bypass"
CONF_RESET_FILTERS = "reset_filters"

ICON_AIR_FILTER = "mdi:air-filter"
ICON_CALL_SPLIT = "mdi:call-split"

ZehnderComfoAirButton = zehnder_comfoair_ns.class_(
    "ZehnderComfoAirButton", button.Button
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ZEHNDER_COMFOAIR_ID): cv.use_id(ZehnderComfoAirComponent),
            cv.Optional(CONF_OPEN_BYPASS): button.button_schema(
                ZehnderComfoAirButton,
                icon=ICON_CALL_SPLIT,
            ),
            cv.Optional(CONF_RESET_FILTERS): button.button_schema(
                ZehnderComfoAirButton,
                icon=ICON_AIR_FILTER,
            ),
        }
    )
)

async def to_code(config):
    var = await cg.get_variable(config[CONF_ZEHNDER_COMFOAIR_ID])

    if CONF_OPEN_BYPASS in config:
        open_bypass = await button.new_button(config[CONF_OPEN_BYPASS])
        cg.add(getattr(var, "set_open_bypass_button")(open_bypass))
    if CONF_RESET_FILTERS in config:
        reset_filters = await button.new_button(config[CONF_RESET_FILTERS])
        cg.add(getattr(var, "set_reset_filters_button")(reset_filters))
