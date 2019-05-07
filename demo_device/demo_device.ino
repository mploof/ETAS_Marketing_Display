// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13

/**************************
         Includes
**************************/
#include <FastLED.h>
#include <SPI.h>
#include "led_seg.h"
#include "battery_cell.h"

// Requires disambiguation with mcp_can.h file in different library
#include "./mcp_can.h"

/**************************
        Data Types
**************************/

typedef struct _cell {
  //  SG_ BCS_VMeasure_01 : 0 | 16@1 + (0.25, 0) [0 | 0] "mV" Vector__XXX
  //  SG_ BCS_VCtrlVal_01 : 16 | 13@1 + (1, 0) [0 | 8191] "" Vector__XXX
  //  SG_ BCS_ErrorFuse_01 : 29 | 1@1 + (1, 0) [0 | 1] "" Vector__XXX
  //  SG_ BCS_ErrorLoadReduction_01 : 30 | 1@1 + (1, 0) [0 | 1] "" Vector__XXX
  //  SG_ BCS_ErrorSense_01 : 31 | 1@1 + (1, 0) [0 | 1] "" Vector__XXX
  //
  //  SG_ BCS_CurrentMeasure_uA_01 m0 : 32 | 16@1 - (1, 0) [0 | 0] "µA" Vector__XXX
  //  SG_ BCS_CurrentMeasure_mA_01 m1 : 32 | 16@1 - (0.1, 0) [0 | 0] "mA" Vector__XXX
  //  SG_ BCS_CurrentMeasure_Coul_01 m3 : 32 | 22@1 - (0.0001, 0) [0 | 0] "mC" Vector__XXX
  //
  //  SG_ BCS_CurrentType_01 M : 54 | 2@1 + (1, 0) [0 | 3] "" Vector__XXX
  //
  //  SG_ BCS_Temperature_01 : 56 | 8@1 - (1, 0) [0 | 127] "°C" Vector__XXX

  uint16_t v_measure; // 16-bits - 0
  uint16_t v_ctrl_val; // 13-bits - 16
  bool error_fuse; // 1-bit - 29
  bool error_load_reduction; // 1-bit - 30
  bool error_sense; // 1-bit - 31

  uint32_t current; // 22-bits - 32
  uint8_t current_type; // 2-bits - 54
  uint8_t temperature; // 8-bits - 56
} Cell;

typedef struct _data_pair {
  uint8_t id;
  uint16_t data;
} DataPair;


/**************************
         Constants
**************************/

#define CELL_1 0x101
#define CELL_2 0x201
#define CELL_3 0x301
#define CELL_4 0x401

#define DATA_PIN 6

const int NUM_LEDS = 136;
const int CELL_COUNT = 4;

const int CELL_BG_PX = 4;
const int HORIZ_PX = 10;
const int VERT_PX = 12;

const int HORIZ_START = 4;
const int VERT_START = 43;
const int CELL_BG_START = 40;
const int CELL_START = 104;

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;

const int cell_trace_px[CELL_COUNT][35] = {
  {92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
  {76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87},
  {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71},
  {44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25}
};

const int horiz_trace_px[11] = {15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};


/**************************
        Global Vars
**************************/

// Array of simulated battery cells
Cell cell_data[CELL_COUNT];

// Define the array of leds
CRGB leds[NUM_LEDS];
LEDSeg horiz;
LEDSeg vert[CELL_COUNT];
LEDSeg cell_bg[CELL_COUNT];
LEDSeg cell_trace[CELL_COUNT];
BatteryCell cell[CELL_COUNT];

MCP_CAN CAN(SPI_CS_PIN);  // Create instance of CAN object

int animation_update_interval = 75;

int min_voltage_mv = 0;
int max_voltage_mv = 5000;

/**************************
      Core Functions
**************************/

void setup()
{
  // Init Serial
  Serial.begin(115200);
  Serial.println(F("Serial ready!"));

  for (int i = 0; i < CELL_COUNT; i++) {
    cell_bg[i] = LEDSeg(&leds[0], CELL_BG_START + i * (VERT_PX + CELL_BG_PX), CELL_BG_PX, true);
    cell[i] = BatteryCell(&leds[0], CELL_START + i * CELL_PX, CELL_PX, false);
  }
  cell_trace[0] = LEDSeg(&leds[0], cell_trace_px[0], 23);
  cell_trace[1] = LEDSeg(&leds[0], cell_trace_px[1], VERT_PX);
  cell_trace[2] = LEDSeg(&leds[0], cell_trace_px[2], VERT_PX);
  cell_trace[3] = LEDSeg(&leds[0], cell_trace_px[3], 23);
  horiz = LEDSeg(&leds[0], horiz_trace_px, HORIZ_PX);


  // Init LEDs
  LEDS.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  int brightness = 35;
  LEDS.setBrightness(brightness);
  char out_msg[100];
  sprintf(out_msg, "LEDs initialized. Brightness: %d", brightness);
  Serial.println(out_msg);

  // Init Cells
  for (int i = 0; i < CELL_COUNT; i++) {
    cell_bg[i].setAnimationRGB(CRGB::Black, SOLID);
    cell_bg[i].updateAnimation();
    cell[i].setChargePct(0.22 * i);
    cell[i].setDisplayStyle(BatteryCell::DISPLAY_SOLID);
  }
  updateTraceAnimation();


  // Init CAN-Bus
  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
    Serial.println(F("CAN BUS Shield init fail"));
    Serial.println(F(" Init CAN BUS Shield again"));
    delay(100);
  }
  Serial.println(F("CAN BUS Shield init ok!"));
}

