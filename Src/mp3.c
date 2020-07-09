/*  mp3.c   TBook mp3 decoder interface to libMAD 
 *  July 2020
 */ 
 
#include <stdio.h>
#include <stdint.h>
#include "mad.h"
#include "tbook.h"

const int BUFF_SIZ = 4096;
const int MAX_WAV_BYTES = 16000*2*200;

struct decode_state {		// decode state passed to mp3 callbacks
  int freq;				// samples/sec from decoded pcm structure

  FILE *fmp3;
  int   in_pos;

  FILE *fwav;

  unsigned char in_buff[ BUFF_SIZ ];
} decoder_state;
//
static char *								wav_header( int hz, int ch, int bips, int data_bytes ){  										// => ptr to valid .wav header
    static char hdr[44] = "RIFFsizeWAVEfmt \x10\0\0\0\1\0ch_hz_abpsbabsdatasize";
    unsigned long nAvgBytesPerSec = bips*ch*hz >> 3;
    unsigned int nBlockAlign      = bips*ch >> 3;

    *(int32_t *)(void*)(hdr + 0x04) = 44 + data_bytes - 8;   /* File size - 8 */
    *(int16_t *)(void*)(hdr + 0x14) = 1;                     /* Integer PCM format */
    *(int16_t *)(void*)(hdr + 0x16) = ch;
    *(int32_t *)(void*)(hdr + 0x18) = hz;
    *(int32_t *)(void*)(hdr + 0x1C) = nAvgBytesPerSec;
    *(int16_t *)(void*)(hdr + 0x20) = nBlockAlign;
    *(int16_t *)(void*)(hdr + 0x22) = bips;
    *(int32_t *)(void*)(hdr + 0x28) = data_bytes;
    return hdr;
}


static enum mad_flow 				input( void *st, struct mad_stream *stream ){																// cb fn to refill buffer from .mp3
/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. 
 */
  struct decode_state *dcdr_st = st;

	if ( dcdr_st->fmp3==NULL ) // file already closed, stop
		return MAD_FLOW_STOP;
	
  int len = fread( &dcdr_st->in_buff, 1, BUFF_SIZ, dcdr_st->fmp3 );     // read next buffer
  if ( len==0 ){  // end of file encountered
		memset( dcdr_st->in_buff, 0, BUFF_SIZ );	// set buff to 0
		fclose( dcdr_st->fmp3 );
		dcdr_st->fmp3 = NULL;		// mark as ready to stop
	}

  dcdr_st->in_pos += len;
  mad_stream_buffer( stream, &dcdr_st->in_buff[0], len==0? BUFF_SIZ : len );		// if 0 => buff of 0's
  return MAD_FLOW_CONTINUE;
}
static enum mad_flow 				hdr( void *st,	struct mad_header const *stream ){																// cb fn for frame header
	printf("hdr: %x \n", (uint32_t) stream );
  return MAD_FLOW_CONTINUE;
}


static inline int 					scale( mad_fixed_t sample ){																								// TODO: replace with dithering?
/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

  sample += (1L << (MAD_F_FRACBITS - 16));  /* round */

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  return sample >> (MAD_F_FRACBITS + 1 - 16);  /* quantize */
}


static enum mad_flow 				output( void *st, struct mad_header const *header, struct mad_pcm *pcm ){		// cb fn to output next frame of .wav
/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */
  struct decode_state *dcdr_st = st;

  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;

  /* pcm->samplerate contains the sampling frequency */

  if ( dcdr_st->freq == 0 ){
    dcdr_st->freq = pcm->samplerate;
    fwrite( wav_header( dcdr_st->freq, pcm->channels, 16, MAX_WAV_BYTES ), 1, 44, dcdr_st->fwav );
  }

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  while (nsamples--) {
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
    fputc( (sample >> 0) & 0xff, dcdr_st->fwav );
    fputc( (sample >> 8) & 0xff, dcdr_st->fwav );

    if (nchannels == 2) {
      sample = scale(*right_ch++);
      fputc( (sample >> 0) & 0xff, dcdr_st->fwav );
      fputc( (sample >> 8) & 0xff, dcdr_st->fwav );
    }
  }

  return MAD_FLOW_CONTINUE;
}

static void 								mp3Err( struct mad_stream *stream, int cd ){
	const char *s = mad_stream_errorstr( stream );
	errLog("Mp3 decode err %d: %s \n", cd, s );
}

static enum mad_flow 				error( void *st, struct mad_stream *stream, struct mad_frame *frame ){		// report mp3 decoding error
/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */	

	mp3Err( stream, stream->error );
//  fprintf(stderr, "decoding error 0x%04x (%s) in buff at offset %d \n",
//	  stream->error, mad_stream_errorstr(stream), dcdr_st->in_pos );
//	  (unsigned int)(stream->this_frame - buffer->start));

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
  return MAD_FLOW_CONTINUE;
}


void 												mp3ToWav( const char *nm ){		// decode nm.mp3 to nm.wav
/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */	
  struct decode_state *dcdr_st = &decoder_state;
	dcdr_st->freq = 0;
	dcdr_st->fmp3 = NULL;
	dcdr_st->in_pos = 0;
	dcdr_st->fwav = NULL;
	memset( &dcdr_st->in_buff, 0, BUFF_SIZ );

  struct mad_decoder decoder;
  int result;

  /* initialize decode state structure */
  char fnm[40];
  strcpy( fnm, nm );
  char *pdot = strrchr(fnm, '.');
  if ( pdot ) *pdot = 0; else pdot = &fnm[ strlen(fnm) ];
  strcpy( pdot, ".mp3" );

  dcdr_st->freq = 0;

  dcdr_st->fmp3 = fopen( fnm, "rb" );

  strcpy( pdot, ".wav" );
  dcdr_st->fwav = fopen( fnm, "wb");

  if ( dcdr_st->fmp3==NULL || dcdr_st->fwav==NULL ){ 
		dbgLog("Mp3 fopen %s: %d %d \n", fnm, dcdr_st->fmp3, dcdr_st->fwav );
		return;
	}
	
  /* configure input, output, and error functions */
  mad_decoder_init( &decoder, dcdr_st,  input, hdr, 0 /* filter */, output, error, 0 /* message */);
  result = mad_decoder_run( &decoder, MAD_DECODER_MODE_SYNC );  // start decoding 
	if ( result!=0 ) 
		mp3Err( NULL, result );
	
  mad_decoder_finish( &decoder );  // release the decoder 
	if ( dcdr_st->fmp3 != NULL ){ fclose( dcdr_st->fmp3 ); dcdr_st->fmp3 = NULL; }
	if ( dcdr_st->fwav != NULL ){ fclose( dcdr_st->fwav ); dcdr_st->fwav = NULL; }
}
