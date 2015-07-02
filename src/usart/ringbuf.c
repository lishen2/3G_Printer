#include "stm32f10x.h"
#include "utils.h"
#include "ringbuf.h"

//check position is in range
static inline int _address_round(int size, int pos)
{
	if (pos >= size){
		pos -= size;
	}

	return pos;
}

//increase position
static inline int _address_increase(int size, int pos)
{
	return _address_round(size, ++pos);
}

void Ringbuf_Init(RingBuf *buf, unsigned char* databuf, int bufsize)
{
	buf->readpos = 0;
	buf->writepos = 0;
	buf->size = bufsize;
	buf->data = databuf;
    buf->lines = 0;
    
	return;
}

static inline void _ringbufIncrease(volatile int* lines)  
{  
    register int old_lines;

    do {
        old_lines = __ldrex((volatile int *)lines);
        ++old_lines;
    } while(__strex(old_lines, (volatile int*)lines));

    return;
}

static inline void _ringbufDescrease(volatile int* lines)  
{  
    register int old_lines;

    do {
        old_lines = __ldrex((volatile int *)lines);
        --old_lines;
    } while(__strex(old_lines, (volatile int*)lines));

    return;
}

/* 必须保证同一时间只能有一个写者 */
int Ringbuf_PutChar(RingBuf *buf, unsigned char ch)
{
	int writepos = buf->writepos;
	int size = buf->size;

	if (buf->readpos != _address_round(size, writepos + 1)){
		buf->data[writepos] = ch;
		writepos = _address_increase(size, writepos);
		buf->writepos = writepos;

        if ('\n' == ch){
			_ringbufIncrease(&(buf->lines));
        }

		return ERROR_SUCCESS;
	}

	return ERROR_FAILED;
}

/* 必须保证同一时间只能有一个读者 */
int Ringbuf_GetChar(RingBuf *buf)
{
	int readpos = buf->readpos;
	int size = buf->size;
    unsigned char ch;

	if (readpos != buf->writepos){
		ch = buf->data[readpos];
		readpos = _address_increase(size, readpos);
		buf->readpos = readpos;
        
        if ('\n' == ch){
            _ringbufDescrease(&(buf->lines));
        }

        return (int)(unsigned int)ch;
	}

	return ERROR_FAILED;
}


