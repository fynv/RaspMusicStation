package com.example.raspmusicstationclient;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;

import android.support.v7.app.ActionBarActivity;
import android.text.Editable;
import android.content.SharedPreferences;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.webkit.WebResourceResponse;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.ToggleButton;
import android.widget.ViewSwitcher;

interface IPByteReciever 
{
	public void RecieveByte(int value);
}

class IPDecodeFromAudio implements Runnable
{
	private static final int RECORDER_SAMPLERATE = 44100;
	private static final int RECORDER_CHANNELS = AudioFormat.CHANNEL_IN_MONO;
	private static final int RECORDER_AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT;
	
	private AudioRecord recorder = null;
	
	private int BufferElements2Rec = 4096;
	private int BytesPerElement = 2;
	private Thread recordingThread = null;
	private boolean isRecording = false;
	private short sData[];
	
	private int regB=0;
	private int regC=0;
	
    private IPByteReciever reciever;
    
	public static native int DecodeAudioBuffer(short sAudioBuffer[]);  
	
	IPDecodeFromAudio(IPByteReciever recv)
	{
		reciever=recv;
		sData = new short[BufferElements2Rec];
        
	}
	
	private void Recognized(int value)
	{
		if (value>=0)
		{
			//Log.i("Audio Decode", String.valueOf(value)); 
			
			if (value==regB) regC++;
			else
			{
				regB=value;
				regC=1;
			}
			if (regC==3)
				regC=0;

			if (regC==2)
			{
				//Log.i("Audio Decode", String.valueOf(value)); 		
				reciever.RecieveByte(value);
			}
			
		}
		else
		{
			regC=0;
		}	
	}
	
	public void run() {
    	while (isRecording) {
    		recorder.read(sData, 0, BufferElements2Rec);
    		int value=DecodeAudioBuffer(sData);
    		short value1=(short)value;
    		short value2=(short)(value>>16);
    		
    		Recognized(value1);
    		Recognized(value2);
    		
        }
    	
    }
	
	void StartRecording()
	{
		recorder = new AudioRecord(MediaRecorder.AudioSource.MIC,
                RECORDER_SAMPLERATE, RECORDER_CHANNELS,
                RECORDER_AUDIO_ENCODING, BufferElements2Rec * BytesPerElement);
		
		recorder.startRecording();
        isRecording = true;
        recordingThread = new Thread(this, "AudioRecorder Thread");
        recordingThread.start();
	}
	
	void StopRecording()
	{
		isRecording = false;
        recorder.stop();
        recorder.release();
        recorder = null;
        recordingThread = null;
	}
	
};

public class MainActivity extends ActionBarActivity {
	static {  
        System.loadLibrary("RaspMusicStationClient");
    }  
	private String m_host;
	private String m_home_page;
	
	private Thread m_commandThread;
	private Boolean m_commandThreadRunning=false;
	
	private Thread m_playThread;
	private Boolean m_playThreadRunning=false;
	
	private Boolean m_CDInfoUpdated=false;	
	private int m_CDTrackNumber;
	private int[] m_CDTrackSectors;
	
	private int m_CurrentTrackID=-1;
	private int m_CurrentSector=0;	
	
	private int m_RecievedIPByteCount;
	
	//private IPDecodeFromAudio m_IPDecoder;
	
	private Boolean m_ListListsUpdated=false;
	private ArrayList<String> m_ListLists;
	
	private int m_ListID=-1;
    private ArrayList<String> m_ListSongs;	
    
    private int m_CurPlayingListID=-1;
    private int m_CurrentSongID=-1;
	

	private Socket SendCommandKeep(String cmd)
	{
		Socket clientSocket;
		PrintWriter socketOut;
		
		 try{
				 clientSocket=new Socket(m_host,30001);
				 socketOut = new PrintWriter(clientSocket.getOutputStream());
				 
				socketOut.println(cmd);
				socketOut.flush();				
			 
			 }catch (UnknownHostException e){
				 e.printStackTrace();
				 return null;
			 }catch (IOException e){
				 e.printStackTrace();
				 return null;
			 }
		 return clientSocket;
	}
	private void SendCommand(String cmd) 
	{ 
		try{
			Socket clientSocket=SendCommandKeep(cmd); 
			 clientSocket.close();
		 }catch (NullPointerException e){
			 e.printStackTrace();
		 }catch (UnknownHostException e){
			 e.printStackTrace();
		 }catch (IOException e){
			 e.printStackTrace();
		 }
	}
	
