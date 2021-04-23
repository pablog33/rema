/**
 * @brief 	this component implements a ring buffer for different integer data type.
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

#include "input_ring_buffer.h"

static input_ring_buffer_ElementType input_ring_buffer_buffer[input_ring_buffer_CONFIG_BUF_SIZE]; /* ring buffer */
static input_ring_buffer_BufSizeType input_ring_buffer_inIdx; /* input index */
static input_ring_buffer_BufSizeType input_ring_buffer_outIdx; /* output index */
static input_ring_buffer_BufSizeType input_ring_buffer_inSize; /* size data in buffer */

/**
 * @brief   puts a new element into the buffer
 * @param	elem   : new element to be put into the buffer
 * @returns error code
 */
uint8_t input_ring_buffer_put(input_ring_buffer_ElementType elem)
{
	uint8_t res = ERR_OK;

	taskENTER_CRITICAL_FROM_ISR();
	if (input_ring_buffer_inSize == input_ring_buffer_CONFIG_BUF_SIZE) {
		res = ERR_TXFULL;
	} else {
		input_ring_buffer_buffer[input_ring_buffer_inIdx] = elem;
		input_ring_buffer_inIdx++;
		if (input_ring_buffer_inIdx == input_ring_buffer_CONFIG_BUF_SIZE) {
			input_ring_buffer_inIdx = 0;
		}
		input_ring_buffer_inSize++;
	}
	taskEXIT_CRITICAL_FROM_ISR(0);
	return res;
}

/**
 * @brief   put a number new element into the buffer.
 * @param   *elem  : pointer to new elements to be put into the buffer
 * @param   nof    : number of elements
 * @returns error code
 */
uint8_t input_ring_buffer_putn(input_ring_buffer_ElementType *elem,
		input_ring_buffer_BufSizeType nof)
{
	uint8_t res = ERR_OK;

	while (nof > 0) {
		res = input_ring_buffer_put(*elem);
		if (res != ERR_OK) {
			break;
		}
		elem++;
		nof--;
	}
	return res;
}

/**
 * @brief  removes an element from the buffer
 * @param  *elemP 	: pinter to where to store the received element
 * @returns error code
 */
uint8_t input_ring_buffer_get(input_ring_buffer_ElementType *elemP)
{
	uint8_t res = ERR_OK;

	taskENTER_CRITICAL_FROM_ISR();
	if (input_ring_buffer_inSize == 0) {
		res = ERR_RXEMPTY;
	} else {
		*elemP = input_ring_buffer_buffer[input_ring_buffer_outIdx];
		input_ring_buffer_inSize--;
		input_ring_buffer_outIdx++;
		if (input_ring_buffer_outIdx == input_ring_buffer_CONFIG_BUF_SIZE) {
			input_ring_buffer_outIdx = 0;
		}
	}
	taskEXIT_CRITICAL_FROM_ISR(0);
	return res;
}

/**
 * @brief 	get a number elements into a buffer.
 * @param   *buf : pointer to buffer where to store the elements
 * @param   nof : number of elements
 * @returns error code
 */
uint8_t input_ring_buffer_getn(input_ring_buffer_ElementType *buf,
		input_ring_buffer_BufSizeType nof)
{
	uint8_t res = ERR_OK;

	while (nof > 0) {
		res = input_ring_buffer_get(buf);
		if (res != ERR_OK) {
			break;
		}
		buf++;
		nof--;
	}
	return res;
}

/**
 * @brief   returns the actual number of elements in the buffer.
 * @returns number of elements in the buffer.
 */
input_ring_buffer_BufSizeType input_ring_buffer_number_of_elements(void)
{
	return input_ring_buffer_inSize;
}

/**
 * @brief	returns the actual number of free elements/space in the buffer.
 * @returns number of elements in the buffer.
 */
input_ring_buffer_BufSizeType input_ring_buffer_number_of_free_elements(void)
{
	return (input_ring_buffer_BufSizeType) (input_ring_buffer_CONFIG_BUF_SIZE
			- input_ring_buffer_inSize);
}

/**
 * @brief	initializes the data structure
 * @returns nothing
 */
void input_ring_buffer_init(void)
{
	input_ring_buffer_inIdx = 0;
	input_ring_buffer_outIdx = 0;
	input_ring_buffer_inSize = 0;
}

