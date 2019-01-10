#include "shared_proc.h"
#include "cal_proc.h"
#include "extra.h"
#include <time.h>

/**************** Mode_Calculation *****************/
const char* mode_name[MODE_NUM] =
{
    "MODE_CLOCK",
    "MODE_COUNTER",
    "MODE_TEXTEDITOR",
    "MODE_DRAWBOARD",
    "MODE_EXTRA" 
};
// when mode_initial is true, mode is initial state.
// so each mode function do initial process
// mode_initial is false,

int mode_initial[MODE_NUM];
void set_mode_initial()
{
    int i;
    for(i=0;i<MODE_NUM;i++)
        mode_initial[i] = TRUE;
}

static void get_device_time(int* hour, int* min, int* sec)
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime (&rawtime);

    *hour = timeinfo->tm_hour;
    *min = timeinfo->tm_min;
    *sec = timeinfo->tm_sec;
}
static void make_msg_time(int hour, int min, unsigned char* data)
{
    data[0] = hour/10;
    data[1] = hour%10;
    data[2] = min/10;
    data[3] = min%10;
}
//if min and hour changes, return 1.
//else return 0
static int time_tkn(int* hour, int* min, int* sec)
{
    (*sec)++;
    if(*sec >= 60)
    {
        (*min)++;
        *sec =0;
        if(*min >= 60)
        {
            (*hour)++;
            *min = 0;
            if(*hour >= 24)
                *hour = 0;    
        }
        return TRUE;
    }
    return FALSE;
}
#define SUBMODE_CHANGETIME 0
#define SUBMODE_NOWTIME 1
void mode_clock(int input)
{
    static int hour, min, sec;
    static clock_t tick_start, tick_end;
    static int submode = SUBMODE_NOWTIME;
    static unsigned char blink;

    //message initialization
    msg_led led;
    led.prefix = VALID;
    msg_fnd fnd;
    fnd.prefix = VALID;

    //mode initialization
    if(mode_initial[MODE_CLOCK])
    {
        fprintf(stdout, "MODE_CLOCK_START!\n");
        submode = SUBMODE_NOWTIME;

        blink = 0x20;

        led.data = 0x80;
        set_output((const char *)(&led), OUTPUT_DEVICE_LED);

        get_device_time(&hour, &min, &sec);
        tick_start = clock();
        make_msg_time(hour,min,fnd.data);
        set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);

        mode_initial[MODE_CLOCK] = FALSE;
    }

    //Routine
    switch(submode)
    {
        case SUBMODE_NOWTIME :
            tick_end = clock();
            if(tick_end - tick_start >= CLOCKS_PER_SEC)
            {
                tick_start = tick_end;
                if (time_tkn(&hour, &min, &sec))
                {
                    make_msg_time(hour, min, fnd.data);
                    set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
                }
            }
        
            switch(input)
            {
                case 0x01:
                    fprintf(stdout, "MODE CHANGE : CHANGE_TIME\n");
                    led.data = blink;
                    set_output((const char *)(&led), OUTPUT_DEVICE_LED);
                    submode = SUBMODE_CHANGETIME;
                break;                
            }
               
        break;
        case SUBMODE_CHANGETIME :
            tick_end = clock();
            if(tick_end - tick_start >= CLOCKS_PER_SEC)
            {
                tick_start = tick_end;
                blink = blink == 0x20 ? 0x10 : 0x20;
                led.data = blink;
                set_output((const char *)(&led), OUTPUT_DEVICE_LED);
            }

            switch(input)
            {
                case 0x01:
                    fprintf(stdout, "MODE CHANGE : NOW_TIME\n");
                    led.data = 0x80;
                    set_output((const char *)(&led), OUTPUT_DEVICE_LED);
                    submode = SUBMODE_NOWTIME;
                break;
                case 0x02:
                    fprintf(stdout, "TIME RESET\n");
                    get_device_time(&hour, &min, &sec);
                    tick_start = clock();
                    make_msg_time(hour,min,fnd.data);
                    set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
                break;
                case 0x04:
                    fprintf(stdout, "HOUR CHANGE\n");
                    hour = (hour+1)%24;
                    make_msg_time(hour,min,fnd.data);
                    set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
                break;
                case 0x08:
                    fprintf(stdout, "MINUTE CHANGE\n");
                    min = (min+1)%60;
                    make_msg_time(hour,min,fnd.data);
                    set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
                break;
            }
        break;

    }
    return;
}

