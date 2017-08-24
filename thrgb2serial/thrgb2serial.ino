#include <SoftwareSerial.h>

// 2017 Till Bovermann
// programmed at IEM music residency
// http://tai-studio.org

/*
//////////////////////////////////////
Reads in 

+ temperature (10k NTC termistor)
+ humidity (SEN-09569-HIH-4030, Sparkfun SEN-09569 breakout)

and convertes them to SI values (degree C and percent relative humidity).
Sends data as 't'-prefixed csv column.

     t<temperature>,<humidity>

conversion based on 

+ Sparkfun's humidity sensor [hookup guide](https://learn.sparkfun.com/tutorials/hih-4030-humidity-sensor-hookup-guide)
+ adafruits Thermistor [hookup guide](https://learn.adafruit.com/thermistor/using-a-thermistor)

//////////////////////////////////////
Reads in [EZO RGB](https://www.atlas-scientific.com/product_pages/probes/ezo-rgb.html) data.

Sends data as 'e'-prefixed csv column:

     e<r>,<g>,<b>,<l>,<p>,<c>,<i>,<e>

*/

// 1x EZO-RGB (rgb, luminosity, distance)
// print to serial port

#define SENDTEMPERATURE true
#define SENDRGB true
#define REFERENCE EXTERNAL

#define BAUDRATE		38400
#define EZOBAUDRATE		38400
#define EZO_NUMRAWELEMS	11
#define EZO_NUMDATA		8

#define pTemp0 			A0
#define pHumidity		A1
#define pEZO_RX 		8
#define pEZO_TX 		9

#define HIH4030_SUPPLY 		5		// Humidity sensor supply voltage, typically 5 V
#define THERMISTORNOMINAL	10000	// resistance at 25 degrees C
#define TEMPERATURENOMINAL	25		// temp. for nominal resistance (almost always 25 C)
#define BCOEFFICIENT 		3950	// The beta coefficient of the thermistor (usually 3000-4000)
#define TEMPRES 			10000	// other resistor value

float temp0, humidity;
SoftwareSerial ezoSerial(pEZO_RX, pEZO_TX);
String ezoString;

// parsing
bool ezoIndices[] = {true, true, true, false, true, false, true, false, true, true, true}; // EZO_NUMRAWELEMS size
float ezoData[EZO_NUMDATA];


//////////////// Temp / Humidity //////////////////////////////////////////////

// read value from sensor, convert to voltage value
float readHumi(int p) {
	return (float)(analogRead(p)) * HIH4030_SUPPLY / 1023;
}

// convert humidity sensor voltage to relative humidity
float hv2rl(float volt) {
	return ((volt / (.0062 * HIH4030_SUPPLY)) - 25.81);
}

// combine relative humidity and temperature to true relative humidity 
float rl2trl (float rl, float temp) {
	return rl / (1.0546 - (0.00216 * temp));
}

float readTemp(int p) {
	float out;
	out = analogRead(p);
	out = (1023.0 / out)  - 1.0;

	out = TEMPRES / out;  // 10K / (1023/ADC - 1)

	out = out / THERMISTORNOMINAL;     // (R/Ro)
	out = log(out);                  // ln(R/Ro)
	out /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
	out += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	out = 1.0 / out;                 // Invert
	out -= 273.15;              
	return out;
}

//////////////// EZO //////////////////////////////////////////////////////////

bool ezoComm () {
	char c;
	if (Serial.available()) {
	  c = Serial.read();
	  ezoSerial.print(c);
	}
	if (ezoSerial.available()) {
	  c = ezoSerial.read();
	  // echo
	  // Serial.print(c);

	  if(c != 13) {
	  	ezoString = ezoString + c;
	  } else {
	  	return true;
	  }
	}
	return false;
}

int writeEzoData (int i, int j, String data) {
	if (ezoIndices[i]){
		ezoData[j] = data.toFloat();
		return j + 1;
	}
	return j;
}

bool ezoParse (String in) {
	int trCurrent = 0;
	int trLast = -1;
	int dataIdx = 0;
	bool isElem = true;

	if(isdigit(in[0])) {
		// Serial.println("is digit");
		for (int i = 0; i < EZO_NUMRAWELEMS; ++i) {
			trCurrent = in.indexOf(',', trLast + 1); 

			if (trCurrent > 0) {
				dataIdx = writeEzoData(i, dataIdx, in.substring(trLast+1, trCurrent));
			} else {
				if (isElem) {
					dataIdx = writeEzoData(i, dataIdx, in.substring(trLast+1, trCurrent));
					isElem = false;
				} else {
					dataIdx = writeEzoData(i, dataIdx, "-1");
				}
			}
			trLast    = trCurrent;
		}
		return true;
	}
	
	return false;
}

///////////////////////////////////////////////////////////////////////////////

void setup() {
	Serial.begin(BAUDRATE);

	ezoSerial.begin(EZOBAUDRATE);
	ezoString.reserve(30);  // RX buffer

	analogReference(REFERENCE);
}

void loop() {
	if (SENDRGB) {
	// EZO communication
		if (ezoComm()){
			if (ezoParse(ezoString)) {
				Serial.print("e");

				for (int i = 0; i < EZO_NUMDATA; ++i)
				{
					Serial.print(ezoData[i]);
					if (i < EZO_NUMDATA-1) {
						Serial.print(",");
					} else {
						Serial.println();
					}
				}
			}
			ezoString = "";
		}
	}

	if (SENDTEMPERATURE) {
		// read temp and humidity
		temp0 = readTemp(pTemp0);
		humidity = rl2trl(hv2rl(readHumi(pHumidity)), temp0);

		Serial.print("t");
		Serial.print(temp0);
		Serial.print(",");
		Serial.println(humidity);
	}

  	// wait for a2d to settle after last reading:
	delay(2);
}
