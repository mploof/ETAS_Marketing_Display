#include "led_seg.h"

static int LEDSeg::g_id = 0;

LEDSeg::LEDSeg() {

}

LEDSeg::LEDSeg(CRGB* p_leds, int* p_px_array, int p_length) {
  m_id = g_id++;
  m_length = p_length;
  m_leds = p_leds;
  m_reversed = false;
  m_new_animation = true;
  m_has_array = true;
  m_px_array = p_px_array;

}

LEDSeg::LEDSeg(CRGB* p_leds, int p_start_px, int p_length, bool p_reversed) {
  m_id = g_id++;
  m_length = p_length;
  m_leds = &p_leds[p_start_px];
  m_reversed = p_reversed;
  m_new_animation = true;
  m_has_array = false;
}

void LEDSeg::setPx(int p_px, CRGB p_color) {
  if(m_reversed) {
    p_px = m_length - 1 - p_px;
  }

  if(m_has_array)
    m_leds[m_px_array[p_px]] = p_color;
  else
    m_leds[p_px] = p_color;
}

void LEDSeg::setPxHSV(int p_px, CHSV p_color) {
  if(m_reversed) {
    p_px = m_length - 1 - p_px;
  }

  if(m_has_array)
    m_leds[m_px_array[p_px]] = p_color;
  else
    m_leds[p_px] = p_color;
}

void LEDSeg::setReversePixels(bool p_reversed) {
  m_reversed = p_reversed;
}

bool LEDSeg::getReversePixels(void) {
    return m_reversed;
}

void LEDSeg::allOff(void) {
  for(int i = 0; i < m_length; i++) {
    m_leds[i] = CRGB::Black;
  }
}

void LEDSeg::setAnimationRGB(CRGB p_color, AnimationType p_type, Direction p_dir = FWD){
  if(p_color != m_color || p_type != m_type || p_dir != m_dir) {
    m_new_animation = true;
  }
  m_color = p_color;
  m_type = p_type;
  m_dir = p_dir;
}

void LEDSeg::setAnimationHSV(CHSV p_color, AnimationType p_type, Direction p_dir = FWD){
  CRGB rgb_color;
  hsv2rgb_rainbow(p_color, rgb_color);
  if(rgb_color != m_color || p_type != m_type || p_dir != m_dir) {
    m_new_animation = true;
  }
  m_color = rgb_color;
  m_type = p_type;
  m_dir = p_dir;
}

void LEDSeg::startAnimation(void) {
  m_animation_active = true;
  m_new_animation = true;
}

void LEDSeg::stopAnimation(void) {
  m_animation_active = false;
}

void LEDSeg::updateAnimation(void) {
  switch(m_type){
    case SOLID:
      for(int i = 0; i < m_length; i++) {
        this->setPx(i, m_color);
      }
      break;
    case CHASE:
      updateChase();
      break;
    case HEARTBEAT:
      updateHeartbeat();
      break;
    case KNIGHT_RIDER:
      updateKnightRider();
      break;
    case INTERLEAVE:
      updateInterleave();
      break;
  }
};

void LEDSeg::updateChase(void) {
  int spacing = 3;
  // int px_count = m_length / spacing;
  int px_count = 10;

  if(m_new_animation) {
    for(int i = 0; i < px_count; i++) {
      m_cur_px[i] = 0 - i * spacing;
    }
    m_new_animation = false;
  }

  for(int i = 0; i < px_count; i++) {
    // Turn off the previous pixel
    if(m_cur_px[i] > 0) {
      if(m_dir == FWD)
        this->setPx(m_cur_px[i] - 1, CRGB(10, 10, 10));
      else
        this->setPx(m_length - m_cur_px[i], CRGB(10, 10, 10));
    }

    // Return to the beginning
    if(m_cur_px[i] >= m_length) {
      m_cur_px[i] = -3;
    }

    // Activate the new pixel
    if(m_cur_px[i] >= 0) {
      if(m_dir == FWD)
        this->setPx(m_cur_px[i], m_color);
      else
        this->setPx(m_length - 1 - m_cur_px[i], m_color);

    }
    m_cur_px[i]++;
  }

}

void LEDSeg::updateHeartbeat(void) {

}

void LEDSeg::updateInterleave(void) {
  int spacing = 3;
  // int px_count = m_length / spacing;
  int px_count = 10;

  if(m_new_animation) {
    for(int i = 0; i < px_count; i++) {
      m_cur_px[i] = 0 - i * spacing;
      m_cur_px_rev[i] = m_length + i * spacing;
    }
    m_new_animation = false;
  }

  // Clear the pixels
  for(int i = 0; i < m_length; i++) {
    this->setPx(i, CRGB(10, 10, 10));
  }

  for(int i = 0; i < px_count; i++) {

    // Turn on the new pixel
    if(m_cur_px[i] >=0 && m_cur_px[i] < m_length) {
      this->setPx(m_cur_px[i], CRGB::Red);
    }
    if(m_cur_px_rev[i] >= 0 && m_cur_px_rev[i] < m_length) {
      this->setPx(m_cur_px_rev[i], CRGB::Aqua);
    }

    m_cur_px[i]++;
    m_cur_px_rev[i]--;

    // Return to the beginning
    if(m_cur_px[i] >= m_length - 1) {
      m_cur_px[i] = -3;
    }
    if(m_cur_px_rev[i] < 0) {
      m_cur_px_rev[i] = m_length + 3;
    }

  }
}

void LEDSeg::updateKnightRider(void) {
  if(m_new_animation) {
    m_knight_rider_px = 0;
    m_knight_rider_dir = 1;
    m_knight_rider_hue_inc = 5;
    this->allOff();
    m_new_animation = false;
    m_hue = 130;
  }

  this->setPxHSV(m_knight_rider_px, CHSV(m_hue, 255, 255));
  this->fadeAll();

  m_hue += m_knight_rider_hue_inc;
  if(m_hue == 125 || m_hue == 255) {
    m_knight_rider_hue_inc *= -1;
  }

  m_knight_rider_px += m_knight_rider_dir;

  if(m_knight_rider_px == m_length || m_knight_rider_px < 0) {
    m_knight_rider_dir = -m_knight_rider_dir;
    m_knight_rider_px += m_knight_rider_dir * 2;
  }

}

void LEDSeg::fadeAll(void) {

  for(int i = 0; i < m_length; i++) {
    if(m_has_array)
      m_leds[m_px_array[i]].nscale8(250);
    else
      m_leds[i].nscale8(250);
  }

}

int LEDSeg::getID(void) {
  return m_id;
}
