#include "GlobalKey.h"
#include "string.h"

char ErrorMess[50];

void Error(char* s)
{
	strncpy(ErrorMess,s,48);
}