#include <string>
#include "hackConsole.h"

using namespace std;

int PlayDir(const char* path, int listID, int startID, int listMode, FILE* fin=stdin, FILE* fout=stdout);

int main(int argc,char *argv[ ])
{
  disable_terminal_return();

  const char* path="/home/pi/Music";
  int listID=0;
  int startID=0;
  int i;
  int listMode=0;
  for (i=1;i<argc;i++)
  {
    string arg=argv[i];
	if (arg=="-l")
    {
      i++; if (i>=argc) return -1;
      listID=atoi(argv[i]);
    }
    else if (arg=="-t")
    {
      i++; if (i>=argc) return -1;
      startID=atoi(argv[i]);
    }
	else if (arg=="-path")
    {
      i++; if (i>=argc) return -1;
      path=argv[i];
    }
    else if (arg=="-ll")
    {
      listMode=1;
    }
	else if (arg=="-ls")
	{
		listMode=2;
	}
    else return -1;    
  }
  
  return PlayDir(path, listID, startID, listMode);
}


