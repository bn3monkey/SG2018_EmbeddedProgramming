package com.example.androidex;

import java.util.Random;
import java.util.StringTokenizer;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;

import javadriver.JavaDriver;

public class MainActivity2 extends Activity{
	
	LinearLayout linear;
	
	LinearLayout[] line;
	Button[][] btn_matrix;
	
	EditText edit_make;
	Button btn_make;
	String str_make;
	
	int prev_row = 0;
	int prev_col = 0;
	int[][] status = null;
	
	int device_width;
	int device_height;
	
	int fd = -1;
	
	private enum direction
	{
		UP, LEFT, DOWN, RIGHT
	};
	final int black = 0;
	
	boolean end_key = false;
	
	/** PuzzleEnd thread **/
	//For reset the puzzle
	Handler puzzleend_Handler =
			new Handler()
			{
				//1. Message context is reset matrix and deactivate pause toggle
				public void handleMessage(Message msg)
				{
					if(msg.what == 0)
					{
						//1. remove all layout
						int row = msg.arg1;
						int col = msg.arg2;
						remove_matrix(row, col);
					}
				}
			};
	class Puzzleend_Thread extends Thread
	{
		Handler local_handler;
		int row, col;
		Puzzleend_Thread(int _row, int _col, Handler handler)
		{
			local_handler = handler;
			row = _row;
			col = _col;
		}
		public void run()
		{
			//1. wait 2 second
			try
			{
				Thread.sleep(2000);
			}
			catch(InterruptedException ex)
			{
				Thread.currentThread().interrupt();
			}
			//2. remove matrixPuzzleEnd
			//Message msg = Message.obtain();
			//msg.what = 0;
			//msg.arg1 = row;
			//msg.arg2 = col;
			//local_handler.sendMessage(msg);
			end_key = false;
			//2. exit Activity
			finish();
		}
	}
	Puzzleend_Thread pthread;
	
