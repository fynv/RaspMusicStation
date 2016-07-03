#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libwebsockets.h>

#include "common.h"
#include <semaphore.h>

#include "FeiSocket.h"
#define SOCKET_PORT 30001
#define WEB_PORT 8080

int CommandListenerProcess(FILE *fin, FILE* fout);

static int callback_http(struct libwebsocket_context *context,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
   switch (reason) 
   {
    case LWS_CALLBACK_HTTP: 
    {
      int n;
      if (len>1)
      	{
      			char urlbuffer[1024];
      			char* inc=(char*)in;
      			inc[len]=0;
      			sprintf(urlbuffer,"/home/pi/web%s",inc);
						libwebsockets_serve_http_file(context, wsi, urlbuffer, "text/html", 0, n);
						break;
      	}
      	else
    		{        			
						libwebsockets_serve_http_file(context, wsi, "/home/pi/web/index.html","text/html", 0, n);
						break;
				}
			if (n < 0 || ((n > 0) && lws_http_transaction_completed(wsi)))
			return -1;
    }
  }
  return 0;

}


void SendCommand(const char* cmd, FILE* fp)
{
   fputs(cmd,fp);  
   fputc('\n',fp); 
   fflush(fp);
}

class FeedBackSession
{
public:
	virtual void FeedBack(const char* text) =0;
	virtual FeedBackSession* Duplicate() const  =0 ;
};

class DupFeedBackSession
{
	FeedBackSession* m_se;
public:
	DupFeedBackSession()
	{
		m_se=0;
	}
	DupFeedBackSession(const FeedBackSession* se)
	{
		if (se)
		  m_se=se->Duplicate();
		else
			m_se=0;
	}
	DupFeedBackSession(const DupFeedBackSession& se)
	{
		if (se.m_se)
		  m_se=se.m_se->Duplicate();
		else
			m_se=0;
	}
	~DupFeedBackSession()
	{
		delete m_se;
	}
	bool IsValid() { return m_se!=0; }
	const DupFeedBackSession& operator = (const FeedBackSession* se)
	{
		delete m_se;
		if (se)
		  m_se=se->Duplicate();
		else
			m_se=0;
		return *this;
	}
	const DupFeedBackSession& operator = (const DupFeedBackSession& se)
	{
		return *this=se.m_se;
	}
	operator FeedBackSession* () 
	{
		return m_se;
	}
	operator const FeedBackSession* () const
	{
		return m_se;
	}
	FeedBackSession* operator->() 
	{ 
		return m_se;		
	}
	FeedBackSession& operator*() 
	{
		return *m_se;
	} 
};


struct FeedBackSocket
{
  bool m_resetSocket;
  DupFeedBackSession m_se;
  sem_t m_start_sem;
  sem_t m_finish_sem;
};

struct CommandFeedBackThreadInfo
{
  FILE* m_fin;
  FeedBackSocket m_CDListSocket;
  FeedBackSocket m_CDStatusSocket;
  FeedBackSocket m_ListListsSocket;
  FeedBackSocket m_ListSongsSocket;
  FeedBackSocket m_ListStatusSocket;
};