	void RunCommandThread(Runnable runnable)
	{
		if (m_commandThreadRunning)
		{
			m_commandThread.interrupt();
			m_commandThreadRunning=false;
		}
		m_commandThread=new Thread(runnable)
		{
			 public void run()
			 {
				 m_commandThreadRunning=true;
				 super.run();
				 m_commandThreadRunning=false;
			 }
		};
		m_commandThread.start();
	}
	
	void RunPlayThread(Runnable runnable)
	{
		if (m_playThreadRunning)
		{
			m_playThreadRunning=false;			
			RunCommandThread(new Runnable(){
	  			public void run(){
	  				SendCommand("Stop");		
	  			}		  			
	  		});			
			try
			{
				m_playThread.join();
			}
			catch (InterruptedException e)
			{
				
			}
		}
		m_playThread=new Thread(runnable)
		{
			 public void run()
			 {
				 m_playThreadRunning=true;
				 super.run();
				 m_playThreadRunning=false;
			 }
		};
		m_playThread.start();
	}
	
	class PlayURLRunnable implements Runnable
	{
		private String m_url;
		public PlayURLRunnable(String url){
			m_url=url;
		}
		public void run(){
			SendCommand("PlayURL "+m_url);	
		}		  	
	}
	
	private void UpdateTrackList()
	{
		String[] trackList=new String[m_CDTrackNumber];
   	    int i;
   	    for (i=0;i<m_CDTrackNumber;i++)
   	    {
   	    	double seconds=(float)m_CDTrackSectors[i]/75.0;
   	    	double minutes=Math.floor(seconds/60.0);
   	    	seconds-=minutes*60.0;
   	    	seconds=Math.ceil(seconds);
   	    	
   	    	if (i==m_CurrentTrackID) trackList[i]="* ";
   	    	else trackList[i]="  ";
   	    	trackList[i]+="Track-"+String.valueOf(i+1)+"\t\t\t"+String.format("%02d",(int)minutes)+":"+String.format("%02d",(int)seconds);
   	    }
   	    ListView listTracks= (ListView) findViewById(R.id.listTracks);
   	    listTracks.setAdapter(new ArrayAdapter<String>(MainActivity.this,
             android.R.layout.simple_list_item_1, trackList));
   	    listTracks.setSelection(m_CurrentTrackID);
		
	}
	
	private Handler RefreshCDListHandler = new Handler(new Handler.Callback() {
		public boolean handleMessage(Message msg) {
		         switch (msg.what) {
		        case 1:
		        {
		        	UpdateTrackList();
		            break;
		        }
		      default:
		      break;
		         }
		         return true;
		}});
	
	 private void RefreshCDList() 
     {
   	  try{
				Socket clientSocket=SendCommandKeep("ListCD");	
				BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
				String line=socketIn.readLine();
				String[] strs=line.split(" "); 
				m_CDTrackNumber= Integer.parseInt(strs[0]);
				int i;
				m_CDTrackSectors=new int[m_CDTrackNumber];
				for (i=0;i<m_CDTrackNumber;i++)
				{
					m_CDTrackSectors[i]=Integer.parseInt(strs[i+1]);
				}				
				clientSocket.close();			
			 
			 }catch (NullPointerException e){
				 e.printStackTrace();
				 return;
			 }catch (UnknownHostException e){
				 e.printStackTrace();
				 return;
			 }catch (IOException e){
				 e.printStackTrace();
				 return;
			 }   
   	  	Message notifyMsg = RefreshCDListHandler.obtainMessage(1, 0, 0, null) ;
   	    RefreshCDListHandler.sendMessage(notifyMsg) ;
   	    m_CDInfoUpdated=true;
     }
	 
