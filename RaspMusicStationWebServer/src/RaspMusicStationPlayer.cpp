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

void* ListPlayBackMonitorThread(void* info)
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
	 sprintf(feedBack,"ListPlayBack %s", buffer);
     SendFeedback(feedBack, param->m_fbstuff);
  }
  SendFeedback("ListPlayBack Over", param->m_fbstuff);
  PlaySound("beep.mp3");
}

enum PlayBackStatus
{
  NO,
  URL,
  CD,
  List
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

    if (s_command=="PlayURL" || s_command=="PlayCD" || s_command=="PlayList" || s_command=="Stop" || s_command=="Quit")
    {
        if (slaveOut) 
        {
          fputc('q',slaveOut);     
          waitpid(pid,0,0);   
          void* ret;
          pthread_join(PlayBackMonitorThreadID,&ret);    
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
				else if (s_command=="PlayList")
        {
		 			int list=0;
          int startSong=0;
          if (strlen(buffer)>9) sscanf(buffer+9,"%d %d",&list, &startSong);
		  		char listID[10]; sprintf(listID,"%d",list);
          char startSongID[10]; sprintf(startSongID,"%d",startSong);
          PlaySound("beep.mp3");
          pid=LaunchChild(slaveOut,slaveIn);    
          if (pid==0)
          {
            execlp("/home/pi/omxdirplayer","omxdirplayer","-l",listID,"-t",startSongID, 0);
          }
          // start play-back monitor thread
          pbtParam.m_fin=slaveIn;  
          pthread_create(&PlayBackMonitorThreadID,NULL,ListPlayBackMonitorThread,&pbtParam);
          pbstatus=List;
        }
    }
    else if (s_command=="Eject")
    {
        if (slaveOut && pbstatus==CD) 
        {
			fputc('q',slaveOut);
			waitpid(pid,0,0);
			void* ret;
			pthread_join(PlayBackMonitorThreadID,&ret);            
          fclose(slaveIn);
          fclose(slaveOut);
          slaveIn=0;
          slaveOut=0;
          pbstatus=NO;
        } 
        system("eject /dev/sr0");
    }
    else if (s_command=="VolDown" || s_command=="VolUp" || s_command=="NextTrack" || s_command=="PrevTrack" || s_command=="CurTrack" || s_command=="NextSong" || s_command=="PrevSong" || s_command=="CurSong")
    {
        if (slaveOut) 
        {
            if (s_command=="VolDown")
              fputc('-',slaveOut);
            else if (s_command=="VolUp")
              fputc('+',slaveOut);
			else if(pbstatus==CD || pbstatus==List)
            {
              if (s_command=="PrevTrack" || s_command=="PrevSong")
                fputc('<',slaveOut);
              else if (s_command=="NextTrack" || s_command=="NextSong")
                fputc('>',slaveOut);
              else if (s_command=="CurTrack" || s_command=="CurSong")
                fputc('t',slaveOut);
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
       waitpid(listPid,0,0);
    } 
	else if (s_command=="ListLists")
	{
		FILE *listOut=0;
       FILE *listIn=0;
       pid_t listPid;	
       listPid=LaunchChild(listOut,listIn);    
       if (listPid==0)
       {
         execlp("/home/pi/omxdirplayer","omxdirplayer", "-ll",0);
       }
	    char buffer[4096];
       while (fgets(buffer,4096,listIn))
         SendFeedback(buffer, fbstuff);
      waitpid(listPid,0,0);
	}
	else if (s_command=="ListSongs")
	{
		   int list=0;
	      if (strlen(buffer)>10) sscanf(buffer+10,"%d",&list);
	      char listID[10]; sprintf(listID,"%d",list);

		   FILE *listOut=0;
       FILE *listIn=0;
       pid_t listPid;	
       listPid=LaunchChild(listOut,listIn);    
       if (listPid==0)
       {
         execlp("/home/pi/omxdirplayer","omxdirplayer", "-ls","-l",listID,0);
       }
	    char buffer[4096];
       while (fgets(buffer,4096,listIn))
         SendFeedback(buffer, fbstuff);
      waitpid(listPid,0,0);
	}

  }

  return 0;
}