void* HostProcess_CommandFeedBackThread(void* info)
{
  CommandFeedBackThreadInfo *tInfo=(CommandFeedBackThreadInfo*)info; 
  FILE* fin=(FILE*)tInfo->m_fin;
  char buffer[4096];
  char command[256];
  bool readingCDFeedBack=false;
  bool readingListLists=false;
  bool readingListSongs=false;
  bool readingListFeedBack=false;
  while (fgets(buffer,4096, fin))
  {
  	//printf("feedback:%s",buffer);
    char *pos;
    if ((pos=strchr(buffer, '\n')) != NULL)
      *pos = '\0';
    if (-1==sscanf(buffer,"%s",command)) continue;
    string s_command=command;
    if (s_command=="CDTrackInfo")
    {
      char* pinfo=buffer+12;
      sem_wait(&tInfo->m_CDListSocket.m_start_sem);
      int len=strlen(pinfo);
      pinfo[len]='\n';
      pinfo[len+1]='\0';
      tInfo->m_CDListSocket.m_se->FeedBack(pinfo);
      sem_post(&tInfo->m_CDListSocket.m_finish_sem);
    }
    else if (s_command=="CDPlayBack")
    {
      if (!readingCDFeedBack)
      {
        sem_wait(&tInfo->m_CDStatusSocket.m_start_sem);
        readingCDFeedBack=true;
      }
      if (tInfo->m_CDStatusSocket.m_resetSocket)
      {
        readingCDFeedBack=false;
        tInfo->m_CDStatusSocket.m_se=0;
        sem_post(&tInfo->m_CDStatusSocket.m_finish_sem);        
        tInfo->m_CDStatusSocket.m_resetSocket=false;
        sem_wait(&tInfo->m_CDStatusSocket.m_start_sem);
        readingCDFeedBack=true;        
      }
      char* pinfo=buffer+11;
      int len=strlen(pinfo);
      pinfo[len]='\n';
      pinfo[len+1]='\0';     
      tInfo->m_CDStatusSocket.m_se->FeedBack(pinfo);
      char command2[256];
      if (-1==sscanf(pinfo,"%s",command2)) continue;
      string s_command2=command2;
      if (s_command2=="Over")
      {
        readingCDFeedBack=false;
        tInfo->m_CDStatusSocket.m_se=0;
        sem_post(&tInfo->m_CDStatusSocket.m_finish_sem);        
      }      
    }
		else if (s_command=="ListLists")
    {
      char* pinfo=buffer+10;
		  if (!readingListLists)
		  {
		    sem_wait(&tInfo->m_ListListsSocket.m_start_sem);
			  readingListLists=true;
		  }
      int len=strlen(pinfo);
      pinfo[len]='\n';
      pinfo[len+1]='\0';
      tInfo->m_ListListsSocket.m_se->FeedBack(pinfo);
	  	char command2[256];
      if (-1==sscanf(pinfo,"%s",command2)) continue;
		  string s_command2=command2;
		  if (s_command2=="#End")
	    {
			  readingListLists=false;
			  sem_post(&tInfo->m_ListListsSocket.m_finish_sem);
		  }
    }
		else if (s_command=="ListSongs")
    {
      char* pinfo=buffer+10;
		  if (!readingListSongs)
		  {
		    sem_wait(&tInfo->m_ListSongsSocket.m_start_sem);
			  readingListSongs=true;
		  }
      int len=strlen(pinfo);
      pinfo[len]='\n';
      pinfo[len+1]='\0';
      tInfo->m_ListSongsSocket.m_se->FeedBack(pinfo);
	  	char command2[256];
      if (-1==sscanf(pinfo,"%s",command2)) continue;
		  string s_command2=command2;
		  if (s_command2=="#End")
	    {
			  readingListSongs=false;
			  sem_post(&tInfo->m_ListSongsSocket.m_finish_sem);
		  }
    }
		else if (s_command=="ListPlayBack")
    {
      if (!readingListFeedBack)
      {
        sem_wait(&tInfo->m_ListStatusSocket.m_start_sem);
        readingListFeedBack=true;
      }
      
      if (tInfo->m_ListStatusSocket.m_resetSocket)
      {
        readingListFeedBack=false;
        tInfo->m_ListStatusSocket.m_se=0;
        sem_post(&tInfo->m_ListStatusSocket.m_finish_sem);        
        tInfo->m_ListStatusSocket.m_resetSocket=false;
        sem_wait(&tInfo->m_ListStatusSocket.m_start_sem);
        readingListFeedBack=true;        
      }
      char* pinfo=buffer+13;
      int len=strlen(pinfo);
      pinfo[len]='\n';

      pinfo[len+1]='\0';      
      tInfo->m_ListStatusSocket.m_se->FeedBack(pinfo);
      char command2[256];
      if (-1==sscanf(pinfo,"%s",command2)) continue;
      string s_command2=command2;
      if (s_command2=="Over")
      {
        readingListFeedBack=false;
        tInfo->m_ListStatusSocket.m_se=0;
        sem_post(&tInfo->m_ListStatusSocket.m_finish_sem);        
      }      
    }
   
 }
}

struct HostStuff
{
	pid_t listenerID;
	FILE* fout;
	FILE* fin;
	CommandFeedBackThreadInfo cfbInfo;
	pthread_mutex_t cmd_lock;
};