	private void UpdateListLists()
	{
		ListView listLists= (ListView) findViewById(R.id.listLists);
		listLists.setAdapter(new ArrayAdapter<String>(MainActivity.this,
             android.R.layout.simple_list_item_1, m_ListLists));	
	}
	 
	 private Handler RefreshListListsHandler = new Handler(new Handler.Callback() {
			public boolean handleMessage(Message msg) {
			         switch (msg.what) {
			        case 1:
			        {
			        	UpdateListLists();
			            break;
			        }
			      default:
			      break;
			         }
			         return true;
			}});
	 
	 private void RefreshListLists() 
     {
   	  try{
				Socket clientSocket=SendCommandKeep("ListLists");	
				BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
				
				m_ListLists=new ArrayList<String>();
				
				while(true)
				{
					String line=socketIn.readLine();
					if (line.equals("#End")) break;
					m_ListLists.add(line);
				}
				clientSocket.close();			
			 
			 }catch (NullPointerException e){
				 e.printStackTrace();
				 return;
			 }catch (UnknownHostException e){
				 e.printStackTrace();
				 return;
			 }catch (IOException e){
				 e.printStackTrace();
				 return;
			 }   
   	  	Message notifyMsg = RefreshListListsHandler.obtainMessage(1, 0, 0, null) ;
   	    RefreshListListsHandler.sendMessage(notifyMsg) ;
   	    m_ListListsUpdated=true;
     }
	 
	private void UpdateListSongs()
	{
		String[] songList=new String[m_ListSongs.size()];
   	    int i;
   	    for (i=0;i<m_ListSongs.size();i++)
   	    {
   	    	if (m_ListID==m_CurPlayingListID && i==m_CurrentSongID) songList[i]="* ";
   	    	else songList[i]="  ";
   	    	songList[i]+=m_ListSongs.get(i);
   	    }
   	    
   	 ListView listSongs= (ListView) findViewById(R.id.listSongs);
		listSongs.setAdapter(new ArrayAdapter<String>(MainActivity.this,
          android.R.layout.simple_list_item_1, songList));
		if (m_ListID==m_CurPlayingListID)
			listSongs.setSelection(m_CurrentSongID);
		
	}
	 
	 private Handler RefreshListSongsHandler = new Handler(new Handler.Callback() {
			public boolean handleMessage(Message msg) {
			         switch (msg.what) {
			        case 1:
			        {
			        	UpdateListSongs();
			            break;
			        }
			      default:
			      break;
			         }
			         return true;
			}});
	 
	 private void RefreshListSongs(int newID) 
     {
		 if (newID<0 || newID==m_ListID) return ;
		 m_ListID=newID;
   	  try{
				Socket clientSocket=SendCommandKeep("ListSongs "+String.valueOf(m_ListID));	
				BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
				
				m_ListSongs=new ArrayList<String>();
				
				while(true)
				{
					String line=socketIn.readLine();
					if (line.equals("#End")) break;
					m_ListSongs.add(line);
				}
				clientSocket.close();			
			 
			 }catch (NullPointerException e){
				 e.printStackTrace();
				 return;
			 }catch (UnknownHostException e){
				 e.printStackTrace();
				 return;
			 }catch (IOException e){
				 e.printStackTrace();
				 return;
			 }   
   	  	Message notifyMsg = RefreshListSongsHandler.obtainMessage(1, 0, 0, null) ;
   	    RefreshListSongsHandler.sendMessage(notifyMsg) ;
     }
	 
	 private void UpdateCDProgress()
	 {
		 int totalSectors=100;
		 if (m_CurrentTrackID>=0)
			 totalSectors=m_CDTrackSectors[m_CurrentTrackID];
		 ProgressBar progressCD= (ProgressBar) findViewById(R.id.progressCD);
		 progressCD.setMax(totalSectors-1);
		 progressCD.setProgress(m_CurrentSector);
	 }
	 