/**
 * @brief   clear (empty) the ring buffer.
 * @returns nothing
 */
void input_ring_buffer_clear(void)
{
	taskENTER_CRITICAL_FROM_ISR();
	input_ring_buffer_init();
	taskEXIT_CRITICAL_FROM_ISR(0);
}

/**
 * @brief   returns an element of the buffer without removing it.
 * @param   index	: index of element. 0 peeks the top element,
 * 										1 the next, and so on.
 * @param   *elemP  : pointer to where to store the received element
 * @returns error code
 */
uint8_t input_ring_buffer_peek(input_ring_buffer_BufSizeType index,
		input_ring_buffer_ElementType *elemP)
{
	uint8_t res = ERR_OK;
	int idx; /* index inside ring buffer */

	taskENTER_CRITICAL_FROM_ISR();
	if (index >= input_ring_buffer_CONFIG_BUF_SIZE) {
		res = ERR_OVERFLOW; /* asking for an element outside of ring buffer size */
	} else if (index < input_ring_buffer_inSize) {
		idx = (input_ring_buffer_outIdx + index)
				% input_ring_buffer_CONFIG_BUF_SIZE;
		*elemP = input_ring_buffer_buffer[idx];
	} else { /* asking for an element which does not exist */
		res = ERR_RXEMPTY;
	}
	taskEXIT_CRITICAL_FROM_ISR(0);
	return res;
}

/**
 * @brief   compares the elements in the buffer.
 * @param	index   : index of element. 0 peeks the top element
 *                     					1 the next, and so on.
 * @param   *elemP 	: pointer to elements to compare with
 * @param   nof     : number of elements to compare
 * @returns zero if elements are the same, -1 otherwise
 */
uint8_t input_ring_buffer_compare(input_ring_buffer_BufSizeType index,
		input_ring_buffer_ElementType *elemP, input_ring_buffer_BufSizeType nof)
{
	uint8_t cmpResult = 0;
	uint8_t res;
	input_ring_buffer_ElementType val;

	while (nof > 0) {
		res = input_ring_buffer_peek(index, &val);
		if (res != ERR_OK) { /* general failure? */
			cmpResult = (uint8_t) -1; /* no match */
			break;
		}
		if (val != *elemP) { /* mismatch */
			cmpResult = (uint8_t) -1; /* no match */
			break;
		}
		elemP++;
		index++;
		nof--;
	}

	return cmpResult;
}

/**
 * @brief   driver de-initialization
 * @returns nothing
 */
/**
 void input_ring_buffer_deinit(void)
 {
 ** Function is implemented as macro in the header file
 }
 */

/**
 * @brief	removes an element from the buffer
 * @returns error code
 */
uint8_t input_ring_buffer_delete(void)
{
	uint8_t res = ERR_OK;

	taskENTER_CRITICAL_FROM_ISR();
	if (input_ring_buffer_inSize == 0) {
		res = ERR_RXEMPTY;
	} else {
		input_ring_buffer_inSize--;
		input_ring_buffer_outIdx++;
		if (input_ring_buffer_outIdx == input_ring_buffer_CONFIG_BUF_SIZE) {
			input_ring_buffer_outIdx = 0;
		}
	}
	taskEXIT_CRITICAL_FROM_ISR(0);
	return res;
}

/**
 * @brief   updates the data of an element.
 * @param   index	: index of element. 0 peeks the top element
 *                           			1 the next, and so on.
 * @param   *elemP  : pointer to where to store the received element
 * @returns error code
 */
uint8_t input_ring_buffer_update(input_ring_buffer_BufSizeType index,
		input_ring_buffer_ElementType *elemP)
{
	uint8_t res = ERR_OK;
	int idx; /* index inside ring buffer */

	taskENTER_CRITICAL_FROM_ISR();
	if (index >= input_ring_buffer_CONFIG_BUF_SIZE) {
		res = ERR_OVERFLOW; /* asking for an element outside of ring buffer size */
	} else if (index < input_ring_buffer_inSize) {
		idx = (input_ring_buffer_outIdx + index)
				% input_ring_buffer_CONFIG_BUF_SIZE;
		input_ring_buffer_buffer[idx] = *elemP; /* replace element */
	} else { /* asking for an element which does not exist */
		res = ERR_RXEMPTY;
	}
	taskEXIT_CRITICAL_FROM_ISR(0);
	return res;
}
