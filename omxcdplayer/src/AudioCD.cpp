#include <stdio.h>
#include <cdio/cdio.h>
#include <string>

#include "AudioCD.h"

AudioCD::AudioCD()
{
  m_cdio=::cdio_open(NULL, DRIVER_UNKNOWN);
  if (!m_cdio) return;
  m_trackFirst = ::cdio_get_first_track_num(m_cdio);

  if (m_trackFirst==CDIO_INVALID_TRACK)
  {
    ::cdio_destroy(m_cdio);
    m_cdio=0;
    return;  
  }

  m_trackCount = ::cdio_get_num_tracks(m_cdio);
  if (m_trackCount == CDIO_INVALID_TRACK)
  {
    ::cdio_destroy(m_cdio);
    m_cdio=0;
    return;   
  }

  m_totalSectors=::cdio_get_track_lba(m_cdio, CDIO_CDROM_LEADOUT_TRACK);
  
  m_info=new TrackInfo[m_trackCount];
  int i;
  for (i = m_trackFirst; i < m_trackFirst+m_trackCount; i++)
  {
    int index= i-m_trackFirst;
    m_info[index].trackID=i;
    if (::cdio_get_track_format(m_cdio, i)==TRACK_FORMAT_AUDIO)
    {
      m_info[index].isAudio=true;
      int temp1 = ::cdio_get_track_lba(m_cdio, i) - CDIO_PREGAP_SECTORS;
      int temp2 = ::cdio_get_track_lba(m_cdio, i + 1) - CDIO_PREGAP_SECTORS;

      temp2 -= temp1;  
      m_info[index].startSector=temp1;
      m_info[index].durationSectors=temp2;
     
    }
    else  m_info[index].isAudio=false;
  }
  
}

AudioCD::~AudioCD()
{
  if (m_cdio)
  {
    delete[] m_info;
    ::cdio_destroy(m_cdio);
  }
}

int AudioCD::GetTrackCount()
{
  if (!m_cdio) return -1;
  return m_trackCount;
}

float AudioCD::SectorsToSeconds(int sectors)
{
  return sectors/(float)CDIO_CD_FRAMES_PER_SEC;
}

int AudioCD::GetTotalSectors()
{
  if (!m_cdio) return -1;
  return m_totalSectors;
}

void AudioCD::_fillBuffer(short* buffer, int i)
{
   cdio_read_audio_sector(m_cdio, buffer, i);  
}


void AudioCD::Play(int i, CDPlayStuff* stuff)
{
  CDPlayStuff defaultStuff;
  if (!stuff) stuff=&defaultStuff;
  TrackInfo info=m_info[i];
  m_CurPlayStartSector=info.startSector;
  m_CurPlayDurationSectors=info.durationSectors;
  _play(stuff);
}







