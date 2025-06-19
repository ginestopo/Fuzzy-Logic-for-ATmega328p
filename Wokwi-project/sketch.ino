#include <SimpleDHT.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define SERVO_PIN 11
#define SENSORHT 7
#define HEAT_RESISTOR 12
int LDR_PIN = A0;

SimpleDHT22 sensorht;
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows
Servo myservo;  // create servo object to control a servo


// LDR Characteristics
const float GAMMA = 0.7;
const float RL10 = 50;

// Function prototypes 
void printLCD(String name, int number = -1, int delayMS = 0, String text = "none");
void updateSensorData();
int getLux();
String getAirQuality();
String getWindDirection();

// Variables to avoid blocking the system
unsigned long previousMillisDisplay = 0;
unsigned long previousMillisTemperature = 0;
unsigned long previousMillisLEDs = 0;
const long displayInterval = 2000; // Screen change interval (in milliseconds)
const long deltaTemperatureInterval = 500; //This is how fast the temperature will be changing when applying hot/cold to the batteries
const long deltaLedIndicatorInterval = 1000; //This is how fast the LED indicator for temperature will be updated
int currentDisplay = 0;
byte lastTemperature = 0;
byte lastHumidity = 0;
int lastLux = 0;
String lastAirQuality = "";
String lastWindDirection = "";
int lastPos = 0;    // variable to store the servo position
int currentBatteryTemp = 10;
String batteryAction = "NONE"; //The action to the batteries can be HEAT, NONE, COOL
int servoPos = 0;

// The 74HC595 uses a type of serial connection called SPI (Serial Peripheral Interface) that requires three pins:
int datapin = 2; 
int clockpin = 3;
int latchpin = 4;

// We'll also declare a global variable for the data we're
// sending to the shift register:
byte data = 0;

/* Hysteresis diagram --> Level 0: Heat battery, Level 1: Do nothing, Level 2: Cool Battery
 *
 *
 *   level
 *   ^
 *   |
 * 4_|
 *   |
 *   |
 *   |
 * 3_|
 *   |
 *   |
 *   | 
 * 2_|. . . . . . . . __________
 *   |                |  |      
 *   |                v  ^      
 *   |                |  |      
 * 1_|. . .___________|__|            
 *   |     |  |       .  .      
 *   |     v  ^       .  .      
 *   |     |  |       .  .      
 * 0_|_____|__|______________________________________|  temperature in centigrades
 *         |  |       |  |      
 *         5  10     45  50    
 *
 */

struct threshold {
    unsigned int low;
    unsigned int high;
    unsigned int level;
};

// number os thresholds
const int NB_THRESHOLDS = 2;

// definition of the thresholds
const struct threshold thresholds[NB_THRESHOLDS] = {
    {5, 10, 1},   // low, high, level
    {45, 50, 2}
};

// Hysteresis function (inspiration from https://github.com/lille-boy/hysteresis/blob/master/hysteresis.c)
unsigned int hysteresis(unsigned int input_temp) {
    static unsigned int current_level = 0;
    static unsigned int prev_temp = 0;

    if (input_temp >= prev_temp) {
        // Ascending - use high threshold
        for (int i = 0; i < NB_THRESHOLDS; i++) {
            if (input_temp >= thresholds[i].high && thresholds[i].level > current_level) {
                current_level = thresholds[i].level;
            }
        }
    } else {
        // Descending - use low threshold
        for (int i = NB_THRESHOLDS - 1; i >= 0; i--) {
            if (current_level == thresholds[i].level && input_temp <= thresholds[i].low) {
                current_level = thresholds[i].level - 1;
                break;
            }
        }
    }

    prev_temp = input_temp;
    return current_level;
}

void clearSerialMonitor() {
  for (int i = 0; i < 50; i++) {
    Serial.println();
  }
}

void shiftWrite(int desiredPin, boolean desiredState){

// This function lets you make the shift register outputs
// HIGH or LOW in exactly the same way that you use digitalWrite().

  bitWrite(data,desiredPin,desiredState); //Change desired bit to 0 or 1 in "data"

  // Now we'll actually send that data to the shift register.
  // The shiftOut() function does all the hard work of
  // manipulating the data and clock pins to move the data
  // into the shift register:

  shiftOut(datapin, clockpin, MSBFIRST, data); //Send "data" to the shift register

  //Toggle the latchPin to make "data" appear at the outputs
  digitalWrite(latchpin, HIGH); 
  digitalWrite(latchpin, LOW); 
}

/*

   _____ ______ _______ _    _ _____  
  / ____|  ____|__   __| |  | |  __ \ 
 | (___ | |__     | |  | |  | | |__) |
  \___ \|  __|    | |  | |  | |  ___/ 
  ____) | |____   | |  | |__| | |     
 |_____/|______|  |_|   \____/|_|     
                                      
                                      
*/

void setup() {
  //------- DHT22 setup -------
  pinMode(SENSORHT, INPUT);
  Serial.println("DHT22 started!");

  //------- LCD setup ----------
  lcd.init(); // initialize the lcd 
  lcd.backlight();

  //------- LDR setup -------
  pinMode(LDR_PIN, INPUT);

  //------- Servo for cooling -----
  myservo.attach(SERVO_PIN);  // attaches the servo on pin 9 to the servo object


  //built-in led
  pinMode(LED_BUILTIN, OUTPUT);

  //heat resistor led
  pinMode(HEAT_RESISTOR, OUTPUT);

  // initialize 74HC595 pins
  pinMode(datapin, OUTPUT);
  pinMode(clockpin, OUTPUT);  
  pinMode(latchpin, OUTPUT);


  Serial.begin(9600);
}


