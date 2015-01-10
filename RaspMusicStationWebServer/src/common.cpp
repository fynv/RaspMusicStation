
#include "common.h"

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
    close(mypipe2[0]);
    close(mypipe[1]);
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

pid_t LaunchChildSymm (FILE* &WriteStream, FILE* &ReadStream)
{
  pid_t pid;
  int mypipe[2];
  int mypipe2[2];
  
  pipe (mypipe);
  pipe (mypipe2);
  pid = fork ();
  if (pid == (pid_t) 0)
  {
    close(mypipe2[0]);
    close(mypipe[1]);
    WriteStream= fdopen (mypipe2[1], "w");    
    setbuf(WriteStream,0);
    ReadStream= fdopen(mypipe[0],"r");
  }
  else
  {
    close(mypipe[0]);
    close(mypipe2[1]);
    WriteStream= fdopen (mypipe[1], "w");    
    setbuf(WriteStream,0);
    ReadStream= fdopen(mypipe2[0],"r");
  }  
  
  return pid;

}

void PlaySound(const char* snd)
{
  FILE *fout=0;
  FILE *fin=0;
  pid_t pid;
  pid=LaunchChild(fout,fin); 
  if (pid==0)
  {
    char buffer[1024];
    sprintf(buffer,"/home/pi/sound/%s", snd);
    execlp("omxplayer","omxplayer", buffer,0);
  }
  else
  {
    waitpid(pid,0,0);
  }  
}

void SendFeedback(const char* feedback, FeedBackStuff stuff)
{
   pthread_mutex_lock(&stuff.m_lock);  
   fputs(feedback,stuff.m_fout);   
   fputc('\n',stuff.m_fout);
   fflush(stuff.m_fout);
   pthread_mutex_unlock(&stuff.m_lock);  
}



