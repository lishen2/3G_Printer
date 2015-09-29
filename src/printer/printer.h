#ifndef _PRINTER_H_
#define _PRINTER_H_

/* 连接打印机使用串口2 */
#define PRINT_USART_PORT USART2

/* 打印当前状态 */
void PRINT_PrintStatus(void);

/* 是否离线 */
#define PRINT_STATUS_N1_OUTLINE       (0x01 << 3)

/* 获取打印机状态 */
u8 PRINT_GetPrintStatus(void);

#endif

