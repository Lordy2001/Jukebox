#include <Wire.h>
#include "freertos/timers.h"

#define SDA_PIN 4
#define SCL_PIN 5

#define rlyXpin 2
#define rlyZpin 4

void initLEDs(void);
void LEDTick(void);
void setLED(uint8_t *);

void GPIOinit(void);
void GPIOtick(void);


void onTimer(void);

uint16_t startupIndex = 0;
uint16_t startupTime = 161;
char * scanList[] = {
    "A1", "B1", "A2", "B2", "A3", "B3", "A4", "B4", "A5", "B5", "A6", "B6", "A7", "B7", "A8", "B8",
    "C1", "D1", "C2", "D2", "C3", "D3", "C4", "D4", "C5", "D5", "C6", "D6", "C7", "D7", "C8", "D8",
    "E1", "F1", "E2", "F2", "E3", "F3", "E4", "F4", "E5", "F5", "E6", "F6", "E7", "F7", "E8", "F8",
    "G1", "H1", "G2", "H2", "G3", "H3", "G4", "H4", "G5", "H5", "G6", "H6", "G7", "H7", "G8", "H8",
    "J1", "K1", "J2", "K2", "J3", "K3", "J4", "K4", "J5", "K5", "J6", "K6", "J7", "K7", "J8", "K8",
    "L1", "M1", "L2", "M2", "L3", "M3", "L4", "M4", "L5", "M5", "L6", "M6", "L7", "M7", "L8", "M8",
    "N1", "P1", "N2", "P2", "N3", "P3", "N4", "P4", "N5", "P5", "N6", "P6", "N7", "P7", "N8", "P8",
    "Q1", "R1", "Q2", "R2", "Q3", "R3", "Q4", "R4", "Q5", "R5", "Q6", "R6", "Q7", "R7", "Q8", "R8",
    "S1", "T1", "S2", "T2", "S3", "T3", "S4", "T4", "S5", "T5", "S6", "T6", "S7", "T7", "S8", "T8",
    "U1", "V1", "U2", "V2", "U3", "V3", "U4", "V4", "U5", "V5", "U6", "V6", "U7", "V7", "U8", "V8","  "
};


hw_timer_t * timer = NULL;

void setup() {
  Wire.begin(); // join i2c bus (address optional for master)
  Serial.begin(9600);
  initLEDs();
  GPIOinit();

  digitalWrite(rlyXpin, LOW);
  digitalWrite(rlyZpin, LOW);
  pinMode(rlyXpin, OUTPUT);
  pinMode(rlyZpin, OUTPUT);


  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 5000, true);
  timerAlarmEnable(timer);
}

bool tick = 0;
uint16_t tickCounter = 0;
bool tick100ms = 0;

//Every 5mS routine:
void IRAM_ATTR onTimer() {
    tick = 1;
}

uint8_t cmd[4];

void loop() {
  if(tick){
    tick = 0;
    LEDTick();
    tickCounter++;
    if(tickCounter >= 20){
      tickCounter = 0;
      tick100ms = 1;
    }
  }    
  if(tick100ms){
    tick100ms = 0;
    if(startupIndex < startupTime){
      setLED((uint8_t*)scanList[startupIndex]);
      startupIndex++;
    }
    GPIOtick();
  }
  
  if(Serial.available()){
      uint8_t bytesRead;
      cmd[0] = 0;
      bytesRead = Serial.readBytesUntil('\n',cmd,4);
      if(bytesRead >= 3 && cmd[0] == '#'){
        switch (cmd[1]){
          case 'X':
            if(cmd[2] == '1')
              digitalWrite(rlyXpin, HIGH);
            else
              digitalWrite(rlyXpin, LOW);
            break;
          case 'Z':
            if(cmd[2] == '1')
              digitalWrite(rlyZpin, HIGH);
            else
              digitalWrite(rlyZpin, LOW);
            break;
          default:
            setLED(&cmd[1]);
            break;
        }
      }
  }


}
