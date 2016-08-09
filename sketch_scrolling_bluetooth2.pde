import processing.serial.*;

// Framework for scrolling graph
// Grab data from serial port, show it on a scrolling graph,
// push a summary to ThingSpeak (cloud repository) every 15
// seconds

// Test data
String [] fileContents  = null ;
boolean firstTime = true ;
int dataIndex = 0 ;
Timer dataPointTimer = null ;
// end  test data

// Serial stuff from:
// Bluetooth looks like a virtual serial port on Win 8/10
Serial bluePort ;
String blueStr ;
String commPortName ;
int commPortIndex = 2 ; // COM6

Timer updateTimer = null ;  // How often to update graph
ScrollingGraph graph = null ;

DataSeries ds = null ;
ThingSpeakChannel thingSpeak = null ;
float gTotalDuration = 0 ;
float gTotalClicks = 0 ;

// One-time init.  Part of Processing's idiom
void setup ( )
{
  // Window and colors
  size ( 700, 300 ) ;
  background ( 0 ) ;
  fill ( #1C73BC ) ;
  stroke ( #FAF30A  ) ;
  
  // Objects to hold the and display the data periodically
  ds = new DataSeries ( width / 2 ) ;
  updateTimer = new Timer ( 100 ) ;
  dataPointTimer = new Timer ( 1000 ) ; // testing only
  graph = new ScrollingGraph ( 100 ) ;
  
  // Access to the data from the detector via bluetooth
  commPortName = Serial.list ( ) [commPortIndex] ; // rfcomm?
  bluePort = new Serial ( this, commPortName, 9600 ) ;
  
  // wraps pushing to the cloud
  thingSpeak = new ThingSpeakChannel ( 15000 ) ;
}

// Event loop.  Part of Processing's idion.
// Update graph periodically with data received from
// seria/bluetooth (zeros if no new data)
// push result to ThingSpeak
void draw ( )
{

  //String tmp = simDataInput ( "testdata.txt") ;
  String blueStr = null ;
  if ( bluePort.available ( ) > 0 )
  {
    blueStr = bluePort.readStringUntil ( '\n' ) ;
  }
  if ( blueStr != null )
  {
    String [] line = splitTokens ( blueStr, "," ) ;
    float clicks = float ( line[1] ) ;
    float duration = float (line[2] ) ;
    ds.add ( new DataPoint ( 0, clicks )  ) ;
    thingSpeak.accumulateAndPush ( clicks, duration ) ;
  }
  graph.plot ( ds ) ;
}

// Utility function to push data on its own thread
// avoids blocking when 'Net is not available
void pushData ( )
{
   final String TS_API_KEY = "WYSYHPDUQ0NOUHLK" ;
   final String url1 = "https://api.thingspeak.com/update?api_key=" ;
   final String url2 = "&field1=" ;
   final String url3 = "&field2=" ;
   
   // loadStrings does an http GET, and ThingSpeak will update with that
   String [] result = loadStrings ( url1 + TS_API_KEY + url2 + str ( gTotalClicks ) +
     url3 + str ( gTotalDuration ) ) ;
   
   // See what we got back, in case of error
   for ( int i = 0 ; i < result.length ; i++ )
   {
     println ( "pushData: " + result[i] + " end pushData" ) ;
   }
   gTotalClicks = 0 ;
   gTotalDuration = 0 ;
}

// Encapsulates (sort of, see use of thread ( ) method) 
// pushing a running total to ThingSpeak
// at a set interval. 
class ThingSpeakChannel
{
 
  int pushInterval = 15000 ;
  Timer pushTimer = null ;
  float totalClicks = 0 ;
  float totalDuration = 0 ;
  
  ThingSpeakChannel ( int pPushInterval )
  {
    pushTimer = new Timer ( pPushInterval ) ;
    pushInterval = pPushInterval ;
  }
  
  void accumulateAndPush ( float pNewClicks, float pNewDuration )
  {
    totalClicks += pNewClicks ;
    totalDuration += pNewDuration ;
    if ( pushTimer.isExpired ( ) )
    {
      gTotalClicks = totalClicks ;  // set global for pushData function
      gTotalDuration = totalDuration ;
      totalClicks = 0 ;
      totalDuration = 0 ;
      thread ( "pushData" ) ;  // invoke pushData on seperate thread
      pushTimer.start ( pushInterval ) ;
    }
  }
  
} ;

// The graph.  Only supports a single data series for now.
// Should be expanded to handle multiples.
class ScrollingGraph
{
  int currentX = 0 ;
  int xIncr = 0 ;
  int interval = 1000 ;
  //float maxY = 400 ;
  float maxY = 400 ;
  DataPoint dummyPoint = new DataPoint ( 0, 0 ) ;
  
  ScrollingGraph ( int pInterval )
  {
    interval = pInterval ;
    println ( "interval=" + interval ) ;
  }

  void labelXAxis ( String title )
  {
    text ( title, (width / 2) - ( title.length ( ) / 2 ), height - 11 ) ;
  } // end labelXAxis ( )

  void labelYAxis ( String title )
  {
    pushMatrix();
    translate(10, height /2);
    rotate(HALF_PI);
    text(title, -50, 0);
    popMatrix();
  } // end labelYAxis ( )

  // Iterate through data series, scale values, plot them
  void plot ( DataSeries ds )
  {
    int x = 0;
    float prevY = 0 ;
    float prevX = 0 ;
    int xIncr = 2 ;
    
    // Time for another data point?
    if ( updateTimer.isExpired ( ) )
    {
      updateTimer.start ( interval ) ;

      background ( 0 ) ;
      labelXAxis ( "Time" ) ;
      ds.add ( dummyPoint ) ;
      DataPoint tmp = ds.first ( ) ;
      maxY = height - 10 ;
      while ( tmp != null )
      {
        float y = map ( tmp.rawValueY, ds.minRaw, ds.maxRaw, 0.0, maxY ) ;
        y = maxY - y ;
        //ellipse ( float ( x ), y, 10, 10 ) ;
        //rect ( float ( x ), y, 5, y ) ;
        //point ( float ( x ), y ) ;
        line ( prevX, prevY, x, y - 15 ) ;
        prevY = y - 15 ;
        prevX = x ;
        x += xIncr ;
        tmp = ds.next ( ) ;
      }
      labelYAxis ( "Bat Clicks" ) ;
    }
  }
} 
;

// Timer -- return true after specified number of milliseconds
class Timer
{
  int startMillis = 0 ;
  int duration = 0 ;

  Timer ( int pDuration )
  {
    this.start ( pDuration ) ;
  }

  void start ( int pDuration )
  {
    duration = pDuration ;
    startMillis = millis ( ) ;
  }

  boolean isExpired ( )
  {
    
    if ( millis ( ) - startMillis > duration )
    {
      //println ( millis ( ) - startMillis ) ;
      return true ;
    } 
    else
    {
      return false ;
    }
  }
} 
;

// Single data point
class DataPoint
{
  public float rawValueX = 0 ;
  public float rawValueY = 0 ; 

  DataPoint ( float pRawValueX, float pRawValueY ) 
  {
    rawValueX = pRawValueX ;
    rawValueY = pRawValueY ;
  }
} 
;

// Collection of Data Points
class DataSeries
{
  ArrayList points = null ;
  int maxElements = 0 ;
  public float maxRaw = 0 ;
  public float minRaw = 0 ;
  int currentDataPoint = 0 ;

  int getNElements ( )
  {
    return points.size ( ) ;
  }

  DataSeries ( int pMaxElements )
  {
    points = new ArrayList<DataPoint> ( ) ;
    maxElements = pMaxElements ;
  }

  // Return the first data point in the ArrayList
  DataPoint first ( )
  {
    currentDataPoint = 0 ;
    return this.next ( ) ;
  }

  // Return the next data point in the ArrayList
  // or null if at end of list
  DataPoint next ( )
  {
    if ( currentDataPoint < points.size ( ) - 1 )
    {
      DataPoint tmp =  ( DataPoint ) points.get ( currentDataPoint ) ;
      currentDataPoint ++ ;
      return tmp ;
    } else
    {
      return null ;
    }
  }

  // Add a datapoint to the list
  synchronized void add ( DataPoint pDp )
  {

    if ( pDp.rawValueY > maxRaw )
    {
      maxRaw = pDp.rawValueY ;
    }

    if ( pDp.rawValueY < minRaw )
    {
      minRaw = pDp.rawValueY ;
    }

    if ( points.size ( ) < maxElements )
    {
      points.add ( pDp ) ;
    } else
    {
      DataPoint tmp = ( DataPoint ) points.get(points.size() - 1) ;
      points.remove ( 0 ) ;
      points.add ( pDp ) ;
    }
  }
} 
;
// ***** Test code - fakes data (no bluetooth/serial) for testing

// waits specified number of milliseconds, then returns the next data point
// read from the file.  Returns null if it is not time for a new
// data point or if no more data from file.
String simDataInput ( String fname )
{
  String tmp = null ;
  String [] lineData = new String [2] ;

  if ( firstTime )
  {
    fileContents = loadStrings ( fname ) ;
    //println ( fileContents ) ;
    firstTime = false ;
    tmp = fileContents[dataIndex] ;
    //println ( tmp ) ;
    lineData = splitTokens ( tmp, "," ) ;
    //println ( lineData ) ;
    dataPointTimer.start ( int ( lineData[0] ) ) ;
    dataIndex ++ ;
    return lineData[1] ;
  }

  // Time for another data point?
  if ( dataPointTimer.isExpired ( ) )
  {

    if ( dataIndex < fileContents.length - 1 )
    {
      tmp = fileContents[dataIndex] ;
      lineData = splitTokens ( tmp, "," ) ;
      dataPointTimer.start ( int ( lineData[0] ) ) ;
      dataIndex ++ ;
      return lineData[1] ;
    }
  }

  return tmp ;
}

// **** end test code
