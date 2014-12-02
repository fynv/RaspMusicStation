#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <string>
#include <sys/wait.h>
#include "pthread.h"

#include "FeiSocket.h"
#define PORT 30001

using namespace std;

pid_t LaunchChild (FILE* &masterWriteStream, FILE* &masterReadStream)
{
  pid_t pid;
  int mypipe[2];
  int mypipe2[2];
  
  pipe (mypipe);
  pipe (mypipe2);
  pid = fork ();
  if (pid == (pid_t) 0)
  {
    dup2(mypipe[0], STDIN_FILENO);
    dup2(mypipe2[1], STDOUT_FILENO);
    close(mypipe[0]);
    close(mypipe[1]);
    setvbuf(stdout, NULL, _IONBF, 0);
  }
  else
  {
    close(mypipe[0]);
    close(mypipe2[1]);
    masterWriteStream= fdopen (mypipe[1], "w");    
    setbuf(masterWriteStream,0);
    masterReadStream= fdopen(mypipe2[0],"r");
  }  
  
  return pid;

}


struct PlayBackMonitorParam
{
  FILE* m_fin;
  bool m_isRunning;
};

void* CommandListenerProcess_PlayBackMonitorThread(void* info)
{
  PlayBackMonitorParam* param=(PlayBackMonitorParam*)(info);
  FILE *fin=param->m_fin;
  param->m_isRunning=true;

  char buffer[1024];
  while (1)
  {
     if (!fgets(buffer,1024,fin)) break;
     printf("OMXPlayer: %s",buffer);
     buffer[15]=0;
     string message=buffer;
     if (message=="have a nice day") break;
  }
  system("omxplayer /home/pi/sound/beep.mp3");  
  param->m_isRunning=false;  
}


int CommandListenerProcess()
{
  system("omxplayer /home/pi/sound/start.wav");
  
  FILE *fout=0;
  FILE *fin=0;
  pid_t pid;

  pthread_t PlayBackMonitorThreadID;
  PlayBackMonitorParam pbtParam;
  pbtParam.m_isRunning=false;

  while (1)
  {
    char buffer[4096];
    char command[256];
    gets(buffer);
    char *pos;
    if ((pos=strchr(buffer, '\n')) != NULL)
      *pos = '\0';
    printf("Command recieved: %s\n", buffer);
    sscanf(buffer,"%s",command);
    string s_command=command;
    
    if (s_command=="Reboot") 
    {
      system("omxplayer /home/pi/sound/reboot.wav");
      system("sudo reboot");
      return 0;
    }
    else if (s_command=="Shutdown") 
    {
      system("omxplayer /home/pi/sound/shutdown.wav");
      system("sudo halt -p");
      return 0;
    }
    
    else if (s_command=="PlayURL" || s_command=="Quit" || s_command=="Stop")
    {
        if (fout) 
        {
          int status;
          pid_t result = waitpid(pid, &status, WNOHANG);
          if (result == 0)
          {
            printf("Child-process is active.\n");
            fputc('q',fout);
            wait(0);
            void* ret;
            pthread_join(PlayBackMonitorThreadID,&ret);            
          }
          else printf("Child-process is not active.\n");
          fclose(fin);
          fclose(fout);
          fin=0;
          fout=0;
        } 
        if (s_command=="Quit") 
        {
          printf("Quit..\n");
          return 0;
        }
        else if (s_command=="PlayURL")
        {
          system("omxplayer /home/pi/sound/beep.mp3");
          char* p_url=buffer+8;
          pid=LaunchChild(fout,fin);    
          if (pid==0)
          {
            execlp("omxplayer","omxplayer", p_url,0);
          }
          // start play-back monitor thread
          pbtParam.m_fin=fin;
          pbtParam.m_isRunning=false;          
          int threadError = pthread_create(&PlayBackMonitorThreadID,NULL,CommandListenerProcess_PlayBackMonitorThread,&pbtParam);
          
        }
    }
    else if (s_command=="VolDown" || s_command=="VolUp")
    {
        if (fout) 
        {
          int status;
          pid_t result = waitpid(pid, &status, WNOHANG);
          if (result == 0)
          {
            if (s_command=="VolDown")
              fputc('-',fout);
            else 
              fputc('+',fout);
          }
         }
    }  

  }

  return 0;
}

void* HostProcess_CommandFeedBackThread(void* info)
{
  FILE* fin=(FILE*)info;
  char buffer[4096];
  while (fgets(buffer,4096, fin))
    printf("%s",buffer);
  exit(0);

}

void SendCommand(const char* cmd, FILE* fp,  pthread_mutex_t lock)
{
   pthread_mutex_lock(&lock);  
   fputs(cmd,fp);   
   pthread_mutex_unlock(&lock);  
}

struct SenderInfo
{
  FILE* fp;
  pthread_mutex_t lock;
};

void* HostProcess_KeyBoardInputThread(void* info)
{
  SenderInfo* sinfo=(SenderInfo*)info;
  char buffer[4096];
  while(1)
  {
    gets(buffer);
    sprintf(buffer,"%s\n",buffer);
    SendCommand(buffer,sinfo->fp,sinfo->lock);
  }
}

int HostProcess(pid_t listenerID, FILE* fout, FILE* fin)
{
  pthread_t CommandFeedBackThread;
  int threadError = pthread_create(&CommandFeedBackThread,NULL,HostProcess_CommandFeedBackThread,fin);

  pthread_t KeyBoardInputThread;
  pthread_mutex_t cmd_lock;
  pthread_mutex_init(&cmd_lock,NULL);  
  
  /*SenderInfo sinfo;
  sinfo.fp=fout;
  sinfo.lock=cmd_lock;

  threadError = pthread_create(&KeyBoardInputThread,NULL,HostProcess_KeyBoardInputThread,&sinfo);*/

  FeiSocketSever server(PORT);
  if (!server.IsValid())
  {
    SendCommand("Quit\n",fout, cmd_lock);
    return -1;
  }

  while (1)
  {
    FeiSocketSession se=server.GetSession();
    if (!se.IsValid()) continue;
    printf("New socket connection established.\n");

    while (1)
    {
      char buffer[4096];
      int len;
      len=se.Recieve(buffer,4096);
      if (len<=0) 
      {
        se.Close();
        break;
      }
      buffer[len]=0;
      SendCommand(buffer,fout, cmd_lock);
    }
    printf("Disconnected.\n");    
  }
  pthread_mutex_destroy(&cmd_lock);  
  
  return 0;
}

int main()
{
  //return CommandListenerProcess();
  FILE *fout=0;
  FILE *fin=0;
  pid_t pid= LaunchChild(fout,fin);

   if (pid==0)
   {
     return CommandListenerProcess();
   }
   else return HostProcess(pid, fout, fin);
    
}

