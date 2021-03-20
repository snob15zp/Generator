#include "GlobalKey.h"
#include "string.h"

const char FN_Read_Template[]="%26s";

char ErrorMess[50];

void Error(char* s)
{
	strncpy(ErrorMess,s,48);
}