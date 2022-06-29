#include <Arduino.h>

// User edit variables
const uint8_t Heat_Watts = 8; // How many watts to deliver to heater
const uint8_t Heat_R = 14.8; // resistance of heater in ohms
const uint16_t Heat_Time = 2000; // How long to run heater in milliseconds
const uint16_t Heat_Arm = 28; // How long to wait after a heater pulse before arming the heater again in min

// Setup other variables and arrays
const float V_per_lsb_BATT = (51.7F/4.7F) * (3.0F/4096.0F); //convert ADC value to volts for battery rail
volatile float V_BATT; // Setup 12V voltage reading variable
volatile float PWM_Scale; // Setup PWM scaling variable
volatile int PWM_Value; // Setup PWM value variable
volatile bool startPWM_flag = false;
volatile bool overCurrent_flag = false;
unsigned long Heater_arm_timer;

// Define Pins
const uint8_t HT_EN = 47;
const uint8_t HT_CTL = 45;
const uint8_t HT_Sig = 29;
const uint8_t OC = 31;

const uint8_t BATT_V = 2;

const uint8_t SD_EN = 42;
const uint8_t SD_CS = 32;
// const uint8_t MISO = 24;
// const uint8_t MOSI = 10;
// const uint8_t SCK = 9;

const uint8_t SDA = 15;
const uint8_t SCL = 13;

const uint8_t MUX_EN = 17;
const uint8_t MUX_A = 22;
const uint8_t MUX_B = 20;

void Run_Heater_ISR(){
  startPWM_flag = true;
}

void overCurrent_ISR(){
  NRF_P1->OUT = 0 << (HT_EN-32);
  HwPWM0.stop();
  pinMode(HT_Sig, OUTPUT);
  digitalWrite(HT_Sig, HIGH);
}

void StartPWM(){
  digitalWrite(HT_EN, HIGH);
  
  delay(500);

  V_BATT = analogRead(BATT_V) * V_per_lsb_BATT;

  PWM_Scale = (float) Heat_Watts / ((V_BATT*V_BATT)/Heat_R);
  if(PWM_Scale > 1){
    // PWM_Value = bit(15)-1;
  } else {
    PWM_Value = round(PWM_Scale * bit(14));
  }

  pinMode(HT_Sig, OUTPUT);
  digitalWrite(HT_Sig, HIGH);
  delay(10);
  digitalWrite(HT_Sig, HIGH);
  pinMode(HT_Sig, INPUT);

  HwPWM0.addPin(HT_CTL);
  HwPWM0.begin();
  HwPWM0.setResolution(14);
  HwPWM0.setClockDiv(PWM_PRESCALER_PRESCALER_DIV_1); //4 = 122hz
  HwPWM0.writePin(HT_CTL, PWM_Value, false);
  startPWM_flag = false;
}

void StopPWM(){
  HwPWM0.stop();
  digitalWrite(HT_EN, LOW);
}

void setup() {
  // Setup Pins
  pinMode(HT_EN, OUTPUT);
  pinMode(HT_CTL, OUTPUT);
  pinMode(HT_Sig, INPUT);
  pinMode(OC, INPUT);

  digitalWrite(HT_CTL, LOW);

  

  //Setup Internal ADC (12 bit, 3V ref, 40us sample time)
  analogReadResolution((int) 12);
  analogReference(AR_INTERNAL_3_0);
  analogSampleTime((uint16_t) 40);

  attachInterrupt(digitalPinToInterrupt(HT_Sig), Run_Heater_ISR, RISING);
  //attachInterrupt(digitalPinToInterrupt(OC), Run_Heater_ISR, RISING);

  // settling time
    delay(250);

  // ignore any power-on-reboot garbage
  startPWM_flag = false;
  overCurrent_flag = false;
}

void loop() {
  if(millis() >= Heater_arm_timer){
    if(startPWM_flag == true){
      if(overCurrent_flag == false){
        Heater_arm_timer = millis() + (Heat_Arm * 1000 *60);
        StartPWM();
      }
    }
  } else {
    startPWM_flag = false;
  }

  delay(Heat_Time);

  if(overCurrent_flag == false){
    StopPWM();
  }

  delay(250);
}

