#include <stdio.h>
#include "stub.h"
#include "utils.h"

vu32 g_jiffies = 0;

static FILE *g_dataFile; 

void STUB_OpenUsartDataFile(const char* path)
{
	g_dataFile = fopen(path, "r");
	return;
}

void STUB_CloseUsartDataFile(void)
{
	fclose(g_dataFile);
}

int USARTIO_ReadLine(void* port, unsigned char* buf, int bufsize)
{
	char* readbuf;

	readbuf = fgets(buf, bufsize, g_dataFile);
	
	if (NULL != readbuf){
		printf("Read:[%s]\n", buf);
		return ERROR_SUCCESS;
	} else {
		return ERROR_FAILED;
	}
}

static void _printAscii(unsigned char ch)
{
	if (ch >= 0x21 && ch <= 0x7e){
		printf("Read:[%c]\n", ch);
	} else {
		printf("Read:[%hhx]\n", ch);
	}

	return;
}

int USARTIO_RecvChar(void* port, unsigned char* ch)
{
	int ret;
	
	ret = fgetc(g_dataFile);
	if (0 == ferror(g_dataFile)){
		*ch = (unsigned char)(unsigned)ret;
		_printAscii((unsigned char)(unsigned)ret);
		return ERROR_SUCCESS;
	} else {
		return ERROR_FAILED;
	}
}

int USARTIO_SendString(void* port, unsigned char* str)
{
	printf("Send:[%s]\n", str);
	return 1;
}

int USARTIO_SendData(void* port, unsigned char* data, int len)
{
	int i;
	
	printf("Begin send data dump:\n");
	for (i = 0; i < len; ++i){
		printf("0x%hhx, ", data[i]);
		if ((i != 0) && (i%7 == 0)) putchar(' ');
		if ((i != 0) && (i%15 == 0)) putchar('\n');
	}
	printf("\nEnd send data dump.\n");
	
	return 1;
}

int TLST_AddTimer(u32 usec, TLST_CB cb)
{
	return 1;
}

void delay_ms(int n){}

int GSM_isReady(void)
{
	return 1;
}

