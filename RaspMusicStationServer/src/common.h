#ifndef _common_h
#define _common_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <string>
#include <sys/wait.h>
#include "pthread.h"

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


#endif


