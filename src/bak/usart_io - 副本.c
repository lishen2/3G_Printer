#include <stdio.h>
#include <string.h>
#include "stm32f10x.h"
#include "ringbuf.h"
#include "usart_io.h"
#include "utils.h"

//���������趨
#define USARTIO_USART1_GPIO       GPIOB
#define USARTIO_USART1_CLK        RCC_APB2Periph_USART1
#define USARTIO_USART1_GPIO_CLK   RCC_APB2Periph_GPIOB
#define USARTIO_USART1_RxPin      GPIO_Pin_7
#define USARTIO_USART1_TxPin      GPIO_Pin_6
#define USARTIO_USART1_IRQ        USART1_IRQn

#define USARTIO_USART2_GPIO      GPIOD
#define USARTIO_USART2_CLK       RCC_APB1Periph_USART2
#define USARTIO_USART2_GPIO_CLK  RCC_APB2Periph_GPIOD
#define USARTIO_USART2_RxPin     GPIO_Pin_6
#define USARTIO_USART2_TxPin     GPIO_Pin_5
#define USARTIO_USART2_IRQ       USART2_IRQn

#define USARTIO_USART3_GPIO      GPIOD
#define USARTIO_USART3_CLK       RCC_APB1Periph_USART3
#define USARTIO_USART3_GPIO_CLK  RCC_APB2Periph_GPIOD
#define USARTIO_USART3_RxPin     GPIO_Pin_9
#define USARTIO_USART3_TxPin     GPIO_Pin_8
#define USARTIO_USART3_IRQ       USART3_IRQn

#define USARTIO_USART4_GPIO      GPIOC
#define USARTIO_USART4_CLK       RCC_APB1Periph_UART4
#define USARTIO_USART4_GPIO_CLK  RCC_APB2Periph_GPIOC
#define USARTIO_USART4_RxPin     GPIO_Pin_11
#define USARTIO_USART4_TxPin     GPIO_Pin_10
#define USARTIO_USART4_IRQ       UART4_IRQn

#define USARTIO_USART5_RX_GPIO      GPIOD
#define USARTIO_USART5_RX_GPIO_CLK  RCC_APB2Periph_GPIOD
#define USARTIO_USART5_RxPin        GPIO_Pin_2
#define USARTIO_USART5_TX_GPIO      GPIOC
#define USARTIO_USART5_TX_GPIO_CLK  RCC_APB2Periph_GPIOC
#define USARTIO_USART5_TxPin    	GPIO_Pin_12
#define USARTIO_USART5_CLK          RCC_APB1Periph_UART5
#define USARTIO_USART5_IRQ       	UART5_IRQn

/* ����1 */
#define USART_U1_IO_READBUF_SIZE  1024 
#define USART_U1_IO_WRITEBUF_SIZE 1024 

/* ����2 */
#define USART_U2_IO_READBUF_SIZE  1024
#define USART_U2_IO_WRITEBUF_SIZE 1024

/* ����3 */
#define USART_U3_IO_READBUF_SIZE  1024 
#define USART_U3_IO_WRITEBUF_SIZE 1024 

/* ����4 */
#define USART_U4_IO_READBUF_SIZE  512 
#define USART_U4_IO_WRITEBUF_SIZE 128 

/* ����5 */
#define USART_U5_IO_READBUF_SIZE  128 
#define USART_U5_IO_WRITEBUF_SIZE 2048

/* ����1ʹ�õĶ�д������ */
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

/* ����2ʹ�õĶ�д������ */
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

/* ����3ʹ�õĶ�д������ */
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

/* ����4ʹ�õĶ�д������ */
static unsigned char g_Usart4ReadDataBuf[USART_U4_IO_READBUF_SIZE]; 
static unsigned char g_Usart4WriteDataBuf[USART_U4_IO_WRITEBUF_SIZE]; 
RingBuf g_Usart4ReadBufHead = {
    0,
    0,
    sizeof(g_Usart4ReadDataBuf),
    g_Usart4ReadDataBuf,
    0
};
RingBuf g_Usart4WriteBufHead = {
    0,
    0,
    sizeof(g_Usart4WriteDataBuf),
    g_Usart4WriteDataBuf,
    0
};

