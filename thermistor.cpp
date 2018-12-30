/*
 * Adapted from: https://playground.arduino.cc/ComponentLib/Thermistor2
 *
 * Inputs ADC Value from Thermistor and outputs Temperature in Celsius
 *  requires: include <math.h>
 * Utilizes the Steinhart-Hart Thermistor Equation:
 *    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]3}
 *    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
 *
 * These coefficients seem to work fairly universally, which is a bit of a
 * surprise.
 *
 * Schematic:
 *   [Ground] -- [10k-pad-resistor] -- | -- [thermistor] --[Vcc (5 or 3.3v)]
 *                                               |
 *                                          Analog Pin 0
 *
 * In case it isn't obvious (as it wasn't to me until I thought about it), the analog ports
 * measure the voltage between 0v -> Vcc which for an Arduino is a nominal 5v, but for (say)
 * a JeeNode, is a nominal 3.3v.
 *
 * The resistance calculation uses the ratio of the two resistors, so the voltage
 * specified above is really only required for the debugging that is commented out below
 *
 * Resistance = PadResistor * (1024/ADC -1)
 *
 * I have used this successfully with some CH Pipe Sensors (http://www.atcsemitec.co.uk/pdfdocs/ch.pdf)
 * which be obtained from http://www.rapidonline.co.uk.
 *
 */

#include "Arduino.h"
#include <math.h>

//#define DEBUG_THERMISTOR

// balance/pad resistor value, set this to the measured resistance of your pad resistor
#define PAD_RESISTANCE 20000
#define THERMISTOR_VCC 3.3

float thermistorTemp(int RawADC) {
    long res = (long)(PAD_RESISTANCE * ((1024.0 / RawADC) - 1));
    float temp = log(res); // Saving the Log(resistance) so not to calculate  it 4 times later
    temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
    temp = temp - 273.15;  // Convert Kelvin to Celsius

#ifdef DEBUG_THERMISTOR
    Serial.print("ADC: ");
    Serial.print(RawADC);
    Serial.print("/1024");                           // Print out RAW ADC Number
    Serial.print(", vcc: ");
    Serial.print((float) THERMISTOR_VCC, 2);
    Serial.print(", pad res: ");
    Serial.print((float) PAD_RESISTANCE / 1000, 3);
    Serial.print(" kOhm, v: ");
    Serial.print(((RawADC * THERMISTOR_VCC) / 1024.0), 3);
    Serial.print(", therm. res: ");
    Serial.print((float) res / 1000, 3);
    Serial.println(" kOhm, ");
#endif

    // Uncomment this line for the function to return Fahrenheit instead.
    //temp = (Temp * 9.0)/ 5.0 + 32.0;                  // Convert to Fahrenheit
    return temp;                                      // Return the Temperature
}

