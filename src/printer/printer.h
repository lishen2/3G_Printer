#ifndef _PRINTER_H_
#define _PRINTER_H_

/* ���Ӵ�ӡ��ʹ�ô���2 */
#define PRINT_USART_PORT USART2

/* ��ӡ��ǰ״̬ */
void PRINT_PrintStatus(void);

/* �Ƿ����� */
#define PRINT_STATUS_N1_OUTLINE       (0x01 << 3)

/* ��ȡ��ӡ��״̬ */
u8 PRINT_GetPrintStatus(void);

#endif