void loop()
{
  checkSerial();
  updateAnimations();
  bool new_msg = checkCANMsg();
  if (new_msg) {

  }

}


/**************************
        Functions
**************************/

void serialSetBackgroundColor(String input_str) {
  input_str = input_str.substring(input_str.indexOf(',') + 1);
  Serial.println(input_str);
  if (input_str[0] == 'r') {
    setBackgroundColor(CRGB::Red);
  }
  else if (input_str[0] == 'y') {
    setBackgroundColor(CRGB::Yellow);
  }
  else if (input_str[0] == 'g') {
    setBackgroundColor(CRGB::Green);
  }
  else if (input_str[0] == 'b') {
    setBackgroundColor(CRGB::Blue);
  }
  else if (input_str[0] == 'w') {
    setBackgroundColor(CRGB::White);
  }
  else if (input_str[0] == 'o') {
    setBackgroundColor(CRGB::Black);
  }
  else {
    Serial.println(F("Invalid background color. Valid colors: r, y, g, b\n"));
  }
}


void serialSetCellPct(String input_str) {
  // Convert char array to String and parse cell num & charge pct
  int cell_num = input_str.substring(0, 1).toInt() - 1;
  float pct = (float)input_str.substring(input_str.indexOf(',') + 1).toInt() / 100.0;

  Serial.print(F("Cell: "));
  Serial.println(cell_num + 1);

  if (pct >= 0.0 && pct <= 1.0) {
    // Set the charge percent
    cell[cell_num].setChargePct(pct);

    updateTraceAnimation();
  }
  else {
    Serial.println(F("Invalid cell charge value\n"));
  }
}


void setBackgroundColor(CRGB color) {
  for (int i = 0; i < CELL_COUNT; i++) {
    cell_bg[i].setAnimationRGB(color, SOLID);
    cell_bg[i].updateAnimation();
  }
}


void updateTraceAnimation() {

  float average_pct = 0;
  for (int i = 0; i < CELL_COUNT; i++) {
    average_pct += cell[i].getChargePct();
  }
  average_pct /= CELL_COUNT;

  int cell_dir[CELL_COUNT] = {0};

  for (int i = 0; i < CELL_COUNT; i++) {
    if (cell[i].getChargePct() > average_pct) {
      cell_dir[i] = 1;
      cell_trace[i].setAnimationRGB(CRGB::Red, CHASE, FWD);
    }
    else {
      cell_trace[i].setAnimationRGB(CRGB::Aqua, CHASE, REV);
    }
  }

  if (cell_dir[0] != cell_dir [1] && cell_dir[2] != cell_dir[3]) {
    horiz.setAnimationRGB(CRGB::Red, INTERLEAVE);
  }
  else if (cell_dir[0] && cell_dir[1]) {
    horiz.setAnimationRGB(CRGB::Red, CHASE, FWD);
  }
  else if (cell_dir[3] && cell_dir[4]) {
    horiz.setAnimationRGB(CRGB::Red, CHASE, REV);
  }
  else {
    // Find the highest cell and update the animation
    int high_cell = 0;
    float high_pct = 0.0;
    for (int i = 0; i < CELL_COUNT; i++) {
      if (cell[i].getChargePct() > high_pct) {
        high_pct = cell[i].getChargePct();
        high_cell = i;
      }
    }

    if (high_cell < 2) {
      horiz.setAnimationRGB(CRGB::Aqua, CHASE, FWD);
    }
    else {
      horiz.setAnimationRGB(CRGB::Aqua, CHASE, REV);
    }

  }
}