/* ����5ʹ�õĶ�д������ */
static unsigned char g_Usart5ReadDataBuf[USART_U5_IO_READBUF_SIZE]; 
static unsigned char g_Usart5WriteDataBuf[USART_U5_IO_WRITEBUF_SIZE]; 
RingBuf g_Usart5ReadBufHead = {
    0,
    0,
    sizeof(g_Usart5ReadDataBuf),
    g_Usart5ReadDataBuf,
    0
};
RingBuf g_Usart5WriteBufHead = {
    0,
    0,
    sizeof(g_Usart5WriteDataBuf),
    g_Usart5WriteDataBuf,
    0
};

/* ���ô��ڲ���ʹ�õ����ݽṹ */
static USART_InitTypeDef USART_InitStructure;

/* ��ָ�����ڷ������� */
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
    } else if (UART4 == port){
        writeBuf = &g_Usart4WriteBufHead;
    } else if (UART5 == port){
        writeBuf = &g_Usart5WriteBufHead;
    } else {
        return ERROR_FAILED;
    }

    /* ������д��д������ */
    ret = Ringbuf_PutChar(writeBuf, ch);
    
    /* ���ж� */
    USART_ITConfig(port, USART_IT_TXE, ENABLE);
    
    return ret;
}

/* ��ָ�����ڶ�ȡ���� */
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
    } else if (UART4 == port){
        readBuf = &g_Usart4ReadBufHead;
    } else if (UART5 == port){
        readBuf = &g_Usart5ReadBufHead;
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

/* ��ָ���˿ڷ���һ���ַ��� */
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
    } else if (UART4 == port){
        writeBuf = &g_Usart4WriteBufHead;
    } else if (UART5 == port){
        writeBuf = &g_Usart5WriteBufHead;
    } else {
        return ERROR_FAILED;
    }

    len = strlen((char*)str);
    for(i = 0; i < len;){
        
        /* ������д��д������ */
        ret = Ringbuf_PutChar(writeBuf, str[i]);        
        /* ���ж� */
        USART_ITConfig(port, USART_IT_TXE, ENABLE);

        if (ERROR_SUCCESS == ret){
            ++i;
        }
    }

    printf("Send:%s", str);

    return ERROR_SUCCESS;
}

/* �Ӵ����ж�ȡһ�в����뻺��
   ��ȡ�ɹ�����ERROR_SUCCESS
   ��ȡʧ�ܷ���ERROR_FAILED
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
    } else if (UART4 == port){
        readBuf = &g_Usart4ReadBufHead;
    } else if (UART5 == port){
        readBuf = &g_Usart5ReadBufHead;
    } else {
        return ERROR_FAILED;
    }

    /* ��������л�û��һ���У�ֱ���˳� */
    if (!RINGBUF_HAVELINE(readBuf)){
        return ERROR_FAILED;
    }

    for (i = 0; i < bufsize;){      

        /* ��ȡ�ַ� */
        ret = Ringbuf_GetChar(readBuf);
        if (ret < 0){
            break;
        }
        
        /* skip '\r' */
        ch = (unsigned char)(unsigned)ret;
        if (ch == '\r'){
        } 
        /* �����\n�������ȡ */
        else if(ch == '\n') {
            ret = ERROR_SUCCESS;
            break;
        } 
        /* ��������������򱣴浽�������� */
        else {
            buf[i] = ch;
            ++i;
        }
    }

    /* ������ĩβ���Ͻ������� */
    (i == bufsize)?(buf[i - 1] = '\0'):(buf[i] = '\0');

    if (ERROR_SUCCESS == ret){
        printf("Read:%s\r\n", buf);
    }
  
    return ret;
}

/* ��ʼ����ͨ��ģ��ͨ��ʹ�õĴ��ڣ�ʹ�ô���2 */
void USARTIO_InitUsart1(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;    

    /*ʹ�ܴ���ʹ�õ�GPIOʱ��*/
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
    USART_InitStructure.USART_BaudRate = 115200;                 /*���ò�����*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*��������λΪ8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*����ֹͣλΪ1λ*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*����żУ��*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*��Ӳ������*/
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*���ͺͽ���*/

    /*���ô���*/
    USART_Init(USART1, &USART_InitStructure);

    /*ʹ�ܴ���1�ķ��ͺͽ����ж�*/
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

    /* ʹ�ܴ��� */
    USART_Cmd(USART1, ENABLE);

    return;
}

