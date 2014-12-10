#ifndef _AudioCD_h
#define _AudioCD_h

class CDPlayStuff
{
public:
  virtual bool IsPlaying() { return true; }
  virtual int GetDB() { return 0;}
  virtual void FeedBack(const char*) {}
  virtual int FeedBackInterval() { return 1;}
};

typedef struct _CdIo CdIo_t;

struct TrackInfo
{
  int trackID;
  bool isAudio;
  int startSector;
  int durationSectors;
};
 
class AudioCD
{
  friend class CDIOBuffer;
  CdIo_t* m_cdio;
  int m_trackFirst;
  int m_trackCount;
  int m_totalSectors;
  TrackInfo *m_info;

  int m_CurPlayStartSector;
  int m_CurPlayDurationSectors;

  void _play(CDPlayStuff* stuff);
  void _fillBuffer(short* buffer, int i);

public:
  AudioCD();
  ~AudioCD();
  
  int GetTrackCount();

  static float SectorsToSeconds(int sectors);

  int GetTotalSectors();
  TrackInfo GetTrackInfo(int i){ return m_info[i]; }

  void Play(int i, CDPlayStuff* stuff=0);

  
};

#endif

