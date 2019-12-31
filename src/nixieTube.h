#ifndef _NixieClock_h
#define _NixieClock_h
//
//    FILE: nixieTube.h
//  AUTHOR: CvG
// PURPOSE: Simple library for Arduino to handle nixie tubes via shift register
// VERSION: 1.0
//

// *********************
//    SETUP NIXIE HW
// ********************* 

#define NIXIE_TYPE_IN12
//#define NIXIE_TYPE_IN14


//DS of 74HC595 Pin 14 SER --> D6
#define SNX4HC595_DATA_PIN D6

//ST_CP of 74HC595 Pin 12 RCK --> D7
#define SNX4HC595_LATCH_PIN D7

//SH_CP of 74HC595 Pin 11 SCK --> D8
#define SNX4HC595_CLOCK_PIN D8

#ifdef NIXIE_TYPE_IN14
//LED pin of nixie modules
#define PIN_LED_NIXIE1 D4
#define PIN_LED_NIXIE2 D3
#define PIN_LED_NIXIE3 D5
#endif

#ifdef NIXIE_TYPE_IN12
    #define PIN_D_ENABLE_HV D5
#endif

//Number of nixie pairs
#define NUM_NIXIE_PAIRS 3

//Pin to force configuration portal during start up
#define PIN_CONFIG_PORTAL D0

class NixieClock
{
public:
    NixieClock();

    byte dispValue[NUM_NIXIE_PAIRS] = {0};
    byte shiftOutByte[NUM_NIXIE_PAIRS] = {0};
    byte ledPWM[NUM_NIXIE_PAIRS] = {0};
    boolean ledEnabled[NUM_NIXIE_PAIRS] = {false};
    boolean groupEnabled[NUM_NIXIE_PAIRS] = {false};
    boolean topColonEnabled[NUM_NIXIE_PAIRS - 1] = {true, true};
    boolean bottomColonEnabled[NUM_NIXIE_PAIRS - 1] = {true, true};
    boolean hvSupplyEnabled = false;
    boolean flagRTCavailable = false;
    boolean flagNTPavailable = false;
    boolean flagWifiavailable = false;

    enum LedEffect : byte
    {
        fader,
        sinus
    };
    enum displayState : byte
    {
        showTime,
        showDate,
        showScreensaver,
        standby
    };
    byte currentDisplayState = showDate;

    void setLedEffekt(LedEffect effect);
    void UpdateDisplay();
    void UpdateLED();
    void update(); //update alle time critical states, call from main loop
    void storeTimeChange();
    void evaluateStatusOfTimeProvider();
    void setCurrentDisplayState(displayState state);

#ifdef NIXIE_TYPE_IN12

#endif

private:
    void SetupNixies();
    void ConvertDataToShiftOutByte();

    long millisLastDispUpdate = 0;
    long millisLastTimeChange = 0;
};

#endif
