// demo: CAN-BUS Shield, receive data with check mode
// send data coming to fast, such as less than 10ms, you can use this way
// loovee, 2014-6-13

/**************************
         Includes
**************************/
#include <FastLED.h>
#include <SPI.h>

// Requires disambiguation with mcp_can.h file in different library
#include "./mcp_can.h"


/**************************
         Constants
**************************/

#define CELL_1 0x101
#define CELL_2 0x201
#define CELL_3 0x301
#define CELL_4 0x401
#define CELL_COUNT 4

#define NUM_LEDS 8
#define DATA_PIN 6


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
        Global Vars
**************************/

// Array of simulated battery cells
Cell cells[CELL_COUNT];

// Define the array of leds
CRGB leds[NUM_LEDS];

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


/**************************
        Functions
**************************/

void setup()
{
  // Init Serial
  Serial.begin(115200);
  Serial.println("Serial ready!");


  // Init LEDs
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS);
  int brightness = 25;
  LEDS.setBrightness(brightness);
  char out_msg[100];
  sprintf(out_msg, "LEDs initialized. Brightness: %d", brightness);
  Serial.println(out_msg);

  // Init CAN-Bus
  while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  Serial.println("CAN BUS Shield init ok!");

  // while(1) {
  //   static int pct = 0;
  //   setCellCharge(pct);
  //   pct += 12;
  //   if(pct > 100) {
  //     pct = 0;
  //   }
  //   delay(250);
  // }
}


void loop()
{

  bool new_msg = checkCANMsg();
  updateAnimation(new_msg);

}

void updateAnimation(bool new_msg){
  static int highest_cell = 0;
  unsigned int highest_val = 0;

  if(new_msg) {
    for(int i = 0; i < CELL_COUNT; i++) {
      if(cells[i].v_measure > highest_val) {
        highest_cell = i;
        highest_val = cells[i].v_measure;
      }
    }

    Serial.print("Highest cell: ");
    Serial.print(highest_cell);
    Serial.print(" Highest val: ");
    Serial.println(highest_val);
    Serial.println("");
  }

  if(highest_cell == 0) {
    chase(120, true);
  }
  else{
    chase(230, false);
  }
}

bool checkCANMsg(){
  unsigned char len = 0;
  unsigned char buf[8];
  bool ret = false;
  if (CAN_MSGAVAIL == CAN.checkReceive())           // check if data coming
  {
    CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

    unsigned long canId = CAN.getCanId();

    Serial.println("-----------------------------");
    Serial.print("Get data from ID: 0x");
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
    Cell* cell = &cells[index];
    cell->v_measure = buf[0] << 8;
    cell->v_measure |= buf[1];
    cell->v_ctrl_val = buf[2] << 5;
    cell->v_ctrl_val |= buf[3] >> 3;
    cell->error_fuse = buf[3] & B100;
    cell->error_load_reduction = buf[3] & B10;
    cell->error_sense = buf[3] & B1;
    cell->current = (uint32_t)buf[4] << 16;
    cell->current |= (uint32_t)buf[5] << 8;
    cell->current |= (uint32_t)buf[6] >> 2;
    cell->current_type = buf[6] & B11;
    cell->temperature = buf[7];

    char out_msg[350];
    sprintf(out_msg,
      "v_measure: %u\n"
      "v_ctrl_val: %u\n"
      "error_fuse: %u\n"
      "error_load_reduction: %u\n"
      "error_sense: %u\n"
      "current_type: %u\n"
      "temperature: %u",
      cell->v_measure, cell->v_ctrl_val, cell->error_fuse, cell->error_load_reduction,
      cell->error_sense, cell->current_type, cell->temperature
    );
    Serial.println(out_msg);
    Serial.print("current: ");
    Serial.println((float)cell->current * 0.0001);
    Serial.println("");

  }

  return ret;
}

void chase(int hue, bool forward){
  const int ACTIVE_LEDS = 3;
  const int MIN_PX = -6;
  const int MAX_PX = NUM_LEDS - 1;
  static int led_locations[ACTIVE_LEDS] = {0, -3, -6};

  allOff();
  for(int i = 0; i < ACTIVE_LEDS; i++) {
    int this_location = led_locations[i];
    if(this_location >= 0) {
      leds[this_location] = CHSV(hue, 255, 255);
    }
  }
  FastLED.show();

  for(int i = 0; i < ACTIVE_LEDS; i++) {
    if(forward) {
      led_locations[i]++;
    }
    else {
      led_locations[i]--;
    }
    if(led_locations[i] > MAX_PX) {
      led_locations[i] = MIN_PX;
    }
    else if(led_locations[i] < MIN_PX) {
      led_locations[i] = MAX_PX;
    }
  }

  // Wait a little bit before we loop around and do it again
  delay(100);

}

void allOff(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }
}

void fadeall() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);
  }
}

void setCellCharge(int milliVolts, int min, int max) {

  float charge_pct = (float)(milliVolts - min) / (float)(max - min);

  int leds_on = (int) (charge_pct / (100.0 / NUM_LEDS)) + 1;

  allOff();
  for(int i = 0; i < leds_on; i++){
    if(i < 2) leds[i] = CRGB::Red;
    else if(i >= 2 && i < 4) leds[i] = CRGB::Yellow;
    else if(i >= 4 && i < 6) leds[i] = CRGB::Green;
    else if(i >= 6) leds[i] = CRGB::Blue;
  }
  FastLED.show();

}