	 private Handler RefreshCDPlayHandler = new Handler(new Handler.Callback() {
			public boolean handleMessage(Message msg) {
			         switch (msg.what) {
			         case 0: // start watching
			         {
			        	 ImageView iv= (ImageView) findViewById(R.id.cdImage);
			        	 iv.setImageResource(R.drawable.cdrotate);
			        	 RotateAnimation cd_animation;
			             cd_animation=new RotateAnimation(0f,360f,Animation.RELATIVE_TO_SELF, 0.5f,Animation.RELATIVE_TO_SELF,0.5f); 
			             cd_animation.setDuration(1000);
			             cd_animation.setInterpolator(new LinearInterpolator());
			             cd_animation.setRepeatCount(-1);
			             
			             iv.startAnimation(cd_animation);     
			             break;
			         }			        	 
			        case 1: // track info
			        {
			        	UpdateTrackList();			        	
			        	break;
			        }
			        case 2: // sector info
			        {
			        	UpdateCDProgress();
			        	break;
			        }
			        case 3: // Over
			        {
			        	m_CurrentTrackID=-1;
			        	m_CurrentSector=0;
			        	UpdateTrackList();	
			        	UpdateCDProgress();
			        	
			        	ImageView iv= (ImageView) findViewById(R.id.cdImage);
			        	iv.clearAnimation();
			        	iv.setImageResource(R.drawable.cdstop);
			        	break;
			        }
			      default:
			      break;
			         }
			         return true;
			}});
	 
	 private void WatchCDFeedBack(Socket clientSocket)
	 {
		 try{
			 {
				 Message notifyMsg = RefreshCDPlayHandler.obtainMessage(0, 0, 0, null) ;
				 RefreshCDPlayHandler.sendMessage(notifyMsg) ;
			}
			 BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
			  	while(true)
				{
					String line=socketIn.readLine();
					String[] strs=line.split(" "); 
					if (strs.length<1) continue;
					if (!m_playThreadRunning) break;
					
					if (strs[0].equals("Over")) break;
					else if(strs[0].equals("track"))
					{
						if (strs.length<2) continue;
						//Log.i("CDPlayBack Track",line);
						m_CurrentTrackID=Integer.valueOf(strs[1]);
						{
							Message notifyMsg = RefreshCDPlayHandler.obtainMessage(1, 0, 0, null) ;
							RefreshCDPlayHandler.sendMessage(notifyMsg) ;
						}
					}
					else if (strs[0].equals("sector"))
					{
						if (strs.length<2) continue;
						//Log.i("CDPlayBack Sector",line);
						m_CurrentSector=Integer.valueOf(strs[1]);
						{
							Message notifyMsg = RefreshCDPlayHandler.obtainMessage(2, 0, 0, null) ;
							RefreshCDPlayHandler.sendMessage(notifyMsg) ;
						}
					}
				}
				clientSocket.close();
		 }catch (NullPointerException e){
			 e.printStackTrace();
			 return;
		 }catch (UnknownHostException e){
			 e.printStackTrace();
			 return;
		 }catch (IOException e){
			 e.printStackTrace();
			 return;
		 }   
		 {
			  Message notifyMsg = RefreshCDPlayHandler.obtainMessage(3, 0, 0, null) ;
			  RefreshCDPlayHandler.sendMessage(notifyMsg) ;
		 }
	 }
	 
	 class PlayCDRunnable implements Runnable
	 {
		 private int m_trackID;
		 public PlayCDRunnable(int trackID) {
			 m_trackID=trackID;
		 }
		 public void run(){
			  try{
				  Socket clientSocket=SendCommandKeep("PlayCD "+String.valueOf(m_trackID));	
			      WatchCDFeedBack(clientSocket);
				 
				 }catch (NullPointerException e){
					 e.printStackTrace();
					 return;
				 }
			  
		 }		
	 }
	 
	 
	 private void PlayCD(int trackID)
	 {
		 if (!m_CDInfoUpdated) {
        	RunCommandThread(new Runnable(){
	  			public void run(){
	  				RefreshCDList();
	  			}		  			
	  		});	
        }
		 
		 RunPlayThread(new PlayCDRunnable(trackID));		
	 }
	 
