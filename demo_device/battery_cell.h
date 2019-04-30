#ifndef _BATTERY_CELL_H_
#def _BATTERY_CELL_H_

class BatteryCell {

public:

  enum DisplayStyle {
    SOLID,
    STEPPED,
    GRADIENT
  };


  BatteryCell(int start_px);
  void setVoltage(int milliVolts, int min, int max);
  void setDisplayStyle(BatteryCell::DisplayStyle style);
  float getChargePct(void);

private:

  int start_px_m;
  float charge_pct_m;

}

#endif
