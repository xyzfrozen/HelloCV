#include"unitlib.h"
constexpr double metersPerFoot=0.3048;
constexpr double kilogramsPerPound=0.45359237;

double celsiusToFahrenheit(double c){return c*1.8+32;}
double fahrenheitToCelsius(double f){return (f-32)/1.8;}
double metersToFeet(double m){return m/metersPerFoot;}
double feetToMeters(double ft){return ft*metersPerFoot;}
double poundsToKilograms(double lb){return lb*kilogramsPerPound;}
double kilogramsToPounds(double kg){return kg/kilogramsPerPound;}