	 private void ResetCDWatch()
	 {
		 if (!m_playThreadRunning)
		 {
			 RunPlayThread( new Runnable(){
		  			public void run(){
		  				 try{
		  					  Socket clientSocket=SendCommandKeep("ResetCDWatch");	
		  					  BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
						      String line=socketIn.readLine();
						      if (line.equals("playing"))
						    	  WatchCDFeedBack(clientSocket);
						      else 
						    	  clientSocket.close();
		  					 
		  					 }catch (NullPointerException e){
		  						 e.printStackTrace();
		  						 return;
		  					 }catch (UnknownHostException e){
		  						 e.printStackTrace();
		  						 return;
		  					 }catch (IOException e){
		  						 e.printStackTrace();
		  						 return;
		  					 }   
		  			}	
			 });	
		 }
	 }
	 
	 private Handler RefreshListPlayHandler = new Handler(new Handler.Callback() {
			public boolean handleMessage(Message msg) {
			         switch (msg.what) {
			         case 0: // start watching
			         {
			        	
			             break;
			         }			        	 
			        case 1: // track info
			        {
			        	UpdateListSongs();			        	
			        	break;
			        }
			        case 2: // sector info
			        {
			        	break;
			        }
			        case 3: // Over
			        {
			        	m_CurPlayingListID=-1;
			        	m_CurrentSongID=-1;
			        	UpdateListSongs();	
			        	break;
			        }
			      default:
			      break;
			         }
			         return true;
			}});
	 
	 private void WatchListFeedBack(Socket clientSocket)
	 {
		 try{
			 {
				 Message notifyMsg = RefreshListPlayHandler.obtainMessage(0, 0, 0, null) ;
				 RefreshListPlayHandler.sendMessage(notifyMsg) ;
			}
			 BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
			  	while(true)
				{
					String line=socketIn.readLine();
					String[] strs=line.split(" "); 
					if (strs.length<1) continue;
					if (!m_playThreadRunning) break;
					
					if (strs[0].equals("Over")) break;
					else if(strs[0].equals("song"))
					{
						if (strs.length<3) continue;
						m_CurPlayingListID=Integer.valueOf(strs[1]);
						m_CurrentSongID=Integer.valueOf(strs[2]);
						{
							Message notifyMsg = RefreshListPlayHandler.obtainMessage(1, 0, 0, null) ;
							RefreshListPlayHandler.sendMessage(notifyMsg) ;
						}
					}
				}
				clientSocket.close();
		 }catch (NullPointerException e){
			 e.printStackTrace();
			 return;
		 }catch (UnknownHostException e){
			 e.printStackTrace();
			 return;
		 }catch (IOException e){
			 e.printStackTrace();
			 return;
		 }   
		 {
			  Message notifyMsg = RefreshListPlayHandler.obtainMessage(3, 0, 0, null) ;
			  RefreshListPlayHandler.sendMessage(notifyMsg) ;
		 }
	 }
	 
	 class PlayListRunnable implements Runnable
	 {
		 private int m_songID;
		 public PlayListRunnable(int songID) {
			 m_songID=songID;
		 }
		 public void run(){
			  try{
				  Socket clientSocket=SendCommandKeep("PlayList "+ String.valueOf(m_ListID)+" "+String.valueOf(m_songID));	
			      WatchListFeedBack(clientSocket);
				 
				 }catch (NullPointerException e){
					 e.printStackTrace();
					 return;
				 }
			  
		 }		
	 }
	 
	 
	 private void PlayList(int songID)
	 {
		RunPlayThread(new PlayListRunnable(songID));		
	 }
	 
	 private void ResetListWatch()
	 {
		 if (!m_playThreadRunning)
		 {
			 RunPlayThread( new Runnable(){
		  			public void run(){
		  				 try{
		  					  Socket clientSocket=SendCommandKeep("ResetListWatch");	
		  					  BufferedReader socketIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
						      String line=socketIn.readLine();
						      if (line.equals("playing"))
						    	  WatchListFeedBack(clientSocket);
						      else 
						    	  clientSocket.close();
		  					 
		  					 }catch (NullPointerException e){
		  						 e.printStackTrace();
		  						 return;
		  					 }catch (UnknownHostException e){
		  						 e.printStackTrace();
		  						 return;
		  					 }catch (IOException e){
		  						 e.printStackTrace();
		  						 return;
		  					 }   
		  			}	
			 });	
		 }
	 }
	 
