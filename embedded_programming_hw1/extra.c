#include "shared_proc.h"
#include "extra.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int score;
long game_elapsed_time;
long note_list[NUM_PAD][MAX_NOTEBUFFER];
int note_head[NUM_PAD];
int note_len[NUM_PAD];

enum game_status
{
    GAME_STATUS_START,
    GAME_STATUS_PROCESS,
    GAME_STATUS_END,
    GAME_STATUS_STATUS_NUM
};
int game_flag[GAME_STATUS_STATUS_NUM];
int encounter[GAME_STATUS_STATUS_NUM];
void debug_note_list()
{
    int i,j;
    fprintf(stdout, "--------note_list---------\n");
    for(i=0;i<NUM_PAD;i++)
    {
        fprintf(stdout, "note : [%d] head : (%d) len : (%d)\n",
            i, note_head[i], note_len[i]);
        for(j=0;j<note_len[i];j++)
            fprintf(stdout, "%s%ld%s ", 
            j == note_head[i] ? "(" : "",
            note_list[i][j],
            j == note_head[i] ? ")" : "");
        fprintf(stdout,"\n\n");
    }
    fprintf(stdout, "--------------------------\n");
}

void init_game()
{
    FILE* fp = fopen("rhythm.txt","r");
    if(fp == NULL)
    {
        fprintf(stderr,"ERROR : FILE OPEN ERROR! (rhythm.txt)\n");
        setKillTrigger();
        return;
    }

    int i=0;
    for(i=0;i<NUM_PAD;i++)
    {
        note_head[i] = 0;
        note_len[i] = 0;
    }

    clock_t temp_clock;
    int temp_note;
    while(fscanf(fp, "%ld %d ",&temp_clock,&temp_note)!=EOF)
    {
        note_list[temp_note][ note_len[temp_note] ] = temp_clock;
        (note_len[temp_note])++;
    }

    game_flag[GAME_STATUS_START] = TRUE;
    game_flag[GAME_STATUS_PROCESS] = FALSE;
    game_flag[GAME_STATUS_END] = FALSE;
    encounter[GAME_STATUS_START] = FALSE;
    encounter[GAME_STATUS_PROCESS] = FALSE;
    encounter[GAME_STATUS_END] = FALSE;

    score = 0;

#ifdef DEBUG_EXTRA
    debug_note_list();
#endif

    fclose(fp);
}

void start_game(int input)
{
    msg_text text;
    text.prefix = VALID;

    if(!game_flag[GAME_STATUS_START])
        return;

    //First into start_game
    if(encounter[GAME_STATUS_START] == FALSE)
    {
        int i;
        for(i=0;i<NUM_PAD;i++)
            note_head[i] = 0;
        memset(text.data,' ',32);
        memcpy(text.data, "PRESS ANY BUTTON TO START!",sizeof(unsigned char)*25);
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        encounter[GAME_STATUS_START] = TRUE;
    }
    else
    {
        if(input == NO_DATA)
            return;

        memset(text.data,' ',32);
        memcpy(text.data, "3",1);
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        usleep(CLOCKS_PER_SEC);
        
        memset(text.data,' ',32);
        memcpy(text.data, "2",1);
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        usleep(CLOCKS_PER_SEC);

        memset(text.data,' ',32);
        memcpy(text.data, "1",1);
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        usleep(CLOCKS_PER_SEC);

        memset(text.data,' ',32);
        memcpy(text.data, "START!!",1);
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        usleep(CLOCKS_PER_SEC/2);


        //initialization
        encounter[GAME_STATUS_START] = FALSE;
        game_flag[GAME_STATUS_START] = FALSE;
        game_flag[GAME_STATUS_PROCESS] = TRUE;
        game_elapsed_time = 0;

        //remove all queue in input_switch for preventing affection of previous input
        queue_elem elem;
        while(msgrcv(input_switch_queue_id, (void *)(&elem), QUEUE_ELEM_SIZE, 0, IPC_NOWAIT)!=-1);
    }

} 

#define ON TRUE
#define OFF FALSE
void dot_onoff(unsigned char* data, int section, int subsection, int onoff)
{
    int n,m;

    section--;
    //Section
    n = 4 + 2*(section/3);
    m = 0x20 >> 2*(section%3);
    //SubSection
    n += subsection/3;
    if(subsection ==2 || subsection ==3)
        m >>=1;

    //ON
    if(onoff)
        data[n] |= m;
    //OFF
    else
        data[n] &= ~m;
}
void dot_section_onoff(unsigned char* data, int section, int onoff)
{
    dot_onoff(data, section, 1 , onoff);
    dot_onoff(data, section, 2 , onoff);
    dot_onoff(data, section, 3 , onoff);
    dot_onoff(data, section, 4 , onoff);
}

