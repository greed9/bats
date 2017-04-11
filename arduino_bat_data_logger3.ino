// Prototype bat call logger.  Count calls, log to SD card

#include <SPI.h>
#include <SD.h>
//#include <Sleep_nm1.h>
//#include <SimpleTimer.h>
#include "RTClib.h"
#include <Wire.h>

/* Pins used by the data logger
   10 - SD card
   4, 5 - RTC
*/

#define TEMP_INTERVAL_MS 10000
#define BAT_CHANNEL 1
#define TEMP_CHANNEL 2

byte led1Pin = 7 ;
byte led2Pin = 8 ;
byte sensorPin = 3 ;
byte light = 0 ;
char _buf[101] = {0};
RTC_DS1307 RTC;

volatile unsigned long int count = 0 ;
static unsigned long int prevCount = 0 ;
volatile byte state = 0 ;
volatile unsigned long startMicros = 0 ;
volatile unsigned long int startCount = 0 ;


// Kludge to get sane times
DateTime getDateTime ( ) 
{
  int i = 0 ;
  DateTime temp = RTC.now ( ) ;
  while ( temp.year ( ) == 2165 && i < 5 )
  {
    temp = RTC.now ( ) ;
    i++ ;
  }
  return temp ;
}

// ISR
void batDetect ( )
{
  count ++ ;
  if ( state == 0 )
  {
    startMicros = micros ( ) ;
    startCount = count ;
    state = 1 ;
  }
}

void setup()
{

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }

  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning())
  {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  Serial.println("initialization done.");

  pinMode ( led1Pin, OUTPUT ) ;
  pinMode ( led2Pin, OUTPUT ) ;
  pinMode ( sensorPin, INPUT_PULLUP ) ;
  pinMode ( light, OUTPUT) ;
  attachInterrupt ( digitalPinToInterrupt ( sensorPin ), batDetect, RISING);
  digitalWrite ( led1Pin, LOW ) ;
  digitalWrite ( led2Pin, LOW ) ;
}

void loop()
{
  DateTime now ;
  String _customLog = customLog(TEMP_INTERVAL_MS, 1);
  char buf[100] = { 0 } ;
  unsigned int delta = count - prevCount ;
  if ( delta )
  {
    digitalWrite ( led1Pin, HIGH ) ;
    prevCount = count ;
  }
  else
  {
    if ( state )
    {
      // Calculate duration (+/- 20ms) in ms.
      unsigned int durationMs = ( micros ( ) - startMicros ) / 1000L ;

      // Calculate delta count
      unsigned int totalCount = count - startCount ;

      // Calculate seconds and fractions of a sec
      unsigned long int ms = millis ( ) ;
      unsigned int secs = ms / 1000 ;
      unsigned int frac = ms % 1000 ;

     
      sprintf ( buf, "%d,%d,%u,%u", secs, frac, durationMs, totalCount ) ;
      now = getDateTime ( ) ;

      writeLog ( BAT_CHANNEL, String ( buf ), _customLog ) ;
      
      state = 0 ;
      digitalWrite ( led1Pin, LOW ) ;
    }
  }

  if (_customLog != "NULL")
  {
    now = getDateTime ( ) ;
    writeLog(TEMP_CHANNEL, String(buf), _customLog);
  }
  delay ( 10 ) ; // allow time for something to happen
}

String customLog(unsigned int _time, int call)
{
  static unsigned long startMillis = millis();

  if (call == 0)
  {
    if (millis() - startMillis > _time)
    {
      char buff[50];
      sprintf(buff, "%d,", 1024 - analogRead(light));
      startMillis = millis();
      return String(buff);
    }
  }
  else
  {
    if (millis() - startMillis > _time)
    {
      char buff[50];
      sprintf(buff, "%d", 1024 - analogRead(light));
      startMillis = millis();
      return String(buff);
    }
  }
  
    return "NULL";
}

void writeLog(int channel, String data, String otherLog)
{
  File fileLog;
  fileLog = SD.open("Log.txt", FILE_WRITE);
  DateTime now = getDateTime();
  
  sprintf(_buf, "%d/%02d/%02d,%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  
  String temp;
  temp = String(channel) + "," + _buf;

  if (channel == 1)
  {
      temp = temp + "," + data;
  }

  if (otherLog != "NULL")
  {
    temp = temp + "," + otherLog;
  }

  Serial.println(temp);
  Serial.flush();
  if (fileLog)
  {
    fileLog.println(temp);
    fileLog.close();
  }
  else
  {
    Serial.println("Could not write Log");
  }
}

char * TimeToString(unsigned long t)
{
  static char str[12];
  long h = t / 3600;
  t = t % 3600;
  int m = t / 60;
  int s = t % 60;
  sprintf(str, "%04ld:%02.0d:%02.0d", h, m, s);
  return str;
}


