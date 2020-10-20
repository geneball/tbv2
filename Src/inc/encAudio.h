// encAudio.h
//   encryption of data for export to Amplio
//

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "mbedtls/aes.h"
#include "mbedtls/pk.h"
#include "mbedtls/base64.h"
#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define AES_KEY_WDS  6
#define BUFF_SIZ  512

typedef struct encryptState {
		bool 								initialized;
		uint32_t 							sessionkey[ AES_KEY_WDS+1 ];
		mbedtls_aes_context 				aes;
		uint8_t								iv[ 16 ];
		mbedtls_pk_context					pk;
        mbedtls_ctr_drbg_context            drbg;
        mbedtls_entropy_context             entr;

} encryptState_t;
extern  encryptState_t  Enc;

extern  const int       KEY_BYTES;
extern  const int       SESS_SIZE;
extern  const int       AES_BLOCK_SIZE;
extern  const uint32_t  SessMARKER;

void                            initRandom( char *pers );                                       // reset RNG before encrypt or decrypt
int 							genRand( void *pRNG, unsigned char* output, size_t out_len );   // fill output with true random data from STM32F412 RNG dev

void 							startEncrypt( char * fname );								    // init session key & save public encrypted copy in 'fname.key'
void 							encryptBlock( const uint8_t *in, uint8_t *out, int len );	    // AES CBC encrypt in[0..len] => out[] (len%16===0)
void 							endEncrypt( void );											    // destroy session key

//void 							tbErr( const char * fmt, ... );								    // report fatal error
void 							TlsErr( char *msg, int cd );                                    // report mbedtls error

// end encAudio.h