void checkSerial() {
  if (Serial.available()) {
    // Wait for all serial data to arrive
    delay(30);

    // Get serial input
    char input[20];
    String input_str;
    int i = 0;
    while (Serial.available()) {
      input[i] = Serial.read();
      i++;
    }
    input_str = String(input);
    Serial.println(input_str);

    if (input_str[0] >= '1' && input_str[0] <= '4') {
      serialSetCellPct(input_str);
    }
    else if (input_str[0] == 'b') {
      serialSetBackgroundColor(input_str);
    }
    else if (input_str[0] == 'u') {
      animation_update_interval = input_str.substring(input_str.indexOf(',') + 1).toInt();
      Serial.print(F("New update interval: "));
      Serial.print(animation_update_interval);
      Serial.println(F("ms\n"));
    }
    else if (input_str.substring(0, 3).equals("min")) {
      int in_val = input_str.substring(input_str.indexOf(',') + 1).toInt();
      if (in_val >= max_voltage_mv) {
        Serial.println("Min voltage must be lower than max voltage");
      }
      else {
        min_voltage_mv = in_val;
        Serial.print("New min voltage: ");
        Serial.println(min_voltage_mv);
        for (int i = 0; i < CELL_COUNT; i++) {
          cell[i].setVoltageRange(min_voltage_mv, max_voltage_mv);
        }
      }
    }
    else if (input_str.substring(0, 3).equals("max")) {
      int in_val = input_str.substring(input_str.indexOf(',') + 1).toInt();
      if (in_val <= min_voltage_mv) {
        Serial.println("Max voltage must be greater than min voltage");
      }
      else {
        max_voltage_mv = in_val;
        Serial.print("New max voltage: ");
        Serial.println(max_voltage_mv);
        for (int i = 0; i < CELL_COUNT; i++) {
          cell[i].setVoltageRange(min_voltage_mv, max_voltage_mv);
        }
      }
    }
  }
}


void updateAnimations(void) {
  static long last_update = 0;
  if (millis() - last_update > animation_update_interval) {
    for (int i = 0; i < CELL_COUNT; i++) {
      cell_trace[i].updateAnimation();
    }
    horiz.updateAnimation();
    FastLED.show();
    last_update = millis();
  }
}

bool checkCANMsg() {
  unsigned char len = 0;
  unsigned char buf[8];
  bool ret = false;
  if (CAN_MSGAVAIL == CAN.checkReceive())           // check if data coming
  {
    CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

    unsigned long canId = CAN.getCanId();

    Serial.println(F("-----------------------------"));
    Serial.print(F("Get data from ID: 0x"));
    Serial.println(canId, HEX);

    for (int i = 0; i < len; i++) // print the data
    {
      Serial.print(buf[i], HEX);
      Serial.print("\t");
    }
    Serial.println();

    unsigned char index;
    switch (canId) {
      case CELL_1:
        index = 0;
        break;
      case CELL_2:
        index = 1;
        break;
      case CELL_3:
        index = 2;
        break;
      case CELL_4:
        index = 3;
        ret = true;
        break;
      default:
        return;
    }

    // Convert the CAN into its constituent signals
    Cell* this_cell = &cell_data[index];
    this_cell->v_measure = buf[0] << 8;
    this_cell->v_measure |= buf[1];
    this_cell->v_ctrl_val = buf[2] << 5;
    this_cell->v_ctrl_val |= buf[3] >> 3;
    this_cell->error_fuse = buf[3] & B100;
    this_cell->error_load_reduction = buf[3] & B10;
    this_cell->error_sense = buf[3] & B1;
    this_cell->current = (uint32_t)buf[4] << 16;
    this_cell->current |= (uint32_t)buf[5] << 8;
    this_cell->current |= (uint32_t)buf[6] >> 2;
    this_cell->current_type = buf[6] & B11;
    this_cell->temperature = buf[7];

    char out_msg[350];
    sprintf(out_msg,
            "v_measure: %u\n"
            "v_ctrl_val: %u\n"
            "error_fuse: %u\n"
            "error_load_reduction: %u\n"
            "error_sense: %u\n"
            "current_type: %u\n"
            "temperature: %u",
            this_cell->v_measure, this_cell->v_ctrl_val, this_cell->error_fuse, this_cell->error_load_reduction,
            this_cell->error_sense, this_cell->current_type, this_cell->temperature
           );
    Serial.println(out_msg);
    Serial.print(F("current: "));
    Serial.println((float)this_cell->current * 0.0001);
    Serial.println("");

    cell[index].setVoltage(this_cell->v_measure);
    updateTraceAnimation();

  }

  return ret;
}


void allOff() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}


void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);
  }
}
