#!/usr/bin/env python

import pigpio
import queue

# Represents one grouping of clicks, as determined by a "quiet-time"
# interval between clicks.
class BatEvent:
    def __init__ ( self, pin, quiet):
        self.pin = pin
        self.quietInterval = quiet
        self.startTick = 0
        self.lastTick = 0 
        self.nClicks = 0
        self.duration = 0 
        self.state = 0
    
    def count(self, tick):
        if self.state == 0:
            self.startTick = tick
            self.lastTick = tick
            self.state = 1
            
        self.nClicks = self.nClicks + 1
        diff = pigpio.tickDiff ( self.lastTick, tick )
        self.lastTick = tick
        
        if diff > self.quietInterval:
            return True
        else:
            self.duration = self.duration + diff
            return False
    
    def restart (self, tick):
        self.startTick = tick
        self.lastTick = tick
        self.nClicks = 0
        self.duration = 0
        self.state = 0
    
# Represents one Ping HC-SR04 sensor connected to one GPIO pin
# A bat detector enqueues a set of information for each click detected
class BatDetector:

    """Class to handle pulses from a single bat detector"""

    def __init__(self, pi, gpioPin, ledPin, q ):
   
        self.pi = pi
        self.gpioPin = gpioPin
        self.ledPin = ledPin
        self.q = q
        self.state = 0 
        self.lastTicks = 0

        self.pi.set_mode(gpioPin, pigpio.INPUT)

        self.pi.set_pull_up_down(gpioPin, pigpio.PUD_UP)

        self.cbA = self.pi.callback(gpioPin, pigpio.FALLING_EDGE, self._pulse)

    def _pulse(self, gpio, level, tick):

        """
        ISR -- sort of
        """

        diff = pigpio.tickDiff ( self.lastTicks, tick ) 
        self.lastTicks = tick 
        t = ( tick, gpio, diff, self.state )
        self.q.put ( t, False )
        if self.state == 0 :
            self.state = 1
        else :
            self.state = 0 
        self.pi.write ( self.ledPin, self.state )

    def cancel(self):

        """
        Cancel this ISR
        """

        self.cbA.cancel()

if __name__ == "__main__":

    import time
    import pigpio

    # Pins for bat detection inputs
    batDetect1 = 17
    batDetect2 = 27
    batDetect3 = 22
    
    # Pins for indicator/debugging outputs
    greenLedPin = 23
    redLedPin = 24
    yellowLedPin = 25
    blueLedPin = 5

    # Get a handle to PIGPIO
    pi = pigpio.pi()
    
    # Instantiate queue to hold events from listeners
    q = queue.Queue ( )
    
    # Turn all the leds off
    pi.write ( greenLedPin, 0 )
    pi.write ( redLedPin, 0 )
    pi.write ( yellowLedPin, 0 )
    pi.write ( blueLedPin, 0 )
    
    # Instantiate the listener for each detector
    #
    bat1 = BatDetector ( pi, batDetect1, blueLedPin, q )
    bat2 = BatDetector ( pi, batDetect2, blueLedPin, q )
    bat3 = BatDetector ( pi, batDetect3, blueLedPin, q )
    
    # Create a BatEvent for each BatDetector.  Only one event
    # per BatDetector can be active at one time
    quiet = 10000
    event1 = BatEvent (batDetect1, quiet)
    event2 = BatEvent (batDetect2, quiet)
    event3 = BatEvent (batDetect3, quiet)
    
    # Dequeue events and flash leds
    while 1:
        ( tick, gpio, diff, state ) = q.get ( )
        #print ( str( tick ) + "," + str (gpio) + "," + str(diff) + "," + str(state) )
        if gpio == batDetect1 :
            pi.write ( greenLedPin, state )
            if event1.count(tick) == True:
                print ( str(event1.pin) + "," + str(event1.nClicks) + "," + str(event1.duration))
                event1.restart(tick)
            
        elif gpio == batDetect2 :
            pi.write ( redLedPin, state )
            if event2.count(tick) == True:
                print ( str(event2.pin) + "," + str(event2.nClicks) + "," + str(event2.duration))
                event2.restart(tick)
            
        else:
            pi.write ( yellowLedPin, state )
            if event3.count(tick) == True:
                print ( str(event3.pin) + "," + str(event3.nClicks) + "," + str(event3.duration))
                event3.restart(tick)
            
        
    time.sleep(300)

    # Clean up
    bat1.cancel ( )
    bat2.cancel ( )
    bat3.cancel ( )

     # Turn all the leds off
    pi.write ( greenLedPin, 0 )
    pi.write ( redLedPin, 0 )
    pi.write ( yellowLedPin, 0 )
    pi.write ( blueLedPin, 0 )
    
    pi.stop()

