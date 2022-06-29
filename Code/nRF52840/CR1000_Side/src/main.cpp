#include <Arduino.h>

uint16_t display_on_time = 30; // how long to keep display on in seconds

// Define Pins
uint8_t OK_LED = 13;  // No errors LED (green)
uint8_t V_MTR = 32;   // Voltmeter FET

uint8_t TR_SIG_Array[5] = {31, 29, 2, 47, 45};

uint8_t CR_TX = 10;   // UART CR1000 TX (RX uC)
uint8_t CR_RX = 9;    // UART CR1000 RX (TX uC)

uint8_t Button_pin = 38;  // Dongle builtin button

unsigned long display_timer; // Setup timer for when display is pushed

volatile bool isButtonPressed = false;
volatile bool HT_Start = false;

uint8_t LED_Array[6] = {13, 15, 17, 20, 22, 24};
volatile bool ERROR_Array[5] = {false, false, false, false, false};

void check_status_ISR() {
  if(!isButtonPressed) {
    isButtonPressed = true;
  }
}

void Heater_Start_ISR(){
  if(!HT_Start){
    HT_Start = true;
  }
}

void display_status(){
  digitalWrite(V_MTR, HIGH);

  bool AllOK = true;
  for(int i = 0; i < 5; i++){
    if(ERROR_Array[i]){
      digitalWrite(LED_Array[i+1], HIGH);
      AllOK = false;
    }
  }
  if(AllOK) digitalWrite(LED_Array[0], HIGH);
}

void HT_ON(){
  for(int i = 0; i < 5; i++){
    if(ERROR_Array[i] != true){
      pinMode(TR_SIG_Array[i], OUTPUT);
      digitalWrite(TR_SIG_Array[i], HIGH);
      delay(10);
      digitalWrite(TR_SIG_Array[i], LOW);
      pinMode(TR_SIG_Array[i], INPUT);
      digitalWrite(CR_RX, HIGH);
      delay(2000);
      digitalWrite(CR_RX, LOW);
      delay(1000);
    } else {

    }
  }
  delay(1000);
  for(int i = 0; i < 5; i++){
    if(digitalRead(TR_SIG_Array[i])){
          ERROR_Array[i] = true;
    }
  }
  HT_Start = false;
}

void setup() {

  NRF_P1->PIN_CNF[CR_TX] = GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos |
                              GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos | 
                              GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos |
                              GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos;

  NRF_P1->PIN_CNF[CR_RX] = GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos |
                              GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos | 
                              GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos |
                              GPIO_PIN_CNF_DRIVE_S0D1 << GPIO_PIN_CNF_DRIVE_Pos;
  digitalWrite(CR_RX,LOW);

  pinMode(Button_pin, INPUT_PULLUP);
  pinMode(V_MTR, OUTPUT);
  for(int i = 0; i < 6; i++){ 
    pinMode(LED_Array[i], OUTPUT);
    digitalWrite(LED_Array[i], LOW);
  }
  for(int i = 0; i < 5; i++){
    pinMode(TR_SIG_Array[i], OUTPUT);
    digitalWrite(TR_SIG_Array[i], LOW);
  }

  attachInterrupt(digitalPinToInterrupt(Button_pin), check_status_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(CR_TX), Heater_Start_ISR, RISING);

  // settling time
    delay(250);

  // ensure starting conditions
    isButtonPressed = false;
    HT_Start = false;

}

void loop() {
  if(HT_Start){
     //display_status();
     //display_timer = millis() + (display_on_time * 1000);
    // isButtonPressed = false;
    HT_Start = false;
    HT_ON();
  }

  if (isButtonPressed) {
    display_status();
    display_timer = millis() + (display_on_time * 1000);
    isButtonPressed = false;
    HT_Start = false;
    // HT_ON();
  }

  if(millis() >= display_timer){
    digitalWrite(V_MTR, LOW);
    for(int i = 0; i < 6; i++){
      digitalWrite(LED_Array[i], LOW);
    }
  }
  delay(250);
}
