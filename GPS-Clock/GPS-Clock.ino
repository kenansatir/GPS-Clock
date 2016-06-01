/*
   GPS Clock by varind 2014
 this code is public domain, enjoy!
 Based on:
 
 Adafruit Ultimate GPS module using MTK33x9 chipset
 ------> http://www.adafruit.com/products/746
 Pick one up today at the Adafruit electronics shop 
 and help support open source hardware & software! -ada
 
 and:
 A set of custom made large numbers for a 16x2 LCD using the 
 LiquidCrystal librabry. Works with displays compatible with the 
 Hitachi HD44780 driver. 
 Made by Michael Pilcher
 2/9/2010
 */

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(13,12,11,10,9,8);

SoftwareSerial mySerial(3, 2); // TX, RX
Adafruit_GPS GPS(&mySerial);
#define GPSECHO  false

#define DSTPIN 5

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

int montharray[]={
  31,28,31,30,31,30,31,31,30,31,30,31};
int seconds;
int minutes;
int hours;
int days;
int months;
int years;
byte leap;
float lat;
float lon;
int x = 0;
int y = 0;
int buttonState1=0;
int buttonState2=0;
int lastseconds;
int lastButtonState1=0;
int lastButtonState2=0;


byte LT[8] = 
{
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte UB[8] =
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte RT[8] =
{
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte LL[8] =
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111
};
byte LB[8] =
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};
byte LR[8] =
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100
};
byte UMB[8] =
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};
byte LMB[8] =
{
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};


void setup()  
{
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);

  // delay(1000);
  // Ask for firmware version
  // mySerial.println(PMTK_Q_RELEASE);

  lcd.begin(20, 4);

  lcd.createChar(8,LT);
  lcd.createChar(1,UB);
  lcd.createChar(2,RT);
  lcd.createChar(3,LL);
  lcd.createChar(4,LB);
  lcd.createChar(5,LR);
  lcd.createChar(6,UMB);
  lcd.createChar(7,LMB);

  pinMode(5, INPUT);  
  pinMode(6, INPUT);  
  pinMode(7, INPUT);  
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO)
    if (c) UDR0 = c;  
  // writing direct to UDR0 is much much faster than Serial.print 
  // but only one character can be written at a time. 
#endif
}


void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

uint32_t timer = millis();
uint32_t animationTimer = millis();


void loop()                     // run over and over again
{

  // in case you are not using the interrupt above, you'll
  // need to 'hand query' the GPS, not suggested :(
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();
  if (animationTimer > millis())  animationTimer = millis();

  // approximately every .1 seconds or so, print out the current stats
  // if (millis() - timer > 100) { 
  //    timer = millis(); // reset the timer
  //    if (GPS.fix) {
  //      Serial.print("Location: ");
  //      Serial.print("Location: ");
  //      Serial.print(GPS.latitude, 4); 
  //      Serial.print(GPS.lat);
  //      Serial.print(", "); 
  //      Serial.print(GPS.longitude, 4); 
  //      Serial.println(GPS.lon);
  //      Serial.print("Speed (knots): "); 
  //      Serial.println(GPS.speed);
  //      Serial.print("Angle: "); 
  //      Serial.println(GPS.angle);
  //      Serial.print("Altitude: "); 
  //      Serial.println(GPS.altitude);
  //      Serial.print("Satellites: "); 
  //      Serial.println((int)GPS.satellites);
  //    }
  //    else{
  //      Serial.println("LOCATING............");
  //    }


  buttonState1 = digitalRead(6);
  buttonState2 = digitalRead(7);

  if (lastButtonState1!=buttonState1){
    lcd.clear();
    lastButtonState1=buttonState1;
  }    
  if (lastButtonState2!=buttonState2){
    lcd.clear();
    lastButtonState2=buttonState2;
  }

  if (buttonState1) displayCurrentLocation();
  if (buttonState2) displayClock();
  if (!buttonState1 && !buttonState2) displayMillis();


}
//}


