#ifndef _FeiSocket_h
#define _FeISocket_h

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

class FeiSocketSession
{
  int _sock;
public:
  FeiSocketSession(int sock) : _sock(sock) {}
  bool IsValid() { return _sock!=-1; }
  bool Send(const void* data, int size);
  int Recieve(void* data, int maxSize);
  void Close();

};

class FeiSocketSever
{
  int _port;
  sockaddr_in _addr;
  int _sock;
public:
  FeiSocketSever(unsigned port);
  ~FeiSocketSever();
  bool IsValid() { return _sock!=-1; }
  FeiSocketSession GetSession();
};

FeiSocketSession ClientConnect(const char* host, unsigned port);


#endif  
