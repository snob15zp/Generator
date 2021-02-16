//e_FunctionReturnState x_FSM_x(void)
//{ 
//	static uint8_t state=0;
//	e_FunctionReturnState rstatel,wrstate;
//	static uint16_t data;
//	
//  rstatel=e_FRS_Processing;

//  switch(state)
//  {
//   case 0:
//		 
//	   state++;
//	   break;
//	 case 1:
//		 data=bq25703InitData[ADCOption].data;
//	        	wrstate=BQ25703_Wr_Check(       bq25703,
//												bq25703InitData[ADCOption].I2cRecord,
//												data,
//												cPriorityDefault,
//												voidfun8
//	        									);
//		 if (e_FRS_Done==wrstate)
//		 {rstatel=e_FRS_Done;
//		  state=0;}
//		 break;
//   default: state=0;
//	};
//	
//  return rstatel;	
//};

