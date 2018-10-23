CC=gcc
STD=-std=c11
OFLAG=-O2
LDIR=-L/usr/lib64/openmpi/lib -Wl,/usr/lib64/openmpi/lib
IDIR=-I/usr/include/openmpi-x86_64
LFLAG=-Wl,--enable-new-dtags -Wl,-rpath
LIBS=-lmpi -pthread

dijkstra: dijkstra.c
	$(CC) $(STD) $(OFLAGS) $(LFLAG) $(LDIR) $(IDIR) $(LIBS) -o $@ $<
