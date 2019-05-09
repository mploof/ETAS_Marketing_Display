#include "battery_cell.h"
#include <FastLED.h>

BatteryCell::BatteryCell(void) {

}

BatteryCell::BatteryCell(CRGB* p_leds, int p_start_px, int p_length, bool p_reversed) {
  m_segment = LEDSeg(p_leds, p_start_px, p_length, p_reversed);
  m_voltage_mv = 0;
  m_min_voltage_mv = 0;  
  m_max_voltage_mv = 8000;
}

void BatteryCell::setVoltageRange(int p_min, int p_max) {
  m_min_voltage_mv = p_min;
  m_max_voltage_mv = p_max;
  setVoltage(m_voltage_mv);
}

float BatteryCell::setVoltage(int p_millivolts) {

  m_voltage_mv = p_millivolts;
  if(m_voltage_mv < m_min_voltage_mv) {
    m_voltage_mv = m_min_voltage_mv;
  }
  else if(m_voltage_mv > m_max_voltage_mv) {
    m_voltage_mv = m_max_voltage_mv;
  }
  m_charge_pct = (float)(p_millivolts - m_min_voltage_mv) / (float)(m_max_voltage_mv - m_min_voltage_mv);

  updateLEDs();

  return m_charge_pct;
}

int BatteryCell::getVoltage(void) {
  return m_voltage_mv;
}

void BatteryCell::setChargePct(float p_pct) {
  m_charge_pct = p_pct;
  m_voltage_mv = p_pct * (float)(m_max_voltage_mv - m_min_voltage_mv) + m_min_voltage_mv;
  Serial.print("Cell voltage: ");
  Serial.print(m_charge_pct * 100);
  Serial.println("%");
  updateLEDs();
}

void BatteryCell::updateLEDs(void) {
  int leds_on = (int) (m_charge_pct * CELL_PX) + 1;
  CRGB color;

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