void IncomingSession(HostStuff* phoststuff, char* buffer, DupFeedBackSession fse)
{
	pthread_mutex_lock(&phoststuff->cmd_lock);  
	
	//printf("cmd:%s\n", buffer);
	
	int len=strlen(buffer);
	if (len<=0) return;
  char *pos;
  if ((pos=strchr(buffer, '\n')) != NULL)
    *pos = '\0';
    
  char command[256];    
  if (-1==sscanf(buffer,"%s",command)) return;	
	
		string s_command=(const char*)command;
	
	  if (s_command=="Shutdown" || s_command=="Reboot")
	  {
	    SendCommand("Quit",phoststuff->fout);
	    if (s_command=="Shutdown")
	    {
	      PlaySound("shutdown.mp3");
	      system("sudo halt -p");
	    }
	    else if (s_command=="Reboot")
	    {
	      PlaySound("reboot.mp3");
	      system("sudo reboot");
	    }
	    exit(0);
	  }
	 	else if (s_command=="ResetCDWatch")
    {
      if (phoststuff->cfbInfo.m_CDStatusSocket.m_se.IsValid())
      {
        fse->FeedBack("playing\n");
        phoststuff->cfbInfo.m_CDStatusSocket.m_resetSocket=true;
	  		SendCommand("CurTrack",phoststuff->fout);
        sem_wait(&phoststuff->cfbInfo.m_CDStatusSocket.m_finish_sem);
        phoststuff->cfbInfo.m_CDStatusSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_CDStatusSocket.m_start_sem);
      }
      else
      {
        fse->FeedBack("notplaying\n");
      }
    }
    else if (s_command=="ResetListWatch")
    {
      if (phoststuff->cfbInfo.m_ListStatusSocket.m_se.IsValid())
      {
        fse->FeedBack("playing\n");
        phoststuff->cfbInfo.m_ListStatusSocket.m_resetSocket=true;
	  		SendCommand("CurSong",phoststuff->fout);
        sem_wait(&phoststuff->cfbInfo.m_ListStatusSocket.m_finish_sem);
        phoststuff->cfbInfo.m_ListStatusSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_ListStatusSocket.m_start_sem);
      }
      else
      {
        fse->FeedBack("notplaying\n");
      }
    }      
    else if (s_command=="test")
		{
			//((FeedBackSession*)fse)->FeedBack("Hello World!");
			fse->FeedBack("Hello World!");
		}      
		else if (s_command=="UpdatePod")
		{
			system("/home/pi/updatepod.sh");
		}     
	  else
  	{
  		SendCommand(buffer,phoststuff->fout);
      if (s_command=="PlayCD")
      {
        sem_wait(&phoststuff->cfbInfo.m_CDStatusSocket.m_finish_sem);
        phoststuff->cfbInfo.m_CDStatusSocket.m_resetSocket=false;
        phoststuff->cfbInfo.m_CDStatusSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_CDStatusSocket.m_start_sem);
      }
      else if (s_command=="ListCD")
      {
        phoststuff->cfbInfo.m_CDListSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_CDListSocket.m_start_sem);
        sem_wait(&phoststuff->cfbInfo.m_CDListSocket.m_finish_sem);
      }
			else if (s_command=="PlayList")
      {
        sem_wait(&phoststuff->cfbInfo.m_ListStatusSocket.m_finish_sem);
        phoststuff->cfbInfo.m_ListStatusSocket.m_resetSocket=false;
        phoststuff->cfbInfo.m_ListStatusSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_ListStatusSocket.m_start_sem);
      }
			else if (s_command=="ListLists")
      {
        phoststuff->cfbInfo.m_ListListsSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_ListListsSocket.m_start_sem);
        sem_wait(&phoststuff->cfbInfo.m_ListListsSocket.m_finish_sem);
      } 
    	else if (s_command=="ListSongs")
      {
        phoststuff->cfbInfo.m_ListSongsSocket.m_se=fse;
        sem_post(&phoststuff->cfbInfo.m_ListSongsSocket.m_start_sem);
        sem_wait(&phoststuff->cfbInfo.m_ListSongsSocket.m_finish_sem);
      } 
  	}
			
	pthread_mutex_unlock(&phoststuff->cmd_lock);  
}


#include <queue>

class WebSocketFeedBackSession : public FeedBackSession
{
	struct libwebsocket_context *m_context;
	struct libwebsocket *m_wsi;
	queue<string> *m_queue;
	bool *m_valid;
	int* m_DuplicateCount;
	
	WebSocketFeedBackSession(struct libwebsocket_context *context, struct libwebsocket *wsi, queue<string> *queue, bool* valid, int* DuplicateCount) 
	 : m_context(context), m_wsi(wsi), m_queue(queue),m_valid(valid), m_DuplicateCount(DuplicateCount)
	 	{
	 		*m_DuplicateCount++;	 		
	 	}
	
	
public:
	WebSocketFeedBackSession(struct libwebsocket_context *context, struct libwebsocket *wsi, queue<string> *queue) 
	 : m_context(context), m_wsi(wsi), m_queue(queue)
	 {
	 		m_valid=new bool;
	 		*m_valid=true;
	 		m_DuplicateCount=new int;
	 		*m_DuplicateCount=1;
	 }
	 	
