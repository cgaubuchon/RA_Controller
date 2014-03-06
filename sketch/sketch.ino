#include <ReefAngel_Features.h>
#include <Globals.h>
#include <RA_Wifi.h>
#include <Wire.h>
#include <OneWire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <InternalEEPROM.h>
#include <RA_NokiaLCD.h>
#include <RA_ATO.h>
#include <RA_Joystick.h>
#include <LED.h>
#include <RA_TempSensor.h>
#include <Relay.h>
#include <RA_PWM.h>
#include <Timer.h>
#include <Memory.h>
#include <InternalEEPROM.h>
#include <RA_Colors.h>
#include <RA_CustomColors.h>
#include <Salinity.h>
#include <RF.h>
#include <IO.h>
#include <ORP.h>
#include <AI.h>
#include <PH.h>
#include <WaterLevel.h>
#include <ReefAngel.h>

#define Mem_UV_Max       100
#define Mem_White_Max    101
#define Mem_Blue_Max     102

int sunrise_min = 30;
int sunrise_hour = 7;

int sunset_min = 00;
int sunset_hour = 18;

int current_day = 0;

int uv_max = 30;
int white_max = 100;
int blue_max = 50;

void initMemory(){

  InternalMemory.write(Mem_UV_Max,30);
  InternalMemory.write(Mem_White_Max, 40);
  InternalMemory.write(Mem_Blue_Max, 25);

}

////// Place global variable code above here


void setup()
{
  // This must be the first line
  ReefAngel.Init();  //Initialize controller
  ReefAngel.AddStandardMenu();
  ReefAngel.Use2014Screen();

  // Ports toggled in Feeding Mode
  ReefAngel.FeedingModePorts = 0;
  // Ports toggled in Water Change Mode
  ReefAngel.WaterChangePorts = Port1Bit | Port2Bit | Port5Bit | Port7Bit;
  // Ports toggled when Lights On / Off menu entry selected
  ReefAngel.LightsOnPorts = Port4Bit;
  // Ports turned off when Overheat temperature exceeded
  ReefAngel.OverheatShutoffPorts = Port4Bit | Port6Bit | Port8Bit;
  // Use T1 probe as temperature and overheat functions
  ReefAngel.TempProbe = T1_PROBE;
  ReefAngel.OverheatProbe = T1_PROBE;
  // Set the Overheat temperature setting
  InternalMemory.OverheatTemp_write( 869 );

  // Ports that are always on, all of them for now
  ReefAngel.Relay.On( Port1 );
  ReefAngel.Relay.On( Port2 );
  ReefAngel.Relay.On( Port3 );
  ReefAngel.Relay.On( Port5 );
  ReefAngel.Relay.On( Port6 );
  //ReefAngel.Relay.On( Port7 ); //ATO Port
  ReefAngel.Relay.On( Port8 );

  initMemory();

}

void loop()
{

  //0 = UV
  //1 = White
  //2 = Blue
  int uv_max = InternalMemory.read(Mem_UV_Max);
  int white_max = InternalMemory.read(Mem_White_Max);
  int blue_max = InternalMemory.read(Mem_Blue_Max);

  if( bitRead( ReefAngel.StatusFlags, LightsOnFlag ) ){ //Turn on lights to 'full'
    ReefAngel.PWM.SetChannel( 0,  uv_max); //UV
    ReefAngel.PWM.SetChannel( 1,  white_max); //White
    ReefAngel.PWM.SetChannel( 2,  blue_max); //Blue
    ReefAngel.Relay.On( Port4 ); //Fuge light
  
  }else{

    //Turn off drivers completely if less than the lowest possible output to lengthen lifespan of LEDs and drivers
    if(PWMParabola(sunrise_hour, sunrise_min, sunset_hour, sunset_min, 10, white_max, 10) < 11){
      ReefAngel.PWM.SetChannel( 1, 0 ); //White
    }
    else{
      ReefAngel.PWM.SetChannel( 1, PWMParabola(sunrise_hour, sunrise_min, sunset_hour, sunset_min, 10, white_max, 10) ); //White
    }

    //BLUE
    if(PWMParabola(sunrise_hour-1, sunrise_min-30, sunset_hour+2, sunset_min+30, 10, blue_max, 20) < 11){
      ReefAngel.PWM.SetChannel( 2,  0);
    }
    else{
      ReefAngel.PWM.SetChannel( 2,  PWMParabola(sunrise_hour-1, sunrise_min-30, sunset_hour+2, sunset_min+30, 10, blue_max, 10)); //UV
    }


    if( hour() >= sunset_hour && hour() < sunset_hour+1  ){
      ReefAngel.PWM.SetChannel( 0, 12 );
    }
    else if(PWMParabola(sunrise_hour, sunrise_min, sunset_hour, sunset_min, 10, uv_max, 10) > 12){
      ReefAngel.PWM.SetChannel( 0, PWMParabola(sunrise_hour, sunrise_min, sunset_hour, sunset_min, 10, uv_max, 10) );
    }
    else{
      ReefAngel.PWM.SetChannel( 0, 0 );
    }

    //Fuge Light
    if( hour() >= sunset_hour || hour() < sunrise_hour-2 ){
      ReefAngel.Relay.On( Port4 );
    }
    else{
      ReefAngel.Relay.Off( Port4 );
    }
  }

  //
  // Auto topoff
  //
  if( ReefAngel.DisplayedMenu != WATERCHANGE_MODE && !ReefAngel.HighATO.IsActive() && ReefAngel.Params.PH < 850 ) {//Only run ATO if not in waterchange mode and PH < 8.5
    
    if( ReefAngel.LowATO.IsActive() ){ //ATO on until low switch is active
      
      ReefAngel.Relay.On(Port7);
      
    }else if( (hour() >= 20 || hour() < 12) && minute() == 0 && second() < 31 && (ReefAngel.Params.PH <= 830)  ){ 
      
      //Dose Kalk for 30 seconds every hour between 8pm and 12pm
      //30sec for 16 hours @ 3.5gal/hr = 0.46666 Gal/day
      ReefAngel.Relay.On(Port7);
      
    }else {
      ReefAngel.Relay.Off(Port7);
    }
    
  }else {
      ReefAngel.Relay.Off(Port7);
  }

  //Never run ATO if high switch is active
  if ( ReefAngel.HighATO.IsActive() ){
    ReefAngel.Relay.Off(Port7);
  }

  //Temp settings
  ReefAngel.StandardHeater(Port6, 730, 765);
  ReefAngel.StandardHeater(Port8, 730, 765);


  // This should always be the last line
  ReefAngel.Portal( "cgaubuchon", "donttouchmyfish" );
  ReefAngel.ShowInterface();
}


