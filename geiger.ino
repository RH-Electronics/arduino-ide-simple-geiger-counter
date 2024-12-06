
/* ARDUINO IDE GEIGER COUNTER ver.2.00 

author: Alex Bogoslavsky RH ELECTRONICS http://rhelectronics.net
Copyright (C) <2017> support@rhelectronics.net

You can purchase the hardware for this project on RH Electronics website. The DIY kit include high quality pcb
and electronics components to solder this project.

Buttons fuctions:
Lower button: mute or un-mute speaker
Upper button: toggle display CPM <-> CPS counter

High Voltage: 400V and 500V tubes are supported. Set JMP for 500V tube, remove JMP for 400V tube.

Warranty Disclaimer:
This Geiger Counter kit is for EDUCATIONAL PURPOSES ONLY. 
You are responsible for your own safety during soldering or using this device!
Any high voltage injury caused by this counter is responsibility or the buyer only. 
DO NOT USE IF YOU DON'T KNOW HOW TO CARRY HIGH VOLTAGE DEVICES! 
The designer and distributor of this kit is not responsible for radiation dose readings 
you can see, or not see, on the display. You are fully responsible for you safety and health
in high radiation area. The counter dose readings cannot be used for making any decisions. 
Software and hardware of the kit provided AS IS, without any warranty for precision radiation measurements. 
When you buy the kit it mean you are agree with the disclaimer!

FOR PERSONAL USAGE ONLY
COMMERCIAL USAGE of source code IS RESTRICTED


*/
//----------------- load parameters--------------------//

// install supplied libraries from lib.zip!
// place Configurator.h in same folder as geiger.ino
#include <Arduino.h>              
#include <LiquidCrystal.h>
#include <SPI.h>
#include <EEPROM.h>
#include "Configurator.h"
#include <math.h>

//-------------------------------------------------------//

//-----------------Define global project variables-------------------------//

#define RAD_LOGGER    true                              // enable serial CPM logging to computer for "Radiation Logger"

unsigned long counts;                                   // counts counter for CPM
static unsigned int cps;                                // cps counter
static unsigned long value[] = {0,0,0,0,0,0};           // array variable for cpm
static unsigned long cpm;                               // cpm variable
static unsigned long rapidCpm;                          // rapidly changed cpm
static unsigned long minuteCpm;                         // minute cpm value for absorbed radiation counter
static unsigned long previousValue;                     // previous cpm value

unsigned long previousMillis = 0;                       // millis counters
unsigned long previousMillis_cps = 0;
unsigned long previousMillis_hv  = 0;
unsigned long previousMillis_pereferal = 0;

int n = 0;                                              // counter for array

int buttonStateDo = 1;                                  // buttons status
int buttonStateUp = 1;

int buzzerFreq = 4000;                                 // buzzer ON 4KHz; buzzer OFF 0Hz
int buzzerStatus = 1;                                  // buzzer status ON
static int displayStatus = 1;                          // default display CPM+dose
  
static float dose;                                     // radiation dose

boolean event = false;                                 // GM tube event received, lets make flag 

int val = 0;                                           // variable to store HV reading
int hv_flag;                                           // read JMP to set HV 
int pwm_run;                                           // variables to store PWM settings
int pwm_idle;
int pwm_adc;
int pwm_step;                                          // pwm duty cycle increasing step at high speed counts, low counts => pwm_step = 0; high counts => pwm_step = 5 

//-------------------Initilize LCD---------------------//
LiquidCrystal lcd(9, 4, 8, 7, 6, 5);
//----------------------------------------------------//