enum EXPRESS_PROP
{
    EXPRESS_DEC,
    EXPRESS_OCT,
    EXPRESS_QUAT,
    EXPRESS_BIN,
    EXPRESS_NUM
};
enum digit_choice
{
    digit100,
    digit10,
    digit1
};
void alter_dec(int number, unsigned char* data)
{
    data[3] = number%10;
    number/=10;
    data[2] = number%10;
    number/=10;
    data[1] = number%10;
    number/=10;
    data[0] = 0;
}
void alter_oct(int number, unsigned char* data)
{
    data[3] = number & 0x7;
    number >>= 3;
    data[2] = number & 0x7;
    number >>= 3;
    data[1] = number & 0x7;
    number >>= 3;
    data[0] = number & 0x7;
}
void alter_quat(int number, unsigned char* data)
{
    data[3] = number & 0x3;
    number >>= 2;
    data[2] = number & 0x3;
    number >>= 2;
    data[1] = number & 0x3;
    number >>= 2;
    data[0] = number & 0x3;
}
void alter_bin(int number, unsigned char* data)
{
    data[3] = number & 0x1;
    number >>= 1;
    data[2] = number & 0x1;
    number >>= 1;
    data[1] = number & 0x1;
    number >>= 1;
    data[0] = number & 0x1;
}
void (*alter_table[4])(int number, unsigned char* data) = 
{
    alter_dec,
    alter_oct,
    alter_quat,
    alter_bin
};
void inc_number(int* number, int digit, int prop)
{
    const int table[4][3] =
    {
        {100, 10, 1},//decimal
        {64, 8, 1},//oct
        {16, 4 ,1},//quat
        {4, 2, 1}//bin
    };
    *number += table[prop][digit];
}
void mode_counter(int input)
{
    static int number;
    static int submode;

    //message initialization
    msg_led led;
    led.prefix = VALID;
    msg_fnd fnd;
    fnd.prefix = VALID;

    //mode initialization
    if(mode_initial[MODE_COUNTER])
    {
        fprintf(stdout, "MODE_COUNTER_START!\n");
        submode = EXPRESS_DEC;
        fprintf(stdout, "NOW MODE : %s\n", submode==0 ? "DEC" :
            submode==1 ? "OCT" : submode==2 ? "QUAT": "BIN");
        number = 0;

        led.data = 0x40;
        set_output((const char *)(&led), OUTPUT_DEVICE_LED);

        alter_dec(number, fnd.data);
        set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);

        mode_initial[MODE_COUNTER] = FALSE;
    }

    //Routine
    switch(input)
    {
        case 0x01:
            submode = (submode + 1) % EXPRESS_NUM;
            fprintf(stdout, "NOW MODE : %s\n", submode==0 ? "DEC" :
            submode==1 ? "OCT" : submode==2 ? "QUAT": "BIN");
            
            led.data = 1 << ((6-submode)+4*(submode==3));
            set_output((const char *)(&led), OUTPUT_DEVICE_LED);

            alter_table[submode](number, fnd.data);
            set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
        break;
        case 0x02:
            inc_number(&number, digit100, submode);
            alter_table[submode](number, fnd.data);
            set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
        break;
        case 0x04:
            inc_number(&number, digit10, submode);
            alter_table[submode](number, fnd.data);
            set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
        break;
        case 0x08:
            inc_number(&number, digit1, submode);
            alter_table[submode](number, fnd.data);
            set_output((const char *)(&fnd), OUTPUT_DEVICE_FND);
        break;
    }
    return;
}

#define NUM_TOUCHPAD 9
#define MAX_IDX_ALPHABET 3
#define SUBMODE_ALPHABET 0
#define SUBMODE_NUMBER 1
const char touchpad[NUM_TOUCHPAD][MAX_IDX_ALPHABET] =
{
    {'.','Q','Z'},
    {'A','B','C'},
    {'D','E','F'},
    {'G','H','I'},
    {'J','K','L'},
    {'M','N','O'},
    {'P','R','S'},
    {'T','U','V'},
    {'W','X','Y'}
};
const char dot_A[10] = {0x1C,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63};
const char dot_1[10] = {0x0c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3f,0x3f};

