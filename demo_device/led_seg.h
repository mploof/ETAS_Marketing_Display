#ifndef _LED_SEG_H_
#define _LED_SEG_H_

#include <FastLED.h>

enum AnimationType {
  SOLID,
  CHASE,
  HEARTBEAT,
  KNIGHT_RIDER,
  INTERLEAVE
};

enum Direction {
  FWD,
  REV
};

typedef struct Animation {
  AnimationType type;
  Direction dir;
};


class LEDSeg {

public:

  LEDSeg();
  LEDSeg(CRGB* p_leds, int* p_px_array, int p_length);
  LEDSeg(CRGB* p_leds, int p_start_px, int p_length, bool p_reversed);
  void setPx(int p_px, CRGB p_color);
  void setPxHSV(int p_px, CHSV p_color);
  void setReversePixels(bool p_reversed);
  bool getReversePixels(void);
  void allOff(void);
  void setAnimationRGB(CRGB p_color, AnimationType p_type, Direction p_dir = FWD);
  void setAnimationHSV(CHSV p_color, AnimationType p_type, Direction p_dir = FWD);
  void startAnimation();
  void stopAnimation();
  void updateAnimation(void);
  int getID(void);
  void fadeAll(void);


private:

  void updateChase(void);
  void updateHeartbeat(void);
  void updateKnightRider(void);
  void updateInterleave(void);

  static int g_id;

  int m_id;
  CRGB* m_leds;
  bool m_reversed;
  int m_length;
  AnimationType m_type;
  Direction m_dir;
  CRGB m_color;
  bool m_animation_active;
  bool m_new_animation;
  bool m_has_array;
  int* m_px_array;
  byte m_hue;
  int m_knight_rider_px;
  int m_knight_rider_dir;
  int m_knight_rider_hue_inc;

  int m_cur_px[10];
  int m_cur_px_rev[10];  

};


#endif
