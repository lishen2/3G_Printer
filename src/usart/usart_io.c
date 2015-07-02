#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "utils.h"

//串口引脚设定
#define USARTIO_USART1_GPIO       GPIOA
#define USARTIO_USART1_CLK        RCC_APB2Periph_USART1
#define USARTIO_USART1_GPIO_CLK   RCC_APB2Periph_GPIOA
#define USARTIO_USART1_RxPin      GPIO_Pin_10
#define USARTIO_USART1_TxPin      GPIO_Pin_9
#define USARTIO_USART1_CtsPin     GPIO_Pin_11
#define USARTIO_USART1_IRQ        USART1_IRQn

#define USARTIO_USART2_GPIO      GPIOA
#define USARTIO_USART2_CLK       RCC_APB1Periph_USART2
#define USARTIO_USART2_GPIO_CLK  RCC_APB2Periph_GPIOA
#define USARTIO_USART2_RxPin     GPIO_Pin_3
#define USARTIO_USART2_TxPin     GPIO_Pin_2
#define USARTIO_USART2_CtsPin    GPIO_Pin_0
#define USARTIO_USART2_IRQ       USART2_IRQn

#define USARTIO_USART3_GPIO      GPIOB
#define USARTIO_USART3_CLK       RCC_APB1Periph_USART3
#define USARTIO_USART3_GPIO_CLK  RCC_APB2Periph_GPIOB
#define USARTIO_USART3_RxPin     GPIO_Pin_11
#define USARTIO_USART3_TxPin     GPIO_Pin_10
#define USARTIO_USART3_CtsPin    GPIO_Pin_13
#define USARTIO_USART3_RtsPin    GPIO_Pin_14
#define USARTIO_USART3_IRQ       USART3_IRQn

/* 串口1 */
#define USART_U1_IO_READBUF_SIZE  32 
#define USART_U1_IO_WRITEBUF_SIZE 512

/* 串口2 */
#define USART_U2_IO_READBUF_SIZE  32
#define USART_U2_IO_WRITEBUF_SIZE 512

/* 串口3 */
#define USART_U3_IO_READBUF_SIZE  4096 
#define USART_U3_IO_WRITEBUF_SIZE 512 


/* 串口1使用的读写缓冲区 */
static unsigned char g_Usart1ReadDataBuf[USART_U1_IO_READBUF_SIZE]; 
static unsigned char g_Usart1WriteDataBuf[USART_U1_IO_WRITEBUF_SIZE]; 
RingBuf g_Usart1ReadBufHead = {
    0,
    0,
    sizeof(g_Usart1ReadDataBuf),
    g_Usart1ReadDataBuf,
    0
};
RingBuf g_Usart1WriteBufHead = {
    0,
    0,
    sizeof(g_Usart1WriteDataBuf),
    g_Usart1WriteDataBuf,
    0
};

/* 串口2使用的读写缓冲区 */
static unsigned char g_Usart2ReadDataBuf[USART_U2_IO_READBUF_SIZE]; 
static unsigned char g_Usart2WriteDataBuf[USART_U2_IO_WRITEBUF_SIZE]; 
RingBuf g_Usart2ReadBufHead = {
    0,
    0,
    sizeof(g_Usart2ReadDataBuf),
    g_Usart2ReadDataBuf,
    0
};
RingBuf g_Usart2WriteBufHead = {
    0,
    0,
    sizeof(g_Usart2WriteDataBuf),
    g_Usart2WriteDataBuf,
    0
};

/* 串口3使用的读写缓冲区 */
static unsigned char g_Usart3ReadDataBuf[USART_U3_IO_READBUF_SIZE]; 
static unsigned char g_Usart3WriteDataBuf[USART_U3_IO_WRITEBUF_SIZE]; 
RingBuf g_Usart3ReadBufHead = {
    0,
    0,
    sizeof(g_Usart3ReadDataBuf),
    g_Usart3ReadDataBuf,
    0
};
RingBuf g_Usart3WriteBufHead = {
    0,
    0,
    sizeof(g_Usart3WriteDataBuf),
    g_Usart3WriteDataBuf,
    0
};

