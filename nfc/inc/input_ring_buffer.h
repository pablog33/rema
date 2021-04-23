#ifndef INPUT_RING_BUFFER_H_
#define INPUT_RING_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define input_ring_buffer_CONFIG_ELEM_SIZE	1
#define input_ring_buffer_CONFIG_BUF_SIZE	10

/* error codes */
#define ERR_OK                          0x00U /*!< OK */
#define ERR_OVERFLOW                    0x04U /*!< Timer overflow. */
#define ERR_RXEMPTY                     0x0AU /*!< No data in receiver. */
#define ERR_TXFULL                      0x0BU /*!< Transmitter is full. */

#if input_ring_buffer_CONFIG_ELEM_SIZE==1
typedef uint8_t input_ring_buffer_ElementType; /* type of single element */
#elif input_ring_buffer_CONFIG_ELEM_SIZE==2
  typedef uint16_t input_ring_buffer_ElementType; /* type of single element */
#elif input_ring_buffer_CONFIG_ELEM_SIZE==4
  typedef uint32_t input_ring_buffer_ElementType; /* type of single element */
#else
  #error "illegal element type size in properties"
#endif
#if input_ring_buffer_CONFIG_BUF_SIZE<256
typedef uint8_t input_ring_buffer_BufSizeType; /* up to 255 elements (index 0x00..0xff) */
#else
  typedef uint16_t input_ring_buffer_BufSizeType; /* more than 255 elements, up to 2^16 */
#endif

uint8_t input_ring_buffer_put(input_ring_buffer_ElementType elem);

uint8_t input_ring_buffer_get(input_ring_buffer_ElementType *elemP);

input_ring_buffer_BufSizeType input_ring_buffer_number_of_elements(void);

void input_ring_buffer_init(void);

input_ring_buffer_BufSizeType input_ring_buffer_number_of_free_elements(void);

void input_ring_buffer_clear(void);

uint8_t input_ring_buffer_peek(input_ring_buffer_BufSizeType index,
		input_ring_buffer_ElementType *elemP);

#define input_ring_buffer_deinit() \
   /* nothing to deinitialize */

uint8_t input_ring_buffer_delete(void);

uint8_t input_ring_buffer_putn(input_ring_buffer_ElementType *elem,
		input_ring_buffer_BufSizeType nof);

uint8_t input_ring_buffer_compare(input_ring_buffer_BufSizeType index,
		input_ring_buffer_ElementType *elemP, input_ring_buffer_BufSizeType nof);

uint8_t input_ring_buffer_getn(input_ring_buffer_ElementType *buf,
		input_ring_buffer_BufSizeType nof);

uint8_t input_ring_buffer_update(input_ring_buffer_BufSizeType index,
		input_ring_buffer_ElementType *elemP);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* INPUT_RING_BUFFER_H_ */
