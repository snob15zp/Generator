# Generator

20.08.04
TrueStudio_prj_02_190315_Drivers_82_86_703_610
200902
Generator_7_RD_acc: 25703 and 28z610 work
Generator_8_RD_acc: actual version. 
Generator_9_RD_acc: 25703 and 28z610 work, i2c modified.
Generator_10_RD_acc: some debug of FSM was done.
Generator_11_RD_acc: i2c tcr on receive was done
Generator_12_RD_acc: actual version. Plan: heating with max power, debug 25703 mode, Analize tps65987 state for repair connector.

GenDisp_VV3_RD1_Work - 20 09 28  display from YM + uGFX_only_Compile.   Display Blue
GenDisp_VV3_RD2_Work - 20 09 30 uGFX display driver works, blue rectengle is showed.
GenDisp_VV3_RD3 20 10 01 Display button. i2c2 has bug when recive (or transmitt ?) 1 byte. Touch Driver was include to program.
GenDisp_VV3_RD4 20 10 01 Buttom+List
GenDisp_VV3_RD5 20 10 03 i2c2 works, touchscreen works
GenDisp_VV3_RD6 20 10 03 i2c2 time control, remove calibration
GenDisp_VV3_RD8 20 10 04  Prepare for UNION 
GenDisp_VV3_RD9 20 10 04  Actual vertion, Prepare for UNION 
GenACC_RD13_ForUnion 20 10 06 Remove unused files
GenACC_RD14_ForUnion 20 10 06 Actual vertion 

GenUnion_DispVV3RD9_Plus_ACC14 - 20 10 07 Works, fixed.
Gen_27_DispVV3RD9_ACC14 - 20 10 07  from GenUnion_DispVV3RD9_Plus_ACC14
Gen_28_DispVV3RD9_ACC14 - 20 10 08  from 27. display debug info. Fix some fiches in ACC: (time error in i2c1, charge voltage)
Gen_29_Disp_ACC 20 10 12 version from 28: fix i2c2 error
Gen_30_Disp_ACC 20 10 12  version from 29 done: Sceleton of Powercontrol, Sceleton of FSM for Displey
Gen_31_D_A_P from 30 syspower sign.
Gen_32_D_A_P from 31 syspower sign on start up
Gen_33_D_A_P from 32 ToDo: GFXDeinit
Gen_34_D_A_P from 33  GFXDeinit fixed: heap[i]=0;hThread=0;
Gen_35_D_A_P from 34. Fix: cold start. Done sleep, but not wakeup.
Gen_36_D_A_P from 35. Done read status 65987 and SOC 28z610
Gen_37_D_A_P from 36. 
Gen_47_D_A_P_Pl_C union form 37, 10
Gen_48_D_A_P_Pl_C Actual vertion. ToDo. Check balancing, Do sleep.
Gen_49_D_A_P_Pl_C Actual vertion from 48 ToDo. Port to STM32G070.
Gen_50_ empty STM32G070.
Gen_51_git from 49 for github.