	/*  private Handler RecieveIPByteHandler = new Handler(new Handler.Callback() {
			public boolean handleMessage(Message msg) {
			         switch (msg.what) {
			        case 1:
			        {
			        	if (m_RecievedIPByteCount>=4) break;
			        	int value=msg.arg1;
			        	 EditText editHost = (EditText) findViewById(R.id.editTextHost);  	
			        	 Editable curString=editHost.getText();
			    		 if (m_RecievedIPByteCount>0)
			    			 curString.append(".");
			    		 curString.append(String.valueOf(value));			        	
			        	 m_RecievedIPByteCount++;
			        	 if (m_RecievedIPByteCount==4)
			        	 {
			        		 ToggleButton btnListenIP=(ToggleButton)findViewById(R.id.btnListenIP);   
			        		 btnListenIP.toggle();
			        	 }
			            break;
			        }
			      default:
			      break;
			         }
			         return true;
			}});*/
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		SharedPreferences settings = getSharedPreferences("RaspMusicStationClient", 0);  
		m_host = settings.getString("Host", "192.168.10.3");
		m_home_page = settings.getString("Home_page", "http://music.qq.com");
		
		EditText editHost = (EditText) findViewById(R.id.editTextHost);  
		editHost.setText(m_host);
		
		EditText editHomePage = (EditText) findViewById(R.id.editTextHomePage);  
		editHomePage.setText(m_home_page);
		
		
		TabHost tabHost = (TabHost) findViewById(R.id.tabhost);  
		tabHost.setup();  
		
	  tabHost.addTab(tabHost.newTabSpec("tab01").setIndicator("",getResources().getDrawable(R.drawable.web))
			   .setContent(R.id.tab01));  
  
      tabHost.addTab(tabHost.newTabSpec("tab02").setIndicator("",getResources().getDrawable(R.drawable.storage))  
                .setContent(R.id.tab02));  
      
      tabHost.addTab(tabHost.newTabSpec("tab03").setIndicator("",getResources().getDrawable(R.drawable.cd)) 
              .setContent(R.id.tab03)); 
      
      tabHost.addTab(tabHost.newTabSpec("tab04").setIndicator("",getResources().getDrawable(R.drawable.settings))   
              .setContent(R.id.tab04)); 
      
      TabWidget tabs=tabHost.getTabWidget(); 
      
      for(int i=0;i < tabs.getChildCount();i++)
      {
       TextView textView = (TextView)tabs.getChildAt(i).findViewById(android.R.id.title);
       textView.setTextSize(10);
       textView.setPadding(0, 0, 0, 0);
      }
      
      tabHost.setOnTabChangedListener(new TabHost.OnTabChangeListener() {  
    	  
          public void onTabChanged(String tabId) {  
            if (tabId=="tab03" && !m_CDInfoUpdated) 
            {
            	RunCommandThread(new Runnable(){
		  			public void run(){
		  				RefreshCDList();
		  			}		  			
		  		});	
            	ResetCDWatch();
            }
            else if (tabId=="tab02" && !m_ListListsUpdated) 
            {
            	RunCommandThread(new Runnable(){
		  			public void run(){
		  				RefreshListLists();
		  			}		  			
		  		});	
            }
          }  
      });  
      
      
      Button btnStop= (Button) findViewById(R.id.btnStop);
	  btnStop.setOnClickListener(new View.OnClickListener (){
		  		 public void onClick(View v){
			  		RunCommandThread(new Runnable(){
			  			public void run(){
			  				SendCommand("Stop");
			  			}		  			
			  		});					  					  
			  }
		  }	  
	  ); 
	  