///////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------SETUP AREA-----------------------------------------//
///////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  // zero some important variables first
  counts   = 0;
  n        = 0;
  cpm      = 0;
  cps      = 0;
  pwm_step = 0;
  
  
  // configure atmega IO

  pinMode(LED, OUTPUT);           // configure led1 pin as output
  digitalWrite(LED, LOW);
  pinMode(LIT, OUTPUT);           // configure led2 pin as output
  digitalWrite(LIT, LOW);
  pinMode(BUZ, OUTPUT);           // configure buzzer pin as output
  digitalWrite(BUZ, LOW);
  
  
  pinMode(2, INPUT);              // set pin INT0 input for capturing GM Tube events; pin has hardware 10K pull up

  pinMode(BUTTON_UP, INPUT);      // set buttons pins as input with internal pullup
  digitalWrite(BUTTON_UP, HIGH);
  pinMode(BUTTON_DO, INPUT);
  digitalWrite(BUTTON_DO, HIGH);  
  
  pinMode(3, INPUT);              // HV switch
  digitalWrite(3, HIGH);    
  pinMode(11, INPUT);             // not used pins, pull high with internal pull-ups
  digitalWrite(11, HIGH);  
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);    
  pinMode(13, INPUT);
  digitalWrite(13, HIGH);    
  
  
  pinMode(A0, INPUT);            // HV measure here
  
  
   
 // start uart on 9600 baud rate; serial logging not used in current version, add it if you need
  Serial.begin(9600);            
  delay(100);
  

 // start LCD display  
  lcd.begin(16, 2);
 // create and load custom characters to lcd memory  
  lcd.createChar(0, bar_0);    // load 7 custom characters in the LCD
  lcd.createChar(1, bar_1);
  lcd.createChar(2, bar_2);
  lcd.createChar(3, bar_3);
  lcd.createChar(4, bar_4);
  lcd.createChar(5, bar_5);

 // extract starting messages from memory and print it on lcd
  lcd.setCursor(0, 0);
  lcd.print("  Arduino IDE   ");

  lcd.setCursor(0, 1);
  lcd.print(" Geiger Counter ");

  blinkLed(3,50);                                              // say hello!
  delay(2000);
  pinMode(10, OUTPUT);          // making this pin output for PWM
  
    
  //---------------starting tube high voltage----------------------//
  
  analogReference(INTERNAL);         // set internal 1.1V vref
  setPwmFrequency(10, PWN_FREQ);     //set PWM frequency 4KHz
  delay(50);  


// prepare lcd
   ReadDisplayStatus();
   SetLCD(displayStatus);

//---------------------set 400V or 500V settings for PWM---------//
   hv_flag = digitalRead(3);      // read jumper settings for HV
   if (hv_flag==HIGH)
   {
     pwm_run = PWM_400_INITIAL;
     pwm_idle = PWM_400_IDLE;
     pwm_adc = ADC_400;
   }
   else
   {
     pwm_run = PWM_500_INITIAL;
     pwm_idle = PWM_500_IDLE;
     pwm_adc = ADC_500;
   }
   
//---------------------------------------------------------------//

//---------------------Allow external interrupts on INT0---------//
  attachInterrupt(0, tube_impulse, FALLING);
  interrupts();
//---------------------------------------------------------------//

}

///////////////////////////////////////////////////////////////////////////////////////
//-----------------------------MAIN PROGRAM CYCLE IS HERE----------------------------//
///////////////////////////////////////////////////////////////////////////////////////

