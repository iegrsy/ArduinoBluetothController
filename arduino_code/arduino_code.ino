/* Connect to Android via serial Bluetooth and demonstrare the use of interrupt

A serial Bluetooth module is used to create a connection with an Android app (created with MIT AppInventor).
Arduino listens for commands to light some leds or show its status. In addition, a timer interrupt makes it check for temperature via a T36 sensor:
if temperature is greater than a threshold a led is lit; every n seconds (where n is a parameter set through the app) a status report is sent to the app.
A simple command structure enables the app to send parameters and values to Arduino and the other way round.

The circuit:
* Yellow led on Pin 3 with 220 Ohm resistor in series
* Green led  on Pin 4 with 220 Ohm resistor in series
* Red led  on Pin 5 with 220 Ohm resistor in series
* T36 connected to 5V, Pin A0 and Gnd
* JY-MCU Bluetooth Wireless Serial Port Module (slave) connected as follows:
    VCC <--> 5V
    GND <--> GND
    TXD <--> Pin 0 (Rx)
    RXD <--> Pin 1 (Tx)
  The Bluetooth module may interfere with PC to Arduino communication: disconnect VCC when programming the board
* the built-in led on Pin 13 is also used

created 2014 
by Paolo Mosconi

This example code is in the public domain.
*/

// Serial Parameters: COM11 9600 8 N 1 
// \r or \n to end command line
// Bluetooth is on Pin 0 & 1 @ 9600 speed

// Command structure
// CMD RED|GREEN|YELLOW=ON|OFF
// CMD TMAX|SECONDS=value
// CMD SECONDS=value
// CMD STATUS
// CMD ROLE1|ROLE2|ROLE3|ROLE4=ON|OFF

// Status message structure
// STATUS RED|GREEN|YELLOW|ROLE1|ROLE2|ROLE3|ROLE4|TMIN|TMAX|SECONDS|TEMP|THIGH=value

float maxTemp = 30.0; // switch on led when temp > maxTemp
int maxTempSensor = (int) ((maxTemp / 100 + .5) * 204.8);
float temperature = 0.0;

int maxSeconds = 10; // send status message every maxSeconds

const int ledPin = 13;   // temperature led
const int tempPin = A0;  // T36 temperature sensor analog input pin

const int led1Pin = 3; // Yellow
const int led2Pin = 4; // Green
const int led3Pin = 5; // Red

const int role1Pin = 7;  // Role 1
const int role2Pin = 8;  // Role 2
const int role3Pin = 9;  // Role 3
const int role4Pin = 10; // Role 4

volatile int tempVal;
volatile int seconds = 0;
volatile boolean tempHigh = false;
volatile boolean statusReport = false;

String inputString = "";
String command = "";
String value = "";
boolean stringComplete = false;

void setup(){
  //start serial connection
  Serial.begin(9600);
  Serial.print("Max T: ");
  Serial.print(maxTemp);
  Serial.print(" Sensor: ");
  Serial.println(maxTempSensor);

  inputString.reserve(50);
  command.reserve(50);
  value.reserve(50);
  
  pinMode(ledPin, OUTPUT); 
  digitalWrite(ledPin, LOW);
  
  pinMode(led1Pin, OUTPUT); 
  pinMode(led2Pin, OUTPUT); 
  pinMode(led3Pin, OUTPUT);
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  digitalWrite(led3Pin, LOW);

  pinMode(role1Pin, OUTPUT); 
  pinMode(role2Pin, OUTPUT); 
  pinMode(role3Pin, OUTPUT); 
  pinMode(role4Pin, OUTPUT); 
  digitalWrite(role1Pin, LOW);
  digitalWrite(role2Pin, LOW);
  digitalWrite(role3Pin, LOW);
  digitalWrite(role4Pin, LOW);

/*
the following code is needed to initialize the timer interrupt and set it to fire every second, the slowest that Arduino can do
for detailed information see: http://www.instructables.com/id/Arduino-Timer-Interrupts/step1/Prescalers-and-the-Compare-Match-Register/
*/
  cli();          // disable global interrupts
  
  // initialize Timer1 for interrupt @ 1000 msec
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B
 
  // set compare match register to desired timer count:
  OCR1A = 15624;
  // turn on CTC mode:
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  
  sei();          // enable global interrupts
}

