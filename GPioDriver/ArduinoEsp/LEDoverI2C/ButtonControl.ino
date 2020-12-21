static uint8_t pinList[] = {
  36,39,34,35,32,33,25,26,27,23,
};

static uint8_t bankPins [] = {
  5,18,19
};

static char keyLookup[][3] ={
  {'A','1','L'},
  {'B','2','M'},
  {'C','3','N'},
  {'D','4','P'},
  {'E','5','Q'},
  {'F','6','R'},
  {'G','7','S'},
  {'H','8','T'},
  {'J',' ','U'},
  {'K',' ','V'},
};
#define NUM_KEYS  10    //Number of keys in the keyLookupTable

#define ClrRlyPin 17
#define LightsRelay 4
#define CoverMotor  2


void GPIOinit() {
  for (int i = 0; i < sizeof(pinList); i++) {
    pinMode(pinList[i], INPUT_PULLUP);
  }
  for (int i = 0; i < sizeof(bankPins); i++) {
    digitalWrite(bankPins[i], LOW);
    pinMode(bankPins[i], OUTPUT);
    //pinMode(bankPins[i], INPUT_PULLUP);
  }
  
  digitalWrite(ClrRlyPin, LOW);
  pinMode(ClrRlyPin, OUTPUT);
  digitalWrite(LightsRelay, LOW);
  pinMode(LightsRelay, OUTPUT);
  digitalWrite(CoverMotor, LOW);
  pinMode(CoverMotor, OUTPUT);

  

}

int bank = -1;
uint8_t GPIOstate = 0;

uint16_t selectTimer;
uint16_t selectionTimeout;

int selection[3];
uint8_t tmpCmd[2];


#define SELECTION_DEBOUNCE  5
#define SELECTION_CLEAR_TIME  4
#define SELECTION_TIMEOUT 300

#define NO_SEL  42
void GPIOtick(){
  char AlphaSel, NumSel;

  switch (GPIOstate) {
    case 0: //Reset poling
      digitalWrite(ClrRlyPin, LOW);
      selectionTimeout = 0;
      selectTimer = 0;
      setBank(-1);
      selection[0] = NO_SEL;
      selection[1] = NO_SEL;
      selection[2] = NO_SEL;
      GPIOstate++;
      break;

    case 1:
      selectTimer++;
      if(selectTimer > SELECTION_CLEAR_TIME){
        selectTimer = 0;
        GPIOstate++;
      }
      break;

    case 2: //Scan all banks at once
      if(readPinlist() != NO_SEL){
        digitalWrite(ClrRlyPin, HIGH);
        GPIOstate++;
        nextBank(); //This moves us to Bank 0;
      }
      break;

    case 3: //Read each bank individually
      selection[bank] = readPinlist();
      nextBank();
    
      //Check if we have a full selection
      if((selection[1] != NO_SEL) && (selection[0]!= NO_SEL || selection[2]!= NO_SEL)){
          selectTimer++;
          if(selectTimer >= SELECTION_DEBOUNCE){
              selectTimer = 0;
              if(selection[0] != NO_SEL && selection[0] < NUM_KEYS){
                AlphaSel = keyLookup[selection[0]][0];
              } else if(selection[2] != NO_SEL && selection[2] < NUM_KEYS) {
                AlphaSel = keyLookup[selection[2]][2];
              } else {
                AlphaSel = '*';
              }

              if(selection[1] != NO_SEL && selection[1] < NUM_KEYS){
                NumSel = keyLookup[selection[1]][1];
              } else {
                NumSel = '*';
              }
              Serial.write('#');
              Serial.write(AlphaSel);
              Serial.write(NumSel);
              Serial.write('\n');

//              tmpCmd[0] = AlphaSel;
//              tmpCmd[1] = NumSel;
//              setLED(&tmpCmd[0]);
              
              GPIOstate = 0;
          }
      } else {
        selectTimer = 0;
      }

      selectionTimeout++;
      if(selectionTimeout > SELECTION_TIMEOUT){
        GPIOstate = 0;
      }
      break;
  }

}

int readPinlist(){
  int pinIndex = NO_SEL;
  
  pinIndex = NO_SEL;
  for (int i = 0; i < sizeof(pinList); i++) {
    if(digitalRead(pinList[i]) == LOW){
      pinIndex = i;
    }
  }
  return pinIndex;
}

void nextBank(){
  bank++;
  if(bank >= 3)
    bank = 0;
  setBank(bank);
}

void setBank(int b){
  bank = b;
  digitalWrite(bankPins[bank], LOW);
  for(int i = 0; i < 3; i++){
    if(bank == -1 || bank == i){
      pinMode(bankPins[i], OUTPUT);
    } else {
      pinMode(bankPins[i], INPUT_PULLUP);  
    }
  }
}
