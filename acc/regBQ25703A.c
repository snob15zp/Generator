/*
 * regBQ25703A.c
 *
 *  Created on: Feb 19, 2019
 *      Author: 2382
 */

#include "regBQ25703A.h"
#include "DriverBQ25703.h"

const t_I2c16InitData bq25703InitData[eBQ25703_NumOfReg]=
{
		{{I2C_OP_WRITE,0x0,		1,2},0x820f/*0x020e 0xe20e*/}, // ChargeOption0()							0
//0b: Disable Low Power Mode. Device in performance mode with battery only.
		//The PROCHOT, current/power monitor buffer and comparator follow register		setting.
//01b: Enabled, 5 sec
//1b: Enable this function. IDPM is disabled when CELL_BATPRESZ goes LOW.
       //IDPM Auto Disable
//0b: Disable <default at POR>
       //Add OTG to CHRG_OK Drive CHRG_OK to HIGH when the device is in OTG mode.
//1b: Set minimum PFM burst frequency to above 25 kHz to avoid audio noise
      //Out-of-Audio Enable
//1b: 800 kHz <default at POR>
//0b: Reserved
      //0b00110110

//0b:Reserved
//0b:Reserved
//0b: Disable LEARN Mode <default at POR>
//1b: 40 //20 <default at POR>
    //IADPT Amplifier Ratio
//1b: 16 <default at POR>
		//IBAT Amplifier Ratio
//1b: Enable LDO mode, Precharge current is set by the ChargeCurrent register
                //and clamped below 384 mA (2 cell – 4 cell) The system is
                //regulated by the MinSystemVoltage register. <default at POR>
//1b: IDPM enabled <default at POR>
//1b: Inhibit Charge
		// 0b00011111

		{{I2C_OP_WRITE,0x02,	1,2},0x0000}, // ChargeCurrent()										1
		{{I2C_OP_WRITE,0x04,	1,2},0x20d0/*0x20d0*/}, // MaxChargeVoltage()										2
		{{I2C_OP_WRITE,0x30,	1,2},0x0200/*0x0210*/}, // ChargeOption1()										3
//0b Turn off IBAT buffer to minimize Iq <default at POR>
//00b: Disable low power PROCHOT <default at POR>
//0b: Turn off PSYS buffer to minimize Iq <default at POR>
//0b: 10 mO <default at POR>
//0b: 10 mO <default at POR>
//1b: 1 uA/W <default at POR>
//0b Reserved
		//00000010
//0b: 2.3 V <default at POR>
//0b: When CMPIN is above internal threshold, CMPOUT is LOW (internal
                   //hysteresis) <default at POR>
//00b: Independent comparator is disabled
//0b: Disable this function <default at POR>
		//Force Power Path Off on comparator
//0b: reserved
//0b: Disable shipping mode <default at POR>
//0b: Disable
		 //Auto Wakeup Enable
		//00000000

		{{I2C_OP_WRITE,0x32,	1,2},0x003D/*0x00BD*//*0x02ba*/}, // ChargeOption2()								4
//00b: 1 ms <default at POR>
		//Input Overload time in Peak Power Mode
//0b: Disable peak power mode triggered by input current overshoot <default at POR>
//0b: Disable peak power mode triggered by system voltage under-shoot <default at POR>
//0b: Not in peak power mode. Write 0 to get out of	overloading cycle.<default at POR>
//0b: Not in relaxation cycle. Write 0 to get out of relaxation cycle. <default at POR>
//00b: 5 ms		Peak power mode overload and relax cycle time.
		//0x0

//1b: Input current limit is set by the lower value of ILIM_HIZ pin and
                   //                   REG0x0F/0E. <default at POR>
//0b: IBAT pin as discharge current. <default at POR>
//1b: 150 mV <default at POR>   //TODO Q2 OCP threshold by sensing Q2 VDS
//1b: 150 mV <default at POR> 	Input current OCP threshold by sensing ACP-ACN =>30A
//1b: ACOC threshold 125% or 200% ICRIT,w.r.t. ILIM2 in REG0x37[7:3]
		//ACOC Enable
//1b: 210% of ICRIT <default at POR>
		//Set MOSFET OCP threshold as percentage of IDPM with current sensed from RAC.
//0b: Disable BATOC //for OTG, I think.
		//Battery discharge overcurrent (BATOC) protection by sensing the voltage
		//across SRN and SRP. Upon BATOC, converter is disabled.
//1b: 200% of PROCHOT IDCHG <default at POR>
		    //0xBD

		{{I2C_OP_WRITE,0x34,	1,2},0x0000 /*0x8800*/}, // ChargeOption3()										5
//1b: Device in Hi-Z mode
//0b: Idle <default at POR> Reset
		//TODO exept VINDPM
//0b: Idle Reset VINDPM Threshold
//0b: Disable OTG <default at POR> //Tablet Power from OTG
//1b: Enable ICO algorithm.
		 //TODO ICO
//000b: Reserved
		//0x88
//000000b: Reserved
//0b: BATFET on during Hi-Z <default at POR>
//0b: PSYS as battery discharge power minus OTG output power <default at POR>
		//0x00

		{{I2C_OP_WRITE,0x36,	1,2},0b0100101001010100}, // ProchotOption0()									6
//01001b: 	Default 150%, ILIM2 Threshold
//01b: 100 us <default at POR>
//0B: Reserved
		//0b01001010
//01b: 6 V (2-4 s) or 3.1 V (1 s) <default at POR>
		//Measure on VSYS with fixed 20-us deglitch time. Trigger when SYS pin voltage is
		//below the threshold.
//0b: Disable pulse extension <default at POR>
//10b: 10 ms <default at POR>
//1b: Idle <default at POR>	PROCHOT Pulse Clear
//0b: 1 ms (must be max) <default at POR> INOM Deglitch Time
		//INOM is always 10% above IDPM in 0x0F/0EH. Measure current between ACP
		//and ACN.	Trigger when the current is above this threshold.
//0b:Reserved
		//0b01010100
		{{I2C_OP_WRITE,0x38,	1,2},0x8100/*0x817f*/}, // ProchotOption1()												7
//100000b: Default: 16384 mA  //IDCHG Threshold
		//Trigger when the discharge current is above the threshold.
		//TODO IDCHG Threshold Dependeses?
//01b: 100 us <default at POR>
		//0b10000001

//0b:Reserved
//0x7f: //0000000b:When all the [6:0] bits are 0, PROCHOT function is disabled.
		//TODO Dependeses?

		{{I2C_OP_WRITE,0x3a,	1,2},0xE0FF/*0xC0FF*/}, // ADCOption()													8
//1b: Continuous update.
//0b: No ADC conversion
//1b: 3.06 V <default at POR> 	ADC input voltage range.
//00000b: Reserved
		//0xa0

//1b: enable EN_ADC_CMPIN
//1b: enable EN_ADC_VBUS
//1b: enable EN_ADC_PSYS
//1b: enable EN_ADC_IIN
//1b: enable EN_ADC_IDCHG
//1b: enable EN_ADC_ICHG
//1b: enable EN_ADC_VSYS
//1b: enable EN_ADC_VBAT
		//0xff

		{{I2C_OP_READ,0x20,	1,2},0x0010}, // ChargerStatus()													9
//0b00010000: After the SYSOVP is removed, the user must write a 0 to this bit
		//to enable the	converter again.
		{{I2C_OP_READ,0x22,	1,2},0}, // ProchotStatus()															10
		{{I2C_OP_READ,0x24,	1,2},0}, // IIN_DPM()																11
		{{I2C_OP_READ,0x26,	1,2},0}, // ADCVBUS/PSYS()															12
		{{I2C_OP_READ,0x28,	1,2},0}, // ADCIBAT()																13
		{{I2C_OP_READ,0x2a,	1,2},0}, // ADCIINCMPIN()															14
		{{I2C_OP_READ,0x2c,	1,2},0}, // ADCVSYSVBAT()															15
		{{I2C_OP_WRITE,0x06,	1,2},0}, // OTGVoltage()
//-
		{{I2C_OP_WRITE,0x08,	1,2},0}, // OTGCurrent()
//-
		{{I2C_OP_WRITE,0x0a,	1,2},0}, // InputVoltage()
//0x0000: 3.2V?
		{{I2C_OP_WRITE,0x0c,    1,2},0x1800}, // MinSystemVoltage()
//((V/256)<<8)&(0b0011111100000000);
		//0x1800 6144mV
		{{I2C_OP_WRITE,0x0e,	1,2},0x0000}, // IIN_HOST()
//0x0000 => 100mA
		{{I2C_OP_READ,0x2e,	1,1},0}, // ManufacturerID() //40h
		{{I2C_OP_READ,0x2f,	1,1},0} // DeviceID() //2Fh

};

