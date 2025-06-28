#pragma once

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"

namespace esphome {
namespace zehnder_comfoair {

class ZehnderComfoAirNumber : public number::Number {
protected:
  void control(float value) override;
};

}  // namespace zehnder_comfoair
}  // namespace esphome
#endif // USE_NUMBER