	  Button btnPrev= (Button) findViewById(R.id.btnPrev);
      btnPrev.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  WebView myWebView = (WebView) findViewById(R.id.webview);
					  myWebView.goBack();					  
				  }
		  }	  
	  ); 
      
      Button btnNext= (Button) findViewById(R.id.btnNext);
      btnNext.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  WebView myWebView = (WebView) findViewById(R.id.webview);
					  myWebView.goForward();			  
				  }
		  }	  
	  ); 
      
      Button btnVolDown= (Button) findViewById(R.id.btnVolDown);
      btnVolDown.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("VolDown");
				  			}		  			
				  		});				  
				  }
		  }	  
	  ); 
      
      Button btnVolUp= (Button) findViewById(R.id.btnVolUp);
      btnVolUp.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("VolUp");	
				  			}		  			
				  		});							  	  
				  }
		  }	  
	  ); 
      
      Button btnReboot= (Button) findViewById(R.id.btnReboot);
      btnReboot.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("Reboot");		
				  			}		  			
				  		});							  	  
				  }
		  }	  
	  ); 
      
      Button btnShutdown= (Button) findViewById(R.id.btnShutdown);
      btnShutdown.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("Shutdown");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnSave= (Button) findViewById(R.id.btnSave);
      btnSave.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					EditText editHost = (EditText) findViewById(R.id.editTextHost);  						
					EditText editHomePage = (EditText) findViewById(R.id.editTextHomePage);  
					  
					  SharedPreferences settings = getSharedPreferences("RaspMusicStationClient", 0);  
						SharedPreferences.Editor editor = settings.edit(); 		
						editor.putString("Host", editHost.getText().toString());
						editor.putString("Home_page", editHomePage.getText().toString());
						editor.commit();    
				  }
		  }	  
	  );      
      
      Button btnPlayCD= (Button) findViewById(R.id.btnPlayCD);
      btnPlayCD.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  PlayCD(0);					  
				  }
		  }	  
	  );  
      
      ListView listTracks= (ListView) findViewById(R.id.listTracks);
      listTracks.setOnItemClickListener(new AdapterView.OnItemClickListener() {
	    	  public void onItemClick(AdapterView<?> parent, View view,
	    	  int position, long id) {
	    		  PlayCD(position);
	    	  }
    	  }
      );
      
      Button btnStopCD= (Button) findViewById(R.id.btnStopCD);
      btnStopCD.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("Stop");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnEject= (Button) findViewById(R.id.btnEject);
      btnEject.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("Eject");
				  				m_CDInfoUpdated=false;
				  				m_CDTrackNumber= 0;
								m_CDTrackSectors=new int[m_CDTrackNumber];
							 	Message notifyMsg = RefreshCDListHandler.obtainMessage(1, 0, 0, null) ;
						   	    RefreshCDListHandler.sendMessage(notifyMsg);
				  			}		  			
				  		});	
				  }
		  }	  
	  ); 
      
      Button btnPrevTrack= (Button) findViewById(R.id.btnPrevTrack);
      btnPrevTrack.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("PrevTrack");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnNextTrack= (Button) findViewById(R.id.btnNextTrack);
      btnNextTrack.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("NextTrack");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnVolDownCD= (Button) findViewById(R.id.btnVolDownCD);
      btnVolDownCD.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("VolDown");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnVolUpCD= (Button) findViewById(R.id.btnVolUpCD);
      btnVolUpCD.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("VolUp");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnRefreshCD= (Button) findViewById(R.id.btnRefreshCD);
      btnRefreshCD.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				RefreshCDList();
				  			}		  			
				  		});	
					  ResetCDWatch();
				  }
		  }	  
	  ); 
      
      
     ///////////
      
      ListView listLists= (ListView) findViewById(R.id.listLists);
      listLists.setOnItemClickListener(new AdapterView.OnItemClickListener() {
	    	  public void onItemClick(AdapterView<?> parent, View view,
	    	  int position, long id) {
	    		      		  
	    		  String listTitleText=m_ListLists.get(position);
	    		  TextView listTitle= (TextView) findViewById(R.id.listTitle);
	    		  listTitle.setText("Songs in list ".concat(listTitleText));
	    		  
	    		  ViewSwitcher switcher= (ViewSwitcher) findViewById(R.id.tab02);
				  switcher.showNext();
				  
				  class RefreshListSongsRunnable implements Runnable
				  {
					  private int m_newID;
					  RefreshListSongsRunnable(int newID)
					  {
						  m_newID=newID;
					  }
						public void run(){
			  				RefreshListSongs(m_newID);
			  			}	
				  }
				  
				  RunCommandThread(new RefreshListSongsRunnable(position));	
				  ResetListWatch();
	    	  }
    	  }
      );
      
      
      
      Button btnReturn2Lists= (Button) findViewById(R.id.btnReturn2Lists);
      btnReturn2Lists.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  
					  ViewSwitcher switcher= (ViewSwitcher) findViewById(R.id.tab02);
					  switcher.showPrevious();
				  }
		  }	  
	  ); 
      
      Button btnPlayList= (Button) findViewById(R.id.btnPlayList);
      btnPlayList.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  PlayList(0);					  
				  }
		  }	  
	  );  
      
      Button btnStopList= (Button) findViewById(R.id.btnStopList);
      btnStopList.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("Stop");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnPrevSong= (Button) findViewById(R.id.btnPrevSong);
      btnPrevSong.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("PrevSong");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnNextSong= (Button) findViewById(R.id.btnNextSong);
      btnNextSong.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("NextSong");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnVolDownList= (Button) findViewById(R.id.btnVolDownList);
      btnVolDownList.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("VolDown");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      Button btnVolUpList= (Button) findViewById(R.id.btnVolUpList);
      btnVolUpList.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  RunCommandThread(new Runnable(){
				  			public void run(){
				  				SendCommand("VolUp");		
				  			}		  			
				  		});							  
				  }
		  }	  
	  ); 
      
      ListView listSongs= (ListView) findViewById(R.id.listSongs);
      listSongs.setOnItemClickListener(new AdapterView.OnItemClickListener() {
	    	  public void onItemClick(AdapterView<?> parent, View view,
	    	  int position, long id) {
	    		  PlayList(position);
	    	  }
    	  }
      );
      
      
     ////////////////
      
      /*m_IPDecoder=new IPDecodeFromAudio(new IPByteReciever(){
    	  public void RecieveByte(int value)
    	  {
    		  Message notifyMsg = RecieveIPByteHandler.obtainMessage(1, value, 0, null) ;
    		  RecieveIPByteHandler.sendMessage(notifyMsg) ;
    	  }
      });*/
      
      /*ToggleButton btnListenIP=(ToggleButton)findViewById(R.id.btnListenIP);
      
      btnListenIP.setOnCheckedChangeListener(new OnCheckedChangeListener(){
		       public void onCheckedChanged(CompoundButton buttonView,boolean isChecked) {   
		    	   if (isChecked) 
		    	   {
		    		   m_RecievedIPByteCount=0;
		    		   EditText editHost = (EditText) findViewById(R.id.editTextHost);  						
		    		   editHost.setText("");
		    		   m_IPDecoder.StartRecording();		    		   
		    	   }
		    	   else m_IPDecoder.StopRecording();	    	   		
		               
		          }                      
       		}
      );   */
      
         
	  
      
      WebView myWebView = (WebView) findViewById(R.id.webview);
      WebSettings webSettings = myWebView.getSettings();
      webSettings.setJavaScriptEnabled(true);
		webSettings.setDomStorageEnabled(true);

      myWebView.setWebViewClient(new WebViewClient() {
			 @Override
			 public boolean shouldOverrideUrlLoading(WebView view, String url) {
				 if (url.contains(".mp3") || url.contains(".m4a")) {
					 RunPlayThread(new PlayURLRunnable(url));
					 return true;
				 }
				 return false;
			 }

			 @Override
			 public WebResourceResponse shouldInterceptRequest(WebView view, String url) {
				 if (url.contains(".mp3") || url.contains(".m4a")) {
					 RunPlayThread(new PlayURLRunnable(url));
					 return new WebResourceResponse("audio", "mp3", null);
				 }
				 return null;
			 }
		 }
      );
      
      myWebView.loadUrl(m_home_page);
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		//getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}
	//@Override
	//public void onConfigurationChanged(Configuration newConfig) {}
	
}