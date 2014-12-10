#include "FeiSocket.h"
#include "stdio.h"
bool FeiSocketSession::Send(const void* data, int size)
{
  if (!IsValid()) return false;
  return send ( _sock, data, size, MSG_NOSIGNAL ) !=-1;
}

int FeiSocketSession::Recieve(void* data, int maxSize)
{
  if (!IsValid()) return -1;
  return recv ( _sock, data, maxSize, 0 );
}

void FeiSocketSession::Close()
{
  close(_sock);
  _sock=-1;
}

FeiSocketSever::FeiSocketSever(unsigned port)
:_port(port)
{
  _sock=socket ( AF_INET, SOCK_STREAM, 0 );
  if (_sock==-1) return;
  
  int on = 1;
  if ( setsockopt ( _sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
  {
    _sock=-1;
    return;
  }

  _addr.sin_family = AF_INET;
  _addr.sin_addr.s_addr = INADDR_ANY;
  _addr.sin_port = htons ( _port );

  int bind_return = bind ( _sock, (sockaddr *) &_addr, sizeof ( _addr ) );
  if ( bind_return == -1 )
  {
    _sock=-1;
    return;
  }

  int listen_return = listen ( _sock, SOMAXCONN );
  if ( listen_return == -1 )
  {
    _sock=-1;
    return;
  }
}

FeiSocketSever::~FeiSocketSever()
{
  close(_sock);
}

FeiSocketSession FeiSocketSever::GetSession()
{
  if (!IsValid()) 
  {
    FeiSocketSession se(-1);
    return se;
  }
  int addr_length = sizeof ( _addr );
  int acc_sock=accept(_sock, (sockaddr *) &_addr, ( socklen_t * ) &addr_length ); 
  FeiSocketSession se(acc_sock);
  return se;
}

FeiSocketSession ClientConnect(const char* host, unsigned port)
{
  int sock=socket ( AF_INET, SOCK_STREAM, 0 );
  if (sock!=-1)
  {
    int on = 1;
    if ( setsockopt ( sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) != -1 )
    {
      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons ( port );

      int status = inet_pton ( AF_INET, host, &addr.sin_addr );
      if ( errno != EAFNOSUPPORT )
      {
        status = connect ( sock, ( sockaddr * ) &addr, sizeof ( addr ) );
        if (status!=-1)
        {
          FeiSocketSession se(sock);
          return se;
        }
      }
     }
   }
   FeiSocketSession se(-1);
   return se;
}




