# Zehnder ventilation component

An ESPHome component to interface with a Zehnder ventilation unit.

Zender ventilation units have a serial RS232 port using a RJ45 (8P8C) socket.
  * Pin 1: 21V
  * Pin 2: RS232 RX
  * Pin 3: RS232 TX
  * Pin 8: GND
The protocol is described in detail [here](http://www.see-solutions.de/sonstiges/Protokollbeschreibung_ComfoAir.pdf).
In the document it says that Pin 1 is 12V, but my WHR930 have 21V. So get a step-down regulator with high enough input voltage range.

You will need an RS232 to TTL adapter to interface with the ESP, as RS232 operates on a different voltage range.

UART bus is required for this component to work:
 * Baud rate should be 9600.
 * 8 data bits (this is the default for the UART bus).
 * No parity (this is the default for the UART bus).
 * 1 stop bit (this is the default for the UART bus).

```yaml
# example configuration:

uart:
  baud_rate: 9600
  rx_pin: GPIO20
  tx_pin: GPIO21

zehnder_comfoair:

sensor:
  - platform: zehnder_comfoair
    bypass_status:
      name: Bypass status
    outside_temperature:
      name: Outside temperature
    supply_temperature:
      name: Supply temperature
    extract_temperature:
      name: Extract temperature
    exhaust_temperature:
      name: Exhaust temperature

binary_sensor:
  - platform: zehnder_comfoair
    filter_full:
      name: Filter full

number:
  - platform: zehnder_comfoair
    level:
      name: Level
    comfort_temperature:
      name: Comfort temperature input
```

# Hub

First, you need to define a Zehnder ComfoAir hub in your configuration.
```yaml
zehnder_comfoair:
```

## Configuration variables:
  * **id** (*Optional*): Manually specify the ID used for code generation. Required if you have multiple hubs.
  * All options from Polling Component.
    * **update_interval** defaults to `60s`.
  * All options from UART Device.

# Sensors

 ```yaml
sensor:
  - platform: zehnder_comfoair
    bypass_status:
      name: Bypass status
    outside_temperature:
      name: Outside temperature
    supply_temperature:
      name: Supply temperature
    extract_temperature:
      name: Extract temperature
    exhaust_temperature:
      name: Exhaust temperature
 ```

## Configuration variables:
  * **bypass_status**: The information for the bypass (%). 0 - closed, 100 - open.
    * All options from Sensor
  * **outside_temperature**: The outside temperature (°C), resolution is 0.5°C.
    * All options from Sensor
  * **supply_temperature**: The supply temperature (°C), resolution is 0.5°C.
    * All options from Sensor
  * **extract_temperature**: The extract temperature (°C), resolution is 0.5°C.
    * All options from Sensor
  * **exhaust_temperature**: The exhaust temperature (°C), resolution is 0.5°C.
    * All options from Sensor

# Binary sensors
```yaml
binary_sensor:
  - platform: zehnder_comfoair
    filter_full:
      name: Filter full
```

## Configuration variables:
  * **filter_full**: The filter status: false - OK, true - needs replacing.
    * All options from Binary Sensor

# Numbers
```yaml
number:
  - platform: zehnder_comfoair
    level:
      name: Level
    comfort_temperature:
      name: Comfort temperature input
```

## Configuration variables:
  * **level**: The ventilation level: 1 - low, 2 - medium, 3 - hight
    * All options from Number
  * **comfort_temperature**: The target comfort temperature: min 12°C, max 28°C.
    * All options from Number
