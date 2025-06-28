import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import (
    CONF_ID,
)

DEPENDENCIES = ["uart"]

CONF_ZEHNDER_COMFOAIR_ID = "zehnder_comfoair_id"

zehnder_comfoair_ns = cg.esphome_ns.namespace("zehnder_comfoair")
ZehnderComfoAirComponent = zehnder_comfoair_ns.class_(
    "ZehnderComfoAirComponent", cg.PollingComponent, uart.UARTDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ZehnderComfoAirComponent),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
