#include <string>
#include <stdio.h>
#include "SoundCodec.h"
#include "audioplay.h"

using namespace std;

void PlayBuffer(short *PCMData, int size);

int main(int argc,char *argv[ ])
{
  int byteCount=argc-1;
  SoundCodec sc;
  sc.m_dataBuffer.resize(byteCount);

  int i;
  for (i=0;i<byteCount;i++)
	  sc.m_dataBuffer[i]=(unsigned char)atoi(argv[i+1]);

  sc.Encode();

  /*FILE *fp=fopen("tmp.raw","wb");
  fwrite(&sc.m_PCMBuffer[0],sizeof(short),sc.m_PCMBuffer.size(),fp);
  fclose(fp);*/

  bcm_host_init();

  PlayBuffer(&sc.m_PCMBuffer[0],sc.m_PCMBuffer.size());

  return 0; 
}


