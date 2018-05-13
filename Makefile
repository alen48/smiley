OBJS = multiProcMain.o multiProcDriver.o
CC = g++
VER = -std=c++11
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

multiProc : $(OBJS)
	$(CC) $(VER) $(LFLAGS) $(OBJS) -o multiProc -pthread

multiProcMain.o : multiProc.h multiProcMain.cc
	$(CC) $(VER) $(CFLAGS) multiProcMain.cc -pthread

multiProcDriver.o : multiProc.h multiProcDriver.cc
	$(CC) $(VER) $(CFLAGS) multiProcDriver.cc

clean:
	@echo Deleting binary files and intermediate files...
	rm -f *.o *~ multiProc inputG inputP* outputP*

tar:
	@echo Compressing files...
	tar cfv multiProc.tar multiProc.h multiProcMain.cc multiProcDriver.cc input
