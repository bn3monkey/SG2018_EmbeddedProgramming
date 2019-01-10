//Rhythm Game

#define NUM_PAD 9
#define MAX_NOTEBUFFER 1000
#define MILISEC 1000

#define DEBUG_EXTRA

//For Debug
void debug_note_list();

//1. Initiation. note file open
//2. it is called by mode_extra() (cal_proc.c)
void init_game();

//1. start mode. wait player to play the game
//2. it is called in play_game()
void start_game(int input);

//1. real game routine
//2. it is called in play_game()
void process_game(int input);

//1. end the game and show the score to player
//2. it is called in play_game()
void end_game(int input);

//1. Playing the game. game routine!
//2. it is called by mode_extra() (cal_proc.c)
void play_game(int input);