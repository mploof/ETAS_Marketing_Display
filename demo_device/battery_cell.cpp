#include "battery_cell.h"

BatteryCell::BatteryCell(int start_px){
  start_px_m = start_px;
};

void BatteryCell::setVoltage(int milliVolts, int min, int max) {

  charge_pct_m = (float)(milliVolts - min) / (float)(max - min);

  int leds_on = (int) (charge_pct / (100.0 / NUM_LEDS)) + 1;

  allOff();
  for(int i = 0; i < leds_on; i++){
    if(i < 2) leds[start_px_m + i] = CRGB::Red;
    else if(i >= 2 && i < 4) leds[start_px_m + i] = CRGB::Yellow;
    else if(i >= 4 && i < 6) leds[start_px_m + i] = CRGB::Green;
    else if(i >= 6) leds[start_px_m + i] = CRGB::Blue;
  }
  FastLED.show();

}

void BatteryCell::setDisplayStyle(BatteryCell::DisplayStyle style);

float BatteryCell::getChargePct(void) {
  return charge_pct_m;
}