/* ��ʼ�����Դ��� */
void USARTIO_InitUsart2(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;  

    /* ʱ�� */
    /*ʹ�ܴ���ʹ�õ�GPIOʱ��*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|USARTIO_USART2_GPIO_CLK, ENABLE);
    /* Enable debug USART Clock */    
    RCC_APB1PeriphClockCmd(USARTIO_USART2_CLK, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

    /* �ж� */
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
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USARTIO_USART2_GPIO, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;                 /*���ò�����*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*��������λΪ8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*����ֹͣλΪ1λ*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*����żУ��*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*��Ӳ������*/
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*���ͺͽ���*/

    /*���ô��� */
    USART_Init(USART2, &USART_InitStructure);

    /*ʹ�ܴ��ڵķ��ͺͽ����ж�*/
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);

    /* ʹ�ܴ��� */
    USART_Cmd(USART2, ENABLE);

    return;
}

/* ��ʼ�����Դ��� */
void USARTIO_InitUsart3(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;  

    /* ʱ�� */
    /*ʹ�ܴ���ʹ�õ�GPIOʱ��*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|USARTIO_USART3_GPIO_CLK, ENABLE);
    /* Enable debug USART Clock */    
    RCC_APB1PeriphClockCmd(USARTIO_USART3_CLK, ENABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);

    /* �ж� */
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

    USART_InitStructure.USART_BaudRate = 115200;                 /*���ò�����*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*��������λΪ8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*����ֹͣλΪ1λ*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*����żУ��*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*��Ӳ������*/
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*���ͺͽ���*/

    /*���ô��� */
    USART_Init(USART3, &USART_InitStructure);

    /*ʹ�ܴ��ڵķ��ͺͽ����ж�*/
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);

    /* ʹ�ܴ��� */
    USART_Cmd(USART3, ENABLE);

    return;
}

/* ��ʼ�����Դ��� */
void USARTIO_InitUsart4(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;  

    /* ʱ�� */
    /*ʹ�ܴ���ʹ�õ�GPIOʱ��*/
    RCC_APB2PeriphClockCmd(USARTIO_USART4_GPIO_CLK, ENABLE);
    /* Enable debug USART Clock */    
    RCC_APB1PeriphClockCmd(USARTIO_USART4_CLK, ENABLE);

    /* �ж� */
    /* Enable the debug USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USARTIO_USART4_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* GPIO */
    /* Configure debug USART Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART4_RxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART4_GPIO, &GPIO_InitStructure);

    /* Configure debug USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART4_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USARTIO_USART4_GPIO, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 9600;                 /*���ò�����*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*��������λΪ8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*����ֹͣλΪ1λ*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*����żУ��*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*��Ӳ������*/
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*���ͺͽ���*/

    /*���ô��� */
    USART_Init(UART4, &USART_InitStructure);

    /*ʹ�ܴ��ڵķ��ͺͽ����ж�*/
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
    USART_ITConfig(UART4, USART_IT_TXE, ENABLE);

    /* ʹ�ܴ��� */
    USART_Cmd(UART4, ENABLE);

    return;
}

/* ��ʼ�����Դ��� */
void USARTIO_InitUsart5(u32 baud)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;  

    /* ʱ�� */
    /*ʹ�ܴ���ʹ�õ�GPIOʱ��*/
    RCC_APB2PeriphClockCmd(USARTIO_USART5_RX_GPIO_CLK|
                           USARTIO_USART5_TX_GPIO_CLK, ENABLE);
    /* Enable debug USART Clock */    
    RCC_APB1PeriphClockCmd(USARTIO_USART5_CLK, ENABLE);

    /* �ж� */
    /* Enable the debug USART Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USARTIO_USART5_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* GPIO */
    /* Configure debug USART Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART5_RxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTIO_USART5_RX_GPIO, &GPIO_InitStructure);

    /* Configure debug USART Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = USARTIO_USART5_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(USARTIO_USART5_TX_GPIO, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baud;                 /*���ò�����*/
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;/*��������λΪ8*/
    USART_InitStructure.USART_StopBits = USART_StopBits_1;     /*����ֹͣλΪ1λ*/
    USART_InitStructure.USART_Parity = USART_Parity_No;        /*����żУ��*/
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;/*��Ӳ������*/
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;  /*���ͺͽ���*/

    /*���ô��� */
    USART_Init(UART5, &USART_InitStructure);

    /*ʹ�ܴ��ڵķ��ͺͽ����ж�*/
    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
    USART_ITConfig(UART5, USART_IT_TXE, ENABLE);

    /* ʹ�ܴ��� */
    USART_Cmd(UART5, ENABLE);

    return;
}