void cursor_move(int* cursor, unsigned char* buffer)
{
    int i;
    (*cursor)++;
    if(*cursor >= 32)
    {
        *cursor = 31;
        for(i=0; i< 31; i++)
            buffer[i] = buffer[i+1];
    }
}
void cursor_back(int* cursor)
{
    (*cursor)--;
    if(*cursor<-1)
        *cursor = -1;
}
void mode_texteditor(int input)
{
    static int count;
    static int cursor;
    static int idx_alphabet;
    static int last_switch;
    static int submode;

    //message initialization
    msg_fnd fnd;
    fnd.prefix = VALID;
    static msg_dot dot;
    dot.prefix = VALID;
    static msg_text text;
    text.prefix = VALID;

    //mode initialization
    if(mode_initial[MODE_TEXTEDITOR])
    {
        fprintf(stdout, "MODE_TEXTEDITOR_START!\n");
        submode = SUBMODE_ALPHABET;
        fprintf(stdout, "NOW MODE : %s\n", submode==0 ? "Alphabet" : "Number");

        last_switch = NO_DATA;

        //fnd
        count = 0;

        //text
        cursor = -1;
        idx_alphabet = 0;
        memset(text.data,' ',32*sizeof(unsigned char));
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);

        //dot
        memcpy(dot.data, dot_A, 10*sizeof(unsigned char));
        set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);

        mode_initial[MODE_TEXTEDITOR] =  FALSE;
        return;
    }

    //Routine
    if(input != NO_DATA)
    {
        count = (count+1)%10000;
        fnd.data[3] = count%10;
        fnd.data[2] = count/10%10;
        fnd.data[1] = count/100%10;
        fnd.data[0] = count/1000%10;
        set_output((const char*)(&fnd), OUTPUT_DEVICE_FND);
    }
    else
        return;

    switch (submode)
    {
        case SUBMODE_ALPHABET :
            if(input == 0x06)
            {
                //buffer clear
                cursor = -1;
                memset(text.data,' ',32*sizeof(unsigned char));
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
            }
            else if(input == 0x30)
            {
                //mode change
                fprintf(stdout, "NOW MODE : %s\n", submode==0 ? "Alphabet" : "Number");
                idx_alphabet = 0;
                submode = SUBMODE_NUMBER;
                memcpy(dot.data, dot_1, 10*sizeof(unsigned char));
                set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
            }
            else if(input ==0xC0)
            {
                text.data[cursor] = ' ';
                cursor_back(&cursor);
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
            }
            else if(input == 0x180)
            {
                //indentation
                cursor_move(&cursor, text.data);
                text.data[cursor] = ' ';
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
            }
            else
            {
                if(input == NO_DATA)
                    break;
                int num = -1;
                while(input != (1 << ++num));

                if(last_switch == input)
                {
                    idx_alphabet = (idx_alphabet + 1) % MAX_IDX_ALPHABET;
                }
                else
                {
                    cursor_move(&cursor, text.data);
                    idx_alphabet = 0;
                }
                text.data[cursor] = touchpad[num][idx_alphabet];
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);               
            }
        break;
        case SUBMODE_NUMBER :
            if(input == 0x06)
            {
                //buffer clear
                cursor = -1;
                memset(text.data,' ',32*sizeof(unsigned char));
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
            }
            else if(input == 0x30)
            {
                //mode change
                fprintf(stdout, "NOW MODE : %s\n", submode==0 ? "Alphabet" : "Number");
                idx_alphabet = 0;
                submode = SUBMODE_ALPHABET;
                memcpy(dot.data, dot_A, 10*sizeof(unsigned char));
                set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
            }
            else if(input ==0xC0)
            {
                text.data[cursor] = ' ';
                cursor_back(&cursor);
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
            }
            else if(input == 0x180)
            {
                //indentation
                cursor_move(&cursor, text.data);
                text.data[cursor] = ' ';
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
            }
            else
            {
                if(input == NO_DATA)
                    break;
                int num = -1;
                while(input != (1 << ++num));

                cursor_move(&cursor, text.data);                    
                text.data[cursor] = num + '1';
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);               
            }
        break;
    }
    last_switch = input;
    return;
}