void displayMillis(){

  if (millis() < 99999995.00){
    lcd.setCursor(0,0);
    float floatMillis = millis();
    float seconds = floatMillis/1000;
    long secondsTenThousand = seconds/10000;
    long secondsThousand = seconds/1000;
    long secondsHundred = seconds/100;
    long secondsTen = seconds/10;
    long secondsOne = seconds;
    long secondsTenth = floatMillis/100;
    long secondsHundredth = floatMillis/10;

    y=1;
    x=1;
    bigdigits(secondsTenThousand);
    x=4;
    bigdigits(secondsThousand-secondsTenThousand*10);
    x=7;
    bigdigits(secondsHundred-secondsThousand*10);
    x=10;
    bigdigits(secondsTen - secondsHundred*10);   
    x=13;
    bigdigits(secondsOne - secondsTen*10);

    lcd.setCursor(0,0);
    lcd.print("                    ");
    lcd.setCursor(1,3);
    lcd.print(millis());

    lcd.setCursor(16,2);
    lcd.print(".");
    lcd.setCursor(17,2);

    lcd.print(secondsTenth - secondsOne*10);
    lcd.print(secondsHundredth- secondsTenth*10);
  }
  else{
    lcd.setCursor(0,0);
    lcd.print("                    ");
    lcd.setCursor(0,1);
    lcd.print("    OUT OF RANGE    ");
    lcd.setCursor(0,2);
    lcd.print("                    ");
    lcd.setCursor(0,3);
    lcd.print("                    ");
  }
}

void displayCurrentLocation(){

  if (millis() - timer > 250) { 
    timer = millis(); // reset the timer
    if (GPS.fix) {

      hours = GPS.hour;
      days = GPS.day;
      months = GPS.month;
      years = GPS.year;
      lat = GPS.latitude;
      lon = GPS.longitude;

      // test dates
      //      hours=8;  
      //      months=7;
      //      days=17;
      //      years=14;

      leapcheck();
      dst();
      timezone(-8);
      twelvehour();

      lcd.setCursor(0,0);

      // print date
      lcd.print(months, DEC); 
      lcd.print('/');
      lcd.print(days, DEC); 
      lcd.print("/");
      lcd.print(years, DEC);   
      lcd.print(" "); 

      // print time
      if (hours <10) lcd.print('0');
      lcd.print(hours, DEC); 
      lcd.print(':');
      if (GPS.minute < 10) lcd.print('0');
      lcd.print(GPS.minute, DEC); 
      lcd.print(':');
      if (GPS.seconds < 10) lcd.print('0');
      lcd.print(GPS.seconds, DEC);

      lcd.setCursor(0,1);

      //  print coords in Decimal Degrees
      //  lat=convertDegMinToDecDeg(GPS.latitude);
      //  lon=convertDegMinToDecDeg(GPS.longitude);
      //  lcd.print("Lat: ");
      //  lcd.print(lat, 4); 
      //  lcd.print(GPS.lat);
      //  lcd.print("\n");
      //  lcd.print("Lon: "); 
      //  lcd.print(lon, 4); 
      //  lcd.print(GPS.lon);
      //  lcd.print("\n");

      // print coords in Degrees, Minutes & Seconds
      int degs = lat/100;
      int mins = lat - degs*100;

      float secs = (lat - degs*100 - mins)*60;
      lcd.print(GPS.lat);
      lcd.print(" "); 
      if (degs<100) lcd.print("0");
      lcd.print(degs); 
      lcd.write(223); 
      lcd.print(" "); 
      if (mins<10) lcd.print("0");
      lcd.print(mins); 
      lcd.print("' "); 
      if (secs<10) lcd.print("0");
      lcd.print(secs, 2); 
      lcd.print('"');
      lcd.print("  "); 

      lcd.setCursor(0,2);

      degs = lon/100;
      mins = lon - degs*100;
      secs = (lon - degs*100 - mins)*60;
      lcd.print(GPS.lon);
      lcd.print(" "); 
      if (degs<100) lcd.print("0");
      lcd.print(degs); 
      lcd.write(223); 

      lcd.print(" ");
      if (mins<10) lcd.print("0");
      lcd.print(mins); 
      lcd.print("' "); 
      if (secs<10) lcd.print("0");
      lcd.print(secs, 2); 
      lcd.print('"'); 
      lcd.setCursor(0,3);


      // print spped in mph
      float mph = GPS.speed*1.15;  // knots to mph
      if (mph<10) lcd.print("0");
      lcd.print(mph, 0); 
      lcd.print("mph ");

      // print altitude in feet
      float alt = GPS.altitude;
      alt=alt*3.28;
      lcd.print(alt, 0);
      lcd.print("'");

      // print fix quatity and number of satellites
      lcd.print(" F:");
      lcd.print(GPS.fixquality, DEC);

      lcd.print(" S:"); 
      lcd.print((int)GPS.satellites);
      if ((int)GPS.satellites < 10) lcd.print(" ");
    }
  }

  if (!GPS.fix){ // if no fix, display animation
    lcd.setCursor(0,0);
    lcd.print("                    ");
    lcd.setCursor(0,1);
    lcd.print("     LOCATING");
    if (millis() - animationTimer > 1000) lcd.print(".");
    if (millis() - animationTimer > 2000) lcd.print(".");
    if (millis() - animationTimer > 3000) lcd.print(".");
    if (millis() - animationTimer > 4000){
      animationTimer = millis(); // reset the timer
      lcd.setCursor(13,1); 
      lcd.print("   "); // clear the .'s
    }
    lcd.setCursor(0,2);
    lcd.print("                    ");
    lcd.setCursor(0,3);
    lcd.print("                    ");
  }

}

