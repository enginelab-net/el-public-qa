Feature: Speed density

Output patches: "AF_SD_Airflow [g/s]"

Required inputs:
"AF_SD_VE [%]"
"AF_SD_P [Pa]"
"AF_SD_C [K]"
"AF_SD_I [K]"
"AF_SD_B [n]"
 
Sub-features:
- None

This submodel will compute the airflow using the Speed density item.

MassAirFlow (gms/s) = ((RPM * Volume/revolution * "AF_SD_P [Pa]") / (R * Temp)) * ("AF_SD_VE [%]" / 100) * (1/60);

Where:
Temp = ("AF_SD_C [K]" * "AF_SD_B [n]") + ("AF_SD_I [K]" + (1 - "AF_SD_B [n]")) 
R = 0.28705 J/g*degK

