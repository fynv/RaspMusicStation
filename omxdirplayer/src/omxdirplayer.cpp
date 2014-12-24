#include "common.h"
#include <dirent.h>
#include <vector>
#include <algorithm>

using namespace std;

void ListDir(const char* path, FeedBackStuff fbstuff)
{
	vector<string> listLists;

		char listpath[1024];
		sprintf(listpath,"%s/list",path);
		FILE *fp=fopen(listpath,"r");
		if (fp)
		{
			char line[1024];
			while(fgets(line,1024,fp))
			{
				char *pos;
				if ((pos=strchr(line, '\n')) != NULL)
				  *pos = '\0';
				string name=line;
				if (name=="#End") break;
				listLists.push_back(name);
			}

			fclose(fp);
		}

		int listSize=listLists.size();

		bool* found=new bool[listSize];
		memset(found,0,sizeof(bool)*listSize);

		DIR *dp;
        struct dirent *dirp;
		dp=opendir(path);

		if (dp)
		{
			while((dirp=readdir(dp))!=NULL)
			{
				if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0))
						continue;

				if (dirp->d_type==DT_DIR)
				{
					string name=dirp->d_name;
					vector<string>::iterator result = find( listLists.begin( ), listLists.end( ), name ); 
					if ( result == listLists.end( ) )
					{
						listLists.push_back(name);
					}
					else
					{
						found[result- listLists.begin( )]=true;
					}
				}
			}

			int i;
			for (i=listSize-1;i>=0;i--)
			{
				if (!found[i])
				{
					vector<string>::iterator it = listLists.begin()+i;
					listLists.erase(it);
				}
			}

			listSize=listLists.size();
			char feedback[1024];
			fp=fopen(listpath,"w");
			for (i=0;i<listSize;i++)
			{
				sprintf(feedback,"ListLists %s", listLists[i].data());
				SendFeedback(feedback,fbstuff);    
				fprintf(fp,"%s\n",listLists[i].data());
			}
			SendFeedback("ListLists #End",fbstuff);
			fprintf(fp,"#End\n");
			fclose(fp);

		}

		delete[] found;
}

void ListSounds(const char* path, int listID, FeedBackStuff fbstuff)
{
	vector<string> listLists;

		char listpath[1024];
		sprintf(listpath,"%s/list",path);
		FILE *fp=fopen(listpath,"r");
		if (fp)
		{
			char line[1024];
			while(fgets(line,1024,fp))
			{
				char *pos;
				if ((pos=strchr(line, '\n')) != NULL)
				  *pos = '\0';
				string name=line;
				if (name=="#End") break;
				listLists.push_back(name);
			}

			fclose(fp);
		}

	if (listID<0 || listID>=listLists.size()) return;

	char songpath[1024];
	sprintf(songpath,"%s/%s",path,listLists[listID].data());

	vector<string> listSongs;

		char songlistpath[1024];
		sprintf(songlistpath,"%s/list",songpath);
		fp=fopen(songlistpath,"r");
		if (fp)
		{
			char line[1024];
			while(fgets(line,1024,fp))
			{
				char *pos;
				if ((pos=strchr(line, '\n')) != NULL)
				  *pos = '\0';
				string name=line;
				if (name=="#End") break;
				listSongs.push_back(name);
			}

			fclose(fp);
		}

		int listSize=listSongs.size();

		bool* found=new bool[listSize];
		memset(found,0,sizeof(bool)*listSize);

		DIR *dp;
        struct dirent *dirp;
		dp=opendir(songpath);

		if (dp)
		{
			while((dirp=readdir(dp))!=NULL)
			{
				if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0) || (strcmp(dirp->d_name,"list")==0))
						continue;

				if (dirp->d_type!=DT_DIR)
				{
					string name=dirp->d_name;
					vector<string>::iterator result = find( listSongs.begin( ), listSongs.end( ), name ); 
					if ( result == listSongs.end( ) )
					{
						listSongs.push_back(name);
					}
					else
					{
						found[result- listSongs.begin( )]=true;
					}
				}
			}

			int i;
			for (i=listSize-1;i>=0;i--)
			{
				if (!found[i])
				{
					vector<string>::iterator it = listSongs.begin()+i;
					listSongs.erase(it);
				}
			}

			listSize=listSongs.size();
			char feedback[1024];
			fp=fopen(songlistpath,"w");
			for (i=0;i<listSize;i++)
			{
				sprintf(feedback,"ListSongs %s", listSongs[i].data());
				SendFeedback(feedback,fbstuff);    
				fprintf(fp,"%s\n",listSongs[i].data());
			}
			SendFeedback("ListSongs #End",fbstuff);
			fprintf(fp,"#End\n");
			fclose(fp);
		}

		delete[] found;
}

struct PlaybackParam
{
	string songpath;
    vector<string> listSongs;
	int m_listID;
	int m_songID;
    bool resetID;
	bool m_IsPlaying;
	FeedBackStuff fbstuff;

