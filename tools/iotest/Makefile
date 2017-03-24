VPATH = ../../src
CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2

C_FLAGS =  -ggdb -rdynamic -m32 -Wall $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
L_FLAGS =  $(PROF) -m32

O_FILES := iotest.o perfmon.o

all: iotest

iotest: $(O_FILES)
	rm -f iotest 
	$(CC) -o iotest $(O_FILES) $(L_FLAGS)

%.o: %.c
	$(CC) -c $(C_FLAGS) -o $@ $<

clean:
	rm -f *.o
