#include <stdio.h>
#include <cmath>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include <semaphore.h> 
#include "audioplay.h"

#define BUFFER_SIZE_SAMPLES 588

static const int samplerate=44100;
static const int bitdepth=16;
static const int nchannels=1;
static const int dest=0;
static const int bufferCount=10;

static const char *audio_dest[] = {"local", "hdmi"};

#define CTTW_SLEEP_TIME 5
#define MIN_LATENCY_TIME 30


void PlayBuffer(short *PCMData, int size)
{

   AUDIOPLAY_STATE_T *st;
   int32_t ret;
  
   int phase = 0;
   int inc = 256<<16;
   int dinc = 0;
   int buffer_bytes = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;

   ret = audioplay_create(&st, samplerate, nchannels, bitdepth, bufferCount, buffer_bytes);
   ret = audioplay_set_dest(st, audio_dest[dest]);

   int sectorNumber= (size-1)/BUFFER_SIZE_SAMPLES+1;
   
   int i;
   for (i=0;i<sectorNumber;i++)
   {
	  uint8_t *buf;
      int16_t *p;
      uint32_t latency;

      while((buf = audioplay_get_buffer(st)) == NULL)
         usleep(CTTW_SLEEP_TIME*1000);

	  p = (int16_t *) buf;

	  if ((i+1)*BUFFER_SIZE_SAMPLES<=size)
	  {
		  memcpy(p,PCMData+i*BUFFER_SIZE_SAMPLES,sizeof(short)*BUFFER_SIZE_SAMPLES);
	  }
	  else
	  {
		  memcpy(p,PCMData+i*BUFFER_SIZE_SAMPLES,sizeof(short)*(size-i*BUFFER_SIZE_SAMPLES));		
		  memset(p+size-i*BUFFER_SIZE_SAMPLES,0,sizeof(short)*((i+1)*BUFFER_SIZE_SAMPLES-size));
	  }

      // try and wait for a minimum latency time (in ms) before
      // sending the next packet
      while((latency = audioplay_get_latency(st)) > (samplerate * (MIN_LATENCY_TIME + CTTW_SLEEP_TIME) / 1000))
         usleep(CTTW_SLEEP_TIME*1000);
      
      //if (i==m_CurPlayStartSector) usleep(CTTW_SLEEP_TIME*1000);

      ret = audioplay_play_buffer(st, buf,  buffer_bytes);
   }

   audioplay_delete(st);

}