void loop()
{
  unsigned long currentMillis = millis();

//--------------------------------makes beep and led---------------------------------//
   if (event == true){                    // make led and beep for every tube event
   PORTC |= _BV (2);                      // digitalWrite (A2, HIGH);
   tone(BUZ, buzzerFreq, LED_TIME);             // http://letsmakerobots.com/node/28278 
   PORTC &= ~_BV (2);                     // digitalWrite (A2, LOW); 
   event = false;
   }
//-----------------------------------------------------------------------------------//


//---------------------------What to do every 5ms ----------------------------------//
  if(currentMillis - previousMillis_hv > 5){      
   previousMillis_hv = currentMillis;
   
   val = analogRead(0);     // read the input pin
   if (val<pwm_adc)
   {
     analogWrite(10, pwm_run+pwm_step);      //correct tube voltage
   }
   else
   {
     analogWrite(10, pwm_idle);      //correct tube voltage
   }
   
 
 }
//------------------------------------------------------------------------------------//

//--------------------------What to do every second-----------------------------------//
 if(currentMillis - previousMillis_pereferal > 1000) {      // check buttons once in a second
    previousMillis_pereferal = currentMillis;
    
    if (readButtonUp()== LOW){                              // toggle display CPM<->CPS
     displayStatus = 1 - displayStatus;
     
     SetLCD(displayStatus);
     
    }
    
    if (readButtonDo()== LOW){                              // mute buzzer
      buzzerStatus = 1 - buzzerStatus;
      
      if (buzzerStatus == 1) 
      {
        buzzerFreq = 4000;
      }
      else
      {
        buzzerFreq = 0;
      }
      
    }
    
    if (displayStatus == 0)                                                            // if CPS display selected then show CPS and bargraph
    {
           clearLcd(4, 0, 5);
           lcd.setCursor(4, 0);
            if (cps>10000) {cps = 10000;}                                                             // limit maximum possible cps
            if (cps>ALARM)  {pwm_step = 3; PORTC |= _BV (1);} else {pwm_step = 0; PORTC &= ~_BV (1);}  // lit second led to say about possible undercount caused by Arduino speed; correct pwm for high speed cunts
           lcd.print(cps);
           DrawBars();
           
    }
    
    
    cps = 0;
 }



//------------------------------------------------------------------------------------//

//-------------------------What to do every 10 seconds-------------------------------------//
 if(currentMillis - previousMillis > 10000) {      // calculating cpm value every 10 seconds
    previousMillis = currentMillis;
    
    
 
 if (displayStatus == 1) {    
      value[n] = counts;
      previousValue = cpm;
     
     cpm = value[0] + value[1] + value[2] + value[3] +value[4] + value[5];

     if (n == 5)
     { 
       n = 0;
     }
     else
     {
       n++;
     }
     

     
//-------check if cpm level changes rapidly and make new recalculation-------//     
     if (previousValue > cpm){        
        rapidCpm = previousValue - cpm;
         if (rapidCpm >= 50){                                // value = 50 works subjectively nice for SBM-20, change it for other tubes or scints.
           cpm = counts * 6;
         }
     }
     
         if (previousValue < cpm){
        rapidCpm = cpm - previousValue;
         if (rapidCpm >= 50){
           cpm = counts * 6;
         }
     }
      
      if (cpm>900000){cpm = 900000;}   // limit maximum possible cpm
      
      dose = cpm * FACTOR_USV;
      
      if (cpm>80000)                           // check if we need to compensate pwm under hig speed counts; lit second led to say about possible undercount caused by Arduino speed.
      {pwm_step = 3; PORTC |= _BV (1);}
      else {pwm_step = 0; PORTC &= ~_BV (1);}
      


// print cpm and dose on lcd  
     clearLcd(0, 1, 6);         
     lcd.setCursor(0, 1);
     lcd.print(cpm);
     
     clearLcd(0, 0, 7);        
     lcd.setCursor(0, 0);
     pFloat(dose);
     
 }

     #if (RAD_LOGGER)
        Serial.print(cpm);   // send cpm data to Radiation Logger 
        Serial.write('\r');  
        Serial.write('\n');
     #endif 


     counts = 0;            // clear variable for next turn
}
//--------------------------------------------------------------------------------------------//



}

///////////////////////////////////////////////////////////////////////////////////////
//-----------------------------MAIN PROGRAM ENDS HERE--------------------------------//
///////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////
//-----------------------------SUB PROCEDURES ARE HERE-------------------------------//
///////////////////////////////////////////////////////////////////////////////////////  


//----------------------------------GM TUBE EVENTS ----------------------------------//
void tube_impulse ()                // interrupt care should short and fast as possible
{ 
  counts++;                         // increase 10 seconds counter
  cps++;                            // increase cps counter
  event = true;                     // make event flag
  
}
//----------------------------------------------------------------------------------//



//------------SECRET ARDUINO PWM----------------//
//  http://playground.arduino.cc/Code/PwmFrequency
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
//------------SECRET ARDUINO PWM----------------//


//------------------read upper button with debounce-----------------//
int readButtonUp() {                         // reads upper button
  buttonStateUp = digitalRead(BUTTON_UP);
  if (buttonStateUp == 1){
    return HIGH;
  }
  else
  {
   delay(100);
    buttonStateUp = digitalRead(BUTTON_UP);
    if (buttonStateUp == 1){
      return HIGH;
    }
    else
    {
      return LOW;
    }
}
 
}
//------------------------------------------------------------------//

