// Based on:
// Example in https://github.com/khoih-prog/megaAVR_TimerInterrupt
// Example in https://github.com/0xPIT/encoder/tree/master

/****************************************************************************************************************************
  ISR_Timers_Array_Simple.ino
  For Arduino megaAVR ATMEGA4809-based boards (UNO WiFi Rev2, NANO_EVERY, etc. )
  Written by Khoi Hoang

  Built by Khoi Hoang https://github.com/khoih-prog/megaAVR_TimerInterrupt
  Licensed under MIT license

  Now with we can use these new 16 ISR-based timers, while consuming only 1 hwarware Timer.
  Their independently-selected, maximum interval is practically unlimited (limited only by unsigned long miliseconds)
  The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
  Therefore, their executions are not blocked by bad-behaving functions / tasks.
  This important feature is absolutely necessary for mission-critical tasks.
*****************************************************************************************************************************/

#if !( defined(__AVR_ATmega4809__) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_AVR_NANO_EVERY) || \
      defined(ARDUINO_AVR_ATmega4809) || defined(ARDUINO_AVR_ATmega4808) || defined(ARDUINO_AVR_ATmega3209) || \
      defined(ARDUINO_AVR_ATmega3208) || defined(ARDUINO_AVR_ATmega1609) || defined(ARDUINO_AVR_ATmega1608) || \
      defined(ARDUINO_AVR_ATmega809) || defined(ARDUINO_AVR_ATmega808) )
#error This is designed only for Arduino or MegaCoreX megaAVR board! Please check your Tools->Board setting
#endif

// These define's must be placed at the beginning before #include "megaAVR_TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

// Select USING_16MHZ     == true for  16MHz to Timer TCBx => shorter timer, but better accuracy
// Select USING_8MHZ      == true for   8MHz to Timer TCBx => shorter timer, but better accuracy
// Select USING_250KHZ    == true for 250KHz to Timer TCBx => shorter timer, but better accuracy
// Not select for default 250KHz to Timer TCBx => longer timer,  but worse accuracy
#define USING_16MHZ     true
#define USING_8MHZ      false
#define USING_250KHZ    false

#define USE_TIMER_0     false
#define USE_TIMER_1     true
#define USE_TIMER_2     false
#define USE_TIMER_3     false

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "megaAVR_TimerInterrupt.h"

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "megaAVR_ISR_Timer.h"

// #include <SimpleTimer.h>              // https://github.com/jfturcot/SimpleTimer

// For oled:
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 // This is a test oled lying around, the final one will be height 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// For interrupt:

ISR_Timer ISR_Timer1;

#ifndef LED_BUILTIN
	#define LED_BUILTIN       13
#endif

#define LED_TOGGLE_INTERVAL_MS        1000L

// You have to use longer time here if having problem because Arduino AVR clock is low, 16MHz => lower accuracy.
// Tested OK with 1ms when not much load => higher accuracy.
#define TIMER1_INTERVAL_MS            1L

volatile uint32_t startMillis = 0;

volatile uint32_t divMillis = 1;

volatile uint32_t deltaMillis2s = 0;
volatile uint32_t deltaMillis5s = 0;

volatile uint32_t previousMillis2s = 0;
volatile uint32_t previousMillis5s = 0;

#include <ClickEncoder.h>

ClickEncoder *encoder1;
int16_t last, value;


void TimerHandler1()
{
	static bool toggle  = false;
	static int timeRun  = 0;

	ISR_Timer1.run();

  if (timeRun % divMillis == 0) encoder1->service(); // run every millisecond

	// Toggle LED every LED_TOGGLE_INTERVAL_MS = 2000ms = 2s, to be DELETED
	if (++timeRun == ((LED_TOGGLE_INTERVAL_MS) / TIMER1_INTERVAL_MS) )
	{
		timeRun = 0;

		//timer interrupt toggles pin LED_BUILTIN
		digitalWrite(LED_BUILTIN, toggle);
		toggle = !toggle;
	}
}

/*
void doingSomething2s()
{
	unsigned long currentMillis  = millis();

	deltaMillis2s    = currentMillis - previousMillis2s;
	previousMillis2s = currentMillis;
}

void doingSomething5s()
{
	unsigned long currentMillis  = millis();

	deltaMillis5s    = currentMillis - previousMillis5s;
	previousMillis5s = currentMillis;
}
*/


void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(115200);
  encoder1 = new ClickEncoder(3, 2, 4);

	while (!Serial);

	Serial.print(F("\nStarting ISR_Timers_Array_Simple on "));
	Serial.println(BOARD_NAME);
	Serial.println(MEGA_AVR_TIMER_INTERRUPT_VERSION);
	Serial.print(F("CPU Frequency = "));
	Serial.print(F_CPU / 1000000);
	Serial.println(F(" MHz"));

	Serial.print(F("TCB Clock Frequency = "));

#if USING_16MHZ
	Serial.println(F("16MHz for highest accuracy"));
#elif USING_8MHZ
	Serial.println(F("8MHz for very high accuracy"));
#else
	Serial.println(F("250KHz for lower accuracy but longer time"));
#endif

	ITimer1.init();

	if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS, TimerHandler1))
	{
		Serial.print(F("Starting  ITimer1 OK, millis() = "));
		Serial.println(millis());
	}
	else
		Serial.println(F("Can't set ITimer1. Select another freq. or timer"));

	// ISR_Timer1.setInterval(2000L, doingSomething2s);
	// ISR_Timer1.setInterval(5000L, doingSomething5s);

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Initialize with I2C address 0x3C

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("The BELA Pepper display");
  display.display();

}

void loop()
{
	// This unadvised blocking task is used to demonstrate the blocking effects onto the execution and accuracy to Software timer
	// You see the time elapse of ISR_Timer still accurate, whereas very unaccurate for Software Timer
	// The time elapse for 2000ms software timer now becomes 3000ms (BLOCKING_TIME_MS)
	// While that of ISR_Timer is still prefect.
	// delay(BLOCKING_TIME_MS);

	// You need this Software timer for non-critical tasks. Avoid abusing ISR if not absolutely necessary
	// You don't need to and never call ISR_Timer.run() here in the loop(). It's already handled by ISR timer.
	// simpleTimer.run();
  value += encoder1->getValue();
  
  if (value != last) {
    last = value;
    Serial.print("Encoder 1 Value: ");
    Serial.println(value);
    display.clearDisplay();  // clear the display
    display.setCursor(0, 0); // set cursor position
    display.print("Encoder 1 Value: "); // print something
    display.println(value); // Print something more and end the line
    display.display(); // Send the text to the display

  }
  
  ClickEncoder::Button b = encoder1->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          encoder1->setAccelerationEnabled(!encoder1->getAccelerationEnabled());
          Serial.print("  Acceleration is ");
          Serial.println((encoder1->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    }
  }  
}