/* 配置串口参数使用的数据结构 */
static USART_InitTypeDef USART_InitStructure;

/* 向指定串口发送数据 */
int USARTIO_SendChar(void* port, unsigned char ch)
{  
    int ret = ERROR_FAILED;
    RingBuf* writeBuf;

    if (USART1 == port){
        writeBuf = &g_Usart1WriteBufHead;
    } else if (USART2 == port){
        writeBuf = &g_Usart2WriteBufHead;
    } else if (USART3 == port){
        writeBuf = &g_Usart3WriteBufHead;
    } else {
        return ERROR_FAILED;
    }

    /* 将数据写入写缓存中 */
    ret = Ringbuf_PutChar(writeBuf, ch);
    
    /* 打开中断 */
    USART_ITConfig(port, USART_IT_TXE, ENABLE);
    
    return ret;
}

/* 从指定串口读取数据 */
int USARTIO_RecvChar(void* port, unsigned char* ch)
{
    int ret = ERROR_FAILED;
    RingBuf* readBuf;

    if (USART1 == port){
        readBuf = &g_Usart1ReadBufHead;
    } else if (USART2 == port){
        readBuf = &g_Usart2ReadBufHead;
    } else if (USART3 == port){
        readBuf = &g_Usart3ReadBufHead;
    } else {
        return ERROR_FAILED;
    }

    ret = Ringbuf_GetChar(readBuf);
    if (ret >= 0){
        *ch = (unsigned char)(unsigned int)ret;
        ret = ERROR_SUCCESS;
    }
    
    return ret;
}

/* 向指定端口发送一个字符串 */
int USARTIO_SendString(void* port, unsigned char* str)
{
    int len, i, ret;
    RingBuf* writeBuf;

    if (USART1 == port){
        writeBuf = &g_Usart1WriteBufHead;
    } else if (USART2 == port){
        writeBuf = &g_Usart2WriteBufHead;
    } else if (USART3 == port){
        writeBuf = &g_Usart3WriteBufHead;
    } else {
        return ERROR_FAILED;
    }

    len = strlen((char*)str);
    for(i = 0; i < len;){
        
        /* 将数据写入写缓存中 */
        ret = Ringbuf_PutChar(writeBuf, str[i]);        
        /* 打开中断 */
        USART_ITConfig(port, USART_IT_TXE, ENABLE);

        if (ERROR_SUCCESS == ret){
            ++i;
        }
    }

    printf("Send:%s", str);

    return ERROR_SUCCESS;
}

/* 向指定端口发送一个字符串 */
int USARTIO_SendData(void* port, unsigned char* data, int len)
{
    int i, ret;
    RingBuf* writeBuf;

    if (USART1 == port){
        writeBuf = &g_Usart1WriteBufHead;
    } else if (USART2 == port){
        writeBuf = &g_Usart2WriteBufHead;
    } else if (USART3 == port){
        writeBuf = &g_Usart3WriteBufHead;
    } else {
        return ERROR_FAILED;
    }

    for(i = 0; i < len;){
        
        /* 将数据写入写缓存中 */
        ret = Ringbuf_PutChar(writeBuf, data[i]);        
        /* 打开中断 */
        USART_ITConfig(port, USART_IT_TXE, ENABLE);

        if (ERROR_SUCCESS == ret){
            ++i;
        }
    }

    return ERROR_SUCCESS;
}

