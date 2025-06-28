#pragma once

#include "coroutine.h"

#include "esphome/core/defines.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace zehnder_comfoair {

using state_machine::Queue;
using state_machine::Coroutine;
using state_machine::Context;

class ZehnderComfoAirComponent : public uart::UARTDevice, public PollingComponent {
  public:
    void setup() override;
    void loop() override;
    void update() override;
    void dump_config() override;

#ifdef USE_SENSOR
    void set_bypass_status_sensor(sensor::Sensor *bypass_status) { this->bypass_status_sensor_ = bypass_status; }
    void set_outside_temperature_sensor(sensor::Sensor *outside_temperature) { this->outside_temperature_sensor_ = outside_temperature; }
    void set_supply_temperature_sensor(sensor::Sensor *supply_temperature) { this->supply_temperature_sensor_ = supply_temperature; }
    void set_extract_temperature_sensor(sensor::Sensor *extract_temperature) { this->extract_temperature_sensor_ = extract_temperature; }
    void set_exhaust_temperature_sensor(sensor::Sensor *exhaust_temperature) { this->exhaust_temperature_sensor_ = exhaust_temperature; }
#endif

#ifdef USE_BINARY_SENSOR
    void set_filter_full_binary_sensor(binary_sensor::BinarySensor *filter_full) { this->filter_full_binary_sensor_ = filter_full; }
#endif

#ifdef USE_NUMBER
    void set_level_number(number::Number *level) { this->level_number_ = level; }
    void set_comfort_temperature_number(number::Number *comfort_temperature_number) { this->comfort_temperature_number_ = comfort_temperature_number; }
#endif

  protected:
    using cmd_t = uint16_t;

    Coroutine<bool> read_array_coro(Context& ctx, uint8_t* data, size_t data_len);
    Coroutine<bool> read_byte_coro(Context& ctx, uint8_t* data) { return this->read_array_coro(ctx, data, 1); }

    Coroutine<bool> send_command(Context& ctx, cmd_t cmd, const uint8_t *data = nullptr, size_t data_len = 0);
    Coroutine<int> read_response(Context& ctx, cmd_t cmd, uint8_t *data, uint8_t data_len);

    Coroutine<bool> query_data(Context& ctx, cmd_t cmd, uint8_t *data, uint8_t data_len);

    void send_ack();
    Coroutine<bool> read_ack(Context& ctx);

    Coroutine<bool> read_escape_sequence(Context& ctx, uint8_t byte, bool skip_mismatched = false);

    float parse_temperature(uint8_t byte);
    uint8_t serialize_temperature(float t);

    Coroutine<void> update_temperatures(Context& ctx);
    Coroutine<void> update_bypass_status(Context& ctx);
    Coroutine<void> update_bypass_control_status(Context& ctx);
    Coroutine<void> update_levels(Context& ctx);
    Coroutine<void> update_faults(Context& ctx);
    Coroutine<void> apply_level(Context& ctx, uint8_t level);
    Coroutine<void> apply_comfort_temperature(Context& ctx, float t);

#ifdef USE_SENSOR
    sensor::Sensor *bypass_status_sensor_;
    sensor::Sensor *outside_temperature_sensor_;
    sensor::Sensor *supply_temperature_sensor_;
    sensor::Sensor *extract_temperature_sensor_;
    sensor::Sensor *exhaust_temperature_sensor_;
#endif

#ifdef USE_BINARY_SENSOR
    binary_sensor::BinarySensor *filter_full_binary_sensor_;
#endif

#ifdef USE_NUMBER
    number::Number *level_number_;
    number::Number *comfort_temperature_number_;
#endif

    int comfort_temperature_pending_updates_ = 0;

    Queue task_queue;
};


}  // namespace empty_uart_component
}  // namespace esphome
