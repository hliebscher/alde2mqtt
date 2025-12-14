import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, switch, number, select
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    UNIT_CELSIUS,
    ICON_THERMOMETER,
    DEVICE_CLASS_TEMPERATURE,
)

CODEOWNERS = ["@hliebscher"]
DEPENDENCIES = ["uart"]

alde_tin_bus_ns = cg.esphome_ns.namespace("alde_tin_bus")
AldeTinBusComponent = alde_tin_bus_ns.class_(
    "AldeTinBusComponent", cg.Component, uart.UARTDevice
)

CONF_AIR_TEMPERATURE = "air_temperature"
CONF_WATER_TEMPERATURE = "water_temperature"
CONF_STATUS = "status"
CONF_POWER = "power"
CONF_FUEL_GAS = "fuel_gas"
CONF_FUEL_ELECTRO = "fuel_electro"
CONF_VENT_SPEED = "vent_speed"
CONF_ELECTRO_POWER = "electro_power"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AldeTinBusComponent),
            cv.Optional(CONF_UPDATE_INTERVAL, default="10s"): cv.update_interval,
            cv.Optional(CONF_AIR_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
            cv.Optional(CONF_WATER_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
            cv.Optional(CONF_STATUS): sensor.sensor_schema(),
            cv.Optional(CONF_POWER): switch.switch_schema(),
            cv.Optional(CONF_FUEL_GAS): switch.switch_schema(),
            cv.Optional(CONF_FUEL_ELECTRO): switch.switch_schema(),
            cv.Optional(CONF_VENT_SPEED): select.select_schema(),
            cv.Optional(CONF_ELECTRO_POWER): select.select_schema(),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    
    if CONF_UPDATE_INTERVAL in config:
        cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL].total_milliseconds))
    
    if CONF_AIR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_AIR_TEMPERATURE])
        cg.add(var.set_air_temperature_sensor(sens))
    
    if CONF_WATER_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_WATER_TEMPERATURE])
        cg.add(var.set_water_temperature_sensor(sens))
    
    if CONF_STATUS in config:
        sens = await sensor.new_sensor(config[CONF_STATUS])
        cg.add(var.set_status_sensor(sens))
    
    if CONF_POWER in config:
        sw = await switch.new_switch(config[CONF_POWER])
        cg.add(var.set_power_switch(sw))
    
    if CONF_FUEL_GAS in config:
        sw = await switch.new_switch(config[CONF_FUEL_GAS])
        cg.add(var.set_fuel_gas_switch(sw))
    
    if CONF_FUEL_ELECTRO in config:
        sw = await switch.new_switch(config[CONF_FUEL_ELECTRO])
        cg.add(var.set_fuel_electro_switch(sw))
    
    if CONF_VENT_SPEED in config:
        sel_config = config[CONF_VENT_SPEED]
        options = ["Aus", "Stufe 1", "Stufe 2", "Stufe 3", "Stufe 4", "Stufe 5", "Stufe 6", "Stufe 7"]
        sel = await select.new_select(sel_config, options=options)
        cg.add(var.set_vent_speed_select(sel))
    
    if CONF_ELECTRO_POWER in config:
        sel_config = config[CONF_ELECTRO_POWER]
        options = ["Aus", "Stufe 1", "Stufe 2", "Stufe 3"]
        sel = await select.new_select(sel_config, options=options)
        cg.add(var.set_electro_power_select(sel))

