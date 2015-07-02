#ifndef _RANGBUF_H_
#define _RANGBUF_H_

typedef struct RingBuf{
	volatile int readpos;
	volatile int writepos;	
	unsigned int size;   //缓存大小，常量
    unsigned char* data;
    volatile int lines;  //标记缓存中行的数量    
}RingBuf;

/* 判断缓冲区是否为空 */
#define RINGBUF_ISEMPTY(buf) ((buf)->readpos == (buf)->writepos)

/* 判断缓冲区中是否有完整的一行 */
#define RINGBUF_HAVELINE(buf) ((buf)->lines > 0)

/* 设置和取消设置行标记 */
#define RINGBUF_INCLINE(buf) (++((buf)->lines))
#define RINGBUF_DECLINE(buf) (--((buf)->lines))

/* init rangbuffe structer, with byte array and array size */
void Ringbuf_Init(RingBuf *buf, unsigned char* databuf, int bufsize);

/* put one char into buf, 
 * return ERROR_SUCCESS on success, 
 * otherwise return ERROR_FAILED
 * */
int Ringbuf_PutChar(RingBuf *buf, unsigned char ch);

/* put one char into buf, 
 * return one char on success, 
 * otherwise return error code which is less then zero
 * */
int Ringbuf_GetChar(RingBuf *buf);

#endif