	~WebSocketFeedBackSession()
	{
		*m_DuplicateCount--;
		if (*m_DuplicateCount==0)
		{
			delete m_valid;
			delete m_DuplicateCount;
		}
	}
	void close()
	{
		*m_valid=false;
	}
	virtual void FeedBack(const char* text)
	{
		if (*m_valid)
		{
			m_queue->push(text);
			libwebsocket_callback_on_writable (m_context, m_wsi);	
		}
	}
	virtual FeedBackSession* Duplicate() const 
	{
		return new WebSocketFeedBackSession(m_context, m_wsi, m_queue, m_valid, m_DuplicateCount);
	}
};

struct PlayBackData
{
	queue<string> * p_queue;
	WebSocketFeedBackSession *p_se;
};

static int
callback_Play(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	HostStuff* phoststuff=(HostStuff*)libwebsocket_context_user(context);
	
	switch (reason) 
	{
		case LWS_CALLBACK_ESTABLISHED:
			{
				((PlayBackData*)user)->p_queue=0;
				break;
			}
		case LWS_CALLBACK_CLOSED:
			{
				queue<string> * _queue = ((PlayBackData*)user)->p_queue;
				delete _queue;
				((PlayBackData*)user)->p_queue=0;
				WebSocketFeedBackSession* se=((PlayBackData*)user)->p_se;
				se->close();
				delete se;
				break;
			}
		case LWS_CALLBACK_RECEIVE:
			{
				queue<string> * _queue = ((PlayBackData*)user)->p_queue;
				delete _queue;
				
				_queue = ((PlayBackData*)user)->p_queue=new queue<string>;
				WebSocketFeedBackSession* se=((PlayBackData*)user)->p_se=new WebSocketFeedBackSession(context, wsi, _queue);
				IncomingSession(phoststuff, (char*)in, se);
				break;
			}
		case LWS_CALLBACK_SERVER_WRITEABLE:
			{
				queue<string> * _queue = *((queue<string> **)user);
				
				unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 1024 +
								  LWS_SEND_BUFFER_POST_PADDING];
				unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];						
				if (!_queue->empty() && lws_partial_buffered (wsi)!=1) 
				{
					string s=_queue->front();
					_queue->pop();
					int n = sprintf((char *)p, s.data());
					libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);			
				}
				if (!_queue->empty()) libwebsocket_callback_on_writable (context, wsi);
				break;
			}
	}
	return 0;
}

// list of supported protocols and callbacks
static struct libwebsocket_protocols protocols[] = 
{
    // first protocol must always be HTTP handler
    {
	"http-only",		/* name */
	callback_http,		/* callback */
	0,	/* per_session_data_size */
	0,			/* max frame size / rx buffer */
    },
    {
		"Play-protocol",
		callback_Play,
		sizeof(PlayBackData),
		0,
	},
    {
        NULL, NULL, 0, 0   // end of list
    }
};

void* HostProcess_WebSocketListener(void* stuff)
{
	HostStuff* phoststuff=(HostStuff*)stuff;
	struct libwebsocket_context *context;
  int opts = 0;
  
  struct lws_context_creation_info info;

	memset(&info, 0, sizeof info);
	info.port = WEB_PORT;
	info.iface = 0;
	info.protocols = protocols;
	info.extensions = libwebsocket_get_internal_extensions();
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.gid = -1;
	info.uid = -1;
	info.options = opts;
	info.user=phoststuff;

	context = libwebsocket_create_context(&info);

    
    if (context == NULL) 
    {
        fprintf(stderr, "libwebsocket init failed\n");
        return 0;
    }
    
    printf("starting server...\n");
    
    // infinite loop, to end this server send SIGTERM. (CTRL+C)
    while (1) 
    {
        libwebsocket_service(context, 50);
        // libwebsocket_service will process all waiting events with their
        // callback functions and then wait 50 ms.
        // (this is a single threaded webserver and this will keep our server
        // from generating load while there are not requests to process)
    }
    
    libwebsocket_context_destroy(context);
    return 0;
	
}

