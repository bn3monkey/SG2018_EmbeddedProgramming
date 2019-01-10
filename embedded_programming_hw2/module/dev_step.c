//setting value to fnd
static void set_fnd(int index, char value, unsigned char* out)
{
    switch(index)
    {
        case 0: out[1] &= 0x0f; out[1] |= (value << 4); break;
        case 1: out[1] &= 0xf0; out[1] |= value; break;
        case 2: out[0] &= 0x0f; out[0] |= (value << 4); break;
        case 3: out[0] &= 0xf0; out[0] |= value; break;
    }
}
//getting value from fnd
static unsigned char get_fnd(int index, unsigned char* out)
{
    switch(index)
    {
        case 0 : return 0x0f & (out[1] >> 4);
        case 1 : return 0x0f & out[1];
        case 2 : return 0x0f & (out[0] >> 4);
        case 3 : return 0x0f & out[0];
    }
    return 0;
}

//in(fnd_index, fnd_value) : parameter value from user process
//out : first step of device output
void fnd_init_step(unsigned char fnd_index, unsigned char fnd_value, unsigned char* out)
{
    out[0] = 0;
    out[1] = 0;

    set_fnd(fnd_index, fnd_value, out);
    //for rotation check
    out[2] |= (1 << (fnd_value-1));

    printk("fnd-output : %d %d %d %d\n",
        get_fnd(0, out),get_fnd(1, out),get_fnd(2,out),get_fnd(3,out));
    //printk("%d %x\n",fnd_value,out[2]);
}

//setting out to next device output
void fnd_step(unsigned char* out)
{
    int i;
    unsigned char now;

    for(i=0;i<4;i++)
    {
        now = get_fnd(i,out);
        if(now != 0)
            break;
    }
    
    //next step but cycle uncomplete
    if(out[2] < 0xff)
    {
        now++;
        if(now > 8) now = 1;
        //set next number
        set_fnd(i, now, out);
        //cycle check count output number
        out[2] |= (1 << (now-1));       
    }
    //next step but cycle complete
    else
    {
        //cycle checker initialize
        out[2] = 0;
        
        //go to next digit
        set_fnd(i, 0, out);
        now++;
        if(now > 8) now = 1;
        i = (i+1) & 0x3;
        set_fnd(i, now, out);

        //cycle check count output number
        out[2] |= (1 << (now-1));  
    }
   
    printk("fnd-output : %d %d %d %d\n",
        get_fnd(0, out),get_fnd(1, out),get_fnd(2,out),get_fnd(3,out));
    //printk("%d\n",out[2]);
}

//in : parameter value from user process
//out : first step of device output
void led_init_step(unsigned char led_value, unsigned char* out)
{
    out[0] = 1 << (8 - led_value);
    out[1] = led_value;
    printk("led-output : %d\n",*out);
}
//setting out to next device output
void led_step(unsigned char* out)
{
    //make next digit
    out[1]++;
    if(out[1] > 8)
        out[1] = 1;

    //set new out
    out[0] = 1 << (8 - out[1]);
    printk("led-output : %d\n",*out);
}

unsigned char fpga_number[10][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};
unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

//in : parameter value from user process
//out : first step of device output
void dot_init_step(unsigned char dot_value, unsigned char* out)
{
    int i;
    for(i=0;i<10;i++)
        out[i] = fpga_number[dot_value][i];

    out[i] = dot_value;

    printk("dot-output : %d\n",out[i]);
}
void dot_off(unsigned char* out)
{
    int i;
    for(i=0;i<10;i++)
        out[i] = fpga_set_blank[i];
}
//setting out to next device output
void dot_step(unsigned char* out)
{
    int i;
    //make next digit
    out[10]++;
    if(out[10] > 8)
        out[10] = 1;

    //set new out
     for(i=0;i<10;i++)
        out[i] = fpga_number[out[10]][i];

    printk("dot-output : %d\n",*out);
}


#define index_first_direct 33
#define index_first_start 34
#define first_end 8
#define index_second_direct 35
#define index_second_start 36
#define second_end 4

const char* student_no = "20121592";
const char* student_name = "JAEHYUK PARK";
const char* text_blank = "                                ";
//in : parameter value from user process
//out : first step of device output
void text_init_step(unsigned char* out)
{
    sprintf(out, "20121592        JAEHYUK PARK    ");
    out[index_first_direct] = 1;
    out[index_first_start] = 0;
    out[index_second_direct] = 1;
    out[index_second_start] = 0;

    printk("text-output : %s\n",out);
    // printk("index_first_start : %d\n", (int)out[index_first_start]);
    // printk("index_second_start : %d\n", (int)out[index_second_start]);
}
void text_off(unsigned char* out)
{
    sprintf(out, text_blank);
    printk("text-output : %s\n",out);
}

//setting out to next device output
void text_step(unsigned char* out)
{
    unsigned char temp;
    int i;

    if(out[index_first_direct] > 0)
    {
         temp = out[index_first_start]+1;
         if(!(0<= temp && temp <= first_end))
         {
             out[index_first_start]--;
             out[index_first_direct] = 0;
         }
         else
            out[index_first_start] = temp;
    }
    else
    {
        temp = out[index_first_start]-1;
         if(!(0<= temp && temp <= first_end))
         {
             out[index_first_start]++;
             out[index_first_direct] = 1;
         }
         else
            out[index_first_start] = temp;
    }
    //printk("index_first_start : %d\n", (int)out[index_first_start]);

    if(out[index_second_direct] > 0)
    {
         temp = out[index_second_start]+1;
         if(!(0<= temp && temp <= second_end))
         {
             out[index_second_start]--;
             out[index_second_direct] = 0;
         }
         else
            out[index_second_start] = temp;
    }
    else
    {
         temp = out[index_second_start]-1;
         if(!(0<= temp && temp <= second_end))
         {
             out[index_second_start]++;
             out[index_second_direct] = 1;
         }
         else
            out[index_second_start] = temp;
    }
    //printk("index_second_start : %d\n", (int)out[index_second_start]);

    sprintf(out, text_blank);
    for(i=0;i<8;i++)
        *(out + out[index_first_start] + i) = *(student_no + i);
    for(i=0;i<12;i++)
        *(out + 16 + out[index_second_start] + i) =  *(student_name + i);

    printk("text-output : %s\n",out);
}