/* 从串口中读取一行并放入缓存
   读取成功返回ERROR_SUCCESS
   读取失败返回ERROR_FAILED
*/
int USARTIO_ReadLine(void* port, unsigned char* buf, int bufsize)
{
    unsigned char ch;
    int i;
    int ret = ERROR_FAILED;
    RingBuf* readBuf;

    if (USART1 == port){
        readBuf = &g_Usart1ReadBufHead;
    } else if (USART2 == port){
        readBuf = &g_Usart2ReadBufHead;
    } else if (USART3 == port){
        readBuf = &g_Usart3ReadBufHead;
    } else {
        return ERROR_FAILED;
    }

    /* 如果缓存中还没有一整行，直接退出 */
    if (!RINGBUF_HAVELINE(readBuf)){
        return ERROR_FAILED;
    }

    for (i = 0; i < bufsize;){      

        /* 读取字符 */
        ret = Ringbuf_GetChar(readBuf);
        if (ret < 0){
            break;
        }
        
        /* skip '\r' */
        ch = (unsigned char)(unsigned)ret;
        if (ch == '\r'){
        } 
        /* 如果是\n则结束读取 */
        else if(ch == '\n') {
            ret = ERROR_SUCCESS;
            break;
        } 
        /* 如果是其他数据则保存到缓冲区中 */
        else {
            buf[i] = ch;
            ++i;
        }
    }

    /* 缓冲区末尾补上结束符符 */
    (i == bufsize)?(buf[i - 1] = '\0'):(buf[i] = '\0');

    if (ERROR_SUCCESS == ret){
        printf("Read:%s\r\n", buf);
    }
  
    return ret;
}

/* 初始化和通信模块通信使用的串口，使用串口2 */
void USARTIO_InitUsart1(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;    

    /*使能串口使用的GPIO时钟*/
    RCC_APB2PeriphClockCmd(USARTIO_USART1_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(USARTIO_USART1_CLK, ENABLE);

    /* Enable the USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USARTIO_USART1_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* GPIO */
    /* Configure USART2 Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART1_RxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART1_GPIO, &GPIO_InitStructure);

	/* Configure USART2 cts as input floating */
    /*GPIO_InitStructure.GPIO_Pin = USARTIO_USART1_CtsPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART1_GPIO, &GPIO_InitStructure);  */

    /* Configure USART2 Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART1_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USARTIO_USART1_GPIO, &GPIO_InitStructure);

    /* USART1 and USART2 configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled */
    USART_InitStructure.USART_BaudRate = 115200;                 /*设置波特率*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*设置数据位为8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*设置停止位为1位*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*无奇偶校验*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*发送和接收*/

    /*配置串口*/
    USART_Init(USART1, &USART_InitStructure);

    /*使能串口1的发送和接收中断*/
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

    /* 使能串口 */
    USART_Cmd(USART1, ENABLE);

    return;
}

/* 初始化调试串口 */
void USARTIO_InitUsart2(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;  

    /* 时钟 */
    /*使能串口使用的GPIO时钟*/
    RCC_APB2PeriphClockCmd(USARTIO_USART2_GPIO_CLK, ENABLE);
    /* Enable debug USART Clock */    
    RCC_APB1PeriphClockCmd(USARTIO_USART2_CLK, ENABLE);

    /* 中断 */
    /* Enable the debug USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USARTIO_USART2_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* GPIO */
    /* Configure debug USART Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART2_RxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART2_GPIO, &GPIO_InitStructure);

    /* Configure debug USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART2_TxPin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USARTIO_USART2_GPIO, &GPIO_InitStructure);

    /* Configure debug USART CTS */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART2_CtsPin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART3_GPIO, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 19200;                 /*设置波特率*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*设置数据位为8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*设置停止位为1位*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*无奇偶校验*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;/*无硬件流控*/
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*发送和接收*/

    /*配置串口 */
    USART_Init(USART2, &USART_InitStructure);

    /*使能串口的发送和接收中断*/
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);

    /* 使能串口 */
    USART_Cmd(USART2, ENABLE);

    return;
}

