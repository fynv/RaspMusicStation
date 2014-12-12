#include "common.h"

struct PlayBackMonitorParam
{
  FILE* m_fin;
  FeedBackStuff m_fbstuff;
};

void* URLPlayBackMonitorThread(void* info)
{
  PlayBackMonitorParam* param=(PlayBackMonitorParam*)(info);
  FILE *fin=param->m_fin;

  char buffer[1024];
  while (1)
  {
     if (!fgets(buffer,1024,fin)) break;
     char *pos;
     if ((pos=strchr(buffer, '\n')) != NULL)
      *pos = '\0';
     char feedBack[1024];
     sprintf(feedBack,"URLPlayBack %s", buffer);
     SendFeedback(feedBack, param->m_fbstuff);
  }
  PlaySound("beep.mp3");
}

void* CDPlayBackMonitorThread(void* info)
{
  PlayBackMonitorParam* param=(PlayBackMonitorParam*)(info);
  FILE *fin=param->m_fin;

  char buffer[1024];
  while (1)
  {
     if (!fgets(buffer,1024,fin)) break;
     char *pos;
     if ((pos=strchr(buffer, '\n')) != NULL)
      *pos = '\0';
     char feedBack[1024];
     sprintf(feedBack,"CDPlayBack %s", buffer);
     SendFeedback(feedBack, param->m_fbstuff);
  }
  SendFeedback("CDPlayBack Over", param->m_fbstuff);
  PlaySound("beep.mp3");
}

enum PlayBackStatus
{
  NO,
  URL,
  CD
};

int CommandListenerProcess(FILE* fout, FILE *fin)
{  
  pthread_mutex_t feedback_lock;
  pthread_mutex_init(&feedback_lock,NULL);  

  FeedBackStuff fbstuff;
  fbstuff.m_fout=fout;
  fbstuff.m_lock=feedback_lock;

  FILE *slaveOut=0;
  FILE *slaveIn=0;
  pid_t pid;

  pthread_t PlayBackMonitorThreadID;
  PlayBackMonitorParam pbtParam;
  pbtParam.m_fbstuff=fbstuff;

  PlayBackStatus pbstatus=NO;

  while (1)
  {
    char buffer[4096];
    char command[256];
    fgets(buffer,4096,fin);
    char *pos;
    if ((pos=strchr(buffer, '\n')) != NULL)
      *pos = '\0';

    if (-1==sscanf(buffer,"%s",command)) continue;
    string s_command=command;

    if (s_command=="PlayURL" || s_command=="PlayCD" || s_command=="Stop" || s_command=="Quit")
    {
        if (slaveOut) 
        {
          int status;
          pid_t result = waitpid(pid, &status, WNOHANG);
          if (result == 0)
          {
            fputc('q',slaveOut);
            waitpid(pid,0,0);
            void* ret;
            pthread_join(PlayBackMonitorThreadID,&ret);            
          }
          fclose(slaveIn);
          fclose(slaveOut);
          slaveIn=0;
          slaveOut=0;
          pbstatus=NO;
        } 
        if (s_command=="Quit") break;
        else if (s_command=="PlayURL")
        {
          PlaySound("beep.mp3");
          char* p_url=buffer+8;
          pid=LaunchChild(slaveOut,slaveIn);    
          if (pid==0)
          {
            execlp("omxplayer","omxplayer", p_url,0);
          }
          // start play-back monitor thread
          pbtParam.m_fin=slaveIn;  
          pthread_create(&PlayBackMonitorThreadID,NULL,URLPlayBackMonitorThread,&pbtParam);
          pbstatus=URL;
        }
        else if (s_command=="PlayCD")
        {
          int startTrack=0;
          if (strlen(buffer)>7) sscanf(buffer+7,"%d",&startTrack);
          char startTrackID[10]; sprintf(startTrackID,"%d",startTrack);
          PlaySound("beep.mp3");
          pid=LaunchChild(slaveOut,slaveIn);    
          if (pid==0)
          {
            execlp("/home/pi/omxcdplayer","omxcdplayer", "-s","-t",startTrackID, 0);
          }
          // start play-back monitor thread
          pbtParam.m_fin=slaveIn;  
          pthread_create(&PlayBackMonitorThreadID,NULL,CDPlayBackMonitorThread,&pbtParam);
          pbstatus=CD;
        }
    }
    else if (s_command=="Eject")
    {
        if (slaveOut && pbstatus==CD) 
        {
          int status;
          pid_t result = waitpid(pid, &status, WNOHANG);
          if (result == 0)
          {
            fputc('q',slaveOut);
            waitpid(pid,0,0);
            void* ret;
            pthread_join(PlayBackMonitorThreadID,&ret);            
          }
          fclose(slaveIn);
          fclose(slaveOut);
          slaveIn=0;
          slaveOut=0;
          pbstatus=NO;
        } 
        system("eject");
    }
    else if (s_command=="VolDown" || s_command=="VolUp" || s_command=="NextTrack" || s_command=="PrevTrack")
    {
        if (slaveOut) 
        {
          int status;
          pid_t result = waitpid(pid, &status, WNOHANG);
          if (result == 0)
          {
            if (s_command=="VolDown")
              fputc('-',slaveOut);
            else if (s_command=="VolUp")
              fputc('+',slaveOut);
            else if(pbstatus==CD)
            {
              if (s_command=="PrevTrack")
                fputc('<',slaveOut);
              else if (s_command=="NextTrack")
                fputc('>',slaveOut);
            }
          }
         }
    } 
    else if (s_command=="ListCD")
    {
       FILE *listOut=0;
       FILE *listIn=0;
       pid_t listPid;	
       listPid=LaunchChild(listOut,listIn);    
       if (listPid==0)
       {
         execlp("/home/pi/omxcdplayer","omxcdplayer", "-l",0);
       }
       char buffer[4096];
       while (fgets(buffer,4096,listIn))
         SendFeedback(buffer, fbstuff);
    } 

  }

  return 0;
}

