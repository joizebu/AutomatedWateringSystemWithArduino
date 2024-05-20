#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>

const int WATERING_DURATION_IN_MS = 5000; // = 5 seconds
const int WATERING_THRESHOLD = 620;

const int HYGROMETER_UPDATE_INTERVAL_IN_S = 2*60*60; // = 2 hours

const int RELAY_PIN = 9;
const int HYGROMETER_PIN = A1;

volatile int waited_time = 0;

// Interrupt service routine for the watch dog timer
// it will be called about every 8s
ISR(WDT_vect){
  waited_time += 8;
}

// setup watchdog timer
void setup_timer(){
  // turn off interrupts 
  cli();
  // delete watchdog timer reset flag
  MCUSR &= ~(1<<WDRF); 
  // set WDCE, to enable changing of watchdog timer setting
  // e.g. for changing prescaler           
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set timer to 8s, disable system reset
  WDTCSR = 1<<WDP0 | 1<<WDP3;
  // enable watchdog timer interrupts
  WDTCSR |= 1<<WDIE;

  // turn on interrupts
  sei();
}

void setup_relais(){
  pinMode(RELAY_PIN, OUTPUT);
  //relais + pump are configured to be active low
  //-> set output to high to deactivate pump
  digitalWrite(RELAY_PIN, HIGH);
}

void enter_sleep(void){
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  sleep_enable();
  sleep_cpu();
  sleep_disable(); 
}

void water_plant(){
  // activate pump
  digitalWrite(RELAY_PIN, LOW);
  // wait a bit to give the pump time to water the plant
  delay(WATERING_DURATION_IN_MS);
  // deactivate pump
  digitalWrite(RELAY_PIN, HIGH);
}

int read_hygrometer(){
  return analogRead(HYGROMETER_PIN);
}

void setup() {
  setup_relais();
  setup_timer();
}

void loop() {
  // sleep for energy conservation; wake_up with watchdog-timer
  enter_sleep();
  
  // turn off interrupts 
  cli();
  // check if your waiting interval is over
  if(waited_time >= HYGROMETER_UPDATE_INTERVAL_IN_S){
    // turn on interrupts
    sei();
    // read the current humidity of the soil
    int hygrometer_value = read_hygrometer();

    // compare the hygrometer value to the threshold 
    // to check if the the soil is too dry.
    // if so, water the plant
    if(hygrometer_value >= WATERING_THRESHOLD){
      water_plant();      
    }
  
    // check if your waiting interval is over
    cli();
    // reset waited time
    waited_time = 0;
  }
  // turn on interrupts
  sei();
}
