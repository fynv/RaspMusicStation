#include <stdio.h>
#include <pthread.h>
#include <string>
#include "AudioCD.h"
#include "audioplay.h"

using namespace std;

struct FeedBackStuff
{
  FILE* m_fout;
  pthread_mutex_t m_lock;
};

void SendFeedback(const char* feedback, FeedBackStuff stuff)
{
   pthread_mutex_lock(&stuff.m_lock);  
   fputs(feedback,stuff.m_fout);   
   fputc('\n',stuff.m_fout);
   fflush(stuff.m_fout);
   pthread_mutex_unlock(&stuff.m_lock);  
}

class CDPlayStuffConcrete : public CDPlayStuff
{
public:
  bool m_IsPlaying;
  bool m_ShowStatus;
  int m_DB;
  FeedBackStuff m_fbstuff;
  virtual bool IsPlaying() { return m_IsPlaying; }
  virtual int GetDB() {return m_DB; }
  virtual void FeedBack(const char* str)
  {
    if (m_ShowStatus)
      SendFeedback(str,m_fbstuff);
  }
  virtual int FeedBackInterval() { return 75;}
};


struct CDParam
{
  AudioCD* m_acd;
  int m_trackID;
  bool resetID;
  CDPlayStuffConcrete m_stuff;  
};

void* PlayCDThread(void* info)
{
  CDParam* param=(CDParam*)info;
  AudioCD* acd=param->m_acd;
  int n=acd->GetTrackCount();
  for (;param->m_trackID<n;param->m_trackID++)
  {
    if (!param->m_stuff.m_IsPlaying) break;
    do
    {
       if (param->m_trackID>=n) param->m_trackID=n-1;
       if (param->m_trackID<0) param->m_trackID=0;
       param->resetID=false;
       param->m_stuff.m_IsPlaying=true;
       if (!acd->GetTrackInfo(param->m_trackID).isAudio) break;  
       char feedback[1024];
       sprintf(feedback,"track %d", param->m_trackID);
       SendFeedback(feedback,param->m_stuff.m_fbstuff);    
       acd->Play(param->m_trackID, &param->m_stuff);
    }
    while (param->resetID);    
  }
  exit(0);
}

int PlayCD(int startTrack, bool showStatus, bool listMode,  FILE* fin, FILE* fout)
{
  bcm_host_init();
  
  pthread_mutex_t feedback_lock;
  pthread_mutex_init(&feedback_lock,NULL); 

  FeedBackStuff fbstuff;
  fbstuff.m_fout=fout;
  fbstuff.m_lock=feedback_lock;

  AudioCD acd;

  if (listMode)
  {
     char buffer[4096];
      int n=acd.GetTrackCount();
      if (n<0) n=0;
      sprintf(buffer,"CDTrackInfo %d",n);
      int i;
      for (i=0;i<n;i++)
      {
        TrackInfo info=acd.GetTrackInfo(i);
        sprintf(buffer,"%s %d",buffer,info.durationSectors);
      }
      sprintf(buffer,"%s",buffer);
      SendFeedback(buffer, fbstuff);
    return 0;
  }

  pthread_t PlayCDThreadID;
  CDParam cdParam;
  cdParam.m_acd=&acd;
  cdParam.m_trackID=startTrack;
  cdParam.m_stuff.m_IsPlaying=true;
  cdParam.m_stuff.m_fbstuff=fbstuff;
  cdParam.m_stuff.m_DB=0;
  cdParam.m_stuff.m_ShowStatus=showStatus;

  cdParam.resetID=false;
  
  pthread_create(&PlayCDThreadID,NULL,PlayCDThread,&cdParam);

  while (1)
  {
    char cmd=fgetc(fin);
    if (cmd=='q' || cmd=='Q')
    {
      cdParam.m_stuff.m_IsPlaying=false;
      void* ret;
      pthread_join(PlayCDThreadID,&ret); 
      break;
    }
    else if (cmd=='+' || cmd=='=')
    {
      cdParam.m_stuff.m_DB+=3;
      char buffer[256];
      sprintf(buffer,"volume %d",cdParam.m_stuff.m_DB);
      SendFeedback(buffer, fbstuff);
    }
    else if (cmd=='-' || cmd=='_')
    {
      cdParam.m_stuff.m_DB-=3;
      char buffer[256];
      sprintf(buffer,"volume %d",cdParam.m_stuff.m_DB);
      SendFeedback(buffer, fbstuff);
    }
    else if (cmd=='>' || cmd=='.')
    {
      cdParam.resetID=true;
      cdParam.m_trackID++;
      cdParam.m_stuff.m_IsPlaying=false;
    }
    else if (cmd=='<' || cmd==',')
    {
      cdParam.resetID=true;
      cdParam.m_trackID--;
      cdParam.m_stuff.m_IsPlaying=false;
    }   
    else if (cmd=='s' || cmd=='S')
    {
      cdParam.m_stuff.m_ShowStatus=!cdParam.m_stuff.m_ShowStatus;
    }
    else if (cmd=='t' || cmd=='T')
    {
      if (cdParam.m_stuff.m_IsPlaying)
      {
        char buffer[256];
        sprintf(buffer,"track %d",cdParam.m_trackID);
        SendFeedback(buffer, fbstuff);
      }
    }
  }

  
  return 0;
}


