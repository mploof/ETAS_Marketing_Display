#ifndef _BATTERY_CELL_H_
#define _BATTERY_CELL_H_

#include "led_seg.h"

const int CELL_PX = 8;




class BatteryCell {

public:

  enum DisplayStyle {
    DISPLAY_SOLID,
    DISPLAY_STEPPED,
    DISPLAY_GRADIENT
  };

  BatteryCell(void);
  BatteryCell(CRGB* p_leds, int p_start_px, int p_length, bool p_reversed);
  void setVoltage(int p_millivolts);
  void setVoltageRange(int p_min, int p_max);
  int getVoltage(void);
  void setDisplayStyle(BatteryCell::DisplayStyle style);
  void setChargePct(float pct);
  float getChargePct(void);
  void allOff(void);
  void updateLEDs(void);

private:

  LEDSeg m_segment;
  float m_charge_pct;
  DisplayStyle m_style;
  int m_voltage_mv;
  int m_min_voltage_mv;
  int m_max_voltage_mv;

};

#endif
