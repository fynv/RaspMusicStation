#include "common.h"
#include <semaphore.h>

int CommandListenerProcess(FILE *fin, FILE* fout);

#include "FeiSocket.h"
#define PORT 30001

struct FeedBackSocket
{
  FeiSocketSession m_se;
  sem_t m_start_sem;
  sem_t m_finish_sem;
};

struct CommandFeedBackThreadInfo
{
  FILE* m_fin;
  FeedBackSocket m_CDListSocket;
  FeedBackSocket m_CDStatusSocket;
};

void* HostProcess_CommandFeedBackThread(void* info)
{
  CommandFeedBackThreadInfo *tInfo=(CommandFeedBackThreadInfo*)info; 
  FILE* fin=(FILE*)tInfo->m_fin;
  char buffer[4096];
  char command[256];
  bool readingCDFeedBack=false;
  while (fgets(buffer,4096, fin))
  {
    char *pos;
    if ((pos=strchr(buffer, '\n')) != NULL)
      *pos = '\0';
    if (-1==sscanf(buffer,"%s",command)) continue;
    string s_command=command;
    if (s_command=="URLPlayBack")
      printf("%s\n",buffer+12);
    else if (s_command=="CDTrackInfo")
    {
      char* pinfo=buffer+12;
      sem_wait(&tInfo->m_CDListSocket.m_start_sem);
      int len=strlen(pinfo);
      pinfo[len]='\n';
      pinfo[len+1]='\0';
      tInfo->m_CDListSocket.m_se.Send(pinfo,len+1);
      sem_post(&tInfo->m_CDListSocket.m_finish_sem);
    }
    else if (s_command=="CDPlayBack")
    {
      if (!readingCDFeedBack)
      {
        sem_wait(&tInfo->m_CDStatusSocket.m_start_sem);
        readingCDFeedBack=true;
      }
      char* pinfo=buffer+11;
      int len=strlen(pinfo);
      pinfo[len]='\n';
      pinfo[len+1]='\0';      
      tInfo->m_CDStatusSocket.m_se.Send(pinfo,len+1);
      char command2[256];
      if (-1==sscanf(pinfo,"%s",command2)) continue;
      string s_command2=command2;
      if (s_command2=="Over")
      {
        sem_post(&tInfo->m_CDStatusSocket.m_finish_sem);
        readingCDFeedBack=false;
      }      
    }
  }
}

void SendCommand(const char* cmd, FILE* fp,  pthread_mutex_t lock)
{
   pthread_mutex_lock(&lock);  
   fputs(cmd,fp);  
   fputc('\n',fp); 
   fflush(fp);
   pthread_mutex_unlock(&lock);  
}


int HostProcess(pid_t listenerID, FILE* fout, FILE* fin)
{
  CommandFeedBackThreadInfo cfbInfo;
  cfbInfo.m_fin=fin;
  pthread_t CommandFeedBackThread;
  int threadError = pthread_create(&CommandFeedBackThread,NULL,HostProcess_CommandFeedBackThread,&cfbInfo);

  pthread_t KeyBoardInputThread;
  pthread_mutex_t cmd_lock;
  pthread_mutex_init(&cmd_lock,NULL);  
  
  FeiSocketSever server(PORT);
  if (!server.IsValid())
  {
    SendCommand("Quit",fout, cmd_lock);
    return -1;
  }

  while (1)
  {
    FeiSocketSession se=server.GetSession();
    if (se.IsValid())
    {
      char buffer[4096];
      char command[256];
      int len;
      len=se.Recieve(buffer,4096);
      if (len<=0) 
      {
        se.Close();
        break;
      }
      buffer[len]=0;
      char *pos;
      if ((pos=strchr(buffer, '\n')) != NULL)
        *pos = '\0';
      if (-1==sscanf(buffer,"%s",command)) continue;
      string s_command=command;
      
      if (s_command=="Shutdown" || s_command=="Reboot")
      {
        SendCommand("Quit",fout, cmd_lock);
        if (s_command=="Shutdown")
        {
          PlaySound("shutdown.mp3");
          system("sudo halt -p");
          return 0;
        }
        else if (s_command=="Reboot")
        {
          PlaySound("reboot.mp3");
          system("sudo reboot");
          return 0;
        }
      }
      else 
      {
        SendCommand(buffer,fout, cmd_lock);
        if (s_command=="PlayCD")
        {
          if (cfbInfo.m_CDStatusSocket.m_se.IsValid())
            sem_wait(&cfbInfo.m_CDStatusSocket.m_finish_sem);
          cfbInfo.m_CDStatusSocket.m_se=se;
          sem_post(&cfbInfo.m_CDStatusSocket.m_start_sem);
        }
        else if (s_command=="ListCD")
        {
          cfbInfo.m_CDListSocket.m_se=se;
          sem_post(&cfbInfo.m_CDListSocket.m_start_sem);
          sem_wait(&cfbInfo.m_CDListSocket.m_finish_sem);
        }       
      }
    }
  }
  pthread_mutex_destroy(&cmd_lock);  
  
  return 0;
}

int main()
{
  PlaySound("start.mp3");
  //return CommandListenerProcess(stdout,stdin);
  FILE *fout=0;
  FILE *fin=0;
  pid_t pid= LaunchChildSymm(fout,fin);

   if (pid==0)
   {
     return CommandListenerProcess(fout,fin);
   }
   else return HostProcess(pid, fout, fin);
}