double convertDegMinToDecDeg (float degMin) {
  double min = 0.0;
  double decDeg = 0.0;

  //get the minutes, fmod() requires double
  min = fmod((double)degMin, 100.0);

  //rebuild coordinates in decimal degrees
  degMin = (int) ( degMin / 100 );
  decDeg = degMin + ( min / 60 );

  return decDeg;
}


void leapcheck(){  // check for leap year
  years=2000+years;
  leap = years%4 == 0 && years%100 !=0 || years%400 == 0;
  if (leap > 0) montharray[1]=29;
  years=years-2000;
}

void timezone(int tzone){  // set time zone and fix date when wrong

  hours = hours + tzone;
  // for time zones UTC-

  if (hours < 0){
    hours= 24+hours;
    if (days==1){ //fix getting to the first day of the month too soon
      days=(montharray[months-2]);  // montharray[]={31,28 (or29) ,31,30,31,30,31,31,30,31,30,31};
      if (months==1){ //Fix year if UTC is Jan 1st and we're not there yet
        days=31; 
        months=12; 
        years=years-1;
      } 
      else months=months-1;
    } 
    else days=days-1;
  }

  // for time zones UTC+
  if (hours > 24){
    hours= hours-24;
    if (days==montharray[months-1]){ // fix not changing month soon enough
      days==1;
      if (months==12){ // Fix year if UTC is Dec 31st and we're past that
        days=1; 
        months=1; 
        years=years+1;
      } 
      else months=months+1;
    } 
    else days=days+1;
  }

}

void dst(){
  if (digitalRead(DSTPIN)==HIGH) hours=hours+1;
  if (hours==24) hours=0;
}

void twelvehour(){
  if (hours>12) hours=hours-12;
  if (hours==0) hours=12;
}

// clock
void displayClock(){

  if (millis() - timer > 250) { 
    timer = millis(); // reset the timer
    if (GPS.fix) {
      lcd.setCursor(19,0); 
      lcd.write(183);
      seconds = GPS.seconds;
      minutes = GPS.minute;
      hours = GPS.hour;
      days = GPS.day;
      months = GPS.month;
      years = GPS.year;

      leapcheck();
      timezone(-8);
      dst();
      twelvehour();     

      lcd.setCursor(6,1); 
      lcd.print(".");
      lcd.setCursor(6,2); 
      lcd.print(".");

      lcd.setCursor(13,1); 
      lcd.print(".");
      lcd.setCursor(13,2); 
      lcd.print(".");

      x=0; 
      y=1;

      int tempHours=hours/10;
      x=0;
      bigdigits(tempHours);
      x=3;
      bigdigits(hours-(10*tempHours));

      int tempMin=minutes/10;
      x=7;
      bigdigits(tempMin);
      x=10;
      bigdigits(minutes-(10*tempMin));

      int tempSeconds=seconds/10;
      x=14;
      bigdigits(tempSeconds);
      x=17;
      bigdigits(seconds-(10*tempSeconds));
    }  
    else{
      lcd.setCursor(19,0); 
      lcd.write(254);
    }
  }
}

void bigdigits(int time){
  if (time==9) custom9();
  if (time==8) custom8();
  if (time==7) custom7();
  if (time==6) custom6();
  if (time==5) custom5();
  if (time==4) custom4();
  if (time==3) custom3();
  if (time==2) custom2();
  if (time==1) custom1();
  if (time==0) custom0O();
}


