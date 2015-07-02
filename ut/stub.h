#ifndef _STUB_H_
#define _STUB_H_

#include "stm32f10x.h"

void STUB_OpenUsartDataFile(const char* path);
void STUB_CloseUsartDataFile(void);
int USARTIO_ReadLine(void* port, unsigned char* buf, int bufsize);
int USARTIO_RecvChar(void* port, unsigned char* ch);
int USARTIO_SendString(void* port, unsigned char* str);
int USARTIO_SendData(void* port, unsigned char* data, int len);

typedef void (*TLST_CB)(void);
int TLST_AddTimer(u32 usec, TLST_CB cb);

#endif
