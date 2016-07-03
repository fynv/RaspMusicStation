#ifndef _common_h
#define _common_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <string>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

using namespace std;

pid_t LaunchChild (FILE* &masterWriteStream, FILE* &masterReadStream);
pid_t LaunchChildSymm (FILE* &WriteStream, FILE* &ReadStream);
void PlaySound(const char* snd);

struct FeedBackStuff
{
  FILE* m_fout;
  pthread_mutex_t m_lock;
};

void SendFeedback(const char* feedback, FeedBackStuff stuff);

unsigned long long GetUSec();

void WaitKill(pid_t pid, unsigned long long uDuration, unsigned long long uInterval = 10000);

#endif