/*


  _      ____   ____  _____  
 | |    / __ \ / __ \|  __ \ 
 | |   | |  | | |  | | |__) |
 | |   | |  | | |  | |  ___/ 
 | |___| |__| | |__| | |     
 |______\____/ \____/|_|     
                             
                             


*/

void loop() {
  unsigned long currentMillis = millis();
  
  // Update sensors continuously
  updateSensorData();
  
  // Update the display in a non-blocking way
  if (currentMillis - previousMillisDisplay >= displayInterval || batteryAction != "NONE") {
    previousMillisDisplay = currentMillis;
    
    if(batteryAction == "NONE"){
      switch(currentDisplay) {
      case 0:
        printLCD("Temperature (*C):", (int)lastTemperature);
        break;
      case 1:
        printLCD("Humidity (%):", (int)lastHumidity);
        break;
      case 2:
        printLCD("Luxes:", lastLux);
        break;
      case 3:
        printLCD("Air quality: ", -1, 0, lastAirQuality);
        break;
      case 4:
        printLCD("Wind Direction: ", -1, 0, lastWindDirection);
        break;
      }

    }else{
      printLCD("Battery Action!: ", -1, 0, batteryAction);
    }
    
    currentDisplay = (currentDisplay + 1) % 5; // Show the next param in the screen

  }

  if (currentMillis - previousMillisLEDs >= deltaLedIndicatorInterval) {
    previousMillisLEDs = currentMillis;
    updateLedIndicator();
  }

  // This part of the code is the one in charge of managing the actions of the hysteresis
  if(currentMillis - previousMillisTemperature >= deltaTemperatureInterval){
    previousMillisTemperature = currentMillis;

    if(currentBatteryTemp != (int)lastTemperature && abs(currentBatteryTemp - (int)lastTemperature)  > 0.5){
      unsigned int currentLevel = hysteresis((int)currentBatteryTemp);

      if(currentLevel == 1){ // the battery does not need action
        batteryAction = "NONE";
        myservo.write(-90);
        digitalWrite(HEAT_RESISTOR, LOW);
        if(currentBatteryTemp - (int)lastTemperature < 0){
        currentBatteryTemp += 1;
        }else{
          currentBatteryTemp -= 1;
        }
      }

      if(currentLevel == 2){ // the battery needs cooling
        batteryAction = "COOL";
        myservo.write(90);
        currentBatteryTemp -= 1;
      }

      if(currentLevel == 0){ // the battery needs heat
        batteryAction = "HEAT";
        digitalWrite(HEAT_RESISTOR, HIGH);
        currentBatteryTemp += 1;
      }
      
      // emulating a clear log to read better
      clearSerialMonitor();

      Serial.println("----------------------------------------------------------");
      Serial.println("Ambient Temperature: " + String((int)lastTemperature));
      Serial.println("Battery Current Temperature: " + String(currentBatteryTemp));
      Serial.println("Current Action: " + batteryAction);
      Serial.println("----------------------------------------------------------");

    }

  }

  //day/night indicator (led ON when night)
  (lastLux > 50) ? digitalWrite(LED_BUILTIN, LOW) : digitalWrite(LED_BUILTIN, HIGH);
}

void updateSensorData() {
  //------- DHT22 data reading ----------
  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
  if(!sensorht.read(SENSORHT, &temperature, &humidity, data)) {
    lastTemperature = temperature;
    lastHumidity = humidity;
  }
  
  // Update other sensors
  lastLux = getLux();
  lastAirQuality = getAirQuality();
  lastWindDirection = getWindDirection();
}

// printLCD without delay
void printLCD(String name, int number = -1, int delayMS = 0, String text = "none"){
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print(name);   
  lcd.setCursor(2, 1);
  if(number != -1)        
    lcd.print(number);
  else
    lcd.print(text);
}

int getLux(){
  int analogValue = analogRead(A0);
  float voltage = analogValue / 1024. * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  return lux;
}

String getAirQuality(){
  int analogValue = analogRead(A1);
  if(analogValue > 515)
    return "Good!";
  else 
    return "Bad :(";
}

String getWindDirection(){
  int y = analogRead(A2);
  int x = analogRead(A3);
  const int DEADZONE = 100;
  int centerX = 512;
  int centerY = 512;

  if (abs(x - centerX) < DEADZONE && abs(y - centerY) < DEADZONE) {
    return "no wind";
  }

  if (abs(x - centerX) > abs(y - centerY)) {
    if (x > centerX + DEADZONE) return "West";
    if (x < centerX - DEADZONE) return "East";
  } else {
    if (y > centerY + DEADZONE) return "North";
    if (y < centerY - DEADZONE) return "South";
  }

  return "wind error";
}

void updateLedIndicator(){
  // constrain the value before mapping
  int constrainedLux = constrain(lastLux, 0, 10000); 
  // map to 0-8 because we have 8 leds
  int ledsToTurnOn = map(constrainedLux, 0, 10000, 0, 9);
  // avois getting out of range
  ledsToTurnOn = constrain(ledsToTurnOn, 0, 8); 
  // Turn on the necessary LEDs
  for (int i = 0; i < ledsToTurnOn; i++) {
    shiftWrite(i, HIGH);
  }
  // Turn off the rest
  for (int i = ledsToTurnOn; i < 8; i++) {
    shiftWrite(i, LOW);
  }
}
