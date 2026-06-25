#pragma once

#ifdef USE_BUTTON
#include "esphome/components/button/button.h"

namespace esphome {
namespace zehnder_comfoair {

class ZehnderComfoAirButton : public button::Button {
protected:
  void press_action() override;
};

}  // namespace zehnder_comfoair
}  // namespace esphome
#endif // USE_NUMBER