/* �жϻ��������Ƿ���������һ�� */
int USARTIO_HaveLine(void *port)
{
    RingBuf* readBuf;

    if (USART1 == port){
        readBuf = &g_Usart1ReadBufHead;
    } else if (USART2 == port){
        readBuf = &g_Usart2ReadBufHead;
    } else if (USART3 == port){
        readBuf = &g_Usart3ReadBufHead;
    } else if (UART4 == port){
        readBuf = &g_Usart4ReadBufHead;
    } else if (UART5 == port){
        readBuf = &g_Usart5ReadBufHead;
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
    /* �ӼĴ����ж������ݷ���������� */
    Ringbuf_PutChar(&g_Usart1ReadBufHead, 
                    (unsigned char)USART_ReceiveData(USART1));
  }
  
  if(USARTIO_IsFlagSet(USART1, USARTIO_FLAG_TXE) == SET)
  { 
    /* �ӻ����ж�ȡ���ݲ�д��Ĵ��� */
    ret = Ringbuf_GetChar(&g_Usart1WriteBufHead);
    if (ret >= 0){
        USART_SendData(USART1, (unsigned short)((unsigned)ret & 0xFF));  
    } 
    /* �������Ϊ����ر��ж� */
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
    /* �ӼĴ����ж������ݷ���������� */
    Ringbuf_PutChar(&g_Usart2ReadBufHead, 
                    (unsigned char)USART_ReceiveData(USART2));
  }
  
  if(USARTIO_IsFlagSet(USART2, USARTIO_FLAG_TXE) == SET)
  { 
    /* �ӻ����ж�ȡ���ݲ�д��Ĵ��� */
    ret = Ringbuf_GetChar(&g_Usart2WriteBufHead);
    if (ret >= 0){
        USART_SendData(USART2, (unsigned short)((unsigned)ret & 0xFF));  
    }
    /* �������Ϊ����ر��ж� */
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
    /* �ӼĴ����ж������ݷ���������� */
    Ringbuf_PutChar(&g_Usart3ReadBufHead, 
                    (unsigned char)USART_ReceiveData(USART3));
  }
  
  if(USARTIO_IsFlagSet(USART3, USARTIO_FLAG_TXE) == SET)
  { 
    /* �ӻ����ж�ȡ���ݲ�д��Ĵ��� */
    ret = Ringbuf_GetChar(&g_Usart3WriteBufHead);
    if (ret >= 0){
        USART_SendData(USART3, (unsigned short)((unsigned)ret & 0xFF));  
    }
    /* �������Ϊ����ر��ж� */
    else {
      USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
    }//else
  }
}

void UART4_IRQHandler(void)
{
  int ret;
  
  if(USARTIO_IsFlagSet(UART4, USARTIO_FLAG_RXNE) == SET)
  {
    /* �ӼĴ����ж������ݷ���������� */
    Ringbuf_PutChar(&g_Usart4ReadBufHead, 
                    (unsigned char)USART_ReceiveData(UART4));
  }
  
  if(USARTIO_IsFlagSet(UART4, USARTIO_FLAG_TXE) == SET)
  { 
    /* �ӻ����ж�ȡ���ݲ�д��Ĵ��� */
    ret = Ringbuf_GetChar(&g_Usart4WriteBufHead);
    if (ret >= 0){
        USART_SendData(UART4, (unsigned short)((unsigned)ret & 0xFF));  
    }
    /* �������Ϊ����ر��ж� */
    else {
      USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
    }//else
  }
}

void UART5_IRQHandler(void)
{
  int ret;
  
  if(USARTIO_IsFlagSet(UART5, USARTIO_FLAG_RXNE) == SET)
  {
    /* �ӼĴ����ж������ݷ���������� */
    Ringbuf_PutChar(&g_Usart5ReadBufHead, 
                    (unsigned char)USART_ReceiveData(UART5));
  }
  
  if(USARTIO_IsFlagSet(UART5, USARTIO_FLAG_TXE) == SET)
  { 
    /* �ӻ����ж�ȡ���ݲ�д��Ĵ��� */
    ret = Ringbuf_GetChar(&g_Usart5WriteBufHead);
    if (ret >= 0){
        USART_SendData(UART5, (unsigned short)((unsigned)ret & 0xFF));  
    }
    /* �������Ϊ����ر��ж� */
    else {
      USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
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