// timer interrupt routine
ISR(TIMER1_COMPA_vect)
{
  tempVal = analogRead(tempPin);
  
  if (tempVal > maxTempSensor) {
    digitalWrite(ledPin, HIGH);
    tempHigh = true;
  }
  else {
    digitalWrite(ledPin, LOW);
    tempHigh = false;
  }

  if (seconds++ >= maxSeconds) {
    statusReport = true;
    seconds = 0;
  }
}

// interpret and execute command when received
// then report status if flag raised by timer interrupt
void loop(){
  int intValue = 0;
  
  if (stringComplete) {
    Serial.println(inputString);
    boolean stringOK = false;
    if (inputString.startsWith("CMD ")) {
      inputString = inputString.substring(4);
      int pos = inputString.indexOf('=');
      if (pos > -1) {
        command = inputString.substring(0, pos);
        value = inputString.substring(pos+1, inputString.length()-1);  // extract command up to \n exluded
        //Serial.println(command);
        //Serial.println(value);
        if (command.equals("RED")) { // RED=ON|OFF
          value.equals("ON") ? digitalWrite(led3Pin, HIGH) : digitalWrite(led3Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("GREEN")) { // GREEN=ON|OFF
          value.equals("ON") ? digitalWrite(led2Pin, HIGH) : digitalWrite(led2Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("YELLOW")) { // YELLOW=ON|OFF
          value.equals("ON") ? digitalWrite(led1Pin, HIGH) : digitalWrite(led1Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("ROLE1")) { // ROLE1=ON|OFF
          value.equals("ON") ? digitalWrite(role1Pin, HIGH) : digitalWrite(role1Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("ROLE2")) { // ROLE2=ON|OFF
          value.equals("ON") ? digitalWrite(role2Pin, HIGH) : digitalWrite(role2Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("ROLE3")) { // ROLE3=ON|OFF
          value.equals("ON") ? digitalWrite(role3Pin, HIGH) : digitalWrite(role3Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("ROLE4")) { // ROLE4=ON|OFF
          value.equals("ON") ? digitalWrite(role4Pin, HIGH) : digitalWrite(role4Pin, LOW);
          stringOK = true;
        }
        else if (command.equals("TMAX")) { // TMAX=value
          intValue = value.toInt();
          if (intValue > 0) {
            maxTemp = (float) intValue;
            maxTempSensor = (int) ((maxTemp / 100 + .5) * 204.8);
            stringOK = true;
          }
        }
        else if (command.equals("SECONDS")) { // SECONDS=value
          intValue = value.toInt();
          if (intValue > 0) {
            maxSeconds = intValue;
            stringOK = true;
          }
        }
      } // pos > -1
      else if (inputString.startsWith("STATUS")) {
        Serial.print("STATUS RED=");
        Serial.println(digitalRead(led3Pin));
        Serial.print("STATUS GREEN=");
        Serial.println(digitalRead(led2Pin));
        Serial.print("STATUS YELLOW=");
        Serial.println(digitalRead(led1Pin));
        Serial.print("STATUS ROLE1=");
        Serial.println(digitalRead(role1Pin));
        Serial.print("STATUS ROLE2=");
        Serial.println(digitalRead(role2Pin));
        Serial.print("STATUS ROLE3=");
        Serial.println(digitalRead(role3Pin));
        Serial.print("STATUS ROLE4=");
        Serial.println(digitalRead(role4Pin));
        Serial.print("STATUS TMAX=");
        Serial.println(maxTemp);
        Serial.print("STATUS SECONDS=");
        Serial.println(maxSeconds);
        Serial.print("STATUS TEMP=");
        Serial.println(temperature);
        Serial.print("STATUS THIGH=");
        Serial.println(tempHigh);
        stringOK = true;
      } // inputString.startsWith("STATUS")
    } // inputString.startsWith("CMD ")
    stringOK ? Serial.println("Command Executed") : Serial.println("Invalid Command");
    // clear the string for next iteration
    inputString = "";
    stringComplete = false;
  } // stringComplete
  
  if (statusReport) {
    temperature = (tempVal * 0.0048828125 - .5) * 100;
    Serial.print("STATUS TEMP=");
    Serial.println(temperature);
    Serial.print("STATUS THIGH=");
    Serial.println(tempHigh);
    statusReport = false;
  }

}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    //Serial.write(inChar);
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline or a carriage return, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n' || inChar == '\r') {
      stringComplete = true;
    } 
  }
}


