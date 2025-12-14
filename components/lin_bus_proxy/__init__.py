import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, text_sensor
from esphome.const import CONF_ID

CODEOWNERS = ["@hliebscher"]
DEPENDENCIES = ["uart"]

lin_bus_proxy_ns = cg.esphome_ns.namespace("lin_bus_proxy")
LinBusProxyComponent = lin_bus_proxy_ns.class_(
    "LinBusProxyComponent", cg.Component
)

CONF_TRUMA_UART = "truma_uart"
CONF_ALDE_UART = "alde_uart"
CONF_PROXY_MODE = "proxy_mode"
CONF_LOGGING_ENABLED = "logging_enabled"
CONF_LOG_SENSOR = "log_sensor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LinBusProxyComponent),
        cv.Required(CONF_TRUMA_UART): cv.use_id(uart.UARTComponent),
        cv.Required(CONF_ALDE_UART): cv.use_id(uart.UARTComponent),
        cv.Optional(CONF_PROXY_MODE, default=True): cv.boolean,
        cv.Optional(CONF_LOGGING_ENABLED, default=True): cv.boolean,
        cv.Optional(CONF_LOG_SENSOR): cv.union(
            cv.use_id(text_sensor.TextSensor),
            text_sensor.text_sensor_schema(),
        ),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    truma_uart = await cg.get_variable(config[CONF_TRUMA_UART])
    cg.add(var.set_truma_uart(truma_uart))
    
    alde_uart = await cg.get_variable(config[CONF_ALDE_UART])
    cg.add(var.set_alde_uart(alde_uart))
    
    cg.add(var.set_proxy_mode(config[CONF_PROXY_MODE]))
    cg.add(var.set_logging_enabled(config[CONF_LOGGING_ENABLED]))
    
    if CONF_LOG_SENSOR in config:
        log_sensor_config = config[CONF_LOG_SENSOR]
        if isinstance(log_sensor_config, str):
            # ID-Referenz (String)
            sens = await cg.get_variable(cv.declare_id(text_sensor.TextSensor)(log_sensor_config))
        else:
            # Vollständiges Schema (Dict)
            sens = await text_sensor.new_text_sensor(log_sensor_config)
        cg.add(var.set_log_sensor(sens))