//------------------read bottom button with debounce-----------------//
int readButtonDo() {                         // reads bottom button
  buttonStateDo = digitalRead(BUTTON_DO);
  if (buttonStateDo == 1){
    return HIGH;
  }
  else
  {
   delay(100);
    buttonStateDo = digitalRead(BUTTON_DO);
    if (buttonStateDo == 1){
      return HIGH;
    }
    else
    {
      return LOW;
    }
}
}

//------------------------------------------------------------------//


//------------------------------------LED BLINK--------------------------------------//

void blinkLed(int i, int time){                        // make beeps and blink signals
    int ii;                                            // blink counter
    for (ii = 0; ii < i; ii++){
      
    //digitalWrite(BUZ, HIGH); 
    PORTC |= _BV (3); // digitalWrite (A3, HIGH);  
    delay(time);
    //digitalWrite(BUZ, LOW);
    PORTC &= ~_BV (3); // digitalWrite (A3, LOW);
    delay(time);
    }
}


//--------------------------------Print zero on lcd----------------------------------//

void drawZerotoLcd(){                               // print zero symbol when need it
  lcd.write('0');
}


//--------------------------------Print float on lcd---------------------------------//
// 
static void pFloat(float dd){                       // avoid printing float as is on lcd
     lcd.print(int(dd));                            // convert it to text before 
     lcd.write('.');                                // sending to lcd
      if ((dd-int(dd))<0.10) {
       drawZerotoLcd(); 
       lcd.print(int(100*(dd-int(dd))));
      }
      else{
       lcd.print(int(100*(dd-int(dd))));      
     }
}

//--------------------------------clear lcd zone-------------------------------------//
void clearLcd(byte x, byte y, byte zone){
  int ii;
  lcd.setCursor (x,y);
  for (ii = 0; ii < zone; ii++){
    lcd.write(' ');
  }
}


//-----------------------------prepare LCD for display status------------------------//
void SetLCD(int a) {
  
  if (a==1) {
      lcd.clear();
      lcd.setCursor(11, 0);  
      lcd.write("\xe4");                          // print Mu , use "u" if have problem with lcd symbols table  
      lcd.write("Sv/h");
      lcd.setCursor(13, 1);
      lcd.print("CPM"); 
      cpm = 0;
      value[0] = 0;
      value[1] = 0;
      value[2] = 0;
      value[3] = 0;
      value[4] = 0;
      value[5] = 0;
      EEPROM.write(0x00, 0x01);
      displayStatus = 1;
  }
  else
  {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CPS:"); 
      cps = 0;
      EEPROM.write(0x00, 0x00);
      displayStatus = 0;
  }
 
}


//-----------------------------------LCD bargraph------------------------------------//  

void DrawBars(){                         // draw log.scale bar for CPS

  double res   =  log10 (cps*2)*50;      // trim bargraph here
  int bb = (int) res;
  unsigned int fullBlock = (bb / 12);   // divide for full "blocks" of 6 bars 
  unsigned int prtlBlock = (bb % 12 );  // calc the remainder of bars
  if (fullBlock >16){                    // prevent writing to 16 blocks
    fullBlock = 16;
    prtlBlock = 0;
  }
  lcd.setCursor(0, 1);
  for (unsigned int i=0; i<fullBlock; i++){
    lcd.write(5);                          // print full blocks
  }
    lcd.write(prtlBlock / 2); // print remaining bars with custom char
    for (int i=(fullBlock + 1); i<16; i++){
    lcd.write(byte(0));   
    }
    }


//--------------------------read display status from EEPROM--------------------------//
void ReadDisplayStatus(){
  byte statusd = EEPROM.read(0x00);
  if (statusd == 1){displayStatus = 1;}
  else {displayStatus = 0;}
}


///////////////////////////////////////////////////////////////////////////////////////
//-----------------------------SUB PROCEDURES ENDS HERE------------------------------//
///////////////////////////////////////////////////////////////////////////////////////     
  


