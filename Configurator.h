// define constants, do not modify pins
#define LED A2           // LED1 for event
#define BUZ A3           // buzzer on Analog Pin 3
#define LIT A1           // LED2
#define ALARM 50         // CPS default alarm level
#define BARGRAPH 2100    // 1500 cpm by default
#define BUTTON_UP A4     // define upper button pin
#define BUTTON_DO A5     // define bottom button pin
#define LED_TIME 1       // for led to be ON; 1-3ms for most LED's. Longer LED time may reduce counting performance on high speed counting

// PWM settings for HV

#define PWM_400_INITIAL 55       // 400V PWM
#define PWM_500_INITIAL 75       // 500V PWM
#define PWM_400_IDLE 29          // idle PWM status for 400V
#define PWM_500_IDLE 30          // idle PWM status for 500V
#define PWN_FREQ 8               // PWM frequency setup 4KHz

// ADC read HV before divider doubler. It mean we need to read 200V for 400V, or 250V for 500V
// ADC uses internal 1100mV VREF (but it not exact value, some revision differ)
// ADC read HV through voltage divider 50Mega-100K
// here you set ADC units 0-1023 to define 400mV for 400V and 500mV for 500V tube
// test real HV with 100M ohm divider on point D3-R7 or with 10M multimeter on terminal block (if you use 10M multimeter then remember about 10M-10M voltage divider with tube load resistor and voltmeter impedance, so 400V read as 200V)
#define ADC_400 400              // ADC readings of HV, depend on atmega vref
#define ADC_500 540



// GM Tube setting
#define FACTOR_USV 0.0057    // Sieverts Conversion Factor, known value from tube datasheet 
/*
SBM-20   0.0057
LND-712  0.0081
SBM-19   0.0021
SI-29BG  0.0082
SI-180G  0.0031
J305     0.0081
SBT11-A  0.0031
SBT-9    0.0117
*/


// bargraph symbols
// blank
byte bar_0[8] = {        
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B00000,
  B00000
}; 

// 1 bar
byte bar_1[8] = {
  B00000,
  B00000,
  B10000,
  B10000,
  B11111,
  B10000,
  B10000,
  B00000
};

// 2 bars
byte bar_2[8] = {
  B00000,
  B00000,
  B11000,
  B11000,
  B11111,
  B11000,
  B11000,
  B00000
};

// 3 bars
byte bar_3[8] = {
  B00000,
  B00000,
  B11100,
  B11100,
  B11111,
  B11100,
  B11100,
  B00000
};

// 4 bars
byte bar_4[8] = {
  B00000,
  B00000,
  B11110,
  B11110,
  B11111,
  B11110,
  B11110,
  B00000
};

// 5 bars
byte bar_5[8] = {
  B00000,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000
};