void custom0O()
{ // uses segments to build the number 0
  lcd.setCursor(x, y);
  lcd.write(8);  
  lcd.write(1); 
  lcd.write(2);
  lcd.setCursor(x, y+1); 
  lcd.write(3);  
  lcd.write(4);  
  lcd.write(5);
}

void custom1()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(2);
  lcd.print(" ");  
  lcd.setCursor(x,y+1);
  lcd.print(" ");  
  lcd.write(255);
  lcd.print(" ");  
}

void custom2()
{
  lcd.setCursor(x,y);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.write(3);
  lcd.write(7);
  lcd.write(7);
}

void custom3()
{
  lcd.setCursor(x,y);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5); 
}

void custom4()
{
  lcd.setCursor(x,y);
  lcd.write(3);
  lcd.write(4);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.print(" ");  
  lcd.print(" ");  
  lcd.write(255);
}

void custom5()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, y+1);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void custom6()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, y+1);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom7()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.print(" ");  
  lcd.write(8);
  lcd.print(" ");  
}

void custom8()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom9()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.print(" ");  
  lcd.print(" ");
  lcd.write(255);
}

void customA()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(255);
}

void customB()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(5);
  lcd.setCursor(x, y+1);
  lcd.write(255);
  lcd.write(7);
  lcd.write(2);
}

void customC()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(4);
  lcd.write(4);
}

void customD()
{
  lcd.setCursor(x, y);
  lcd.write(255);  
  lcd.write(1); 
  lcd.write(2);
  lcd.setCursor(x, y+1); 
  lcd.write(255);  
  lcd.write(4);  
  lcd.write(5);
}

void customE()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, y+1);
  lcd.write(255);
  lcd.write(7);
  lcd.write(7); 
}

void customF()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, y+1);
  lcd.write(255);
  lcd.print(" ");  
  lcd.print(" ");  
}

void customG()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(4);
  lcd.write(2);
}

void customH()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(4);
  lcd.write(255);
  lcd.setCursor(x, y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(255); 
}

void customI()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(255);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
}

void customJ()
{
  lcd.setCursor(x,y);
  lcd.print(" ");  
  lcd.print(" ");  
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(4);
  lcd.write(4);
  lcd.write(5);
}

void customK()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(4);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(2); 
}

void customL()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.print(" ");  
  lcd.print(" ");  
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(4);
  lcd.write(4);
}

void customM()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(3);
  lcd.write(5);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(254);
  lcd.write(255);
}

void customN()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(2);
  lcd.write(254);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(3);
  lcd.write(5);
}

void customP()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.write(255);
  lcd.print(" ");  
  lcd.print(" ");  
}

void customQ()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(1);
  lcd.write(2);
  lcd.print(" ");  
  lcd.setCursor(x, y+1);
  lcd.write(3);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
}

void customR()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(2); 
}

void customS()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, y+1);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void customT()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(255);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(254);
  lcd.write(255);
  lcd.print(" ");  
}

void customU()
{
  lcd.setCursor(x, y);
  lcd.write(255);  
  lcd.write(254); 
  lcd.write(255);
  lcd.setCursor(x, y+1); 
  lcd.write(3);  
  lcd.write(4);  
  lcd.write(5);
}

void customV()
{
  lcd.setCursor(x, y);
  lcd.write(3);  
  lcd.write(254);
  lcd.write(254); 
  lcd.write(5);
  lcd.setCursor(x, y+1); 
  lcd.print(" ");  
  lcd.write(2);  
  lcd.write(8);
}

void customW()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(254);
  lcd.write(254);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(8);
  lcd.write(2);
  lcd.write(5);
}

void customX()
{
  lcd.setCursor(x,y);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.write(8);
  lcd.write(254);
  lcd.write(2); 
}

void customY()
{
  lcd.setCursor(x,y);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.print(" ");  
  lcd.write(255);
  lcd.print(" ");  
}

void customZ()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(6);
  lcd.write(5);
  lcd.setCursor(x, y+1);
  lcd.write(8);
  lcd.write(7);
  lcd.write(4);
}

void customqm()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, y+1);
  lcd.write(254);
  lcd.write(7);
  lcd.print(" ");  
}

void customsm()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.print(" ");  
  lcd.print(" ");  
  lcd.setCursor(x, y+1);
  lcd.write(7);
  lcd.print(" ");  
  lcd.print(" ");  
}




