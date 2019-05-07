// CAN Send Example
//

#include <mcp_can.h>
#include <SPI.h>

#define CELL_COUNT 4
#define BITS_PER_BYTE 8

MCP_CAN CAN0(10);     // Set CS to pin 10

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

Cell cells[CELL_COUNT];

void randomizeCell(Cell* cell) {
  cell->v_measure = random(0, 5000);
  cell->v_ctrl_val = random(0, 8192);
  cell->error_fuse = random(0, 2);
  cell->error_load_reduction = random(0, 2);
  cell->error_sense = random(0, 2);
  cell->current = random(0, 4194304);
  cell->current_type = random(0, 4);
  cell->temperature = random(0, 128);
}

void setup()
{
  Serial.begin(115200);

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
}

// SG_ BCS_VMeasure_01 : 0|16@1+ (0.25,0) [0|0] "mV" Vector__XXX



void loop()
{

  for (int i = 0; i < CELL_COUNT; i++) {
    randomizeCell(&cells[i]);

    uint8_t data[8] = {0};
    data[0] = (uint8_t)(cells[i].v_measure >> BITS_PER_BYTE);
    data[1] = (uint8_t)(cells[i].v_measure);
    data[2] = (uint8_t)(cells[i].v_ctrl_val >> 5);
    data[3] = (uint8_t)(cells[i].v_ctrl_val << 3);
    data[3] |= cells[i].error_fuse ? B100 : B000;
    data[3] |= cells[i].error_load_reduction ? B10 : B00;
    data[3] |= cells[i].error_sense ? B1 : B0;
    data[4] = (uint8_t) (cells[i].current >> 16);
    data[5] = (uint8_t) (cells[i].current >> 8);
    data[6] = (uint8_t) (cells[i].current << 2);
    data[6] |= (uint8_t) cells[i].current_type;
    data[7] = (uint8_t) cells[i].temperature;


    // send data:  ID = 0x100, Standard CAN Frame, Data length = 8 bytes, 'data' = array of data bytes to send
    // 257, 513, 769, 1025
    int addr;
    switch (i) {
      case 0:
        addr = 257;
        break;
      case 1:
        addr = 513;
        break;
      case 2:
        addr = 769;
        break;
      case 3:
        addr = 1025;
        break;
    }
    byte sndStat = CAN0.sendMsgBuf(addr, 0, 8, data);
    if (sndStat == CAN_OK) {
      Serial.println("Message Sent Successfully!");
    } else {
      Serial.println("Error Sending Message...");
    }
    delay(100);   // send data per 100ms
  }
  delay(5000);
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
