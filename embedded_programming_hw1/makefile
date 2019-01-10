CC = arm-none-linux-gnueabi-gcc
CFLAGS = -static -W -Wall
TARGET = HW1_20121592
CFILE = extra.c shared_proc.c input_proc.c cal_proc.c output_proc.c main_proc.c
#OBJS = shared_proc.o input_proc.o cal_proc.o output_proc.o main_proc.o

all : #$(TARGET)
#$(TARGET) : $(OBJS)
#	$(CC) $(CFLAGS) $(TARGET) $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(CFILE) 

clean:
	sudo rm -f $(OBJS) $(TARGET)