	/** Device thread **/
	//For control Device
	Handler Device_Handler =
			new Handler()
			{
				//1. Message context is reset matrix and deactivate pause toggle
				public void handleMessage(Message msg)
				{
					if(msg.what == 0)
					{
						//1. reset matrix
						int row = msg.arg1;
						int col = msg.arg2;
						reset_matrix(row, col);
					}
				}
			};
	class Device_Thread extends Thread
	{
		Handler local_handler;
		int row, col;
		Device_Thread(int _row, int _col, Handler handler)
		{
			local_handler = handler;
			row = _row;
			col = _col;
		}
		public void run()
		{
			//1. Java Driver Open
			fd = JavaDriver.driver_open();
			//2. Java Driver Write
			JavaDriver.driver_write(fd, 0);
			//3. if write is awaken, Java Driver Close
			JavaDriver.driver_close(fd);
			
			//4. if end_key activates, start Resetting!
			if(end_key)
			{
				Message msg = Message.obtain();
				msg.what = 0;
				msg.arg1 = row;
				msg.arg2 = col;
				local_handler.sendMessage(msg);
			}
		}
	}
	Device_Thread dthread;
	
	
	//make matrix and match the number
	private Point mapping_matrix(int row, int col)
	{
		//1. match each number at normal buttons
		status = new int[row][col];
		int count = 1;
		for(int i=0;i<row;i++)
			for(int j=0;j<col;j++)
			{
				status[i][j] = count;
				count++;
			}
		//2. give 'black' to Rightmost bottom button
		status[row-1][col-1] = black;
		//3. return black button position
		return new Point(col-1, row-1);
	}
	//move black button to direction which it wants to move
	private Point remapping_matrix(int row, int col, int black_x, int black_y, direction dir)
	{
		int new_x = black_x;
		int new_y = black_y;
		boolean available = false;
		
		//1. check direction available whether it can go
		switch(dir)
		{
		case UP:
			if(new_y-1 >= 0)
			{
				new_y -= 1;
				available = true;
			}
			break;
		case LEFT:
			if(new_x-1 >= 0)
			{
				new_x -= 1;
				available = true;
			}
			break;
		case DOWN:
			if(new_y + 1 < row)
			{
				new_y += 1;
				available = true;
			}
			break;
		case RIGHT:
			if(new_x + 1 < col)
			{
				new_x += 1;			
				available = true;
			}
			break;
		}
		//2. if the direction is available, swap black button and normal button
		if(available)
		{
			status[black_y][black_x] = status[new_y][new_x];
			status[new_y][new_x] = black;
		}
		//3. return new black button position
		return new Point(new_x, new_y);
	}
	//check if you solve the puzzle
	private boolean check_matrix(int row, int col)
	{
		int count = 1;
		int end = row*col-1;
		int x,y;
		//1. check if normal button fits
		for(int i=0;i<end;i++)
		{
			y = i / col;
			x = i - col * y;
			if(status[y][x] != count)
				return false;
			count++;
		}
		//2. check if black button fits
		y = end/ col;
		x = end - col * y;
		if(status[y][x] == black)
		{
			return true;
		}
		return false;
	}
	//shuffle the buttons
	private Point shuffle_matrix(int row, int col, int black_x, int black_y)
	{
		int outer_trial = 20;
		int inner_trial;
		Point nextpos = new Point(black_x, black_y);
		Random random = new Random();
		while(outer_trial-- >= 0)
		{
			inner_trial = outer_trial;
			//1. move black button randomly inner_trial times
			while(inner_trial-- >= 0)
			{
				
				switch(random.nextInt(4))
				{
				case 0: nextpos = remapping_matrix(row, col, nextpos.x, nextpos.y, direction.UP); break;
				case 1: nextpos = remapping_matrix(row, col, nextpos.x, nextpos.y, direction.LEFT); break;
				case 2: nextpos = remapping_matrix(row, col, nextpos.x, nextpos.y, direction.DOWN); break;
				case 3: nextpos = remapping_matrix(row, col, nextpos.x, nextpos.y, direction.RIGHT); break;
				}
			}
			//2. however moved matrix is solved matrix, move black button again
			if(!check_matrix(row,col))
				break;
		}
		//3. return black button position
		return nextpos;
	}
	// remove dynamic layout
	private void remove_matrix(int row, int col)
	{
		//1. delete buttons
		for(int i=0;i<row;i++)
			for(int j=0;j<col;j++)
					line[i].removeView(btn_matrix[i][j]);
		//2. delete linear layouts
		for(int i=0;i<row;i++)
			linear.removeView(line[i]);	
	}
	// make dynamic layout
	private void make_matrix(int row, int col)
	{		
		line = new LinearLayout[row];
		btn_matrix = new Button[row][col];
		
		for(int i=0;i<row;i++)
		{
			//1. make each line(layout)
			line[i] = new LinearLayout(this);
			line[i].setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
			for(int j=0;j<col;j++)
			{
				//2. make the button
				btn_matrix[i][j] = new Button(this);
				btn_matrix[i][j].setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
				btn_matrix[i][j].setWidth(device_width/col - 2);
				btn_matrix[i][j].setHeight(device_height/(row << 1));
				
				btn_matrix[i][j].setText(Integer.toString(status[i][j]));
				//3. if the button is black button, change black
				if(status[i][j] == black)
				{
					btn_matrix[i][j].setBackgroundColor(Color.BLACK);
				}
				//4. add button to line
				line[i].addView(btn_matrix[i][j]);
			}
			//5. add line to parent linear_layout
			linear.addView(line[i]);
		}
	}
	// make click event to normal button which is adjacent to black button
	private void make_click(int row, int col, int black_x, int black_y)
	{
		OnClickListener ltn;
		final int r = row;
		final int c = col;
		final int x = black_x;
		final int y = black_y;
		
		//1. whether button exists for four direction, make event or skip.
		//   if pause is activated, do not refresh
		if(y-1 >= 0)
		{
			ltn = new OnClickListener(){
				public void onClick(View v){
					if(end_key == false)
					{
						int count;
						count = JavaDriver.driver_read(fd);
						JavaDriver.driver_ioctl(fd, count + 1);
						refresh_matrix(r, c, x, y, direction.UP);
					}
				}
			};
			btn_matrix[y-1][x].setOnClickListener(ltn);
		}
		if(x-1>=0)
		{
			ltn = new OnClickListener(){
				public void onClick(View v){
					if(end_key == false)
					{
						int count;
						count = JavaDriver.driver_read(fd);
						JavaDriver.driver_ioctl(fd, count + 1);
						refresh_matrix(r, c, x, y, direction.LEFT);
					}
				}
			};
			btn_matrix[y][x-1].setOnClickListener(ltn);
		}
		if(y+1 < r)
		{
			ltn = new OnClickListener(){
				public void onClick(View v){
					if(end_key == false)
					{
						int count;
						count = JavaDriver.driver_read(fd);
						JavaDriver.driver_ioctl(fd, count + 1);
						refresh_matrix(r, c, x, y, direction.DOWN);
					}
				}
			};
			btn_matrix[y+1][x].setOnClickListener(ltn);
		}
		if(x+1 < c)
		{
			ltn = new OnClickListener(){
				public void onClick(View v){
					if(end_key == false)
					{
						int count;
						count = JavaDriver.driver_read(fd);
						JavaDriver.driver_ioctl(fd, count + 1);
						refresh_matrix(r, c, x, y, direction.RIGHT);
					}
				}
			};
			btn_matrix[y][x+1].setOnClickListener(ltn);
		}	
	}
	//1. make the puzzle first time
	private void init_matrix(int row, int col)
	{
		Point pos;
		//1. remove previous layout
		remove_matrix(prev_row, prev_col);
		//1-2. remove previous driver setting
		if(fd >= 0)
			JavaDriver.driver_write(fd, 1);
		
		prev_row = row;
		prev_col = col;
		//2. match number to matrix and shuffle it
		pos = mapping_matrix(row, col);
		pos = shuffle_matrix(row, col, pos.x, pos.y);
		//3. make layout
		make_matrix(row, col);
		//4. make button event
		make_click(row, col, pos.x, pos.y);
		
		//5. start the puzzle end thread
		dthread=new Device_Thread(row, col, Device_Handler);
		dthread.setDaemon(true);
		dthread.start();
		
	}
	//2. refresh the puzzle by button event
	private void refresh_matrix(int row, int col, int black_x, int black_y, direction dir)
	{
		Point pos;
		//1. remove previous layout
		remove_matrix(row, col);
		//2. move button
		pos = remapping_matrix(row, col, black_x, black_y, dir);
		//3. refresh layout
		make_matrix(row, col);
		//4. refresh button event
		make_click(row, col, pos.x, pos.y);
		//5. if puzzle is solved, reset!
		if(check_matrix(row,col))
		{
			end_key = true;
			JavaDriver.driver_write(fd, 1);
		}
	}
	//3. reset the puzzle
	private void reset_matrix(int row, int col)
	{
		//1. start the puzzle end thread
		pthread=new Puzzleend_Thread(row, col, puzzleend_Handler);
		pthread.setDaemon(true);
		pthread.start();
	}
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		
		//1. get the display width and length
		Display mDisplay = getWindowManager().getDefaultDisplay();
		DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);
		device_width = dm.widthPixels;
		device_height = dm.heightPixels;
		
		//2. get the parent linear layout
		linear = (LinearLayout)findViewById(R.id.container2);
		
		//3. get edit_text
		edit_make = (EditText)findViewById(R.id.edit_make);
		
		//4. get start button
		btn_make = (Button)findViewById(R.id.btn_make);
		//4-2. make the event of start button
		OnClickListener ltn=new OnClickListener(){
			public void onClick(View v){
				int row = 0, col = 0;
				//4-2(1). get row, col from string in edit_text
				str_make=edit_make.getText().toString();
				StringTokenizer token_make = new StringTokenizer(str_make);
				try
				{
					row = Integer.parseInt(token_make.nextToken());
					col = Integer.parseInt(token_make.nextToken());
				}
				catch(Exception e)
				{
					//do nothing
				}
				//4-2(2). if row,col is available, make the puzzle
				if(row >= 2 && row <= 4 && col >=2 && col <= 4)
				{
					init_matrix(row, col);
				}
			}
		};
		//4-3. attach event to button
		btn_make.setOnClickListener(ltn);
		//4-4. set the text
		btn_make.setText("Make Buttons");
	}

}
