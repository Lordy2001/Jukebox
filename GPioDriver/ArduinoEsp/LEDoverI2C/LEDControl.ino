#define LEDADDR       0x24
#define IODIRA_REG    0x00
#define GPIOLAT_REG   0x14
#define DIR_OUTPUT    0x00

#define LEDOff (uint16_t)(0 | 1<<8 | 1<<9 | 1<<10 | 1<<11 | 1<<12 | 1<<13)

#define LETTERS_START 'A'
#define LETTERS_END 'V'
static const uint16_t Letters[] = {        
        (uint16_t)(1<<15 |(0x3F00 & ~(1<<8 | 1<<9))),   //'A'
        (uint16_t)(1<<15 |(0x3F00 & ~(1<<10 | 1<<11))), //'B'
        (uint16_t)(1<<15 |(0x3F00 & ~(1<<12 | 1<<13))), //'C'
        (uint16_t)(1<<14 |(0x3F00 & ~(1<<8 | 1<<9))),   //'D'
        (uint16_t)(1<<14 |(0x3F00 & ~(1<<10 | 1<<11))), //'E'
        (uint16_t)(1<<14 |(0x3F00 & ~(1<<12 | 1<<13))), //'F'
        (uint16_t)(1<<7  |(0x3F00 & ~(1<<8 | 1<<9))),   //'G'
        (uint16_t)(1<<7  |(0x3F00 & ~(1<<10 | 1<<11))), //'H'
        0x00,                                           //'I'
        (uint16_t)(1<<7  |(0x3F00 & ~(1<<12 | 1<<13))), //'J'
        (uint16_t)(1<<6  |(0x3F00 & ~(1<<8 | 1<<9))),   //'K'
        (uint16_t)(1<<6  |(0x3F00 & ~(1<<10 | 1<<11))), //'L'
        (uint16_t)(1<<6  |(0x3F00 & ~(1<<12 | 1<<13))), //'M'
        (uint16_t)(1<<5  |(0x3F00 & ~(1<<8 | 1<<9))),   //'N'
        0x00,                                           //'O'
        (uint16_t)(1<<5  |(0x3F00 & ~(1<<10 | 1<<11))), //'P'
        (uint16_t)(1<<5  |(0x3F00 & ~(1<<12 | 1<<13))), //'Q'
        (uint16_t)(1<<4  |(0x3F00 & ~(1<<8 | 1<<9))),   //'R'
        (uint16_t)(1<<4  |(0x3F00 & ~(1<<10 | 1<<11))), //'S'
        (uint16_t)(1<<4  |(0x3F00 & ~(1<<12 | 1<<13))), //'T'
        (uint16_t)(1<<3  |(0x3F00 & ~(1<<8 | 1<<9))),   //'U'
        (uint16_t)(1<<3  |(0x3F00 & ~(1<<10 | 1<<11))),  //'V'
};

#define NUMBERS_START '1'
#define NUMBERS_END  '8'
static const uint16_t Numbers[] = {
        (uint16_t)(1<<2  |(0x3F00 & ~(1<<8 | 1<<9))),     //'1'
        (uint16_t)(1<<2  |(0x3F00 & ~(1<<10 | 1<<11))),   //'2'
        (uint16_t)(1<<2  |(0x3F00 & ~(1<<12 | 1<<13))),   //'3'
        (uint16_t)(1<<1  |(0x3F00 & ~(1<<8 | 1<<9))),     //'4'
        (uint16_t)(1<<1  |(0x3F00 & ~(1<<10 | 1<<11))),   //'5'
        (uint16_t)(1<<1  |(0x3F00 & ~(1<<12 | 1<<13))),   //'6'
        (uint16_t)(1<<0  |(0x3F00 & ~(1<<8 | 1<<9))),     //'7'
        (uint16_t)(1<<0  |(0x3F00 & ~(1<<10 | 1<<11)))    //'8'
};

bool  LEDToggle = 0;
uint16_t LEDCodes[2] = {LEDOff,LEDOff};

void initLEDs(void){
    write2ByteReg(LEDADDR,GPIOLAT_REG,LEDOff);
    write2ByteReg(LEDADDR,IODIRA_REG,DIR_OUTPUT);
}

void LEDTick(void){
  write2ByteReg(LEDADDR,GPIOLAT_REG,LEDOff);
  write2ByteReg(LEDADDR,GPIOLAT_REG,LEDCodes[LEDToggle]);
  LEDToggle =  LEDToggle ? 0 : 1 ;
}

void setLED(uint8_t * newStr){
  LEDCodes[0] = getLEDCode(newStr[0]);
  LEDCodes[1] = getLEDCode(newStr[1]);
}

uint16_t getLEDCode(char ch){
  if(ch >= NUMBERS_START && ch <= NUMBERS_END){
      return Numbers[ch-NUMBERS_START];
  } 
  if(ch >= LETTERS_START && ch <= LETTERS_END){
      return Letters[ch-LETTERS_START];
  } 
  return LEDOff;
}

void write2ByteReg(uint8_t addr, uint8_t reg, uint16_t data){
      Wire.beginTransmission(addr); // transmit to device #8
      Wire.write(reg);
      Wire.write((uint8_t)(data & 0xFF));              
      data >>= 8;
      Wire.write((uint8_t)(data & 0xFF));              
      Wire.endTransmission();    // stop transmitting
}