class FeiSocketFeedBackSession : public FeedBackSession
{
	FeiSocketSession m_se;
public:
	FeiSocketFeedBackSession(FeiSocketSession se) : m_se(se) {}
	virtual void FeedBack(const char* text)
	{
		int len=strlen(text);
		m_se.Send(text,len);
	}
	virtual FeedBackSession* Duplicate() const 
	{
		return new FeiSocketFeedBackSession(m_se);
	}
};

int HostProcess(pid_t listenerID, FILE* fout, FILE* fin)
{
	HostStuff hoststuff;
	hoststuff.listenerID=listenerID;
	hoststuff.fout=fout;
	hoststuff.fin=fin;
	
  hoststuff.cfbInfo.m_fin=fin;
  pthread_t CommandFeedBackThread;
  int threadError = pthread_create(&CommandFeedBackThread,NULL,HostProcess_CommandFeedBackThread,&hoststuff.cfbInfo);

  pthread_mutex_init(&hoststuff.cmd_lock,NULL);  
  
  sem_init(&hoststuff.cfbInfo.m_CDStatusSocket.m_finish_sem,0,1);  
  sem_init(&hoststuff.cfbInfo.m_CDStatusSocket.m_start_sem,0,0);  

  sem_init(&hoststuff.cfbInfo.m_CDListSocket.m_finish_sem,0,0);  
  sem_init(&hoststuff.cfbInfo.m_CDListSocket.m_start_sem,0,0);  

  sem_init(&hoststuff.cfbInfo.m_ListStatusSocket.m_finish_sem,0,1);  
  sem_init(&hoststuff.cfbInfo.m_ListStatusSocket.m_start_sem,0,0); 

  sem_init(&hoststuff.cfbInfo.m_ListListsSocket.m_finish_sem,0,0);  
  sem_init(&hoststuff.cfbInfo.m_ListListsSocket.m_start_sem,0,0);  

  sem_init(&hoststuff.cfbInfo.m_ListSongsSocket.m_finish_sem,0,0);  
  sem_init(&hoststuff.cfbInfo.m_ListSongsSocket.m_start_sem,0,0); 
  
  pthread_t WebSocketListenerThread;
  
  threadError = pthread_create(&WebSocketListenerThread,NULL,HostProcess_WebSocketListener,&hoststuff);  
  
  FeiSocketSever server(SOCKET_PORT);
  if (server.IsValid())
  {	
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
	      
	      FeiSocketFeedBackSession fse(se);
				IncomingSession(&hoststuff, buffer, &fse);
				
	    }
	  }
	}
  
  void * ret;
	pthread_join(WebSocketListenerThread,&ret);
	
    
  pthread_mutex_destroy(&hoststuff.cmd_lock);  
  sem_destroy(&hoststuff.cfbInfo.m_CDStatusSocket.m_finish_sem);  
  sem_destroy(&hoststuff.cfbInfo.m_CDStatusSocket.m_start_sem);  

  sem_destroy(&hoststuff.cfbInfo.m_CDListSocket.m_finish_sem);  
  sem_destroy(&hoststuff.cfbInfo.m_CDListSocket.m_start_sem);  

  sem_destroy(&hoststuff.cfbInfo.m_ListStatusSocket.m_finish_sem);  
  sem_destroy(&hoststuff.cfbInfo.m_ListStatusSocket.m_start_sem); 

  sem_destroy(&hoststuff.cfbInfo.m_ListListsSocket.m_finish_sem);  
  sem_destroy(&hoststuff.cfbInfo.m_ListListsSocket.m_start_sem);  

  sem_destroy(&hoststuff.cfbInfo.m_ListSongsSocket.m_finish_sem);  
  sem_destroy(&hoststuff.cfbInfo.m_ListSongsSocket.m_start_sem);   
  
  return 0;
}

#include "GetIP.h"

void PlayIP(const unsigned char ip[4])
{
  FILE *fout=0;
  FILE *fin=0;
  pid_t pid;
  pid=LaunchChild(fout,fin); 
  if (pid==0)
  {
    char strings[4][4];
	sprintf(strings[0],"%d",ip[0]);
	sprintf(strings[1],"%d",ip[1]);
	sprintf(strings[2],"%d",ip[2]);
	sprintf(strings[3],"%d",ip[3]);	
    execlp("/home/pi/omxbyteplayer","omxbyteplayer", strings[0],strings[1],strings[2],strings[3],0);
  }
  else
  {
    waitpid(pid,0,0);
  }  
}

int main()
{
	unsigned char ip[4];
  /*if (GetIP(ip))
	  PlayIP(ip);*/
	  
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
