#include <OneWire.h>
//#include <DallasTemperature.h>

// DEBUG
#include <SoftwareSerial.h>
#define RX 3
#define TX 4
SoftwareSerial Serial(RX, TX);
// DEBUG

#define PWM_OUT_PIN 1
#define TEMP_PIN    2
#define PIEZO_PIN   0

#define TEMP_PANIC  65

OneWire oneWire(TEMP_PIN);
//DallasTemperature sensors(&oneWire);

int CTemp = 0;    // Current temperature
int CPWM  = 0;    // Current PWM-frequency
unsigned long lastTempWarnMillis = 0;
unsigned long lastSensWarnMillis = 0;

void setup(void) {
  //sensors.begin();
  pinMode(PWM_OUT_PIN, OUTPUT);
  pinMode(PIEZO_PIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("init");
  lastTempWarnMillis = millis();
  lastSensWarnMillis = millis();
}

void loop(void) {
  Serial.print("> ");
  CTemp = getTemp();

  /* If the temperature returned is -127C, the sensor doesn't work
   * I havent any code for other extreme cases, as i don't think it is nececarry
   * Play sound, print to console, make sure fan runs on max just to be sure,
   * return whitout running any other code */
  if (CTemp == -127) {

    if ((millis() - lastSensWarnMillis) > 10000) {
      CPWM = 255;
      analogWrite(PWM_OUT_PIN, CPWM);

      // Annoying sound
      Tone(PIEZO_PIN, 440, 150);  // C
      delay(100);
      Tone(PIEZO_PIN, 440, 150);  // C
      delay(100);
      Tone(PIEZO_PIN, 440, 150);  // C
      delay(100);
      Tone(PIEZO_PIN, 440, 150);  // C
      lastSensWarnMillis = millis();
    }

    Serial.println("   ERR...");
    return;
  }

  /* I decided to use a simple linear scale to determine the fan-speed,
   * the function i ended up with was (where f is fan speed, and x is temp)
   *   f(x) = 7.8x + 135
   * When temp=25, fan=60 ... when temp=50, fan=255
   * wheras the numbers below */
  CPWM = (CTemp * 7.8) - 135;

  if (CPWM > 255) { CPWM = 255; }  // Just to be sure, scale max value
  if (CPWM < 0) { CPWM = 0; }      // Just to be sure, scale min value
  if (CTemp >= 50) { CPWM = 255; } // Should follow the function above, but just to be sure
  if (CTemp <= 29) { CPWM = 0; }   // I dont think the fan is nececarry under 25 degrees

  // Set fan speed now, just in case the code below crashes. So we are sure that
  // the fan spins anyways. That beeing the most important of course.
  analogWrite(PWM_OUT_PIN, CPWM);

  // Play a warning sound every twenty seconds if the temperature is to high
  if (CTemp >= TEMP_PANIC) {
    if ((millis() - lastTempWarnMillis) > 15000) {
      // Annoying sound
      Tone(PIEZO_PIN, 440, 300);  // C
      Tone(PIEZO_PIN, 370, 150);  // F#
      Tone(PIEZO_PIN, 440, 300);  // C
      Tone(PIEZO_PIN, 370, 150);  // F#
      lastTempWarnMillis = millis();
    }
  }

  Serial.print("   ");
  Serial.print(CTemp);
  Serial.print(" C   PWM: ");
  Serial.println(CPWM);
}

int getTemp() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];

  if ( !oneWire.search(addr) ) {
      Serial.print("No sensor found.");
      oneWire.reset_search();
      delay(250);
      return - 127;
  }
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!");
      return -127;
  }

  switch (addr[0]) {
    case 0x10:
      Serial.print("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.print("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.print("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.print("   Device is not a DS18x20 family device.");
      return -127;
  }
  oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);
  // we might do a oneWire.depower() here, but the reset will take care of it.
  present = oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = oneWire.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  lastSensWarnMillis = millis(); // we will get falses, so the sensor should beep if it is 10 seconds since it actually worked
  return (int)raw / 16.0;
}

// Halting tone playing method, (PWM)
void Tone(byte pin, uint16_t frequency, uint16_t duration) {
  unsigned long startTime  = millis();
  unsigned long halfPeriod = 1000000L / frequency / 2;

  while ((millis() - startTime) < duration) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(pin, LOW);
    delayMicroseconds(halfPeriod);
  }
}
