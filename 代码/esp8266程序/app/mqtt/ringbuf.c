/**
* \file
*		Ring Buffer library
*/

#include "ringbuf.h"


/**
* \brief init a RINGBUF object
* \param r pointer to a RINGBUF object
* \param buf pointer to a byte array
* \param size size of buf
* \return 0 if successfull, otherwise failed
*/

// ���в�����ʼ��������[ԭ/��/дָ��]��[װ������]��[���гߴ�]
//==============================================================================
I16 ICACHE_FLASH_ATTR RINGBUF_Init(RINGBUF *r, U8* buf, I32 size)
{
	if(r == NULL || buf == NULL || size < 2) return -1;
	
	r->p_o = r->p_r = r->p_w = buf;	// [ԭ/��/дָ��]ָ����ͬλ��(�����׵�ַ)
	r->fill_cnt = 0;
	r->size = size;
	
	return 0;
}
//==============================================================================

/**
* \brief put a character into ring buffer
* \param r pointer to a ringbuf object
* \param c character to be put
* \return 0 if successfull, otherwise failed
*/

// �򡾵�ǰдָ��ָ�򴦡�д�롾����2��
//==================================================================================================================================
I16 ICACHE_FLASH_ATTR RINGBUF_Put(RINGBUF *r, U8 c)
{
	if(r->fill_cnt>=r->size)return -1;		// �ж��Ƿ��������						// ring buffer is full, this should be atomic operation
	
	r->fill_cnt++;							// װ������++							// increase filled slots count, this should be atomic operation

	*r->p_w++ = c;							// �򡾵�ǰдָ��ָ�򴦡�д�롾����2��	// put character into buffer
	
	if(r->p_w >= r->p_o + r->size)			// �ж�дָ���Ƿ񳬳�����				// rollback if write pointer go pass
		r->p_w = r->p_o;					// дָ��ص�������ָ��					// the physical boundary
	
	return 0;
}
//==================================================================================================================================


/**
* \brief get a character from ring buffer
* \param r pointer to a ringbuf object
* \param c read character
* \return 0 if successfull, otherwise failed
*/

// �ӡ���ǰ��ָ��ָ�򴦡��������ݣ���ֵ��������2��ָ��ı���
//=======================================================================================================================================
I16 ICACHE_FLASH_ATTR RINGBUF_Get(RINGBUF *r, U8* c)
{
	if(r->fill_cnt<=0)return -1;			// ���г���							// ring buffer is empty, this should be atomic operation
	
	r->fill_cnt--;							// ����װ������--					// decrease filled slots count

	*c = *r->p_r++;							// ��ȡ[��ǰ��ָ��ָ��]��ֵ		// get the character out
	
	if(r->p_r >= r->p_o + r->size)			// �ж϶�ָ���Ƿ񳬳�����			// rollback if write pointer go pass
		r->p_r = r->p_o;					// ��ָ��ص�������ʼ				// the physical boundary
	
	return 0;
}
//=======================================================================================================================================
