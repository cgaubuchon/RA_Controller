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
#include <Humidity.h>
#include <DCPump.h>
#include <PAR.h>
#include <ReefAngel.h>

////// Place global variable code below here

#define Kalk_Dose_Interval     103

////// Place global variable code above here
void initMemory(){

  InternalMemory.write(Kalk_Dose_Interval, 59);
  
//  InternalMemory.StdLightsOnHour_write( 8 );
//  InternalMemory.StdLightsOnMinute_write( 30 );
//  InternalMemory.StdLightsOffHour_write( 19 );
//  InternalMemory.StdLightsOffMinute_write( 0 );
//  
//  InternalMemory.HeaterTempOn_write( 780 );
//  InternalMemory.HeaterTempOff_write( 790 );
  InternalMemory.OverheatTemp_write( 850 );

}

void setup()
{
    // This must be the first line
    ReefAngel.Init();  //Initialize controller
    ReefAngel.Use2014Screen();  // Let's use 2014 Screen 
    // Ports toggled in Feeding Mode
    ReefAngel.FeedingModePorts = 0;
    // Ports toggled in Water Change Mode
    ReefAngel.WaterChangePorts = Port1Bit;
    // Ports toggled when Lights On / Off menu entry selected
    ReefAngel.LightsOnPorts = Port2Bit | Port3Bit | Port4Bit;
    // Ports turned off when Overheat temperature exceeded
    ReefAngel.OverheatShutoffPorts = Port3Bit | Port8Bit;
    // Use T1 probe as temperature and overheat functions
    ReefAngel.TempProbe = T1_PROBE;
    ReefAngel.OverheatProbe = T1_PROBE;
    // Set the Overheat temperature setting
    InternalMemory.OverheatTemp_write( 850 );


    // Ports that are always on
    ReefAngel.Relay.On( Port1 );
    ReefAngel.Relay.On( Port5 );

    ////// Place additional initialization code below here
    initMemory();

    ////// Place additional initialization code above here
}

void loop()
{
    //
    // Lights
    //
    int sunrise_min = InternalMemory.StdLightsOnHour_read();
    int sunrise_hour = InternalMemory.StdLightsOnMinute_read();
    int sunset_min = InternalMemory.StdLightsOffHour_read();
    int sunset_hour = InternalMemory.StdLightsOffHour_read();
    
    ReefAngel.StandardLights( Port2, sunrise_hour-1, sunrise_min, sunset_hour, sunset_min+5 ); //Light fan - 5 minutes delayed off
    ReefAngel.StandardLights( Port3, sunrise_hour-1, sunrise_min, sunset_hour-1, sunset_min ); //Day lights - 
    ReefAngel.StandardLights( Port4, sunrise_hour, sunrise_min, sunset_hour, sunset_min ); //Actinic Lights
    
    //Chiller
    ReefAngel.StandardFan( Port7, InternalMemory.ChillerTempOn_read(), InternalMemory.ChillerTempOff_read() );
    
    //Heaters
    ReefAngel.StandardHeater( Port8, InternalMemory.HeaterTempOn_read(), InternalMemory.HeaterTempOff_read() );
    
    //
    // Auto topoff
    //
    if( ReefAngel.DisplayedMenu != WATERCHANGE_MODE && !ReefAngel.HighATO.IsActive() && ReefAngel.Params.PH < 830 ) {//Only run ATO if not in waterchange mode and PH < 8.5
      
      if( ReefAngel.LowATO.IsActive() ){ //ATO on until low switch is active
        ReefAngel.Relay.On(Port6);
      }else if( (hour() >= 20 || hour() < 12) && minute() == 0 && second() < InternalMemory.read(Kalk_Dose_Interval) && (ReefAngel.Params.PH <= 828)  ){ 
        //Dose Kalk for 30 seconds every hour between 8pm and 12pm
        //59sec for 16 hours @ 3.5gal/hr = 0.917777568 Gal/day
        ReefAngel.Relay.On(Port6);
      }else {
        ReefAngel.Relay.Off(Port6);
      }
    }else {
        ReefAngel.Relay.Off(Port6);
    }
  
    //Never run ATO if high switch is active
    if ( ReefAngel.HighATO.IsActive() ){
      ReefAngel.Relay.Off(Port6);
    }

    // This should always be the last line
    ReefAngel.Portal( "cgaubuchon" );
    ReefAngel.ShowInterface();
}