inline uint16_t BQ25703_ChargeCurrent_Eval(uint16_t I)//arg in mA
{
	//In order to prevent any accidental SW mistakes, the host sets low input current limit (a few hundred milliamps)
	//when device is out of HIZ.
	return ((I/64)<<6)&(0b0001111111000000);//10 Om res
}
inline uint16_t BQ25703_ChargeVoltage_Eval(uint16_t V)//arg in mV
{
	return ((V/16)<<4)&(0b0111111111110000);
}
inline uint16_t BQ25703_MinSystemVoltage_Eval(uint16_t V)//arg in mV
{
	return ((V/256)<<8)&(0b0011111100000000);
}
inline uint16_t BQ25703_IIN_HOST_Eval(uint16_t I)//arg in mA
{
	//With code 0, the input current limit is 50 mA.
	if (I<50) {I=50;};//  10 Ohm //if (I<100) {I=100;};
	return (((I)/50)<<8)&(0x7f00);//10 Ohm //5 Om RES
}

inline uint16_t BQ25703_InputVoltage_Eval(uint16_t V)//arg in mA
{
	if (V<3200) {V=3200;};
	return (((V)/64)<<6)&(0x3fc0);//
}

inline uint16_t BQ25703_IBAT_CH(uint16_t data16)
{
	return (data16>>8)*64;
}
inline uint16_t BQ25703_IBAT_DCH(uint16_t data16)   
{
	return (data16&0xff)*256;
}
inline uint16_t BQ25703_VSYS(uint16_t data16)//V. mV
{
	return (data16>>8)*64+2880;
}
inline uint16_t BQ25703_VBAT(uint16_t data16) //V. mV  
{
	return (data16&0xff)*64+2880;
}