  FILE *fout;
  FILE *fin;
  pid_t pid;
};

void* PlaybackThread(void* info)
{
	PlaybackParam* param=(PlaybackParam*)info;
	int n=param->listSongs.size();
	for (;param->m_songID<n;param->m_songID++)
	{
		if (!param->m_IsPlaying) break;
		do
		{
		   if (param->m_songID>=n) param->m_songID=n-1;
		   if (param->m_songID<0) param->m_songID=0;
		   param->resetID=false;
		   param->m_IsPlaying=true;
		   char feedback[1024];
		   sprintf(feedback,"song %d %d", param->m_listID, param->m_songID);
		   SendFeedback(feedback,param->fbstuff); 
		   char fullPath[2048];
		   sprintf(fullPath,"%s/%s",param->songpath.data(),param->listSongs[param->m_songID].data());

			  param->pid=LaunchChild(param->fout,param->fin); 
			  if (param->pid==0)
			  {
				execlp("omxplayer","omxplayer", fullPath,0);
			  }
			  else
			  {
				waitpid(param->pid,0,0);
			  }  

		}
		while (param->resetID);    

	}
	exit(0);
}

void PlayList(const char* path, int listID, int startID, FILE* fin, FeedBackStuff fbstuff)
{
	vector<string> listLists;

		char listpath[1024];
		sprintf(listpath,"%s/list",path);
		FILE *fp=fopen(listpath,"r");
		if (fp)
		{
			char line[1024];
			while(fgets(line,1024,fp))
			{
				char *pos;
				if ((pos=strchr(line, '\n')) != NULL)
				  *pos = '\0';
				string name=line;
				if (name=="#End") break;
				listLists.push_back(name);
			}

			fclose(fp);
		}

	if (listID<0 || listID>=listLists.size()) return;

	char songpath[1024];
	sprintf(songpath,"%s/%s",path,listLists[listID].data());

	vector<string> listSongs;

		char songlistpath[1024];
		sprintf(songlistpath,"%s/list",songpath);
		fp=fopen(songlistpath,"r");
		if (fp)
		{
			char line[1024];
			while(fgets(line,1024,fp))
			{
				char *pos;
				if ((pos=strchr(line, '\n')) != NULL)
				  *pos = '\0';
				string name=line;
				if (name=="#End") break;
				listSongs.push_back(name);
			}

			fclose(fp);
		}



	pthread_t PlaybackThreadID;
    PlaybackParam playParam;

	playParam.songpath=songpath;
    playParam.listSongs=listSongs;
	playParam.m_listID=listID;
	playParam.m_songID=startID;
    playParam.resetID=false;
	playParam.m_IsPlaying=true;
	playParam.fbstuff=fbstuff;

	playParam.fout=0;
	playParam.fin=0;

	pthread_create(&PlaybackThreadID,NULL,PlaybackThread,&playParam);

   while (1)
  {
    char cmd=fgetc(fin);
    if (cmd=='q' || cmd=='Q')
    {
      playParam.m_IsPlaying=false;
	  if (playParam.fout)
		  fputc('q',playParam.fout);

      void* ret;
      pthread_join(PlaybackThreadID,&ret); 
      break;
    }
	else if (cmd=='+' || cmd=='=')
    {
      if (playParam.fout)
		  fputc('+',playParam.fout);
    }
    else if (cmd=='-' || cmd=='_')
    {
      if (playParam.fout)
		  fputc('-',playParam.fout);
    }
	else if (cmd=='>' || cmd=='.')
    {
      playParam.resetID=true;
      playParam.m_songID++;
      playParam.m_IsPlaying=false;
	  if (playParam.fout)
		  fputc('q',playParam.fout);
    }
    else if (cmd=='<' || cmd==',')
    {
      playParam.resetID=true;
      playParam.m_songID--;
      playParam.m_IsPlaying=false;
	   if (playParam.fout)
		  fputc('q',playParam.fout);
    }  
	else if (cmd=='t' || cmd=='T')
    {
      if (playParam.m_IsPlaying)
      {
        char buffer[256];
        sprintf(buffer,"song %d %d",listID, playParam.m_songID);
        SendFeedback(buffer, fbstuff);
      }
    }
   }



}

int PlayDir(const char* path, int listID,  int startID, int listMode, FILE* fin, FILE* fout)
{
  pthread_mutex_t feedback_lock;
  pthread_mutex_init(&feedback_lock,NULL); 

  FeedBackStuff fbstuff;
  fbstuff.m_fout=fout;
  fbstuff.m_lock=feedback_lock;

	if (listMode==1)
	{
		ListDir(path,fbstuff);
		return 0;
	}
	else if (listMode==2)
	{
		ListSounds(path,listID,fbstuff);
		return 0;
	}
	else
	{
		PlayList(path,listID,startID,fin,fbstuff);
		return 0;
	}
}

