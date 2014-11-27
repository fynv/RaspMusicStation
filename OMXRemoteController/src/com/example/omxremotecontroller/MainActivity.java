package com.example.omxremotecontroller;

import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;

import android.support.v7.app.ActionBarActivity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ClipData.Item;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.StrictMode;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends ActionBarActivity {

	private String m_host;
	private String m_home_page;
	
	private void SendCommand(String cmd)
	{
		Socket clientSocket;
		PrintWriter socketOut;
		
		 try{
				 clientSocket=new Socket(m_host,30001);
				 socketOut = new PrintWriter(clientSocket.getOutputStream());
				 
				socketOut.println(cmd);
				socketOut.flush();				
				 
				 clientSocket.close();
			 
			 }catch (UnknownHostException e){
				 e.printStackTrace();
				 return;
			 }catch (IOException e){
				 e.printStackTrace();
				 return;
			 }
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		if (android.os.Build.VERSION.SDK_INT > 9) {
		    StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
		    StrictMode.setThreadPolicy(policy);
		}
		
		SharedPreferences settings = getSharedPreferences("OMXRemoteController", 0);  
		m_host = settings.getString("Host", "192.168.10.3");
		m_home_page = settings.getString("Home_page", "http://music.baidu.com");
		
		EditText editHost = (EditText) findViewById(R.id.editTextHost);  
		editHost.setText(m_host);
		
		EditText editHomePage = (EditText) findViewById(R.id.editTextHomePage);  
		editHomePage.setText(m_home_page);
		
		
		TabHost tabHost = (TabHost) findViewById(R.id.tabhost);  
		tabHost.setup();  
		
	  tabHost.addTab(tabHost.newTabSpec("tab01").setIndicator("Web")
			   .setContent(R.id.tab01));  
  
      tabHost.addTab(tabHost.newTabSpec("tab02").setIndicator("Storage")  
                .setContent(R.id.tab02));  
      
      tabHost.addTab(tabHost.newTabSpec("tab03").setIndicator("CD")  
              .setContent(R.id.tab03)); 
      
      tabHost.addTab(tabHost.newTabSpec("tab04").setIndicator("System")  
              .setContent(R.id.tab04)); 
      
      TabWidget tabs=tabHost.getTabWidget(); 
      
      for(int i=0;i < tabs.getChildCount();i++)
      {
       TextView textView = (TextView)tabs.getChildAt(i).findViewById(android.R.id.title);
       textView.setTextSize(10);
       textView.setPadding(0, 0, 0, 0);
      }
      
      Button btnStop= (Button) findViewById(R.id.btnStop);
	  btnStop.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  SendCommand("Stop");
					  					  
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
					  SendCommand("VolDown");		  
				  }
		  }	  
	  ); 
      
      Button btnVolUp= (Button) findViewById(R.id.btnVolUp);
      btnVolUp.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  SendCommand("VolUp");		  
				  }
		  }	  
	  ); 
      
      Button btnReboot= (Button) findViewById(R.id.btnReboot);
      btnReboot.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  SendCommand("Reboot");		  
				  }
		  }	  
	  ); 
      
      Button btnShutdown= (Button) findViewById(R.id.btnShutdown);
      btnShutdown.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					  SendCommand("Shutdown");		  
				  }
		  }	  
	  ); 
      
      Button btnSave= (Button) findViewById(R.id.btnSave);
      btnSave.setOnClickListener(new View.OnClickListener (){
				  public void onClick(View v){
					EditText editHost = (EditText) findViewById(R.id.editTextHost);  						
					EditText editHomePage = (EditText) findViewById(R.id.editTextHomePage);  
					  
					  SharedPreferences settings = getSharedPreferences("OMXRemoteController", 0);  
						SharedPreferences.Editor editor = settings.edit(); 		
						editor.putString("Host", editHost.getText().toString());
						editor.putString("Home_page", editHomePage.getText().toString());
						editor.commit();    
				  }
		  }	  
	  );      
      
     
	  
      
      WebView myWebView = (WebView) findViewById(R.id.webview);
      WebSettings webSettings = myWebView.getSettings();
      webSettings.setJavaScriptEnabled(true);
      
      myWebView.setWebViewClient(new WebViewClient(){       
          public boolean shouldOverrideUrlLoading(WebView view, String url) {       
	              if (url.indexOf(".mp3")>0)
	              {    
	            	  SendCommand("PlayURL "+url);
	                  return true; 
	              }
	              return false;
	          }       
      	}
      );   
      
      myWebView.loadUrl(m_home_page);
      
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
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
}
