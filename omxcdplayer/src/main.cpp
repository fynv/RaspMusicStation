#include <string>
#include "hackConsole.h"

using namespace std;

int PlayCD(int startTrack, bool showStatus, bool listMode, FILE* fin=stdin, FILE* fout=stdout);

int main(int argc,char *argv[ ])
{
  disable_terminal_return();
  
  bool showStatus=false;
  int startTrack=0;
  int i;
  bool listMode=false;
  for (i=1;i<argc;i++)
  {
    string arg=argv[i];
    if (arg=="-t")
    {
      i++; if (i>=argc) return -1;
      startTrack=atoi(argv[i]);
    }
    else if (arg=="-s")
    {
      showStatus=true;
    }
    else if (arg=="-l")
    {
      listMode=true;
    }
    else return -1;    
  }
  
  return PlayCD(startTrack,showStatus, listMode);
}


