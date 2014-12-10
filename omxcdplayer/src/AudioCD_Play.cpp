#include <stdio.h>
#include <cmath>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include <semaphore.h> 
#include "audioplay.h"
#include "AudioCD.h"

#define BUFFER_SIZE_SAMPLES 588

static const int samplerate=44100;
static const int bitdepth=16;
static const int nchannels=2;
static const int dest=0;
static const int bufferCount=10;

static const char *audio_dest[] = {"local", "hdmi"};

#define CTTW_SLEEP_TIME 5
#define MIN_LATENCY_TIME 30

class CDIOBuffer
{
  AudioCD* m_acd;
  int m_bufferNum;
  char *m_buffer;
  int m_curBufferID;
  int m_curGetBufferID;
  int m_i;
  pthread_t m_IOThreadID;
  sem_t m_sem1;
  sem_t m_sem2;  
  bool m_running;
  static void* _IOThread(void* info)
  {
    int buffer_bytes = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;
    CDIOBuffer* bufferObj=(CDIOBuffer*)info;
    AudioCD* acd=bufferObj->m_acd;
    for (;bufferObj->m_i<acd->m_CurPlayStartSector+acd->m_CurPlayDurationSectors;bufferObj->m_i++)
    {
      sem_wait(&bufferObj->m_sem2);
      if (!bufferObj->m_running) break;
      acd->_fillBuffer((short*)(bufferObj->m_buffer+bufferObj->m_curBufferID*buffer_bytes),bufferObj->m_i);  
      sem_post(&bufferObj->m_sem1);     
      bufferObj->m_curBufferID=(bufferObj->m_curBufferID+1)%bufferObj->m_bufferNum;
    } 
    return 0;
  }
public:
  CDIOBuffer(AudioCD* acd, int bufferNum):
   m_acd(acd), m_bufferNum(bufferNum), m_curGetBufferID(0), m_running(true)
  {
     sem_init(&m_sem1,0,0);  
     sem_init(&m_sem2,0,0);  

    int buffer_bytes = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;
    m_buffer=new char[buffer_bytes*bufferNum];
    for (m_curBufferID=0, m_i=acd->m_CurPlayStartSector; 
         m_curBufferID<acd->m_CurPlayDurationSectors && m_curBufferID<bufferNum; 
         m_curBufferID++, m_i++)
    {
      m_acd->_fillBuffer((short*)(m_buffer+m_curBufferID*buffer_bytes),m_i);
      sem_post(&m_sem1);
    }
    m_curBufferID=m_curBufferID%bufferNum;    
    pthread_create(&m_IOThreadID,NULL,_IOThread,this);
    
  }
  ~CDIOBuffer()
  {
    m_running=false;
    sem_post(&m_sem2);
    void* ret;
    pthread_join(m_IOThreadID,&ret); 
    sem_destroy(&m_sem2);
    sem_destroy(&m_sem1);
    delete[] m_buffer;
  }
  void GetBuffer(void* buffer)
  {
    int buffer_bytes = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;
    sem_wait(&m_sem1);
    memcpy(buffer,m_buffer+m_curGetBufferID*buffer_bytes, buffer_bytes);
    m_curGetBufferID=(m_curGetBufferID+1)%m_bufferNum;
    sem_post(&m_sem2);
  }
};

static void AdjustVolume_basic(short* buf, float k)
{
  int i;
  for (i=0;i<BUFFER_SIZE_SAMPLES*2;i++)
  {
     float v=(float)buf[i];
     v*=k; 
     if (v>32767.0f) v=32767.0f; 
     if (v<-32767.0f) v=-32767.0f; 
     buf[i]=(short)v;
  }
}

static void AdjustVolume(short* buf, int DB)
{
  float k=powf(10.0f,(float)DB*0.1f);
  if (k<1.0f) 
  {
    AdjustVolume_basic(buf,k);
    return;
  }
  if (k<2.0f)
  {
    float a=0.25f*k*k/(k-1.0f);
    float b=0.25f*(k-2.0f)*(k-2.0f)/(k-1.0f);
    float plim=2.0f/k-1.0f;
    float nlim=-plim;
    int i;
    for (i=0;i<BUFFER_SIZE_SAMPLES*2;i++)
    {
      float v=(float)buf[i]/(32767.0f);
      if (v>=nlim && v<=plim)
        v*=k;
      else if (v>plim)
        v=-a*v*(v-2.0f)-b;
      else 
        v=a*v*(v+2.0f)+b;
      buf[i]=(short)(v*32767.0f);
    }
  }
  else
  {
    float a=0.25*k;
    float plim=2.0f/k;
    float nlim=-plim;
    int i;
    for (i=0;i<BUFFER_SIZE_SAMPLES*2;i++)
    {
      float v=(float)buf[i]/(32767.0f);
      if (v<nlim)
        v=-1.0f;
      else if (v<0.0f)
        v=a*v*(4.0f+k*v);
      else if (v<plim)
        v=a*v*(4.0f-k*v);
      else 
        v=1.0f;
     
      buf[i]=(short)(v*32767.0f);
    }
    
  }
}

void AudioCD::_play(CDPlayStuff* stuff)
{
   AUDIOPLAY_STATE_T *st;
   int32_t ret;
  
   int phase = 0;
   int inc = 256<<16;
   int dinc = 0;
   int buffer_bytes = (BUFFER_SIZE_SAMPLES * bitdepth * OUT_CHANNELS(nchannels))>>3;

   ret = audioplay_create(&st, samplerate, nchannels, bitdepth, bufferCount, buffer_bytes);
   ret = audioplay_set_dest(st, audio_dest[dest]);

   int FeedBackInterval=stuff->FeedBackInterval();

   CDIOBuffer iobuf(this,bufferCount);

   int i;
   for (i=m_CurPlayStartSector;i<m_CurPlayStartSector+m_CurPlayDurationSectors;i++)
   {
      if (!stuff->IsPlaying()) break;
      uint8_t *buf;
      int16_t *p;
      uint32_t latency;

      while((buf = audioplay_get_buffer(st)) == NULL)
         usleep(CTTW_SLEEP_TIME*1000);

      p = (int16_t *) buf;

      // fill the buffer
      // _fillBuffer(p, i);
      iobuf.GetBuffer(p);
      int DB=stuff->GetDB();
      if (DB!=0) AdjustVolume(p,DB);
  
      int index=i-m_CurPlayStartSector;
      if (index%FeedBackInterval== FeedBackInterval-1)
      {
        char buffer[256];
        sprintf(buffer,"sector %d",index);
        stuff->FeedBack(buffer);
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