/* 初始化调试串口 */
void USARTIO_InitUsart3(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;  

    /* 时钟 */
    /*使能串口使用的GPIO时钟*/
    RCC_APB2PeriphClockCmd(USARTIO_USART3_GPIO_CLK, ENABLE);
    /* Enable debug USART Clock */    
    RCC_APB1PeriphClockCmd(USARTIO_USART3_CLK, ENABLE);

    /* 中断 */
    /* Enable the debug USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USARTIO_USART3_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* GPIO */
    /* Configure debug USART Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART3_RxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART3_GPIO, &GPIO_InitStructure);

    /* Configure debug USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART3_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USARTIO_USART3_GPIO, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;                 /*设置波特率*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*设置数据位为8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*设置停止位为1位*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*无奇偶校验*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*发送和接收*/

    /*配置串口 */
    USART_Init(USART3, &USART_InitStructure);

    /*使能串口的发送和接收中断*/
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);

    /* 使能串口 */
    USART_Cmd(USART3, ENABLE);

    return;
}

/* 判断缓冲区中是否有完整的一行 */
int USARTIO_HaveLine(void *port)
{
    RingBuf* readBuf;

    if (USART1 == port){
        readBuf = &g_Usart1ReadBufHead;
    } else if (USART2 == port){
        readBuf = &g_Usart2ReadBufHead;
    } else if (USART3 == port){
        readBuf = &g_Usart3ReadBufHead;
    } else {
        return ERROR_SUCCESS;
    }

    if (RINGBUF_HAVELINE(readBuf)){
        return ERROR_SUCCESS;
    } else {
        return ERROR_FAILED;
    }
}

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
  int ret;
  
  if(USARTIO_IsFlagSet(USART1, USARTIO_FLAG_RXNE) == SET)
  {
    /* 从寄存其中读出数据放入读缓存中 */
    Ringbuf_PutChar(&g_Usart1ReadBufHead, 
                    (unsigned char)USART_ReceiveData(USART1));
  }
  
  if(USARTIO_IsFlagSet(USART1, USARTIO_FLAG_TXE) == SET)
  { 
    /* 从缓存中读取数据并写入寄存器 */
    ret = Ringbuf_GetChar(&g_Usart1WriteBufHead);
    if (ret >= 0){
        USART_SendData(USART1, (unsigned short)((unsigned)ret & 0xFF));  
    } 
    /* 如果缓存为空则关闭中断 */
    else {
      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    }//else
  }//if
}

/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
  int ret;
  
  if(USARTIO_IsFlagSet(USART2, USARTIO_FLAG_RXNE) == SET)
  {
    /* 从寄存其中读出数据放入读缓存中 */
    Ringbuf_PutChar(&g_Usart2ReadBufHead, 
                    (unsigned char)USART_ReceiveData(USART2));
  }
  
  if(USARTIO_IsFlagSet(USART2, USARTIO_FLAG_TXE) == SET)
  { 
    /* 从缓存中读取数据并写入寄存器 */
    ret = Ringbuf_GetChar(&g_Usart2WriteBufHead);
    if (ret >= 0){
        USART_SendData(USART2, (unsigned short)((unsigned)ret & 0xFF));  
    }
    /* 如果缓存为空则关闭中断 */
    else {
      USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
    }//else
  }
}

/**
  * @brief  This function handles USART3 global interrupt request.
  * @param  None
  * @retval None
  */
void USART3_IRQHandler(void)
{
  int ret;
  
  if(USARTIO_IsFlagSet(USART3, USARTIO_FLAG_RXNE) == SET)
  {
    /* 从寄存其中读出数据放入读缓存中 */
    Ringbuf_PutChar(&g_Usart3ReadBufHead, 
                    (unsigned char)USART_ReceiveData(USART3));
  }
  
  if(USARTIO_IsFlagSet(USART3, USARTIO_FLAG_TXE) == SET)
  { 
    /* 从缓存中读取数据并写入寄存器 */
    ret = Ringbuf_GetChar(&g_Usart3WriteBufHead);
    if (ret >= 0){
        USART_SendData(USART3, (unsigned short)((unsigned)ret & 0xFF));  
    }
    /* 如果缓存为空则关闭中断 */
    else {
      USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
    }//else
  }
}

  
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
	USARTIO_SendChar(USART1, ch);
	return(ch);
}


