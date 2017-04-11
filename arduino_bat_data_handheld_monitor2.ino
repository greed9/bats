// Prototype bat call logger.  Count calls, log to SD card

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <stdint.h>
#include <TFTv2.h>
#include <SeeedTouchScreen.h> 

#define YP A2   // must be an analog pin, use "An" notation!
#define XM A1   // must be an analog pin, use "An" notation!
#define YM 14   // can be a digital pin, this is A0
#define XP 17   // can be a digital pin, this is A3
#define TS_MINX 116*2
#define TS_MAXX 890*2
#define TS_MINY 83*2
#define TS_MAXY 913*2

typedef struct rect_coord_struct 
{
  int x ;
  int y ;
  int wide ;
  int high ;
  int color ;
} Rect ;

typedef struct tft_text_struct
{
  int x ;
  int y ;
  int size ;
  int color ;
  char* msg ;
} Text ;

byte beepPin = 4 ;
byte sensorPin = 3 ;
byte ledPin = 5 ;

volatile unsigned long int count = 0 ;
static unsigned long int prevCount = 0 ;
volatile byte state = 0 ;
volatile unsigned long startMicros = 0 ;
volatile unsigned long int startCount = 0 ;
unsigned int durationMs = 0;
TouchScreen ts = TouchScreen(XP, YP, XM, YM);
boolean doBeep = true ;

/* flash? */ Rect resetButtonOutline = { 0, 100, 120, 60, BLUE } ;
/* flash? */ Rect muteButtonOutline = { 120, 100, 120, 60, BLUE } ;
/* flash? */ Rect eraseNumbers = { 0, 180, 300, 300, BLACK } ;
Text counter = { 0, 180, 4, CYAN, NULL } ;
/* flash? */ Text resetButtonText = { 0, 110, 4, RED, "Reset" } ;
/* flash? */ Text muteButtonText = { 120, 110, 4, YELLOW, "Beep" } ;
/* flash? */ Rect muteButtonRect = { 120, 110, 120, 60, BLACK } ;


// ISR
void batDetect ( )
{
  count ++ ;
  if ( doBeep )
  {
    if ( ( count % 8 ) == 0  )
    {
      digitalWrite ( beepPin, HIGH ) ;
      digitalWrite ( ledPin, HIGH ) ;
    }
    else
    {
      digitalWrite ( beepPin, LOW ) ;
      digitalWrite ( ledPin, LOW ) ;
    }
  }
 
  if ( state == 0 )
  {
    startMicros = micros ( ) ;
    startCount = count ;
    state = 1 ;
  }
}

void drawRect ( Rect bl )
{
  Tft.drawRectangle(bl.x, bl.y, bl.wide, bl.high, bl.color);
}

void fillRect ( Rect bl )
{
  Tft.fillRectangle(bl.x, bl.y, bl.wide, bl.high, bl.color);
}

void drawText ( Text t )
{
  Tft.drawString(t.msg, t.x, t.y, t.size, t.color);
}

void setup()
{

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  
  TFT_BL_ON;      // turn on the background light
  Tft.TFTinit();  // init TFT library

  Serial.println("initialization done.");

  pinMode ( beepPin, OUTPUT ) ;
  pinMode ( sensorPin, INPUT_PULLUP ) ;
  pinMode ( ledPin, OUTPUT ) ;
  attachInterrupt ( digitalPinToInterrupt ( sensorPin ), batDetect, RISING);

  drawRect ( resetButtonOutline ) ;
  drawText ( resetButtonText ) ;
  drawRect ( muteButtonOutline ) ;
  drawText ( muteButtonText ) ;
  writeNumbers ( count ) ;
  
  digitalWrite ( beepPin, LOW ) ;
  digitalWrite ( ledPin, LOW ) ;
}

void loop()
{
  
  touchScreen();
  
  unsigned int delta = count - prevCount ;
  if ( delta )
  {
    prevCount = count ;
  }
  else
  {
    if ( state )
    {
      // Calculate duration (+/- 20ms) in ms.
      durationMs = ( micros ( ) - startMicros ) / 1000L ;

      // Calculate delta count
      unsigned int totalCount  = count - startCount ;

      // Calculate seconds and fractions of a sec
      unsigned long int ms = millis ( ) ;
      unsigned int secs = ms / 1000 ;
      unsigned int frac = ms % 1000 ;
  
      writeNumbers ( count ) ;
      
      state = 0 ;
     
    }
  }

  // allow time for something to happen
  delay ( 100 ) ; 

  // Shut off LED in case ISR left it on
  digitalWrite ( ledPin, LOW ) ;
}

// Handle touchscreen inputs
void touchScreen()
{
  Point p = ts.getPoint();

  if (p.z == 0)
    return;
    
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, 240);
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, 320);
  
  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  if ( isPressed ( resetButtonOutline, p ) )
  {
    count = 0;
    prevCount = 0 ;
    
    Tft.fillRectangle(0, 180, 300,300 , BLACK);
    digitalWrite ( beepPin, LOW ) ;
    writeNumbers ( count ) ;
    muteButtonText.msg = "Beep" ;
    fillRect ( muteButtonRect ) ;
    drawRect ( muteButtonOutline ) ;
    drawText ( muteButtonText ) ;
    doBeep = true ;
  }

  if ( isPressed ( muteButtonOutline, p ) )
  {
    doBeep = false ;
    muteButtonText.msg = "Mute" ;
    fillRect ( muteButtonRect ) ;
    drawRect ( muteButtonOutline ) ;
    drawText ( muteButtonText ) ;
  }
}

// See if particular rectangle touched
boolean isPressed ( Rect r, Point p ) 
{
  if (p.x > r.x && p.x < r.x + r.wide)
  {
    Serial.println("We did it");
    if (p.y > r.y && p.y < r.y + r.high)
    {
      return true;
    }
  }
  
  return false;
}

void writeNumbers( int num )
{
  char buf[100] = { 0 } ;
  sprintf ( buf, "% 7lu", count) ;
  counter.msg = buf ;
  Serial.println ( count ) ;
  Serial.flush();

  fillRect ( eraseNumbers ) ;
  drawText ( counter );

}




