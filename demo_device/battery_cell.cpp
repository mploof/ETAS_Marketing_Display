#include "battery_cell.h"
#include <FastLED.h>

BatteryCell::BatteryCell(void) {

}

BatteryCell::BatteryCell(CRGB* p_leds, int p_start_px, int p_length, bool p_reversed) {
  m_segment = LEDSeg(p_leds, p_start_px, p_length, p_reversed);
}

void BatteryCell::setVoltage(int p_milliVolts, int p_min, int p_max) {

  m_charge_pct = (float)(p_milliVolts - p_min) / (float)(p_max - p_min);

  Serial.print("Cell voltage: ");
  Serial.print(m_charge_pct * 100);
  Serial.println("%");

  updateLEDs();
}

void BatteryCell::setChargePct(float p_pct) {
  m_charge_pct = p_pct;
  Serial.print("Cell voltage: ");
  Serial.print(m_charge_pct * 100);
  Serial.println("%");
  updateLEDs();
}

void BatteryCell::updateLEDs(void) {
  int leds_on = (int) (m_charge_pct * CELL_PX) + 1;
  CRGB color;

  Serial.print("LEDs on: ");
  Serial.print(leds_on);
  Serial.println("\n");

  this->allOff();

  switch(m_style) {
    case DISPLAY_SOLID:
    {
      if(m_charge_pct < 0.25) color = CRGB::Red;
      else if(m_charge_pct >= 0.25 && m_charge_pct < 0.5) color = CRGB::Yellow;
      else if(m_charge_pct >= 0.5 && m_charge_pct < 0.75) color = CRGB::Green;
      else if(m_charge_pct >= 0.75) color = CRGB::Blue;

      for(int i = 0; i < leds_on; i++){
        m_segment.setPx(i, color);
      }
      break;
    }
    case DISPLAY_STEPPED:
    {
      for(int i = 0; i < leds_on; i++){
        if(i < 2) color = CRGB::Red;
        else if(i >= 2 && i < 4) color = CRGB::Yellow;
        else if(i >= 4 && i < 6)color = CRGB::Green;
        else if(i >= 6) color = CRGB::Blue;
        m_segment.setPx(i, color);
      }
      break;
    }
    case DISPLAY_GRADIENT:
    {
      break;
    }
  }

  FastLED.show();
}

void BatteryCell::setDisplayStyle(BatteryCell::DisplayStyle p_style) {
  m_style = p_style;
  updateLEDs();
}

float BatteryCell::getChargePct(void) {
  return m_charge_pct;
}

void BatteryCell::allOff(void) {
  for(int i = 0; i < CELL_PX; i++){
    m_segment.setPx(i, CRGB::Black);
  }
}
