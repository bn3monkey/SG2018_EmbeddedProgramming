package com.example.androidex;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;


public class MainActivity extends Activity{
	
	LinearLayout linear;
	
	Button btn_start;
	TextView text_stuid;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		linear = (LinearLayout)findViewById(R.id.container);
		
		//about Start button
		btn_start=(Button)findViewById(R.id.btn_start);
		//set listener of button start
		OnClickListener listener=new OnClickListener(){
			public void onClick(View v){
				Intent intent=new Intent(MainActivity.this, MainActivity2.class);
				startActivity(intent);
			}
		};
		//link listener to button start
		btn_start.setOnClickListener(listener);
		//set the text to start button
		btn_start.setText("Start game");
		btn_start.setGravity(Gravity.CENTER);
		
		//set the text to text view
		text_stuid = (TextView)findViewById(R.id.text_stuid);
		text_stuid.setText("20121592");
		
		
	}

}
