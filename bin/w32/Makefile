#  This Makefile builds two executables:
#     g32shell.exe: performs globbing on its command line arguments
#     w32shell.exe: performs no globbing
#
#  The MING32 is assumed.
CC = gcc
CFLAGS = -O2 -W -Wall
LDFLAGS = -s

COMPILE = $(CC) $(CFLAGS) $(LDFLAGS)
EXECS = g32shell.exe w32shell.exe

all: $(EXECS)
g32shell.exe: w32shell.c
	$(COMPILE) -o g32shell w32shell.c
w32shell.exe: w32shell.c
	$(COMPILE) -DNO_GLOBBING -o w32shell w32shell.c

