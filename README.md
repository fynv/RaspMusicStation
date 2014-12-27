RaspMusicStation 树梅派音乐站
================

Tools for setting-up Raspberry Pi as a music server which can be controlled from an Android device through Wifi

本项目旨在开发一组工具，用来将树梅派设置为一个可以从安卓设备通过Wifi进行远程遥控的音乐服务器。

* Hardware Assumptions 硬件

   - Raspberry Pi 树梅派
   - Loudspeaker 音箱
   - USB CD-ROM (Optional) USB 光驱 (可选）
   - No Monitor! 不连接显示器！

* Server OS; Raspbian

* Expected Functions 预期功能

   From the client Android application,commands are send to the server program running on Raspberry Pi. The latter will play digital music from the following sources:
   
   由安卓上运行的客户端程序向树梅派上运行的服务器程序发出指令，使后者播放以下几种来源的数字音乐：

   - Web music 网上音乐: eg. mp3 links as URLs 例如 URLs形式的mp3链接
   - Storage music 储存的音乐: eg. mp3 files stored on the SD card of Raspberry Pi 例如储存在树梅派SD卡上的mp3文件
   - CD Audio CD音乐: from the CD-ROM 由光驱播放

* Existing tools and tools to build 现有工具和要搭建的工具

  Existing: 现有的

  - omxplayer: hardware accelerated command-line music/video player, can play web music and storage music. 
               一个支持硬件加速的命令行音乐/视频播放器，可以播放网上音乐和储存的音乐

  - XBMC: a huge multi-media system
          一个庞大的多媒体系统

  With omxplayer, we can already play all kinds of music except CD Audio. 用omxplayer，我们已经可以播放除了音乐CD之外的各种音乐。
  As one of its many functions, XBMC supports CD Audio playback. 作为其众多功能之一，XBMC支持音乐CD的播放。

  To build: 要搭建的


  -  A server program 服务器程序

  The server program runs in Raspbian. 
  服务器程序运行于Raspbian操作系统。

  It recieves commands from client through TCP socket. 
  它通过TCP socket接收客户端程序发来的指令。

  It calls the players to play music.
  它调用播放器播放音乐

  It sends back play-back status to the client.
  它向客户端发回播放状态信息


  -  A client program 客户端程序
  
  The client program runs in Android.
  客户端程序运行于安卓系统。

  User select the music resource to play from the UI of the client program.
  用户通过程序界面选择要播放的音乐资源
  
  It sends commands to server through TCP socket. 
  它通过TCP socket 向服务器发送指令


  - An additional command-line music player for CD audio 额外写一个命令行的CD音乐播放器

  Pick up some code from XBMC to build it. 
  从XBMC中借用一些代码来搭建这个播放器


* Status of the initial upload 初步上传的代码的状态

  - There is a initial server program OMXHost. 
  初步的服务器程序OMXHost. 

  - There is a initial client program OMXRemoteController
  初步的客户端程序OMXRemoteController/

  The system can only play-back web music for now.
  目前系统只能播放网上音乐





  