#define BITMAP_COL 7
#define BITMAP_ROW 10
void mode_drawboard(int input)
{
    static int count;
    static int row, col;
    static clock_t clock_start, clock_end;
    static int isblink, blink;
    
    int i;
    
    //message initialization
    msg_fnd fnd;
    fnd.prefix = VALID;
    static msg_dot dot;
    dot.prefix = VALID;

    //initialization
    if(mode_initial[MODE_DRAWBOARD])
    {
        count = 0;

        clock_start = clock();
        isblink = TRUE;
        blink = TRUE;

        row = col = 0;

        memset(dot.data, 0, sizeof(char)*10);
        set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
        mode_initial[MODE_DRAWBOARD] = FALSE;
    }
    //counter
    if(input != NO_DATA)
    {
        count = (count+1)%10000;
        fnd.data[3] = count%10;
        fnd.data[2] = count/10%10;
        fnd.data[1] = count/100%10;
        fnd.data[0] = count/1000%10;
        set_output((const char*)(&fnd), OUTPUT_DEVICE_FND);
    }

    switch(input)
    {
        case 0x01:
            clock_start = clock();
            isblink = TRUE;
            blink = TRUE;

            row = col = 0;
            memset(dot.data, 0, sizeof(char)*10);
            set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
        break;
        case 0x02:
            row = (row + 1) % BITMAP_ROW;
        break;
        case 0x04:
            isblink = isblink ? FALSE : TRUE;
        break;
        case 0x08:
            col = (col + BITMAP_COL -1) % BITMAP_COL;
        break;
        case 0x10:
            (dot.data)[BITMAP_ROW -1 - row] |= (1<<(BITMAP_COL -1 -col));
            set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
        break;
        case 0x20:
            col = (col + 1) % BITMAP_COL;
        break;
        case 0x40:
            memset(dot.data, 0, sizeof(char)*10);
            set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
        break;
        case 0x80:
            row = (row+BITMAP_ROW-1)%BITMAP_ROW;
        break;
        case 0x100:
            for(i=0;i<10;i++)
                (dot.data)[i] = ~(dot.data)[i];
            set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
        break;
    }

    //cursor blinking
    if(isblink)
    {
        clock_end = clock();
        if(clock_end - clock_start >= CLOCKS_PER_SEC)
        {
            clock_start = clock_end;
            
            unsigned char temp = dot.data[BITMAP_ROW -1 -row];
            if(blink)
                (dot.data)[BITMAP_ROW -1 - row] |= (1<<(BITMAP_COL -1 -col));
            else
                (dot.data)[BITMAP_ROW -1 - row] &= ~(1<<(BITMAP_COL -1 -col));
            blink = blink ? 0 : 1;
            set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
            dot.data[BITMAP_ROW -1 -row] = temp;
        }
    }
    return;
}
void mode_extra(int input)
{
    /*
    if(input == NO_DATA)
        return;
    printf("input : %d\n", input);
    return;
    */

    if(mode_initial[MODE_EXTRA])
    {
        init_game();
        mode_initial[MODE_EXTRA] = FALSE;
    }
    else
    {
        play_game(input);
    }
}
void (*mode_cal_table[MODE_NUM]) (int input)
 = {mode_clock, mode_counter, mode_texteditor, mode_drawboard, mode_extra};

void mode_cal()
{
    int current_mode;
    queue_elem elem;

    current_mode = *vaddr_current_mode;
    
    if(msgrcv(input_switch_queue_id, (void *)(&elem), QUEUE_ELEM_SIZE, 0, IPC_NOWAIT)==-1)
           *(int *)elem.data = NO_DATA;
    
    int param = *(int *)elem.data;
    mode_cal_table[current_mode] (param);
}

/********************************************/
/********* Side Calculation *****************/
void side_released()
{
    //Nothing to do!
}
void side_exit()
{
    clear_output();
    setKillTrigger();
    fprintf(stdout, "End Program!\n");
}
void side_nextmode()
{
    int temp = *vaddr_current_mode;
    temp = (temp+1)%5;
    *vaddr_current_mode = temp;
    clear_output();
    set_mode_initial();
    lock_sema(reset_message_sema_id);
    fprintf(stdout, "Now Mode : %s!\n", mode_name[temp]);
}
void side_prevmode()
{
    int temp = *vaddr_current_mode;
    temp = (temp+4)%5;
    *vaddr_current_mode = temp;
    clear_output();
    set_mode_initial();
    lock_sema(reset_message_sema_id);
    fprintf(stdout, "Now Mode : %s!\n", mode_name[temp]);
}
void (*side_cal_table[SIDE_INPUT_NUM]) () =
{
    side_released,
    side_exit,
    side_nextmode,
    side_prevmode
};

void side_cal()
{
    queue_elem elem;
    *(int *)elem.data = SIDE_RELEASED;
    msgrcv(input_mode_queue_id, (void *)(&elem), QUEUE_ELEM_SIZE, 0, IPC_NOWAIT);
    side_cal_table[*(int *)elem.data] ();
}
/********************************************/


/**** initialization & Destroy function *****/

int cal_init()
{
    set_mode_initial();
    return TRUE;
}
/******************************************/


void cal_proc()
{
    if(!cal_init())
    {
        setKillTrigger();
        fprintf(stderr, "ERROR : Calculation Process Error!\n");
        goto END_CALPROC;
    }

    //if Trigger is On, End below iteration and exit the function.
    while(!getKillTrigger())
    {
        side_cal();
        
        mode_cal();
    }

    END_CALPROC:
        return;
    
}