void process_game(int input)
{
    int head;
    msg_text text;
    text.prefix = VALID;
    static msg_dot dot;
    dot.prefix = VALID;

    if(!game_flag[GAME_STATUS_PROCESS])
        return;

    //First into start_game
    if(encounter[GAME_STATUS_PROCESS] == FALSE)
    {
        memset(text.data,' ',32);
        memcpy(text.data, "NOW PLAYING!",sizeof(unsigned char)*12);
        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        encounter[GAME_STATUS_PROCESS] = TRUE;

        memset(dot.data,0,10);
        set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
    }
    else
    {
        if(input == 0x03)
        {
            encounter[GAME_STATUS_PROCESS] = FALSE;
            game_flag[GAME_STATUS_PROCESS] = FALSE;
            game_flag[GAME_STATUS_END] = TRUE;
        }

        //Find Exit Condition
        int exit_flag;
        for(exit_flag=0;exit_flag<NUM_PAD;exit_flag++)
        {
            if(note_head[exit_flag] < note_len[exit_flag]) break;
        }
        if(exit_flag == NUM_PAD)
        {
            encounter[GAME_STATUS_PROCESS] = FALSE;
            game_flag[GAME_STATUS_PROCESS] = FALSE;
            game_flag[GAME_STATUS_END] = TRUE;
            return;
        }

        usleep(MILISEC);
        long int laps;
        game_elapsed_time++;

        
        int i;
        for(i=0;i<NUM_PAD;i++)
        {
            head = note_head[i];
            laps = note_list[i][head] - game_elapsed_time;
            //printf("(%d)laps:%ld ",i,laps);

            //1. SHOW NOTE TO DOT MATRIX
            if(200<= laps && laps < 300)
            {
                dot_onoff(dot.data, i+1, 1, ON);
            }
            else if(100<= laps && laps < 200)
            {
                dot_onoff(dot.data, i+1, 2, ON);
            }
            else if(0 <= laps &&laps < 100)
            {
                dot_onoff(dot.data, i+1, 3, ON);
            }
            else if(-100<=laps && laps < 0)
            {
                dot_onoff(dot.data, i+1, 4, ON);                
            }
            else if(laps < -100)
            {
                dot_section_onoff(dot.data, i+1, OFF);
                (note_head[i])++;
            }

            //2. SYNC INPUT AND NOTE
            if(input == NO_DATA)
                continue;
            if(input != (1 << i))
                continue;

            if(200<= laps && laps < 300)
            {
                score += 5;
                memset(text.data,' ',32);
                memcpy(text.data, "BAD!",sizeof(unsigned char)*4);
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
                dot_section_onoff(dot.data, i+1, OFF);
                (note_head[i])++;
            }
            else if(100<= laps && laps < 200)
            {
                score += 10;
                memset(text.data,' ',32);
                memcpy(text.data, "SOSO!",sizeof(unsigned char)*5);
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
                dot_section_onoff(dot.data, i+1, OFF);
                (note_head[i])++;
            }
            else if(0 <= laps &&laps < 100)
            {
                score += 20;
                memset(text.data,' ',32);
                memcpy(text.data, "GOOD!",sizeof(unsigned char)*5);
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
                dot_section_onoff(dot.data, i+1, OFF);
                (note_head[i])++;
            }
            else if(-100<=laps && laps < 0)
            {
                score += 40;
                memset(text.data,' ',32);
                memcpy(text.data, "VERY GOOD!",sizeof(unsigned char)*10);
                set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
                dot_section_onoff(dot.data, i+1, OFF);
                (note_head[i])++;                
            }

        }
        set_output((const char *)(&dot), OUTPUT_DEVICE_DOT);
        
    }

}
void end_game(int input)
{
    msg_text text;
    text.prefix = VALID;

    if(!game_flag[GAME_STATUS_END])
        return;

    //First into start_game
    if(encounter[GAME_STATUS_END] == FALSE)
    {
        memset(text.data,' ',32);
        memcpy(text.data, "9 BUTTON TORETRY-SCORE : ",25);
        
        text.data[26] = score/1000%10 +'0';
        text.data[27] = score/100%10 +'0';
        text.data[28] = score/10%10 +'0';
        text.data[29] = score%10 +'0'; 

        set_output((const char *)(&text), OUTPUT_DEVICE_TEXT);
        encounter[GAME_STATUS_END] = TRUE;
    }
    else
    {
        if(input != 0x100)
            return;

        //initialization
        encounter[GAME_STATUS_END] = FALSE;
        game_flag[GAME_STATUS_END] = FALSE;
        game_flag[GAME_STATUS_START] = TRUE;

    }
}

void play_game(int input)
{
    start_game(input);
    process_game(input);
    end_game(input);
}