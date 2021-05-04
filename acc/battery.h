/*
 * battery.h
 *
 *  Created on: 6 ìàÿ 2019 ã.
 *      Author: RD
 */


/*
From  NCR18650B
Temperature
Charge*: 0 to +45°C
Discharge:-20 to +60°C
Storage:-20 to +50°C

C=3250 mAh
Charge 1/2 C
65mA cut-off at 25°C
At temperatures below 10°C, charge at a 0.25C rate.


Brand Name: Liter energy battery
Type: Li-polymer
Charging voltage: : 4.35V
Discharging voltage: : 2.75V
c = 5500 mAh (>3500)

Recalc
cut-off = 110


*/
#ifndef BATTERY_H_
#define BATTERY_H_

#define VOLTAGE_MIN (3000)
#define VOLTAGE_CHARGED (4250)
#define VOLTAGE_FAST_CHARGE (6144/2)
#define VOLTAGE_CHARGE_RENEWAL (4100)
#define VOLTAGE_CHARGE_END (4150)

//
#define TEMP_MIN (100)
#define TEMP_REC_MIN (1100)
#define TEMP_HI_MIN (3900)
#define TEMP_MAX (4400)

//
#define BatteryC (5500)
#define BatteryIChargeCutOff (130)
#define BatteryImax 1000 //2350

typedef enum  {e_BatteryState_Charge,e_BatteryState_Rest} e_BatteryState;

//extern int BatteryFSM(int T,int V, int I); //return charge current
extern int fChargeCurrent(int Tval,int Vval);

#endif /* BATTERY_H_ */
