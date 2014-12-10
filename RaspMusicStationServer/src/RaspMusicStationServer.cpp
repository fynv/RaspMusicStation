#include "common.h"

int CommandListenerProcess(FILE *fin, FILE* fout);

#include "FeiSocket.h"
#define PORT 30001

void* HostProcess_CommandFeedBackThread(void* info)
{
  FILE* fin=(FILE*)info;
  char buffer[4096];
  while (fgets(buffer,4096, fin))
    printf("%s",buffer);
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
  pthread_t CommandFeedBackThread;
  int threadError = pthread_create(&CommandFeedBackThread,NULL,HostProcess_CommandFeedBackThread,fin);

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
    if (!se.IsValid()) continue;

    while (1)
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
      sscanf(buffer,"%s",command);
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
      else SendCommand(buffer,fout, cmd_lock);
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

