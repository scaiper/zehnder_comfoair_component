#include "zehnder_comfoair.h"
#ifdef USE_NUMBER
#include "number.h"
#endif

#include "esphome/core/log.h"

namespace esphome {
namespace zehnder_comfoair {

static const char *TAG = "zehnder_comfoair_component.component";

static const uint8_t CODE_ESCAPE = 0x07;

static const uint8_t CODE_START = 0xF0;
static const uint8_t CODE_END = 0x0F;
static const uint8_t CODE_ACK = 0xF3;

static const uint8_t CKSUM_INIT = 173;

constexpr auto MAX_DATA_SIZE = 32;
//constexpr auto MAX_MESSAGE_SIZE = 2 + 2 + 1 + MAX_DATA_SIZE * 2 + 1 + 2;

constexpr uint8_t OUTSIDE_TEMP_MASK = 0x01;
constexpr uint8_t SUPPLY_TEMP_MASK = 0x02;
constexpr uint8_t EXTRACT_TEMP_MASK = 0x04;
constexpr uint8_t EXHAUST_TEMP_MASK = 0x08;

void ZehnderComfoAirComponent::setup() {
#ifdef USE_NUMBER
  this->level_number_->add_on_state_callback([this](float value) {

    this->task_queue.enqueue([this, value](Context& ctx) -> Coroutine<void> {
      co_await this->apply_level(ctx, static_cast<uint8_t>(value));
    });

  });

  this->comfort_temperature_number_->add_on_state_callback([this](float value) {
    ++this->comfort_temperature_pending_updates_;

    this->task_queue.enqueue([this, value](Context& ctx) -> Coroutine<void> {
      co_await this->apply_comfort_temperature(ctx, value);
      --this->comfort_temperature_pending_updates_;
    });

  });
#endif
}

void ZehnderComfoAirComponent::loop() {
  task_queue.poll();
}

void ZehnderComfoAirComponent::update() {
  this->task_queue.enqueue([this](Context& ctx) -> Coroutine<void> {
    co_await this->update_temperatures(ctx);
    co_await this->update_bypass_status(ctx);
    co_await this->update_faults(ctx);
  });
}

void ZehnderComfoAirComponent::dump_config(){
  ESP_LOGCONFIG(TAG, "Zehnder ComfoAir component");
}

Coroutine<bool> ZehnderComfoAirComponent::send_command(Context& ctx, ZehnderComfoAirComponent::cmd_t cmd, const uint8_t *data, size_t data_len) {
  if (data_len > MAX_DATA_SIZE) {
    ESP_LOGE(TAG, "data is longer than %d bytes", MAX_DATA_SIZE);
    co_return false;
  }

  uint8_t cksum = CKSUM_INIT;

  // start sequence
  this->write_array(std::to_array<uint8_t>({CODE_ESCAPE, CODE_START}));

  // command
  {
    std::array<uint8_t, sizeof(cmd_t)> cmd_buf;
    for (size_t i = 0; i < sizeof(cmd_t); ++i) {
      uint8_t b = (cmd >> (8*i)) & 0xFF;
      cmd_buf[sizeof(cmd_t) - i - 1] = b;
      cksum += b;
    }
    this->write_array(cmd_buf);
  }

  // data length
  this->write_byte(data_len);
  cksum += data_len;

  // data
  if (data_len > 0) {
    std::array<uint8_t, MAX_DATA_SIZE * 2> buf;

    size_t pos = 0;
    for (size_t i = 0; i < data_len; ++i) {
      buf[pos++] = data[i];
      cksum += data[i];

      if (data[i] == CODE_ESCAPE) {
          buf[pos++] = CODE_ESCAPE;
      }
    }

    this->write_array(buf.data(), pos);
  }

  // checksum
  this->write_byte(cksum);

  // end sequence
  this->write_array(std::to_array<uint8_t>({CODE_ESCAPE, CODE_END}));

  // ACK
  if (!co_await this->read_ack(ctx)) {
    co_return false;
  }

  co_return true;
}

Coroutine<int> ZehnderComfoAirComponent::read_response(Context& ctx, cmd_t cmd, uint8_t *data, uint8_t data_len) {
  uint8_t cksum = CKSUM_INIT;

  // start sequence
  if (!co_await this->read_escape_sequence(ctx, CODE_START)) {
    co_return -1;
  }

  // command
  {
    std::array<uint8_t, sizeof(cmd_t)> buf;
    if (!co_await this->read_array_coro(ctx, buf.data(), buf.size())) {
      ESP_LOGW(TAG, "Failed to read command");
      co_return -1;
    }

    cmd_t received_cmd;
    for (auto b : buf) {
      received_cmd = (received_cmd << 8) | b;
      cksum += b;
    }

    cmd_t expected_cmd = cmd + 1;
    if (received_cmd != expected_cmd) {
      ESP_LOGW(TAG, "Command mismatch: %x != %x", expected_cmd, received_cmd);
      co_return -1;
    }
  }

  // data length
  uint8_t received_data_len = 0;
  if (!co_await this->read_byte_coro(ctx, &received_data_len)) {
      ESP_LOGW(TAG, "Failed to read data length");
      co_return -1;
  }
  cksum += received_data_len;

  if (received_data_len > data_len) {
    ESP_LOGE(TAG, "Buffer too small: %d < %d", data_len, received_data_len);
    co_return -1;
  }

  // data
  for (unsigned i = 0; i < received_data_len; ++i) {
    uint8_t b;
    if (!co_await this->read_byte_coro(ctx, &b)) {
      ESP_LOGW(TAG, "Failed to read data");
      co_return -1;
    }
    if (b == CODE_ESCAPE) {
      if (!co_await this->read_byte_coro(ctx, &b)) {
        ESP_LOGW(TAG, "Failed to read data");
        co_return -1;
      }
      if (b != CODE_ESCAPE) {
        ESP_LOGW(TAG, "Invalid escape sequence %x%x", CODE_ESCAPE, b);
        co_return -1;
      }
    }
    data[i] = b;
    cksum += b;
  }

  // checksum
  uint8_t received_cksum;
  if (!co_await this->read_byte_coro(ctx, &received_cksum)) {
    ESP_LOGW(TAG, "Failed to read checksum");
    co_return -1;
  }
  if (received_cksum != cksum) {
    ESP_LOGW(TAG, "Checksum mismatch: %x != %x", cksum, received_cksum);
    co_return -1;
  }

  // end sequence
  if (!co_await this->read_escape_sequence(ctx, CODE_END)) {
    co_return -1;
  }

  // ACK
  this->send_ack();

  co_return received_data_len;
}

Coroutine<bool> ZehnderComfoAirComponent::query_data(Context& ctx, cmd_t cmd, uint8_t *data, uint8_t data_len) {
  if (!co_await this->send_command(ctx, cmd)) {
    co_return 0;
  }

  auto len = co_await this->read_response(ctx, cmd, data, data_len);
  if (len < 0) {
    ESP_LOGW(TAG, "Failed to read response");
    co_return 0;
  }

  if (len != data_len) {
    ESP_LOGE(TAG, "Unexpected response size: %d != %d", len, data_len);
    co_return 0;
  }

  co_return len;
}

void ZehnderComfoAirComponent::send_ack() {
  this->write_array(std::to_array<uint8_t>({CODE_ESCAPE, CODE_ACK}));
}

Coroutine<bool> ZehnderComfoAirComponent::read_ack(Context& ctx) {
  auto ok = co_await this->read_escape_sequence(ctx, CODE_ACK, true);
  if (!ok) {
    ESP_LOGW(TAG, "Failed to get ACK");
  }
  co_return ok;
}

Coroutine<bool> ZehnderComfoAirComponent::read_escape_sequence(Context& ctx, uint8_t byte, bool skip_mismatched) {
  uint8_t b;

  do {
    do {
      if (!co_await this->read_byte_coro(ctx, &b)) {
        ESP_LOGW(TAG, "Failed to read escape sequence");
        co_return false;
      }
    } while (skip_mismatched && b != CODE_ESCAPE);

    if (b != CODE_ESCAPE) {
      ESP_LOGW(TAG, "Unexpected byte %x", b);
      co_return false;
    }

    if (!co_await this->read_byte_coro(ctx, &b)) {
      ESP_LOGW(TAG, "Failed to read escape sequence");
      co_return false;
    }
  } while (skip_mismatched && b != byte);

  if (b != byte) {
    ESP_LOGW(TAG, "Invalid escape sequence: %x != %x", b, byte);
    co_return false;
  }

  co_return true;
}

float ZehnderComfoAirComponent::parse_temperature(uint8_t byte) {
  int raw = byte;
  if (raw >= 128) raw -= 256;

  return static_cast<float>(raw) / 2 - 20;
}

uint8_t ZehnderComfoAirComponent::serialize_temperature(float t) {
  return std::round((t + 20) * 2);
}

Coroutine<void> ZehnderComfoAirComponent::update_temperatures(Context& ctx) {
#ifdef USE_SENSOR
  std::array<uint8_t, 9> data;
  if (!co_await this->query_data(ctx, 0x00D1, data.data(), data.size())) {
    ESP_LOGW(TAG, "Failed to get temperatures");
    co_return;
  }

  auto flags = data[5];

  auto update_sensor = [flags, this](sensor::Sensor *sensor, uint8_t flag_mask, uint8_t raw_value) {
    if (sensor == nullptr) return;
    if (flags & flag_mask) {
        auto t = this->parse_temperature(raw_value);
        sensor->publish_state(t);
    } else {
        sensor->publish_state(NAN);
    }
  };

  if (this->comfort_temperature_pending_updates_ == 0 && this->comfort_temperature_number_ != nullptr) {
    auto comfort_t = this->parse_temperature(data[0]);
    if (!this->comfort_temperature_number_->has_state() || this->comfort_temperature_number_->state != comfort_t) {
        this->comfort_temperature_number_->publish_state(comfort_t);
    }
  }
  update_sensor(this->outside_temperature_sensor_, OUTSIDE_TEMP_MASK, data[1]);
  update_sensor(this->supply_temperature_sensor_, SUPPLY_TEMP_MASK, data[2]);
  update_sensor(this->extract_temperature_sensor_, EXTRACT_TEMP_MASK, data[3]);
  update_sensor(this->exhaust_temperature_sensor_, EXHAUST_TEMP_MASK, data[4]);
#endif

  co_return;
}

Coroutine<void> ZehnderComfoAirComponent::update_bypass_status(Context& ctx) {
#ifdef USE_SENSOR
  if (this->bypass_status_sensor_ == nullptr) {
    co_return;
  }

  std::array<uint8_t, 4> data;
  if (!co_await this->query_data(ctx, 0x000D, data.data(), data.size())) {
    ESP_LOGW(TAG, "Failed to get bypass status");
    co_return;
  }

  auto bypass_status = data[0];
  if (bypass_status != 0xFF) {
    this->bypass_status_sensor_->publish_state(bypass_status);
  }
#endif

  co_return;
}

Coroutine<void> ZehnderComfoAirComponent::update_bypass_control_status(Context& ctx) {
  std::array<uint8_t, 7> data;
  if (!co_await this->query_data(ctx, 0x00DF, data.data(), data.size())) {
    ESP_LOGW(TAG, "Failed to get bypass control status");
    co_return;
  }

  ESP_LOGD(TAG, "Bypass control status: %x %x %x %x %x %x %x", data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
}

Coroutine<void> ZehnderComfoAirComponent::update_levels(Context& ctx) {
#ifdef USE_NUMBER
  std::array<uint8_t, 14> data;
  if (!co_await this->query_data(ctx, 0x00CD, data.data(), data.size())) {
    ESP_LOGW(TAG, "Failed to get ventilation levels");
    co_return;
  }

  uint8_t raw_level = data[8];
  if (raw_level >= 1) {
    uint8_t level = raw_level - 1;
    this->level_number_->publish_state(level);
  }
#endif

  co_return;
}

Coroutine<void> ZehnderComfoAirComponent::update_faults(Context& ctx) {
  std::array<uint8_t, 17> data;
  if (!co_await this->query_data(ctx, 0x00D9, data.data(), data.size())) {
    ESP_LOGW(TAG, "Failed to get faults");
    co_return;
  }

#ifdef USE_BINARY_SENSOR
  if (this->filter_full_binary_sensor_ != nullptr) {
    auto filter_full = static_cast<bool>(data[8]);
    this->filter_full_binary_sensor_->publish_state(filter_full);
  }
#endif

  ESP_LOGD(TAG, "Faults: A:%x E:%x EA:%x A(high):%x", data[0], data[1], data[9], data[15]);
}

Coroutine<void> ZehnderComfoAirComponent::apply_level(Context& ctx, uint8_t level) {
  uint8_t raw_level = level + 1;
  if (!co_await this->send_command(ctx, 0x0099, &raw_level, 1)) {
    ESP_LOGW(TAG, "Failed to apply level");
    co_return;
  }
}

Coroutine<void> ZehnderComfoAirComponent::apply_comfort_temperature(Context& ctx, float t) {
    uint8_t raw_temp = this->serialize_temperature(t);
    if (!co_await this->send_command(ctx, 0x00D3, &raw_temp, 1)) {
        ESP_LOGW(TAG, "Failed to apply comfort temperature");
        co_return;
    }
}

Coroutine<bool> ZehnderComfoAirComponent::read_array_coro(Context&, uint8_t* data, size_t data_len) {
  while (static_cast<size_t>(this->available()) < data_len) {
    co_await std::suspend_always{};
  }

  co_return this->read_array(data, data_len);
}

#ifdef USE_NUMBER
void ZehnderComfoAirNumber::control(float value) {
  this->publish_state(value);
}
#endif

}  // namespace zehnder_comfoair
}  // namespace esphome
