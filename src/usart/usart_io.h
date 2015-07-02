#ifndef _USART_IO_H_
#define _USART_IO_H_

#define USARTIO_FLAG_RXNE  0x20
#define USARTIO_FLAG_TXE   0x80

#define USARTIO_IsFlagSet(USART_ADDR, FLAG) (0 != (USART_ADDR->SR & FLAG))

/* 接口暴露给中断处理函数使用 */
extern RingBuf g_Usart1ReadBufHead;
extern RingBuf g_Usart1WriteBufHead;
extern RingBuf g_Usart2ReadBufHead;
extern RingBuf g_Usart2WriteBufHead;
extern RingBuf g_Usart3ReadBufHead;
extern RingBuf g_Usart3WriteBufHead;

/* @brief 向指定串口发送一个字符
* @param void* port           指定端口，比如: USART1
* @param unsigned char ch   要发送的字符
* @return 成功返回ERROR_SUCCESS,失败返回ERROR_FAILED
*/
int USARTIO_SendChar(void* port, unsigned char ch);

/* @brief 向指定串口读取一个字符
* @param void* port           指定端口，比如: USART1
* @param unsigned char ch   要读取的字符
* @return 成功返回ERROR_SUCCESS,失败返回ERROR_FAILED
*/
int USARTIO_RecvChar(void* port, unsigned char* ch);

/* 向指定端口发送一个字符串 */
int USARTIO_SendString(void* port, unsigned char* str);

/* 向串口发送二进制数据 */
int USARTIO_SendData(void* port, unsigned char* data, int len);

/* 从指定端口接收一行 */
int USARTIO_ReadLine(void* port, unsigned char* buf, int bufsize);

void USARTIO_InitUsart1(void);
void USARTIO_InitUsart2(void);
void USARTIO_InitUsart3(void);

/* 判断缓存是否为空 */
int USARTIO_HaveLine(void *port);

#endif

