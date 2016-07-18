OBJDIR := obj
VPATH = ./tests
AARCHON_INC ?= /home/m256ada/include
AARCHON_LIB ?= /home/m256ada/lib

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell hg branch)"\"
PARENT	:= \""$(shell hg summary | grep parent | sed 's/parent: //')"\"

C_FLAGS =  -ggdb -rdynamic -m32 -Wall $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
L_FLAGS =  $(PROF) -m32 -llua -ldl -lcrypt -lm -lsqlite3

O_FILE_NAMES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     alchemy.o alias.o auth.o ban.o board.o buffer.o clanwar.o comm.o const.o crafting.o db.o db2.o \
     enchant.o effects.o fight.o fight2.o flags.o handler.o healer.o hunt.o \
     interp.o lookup.o magic.o magic2.o mem.o mob_cmds.o mob_prog.o \
     nanny.o olc.o olc_act.o olc_mpcode.o olc_save.o penalty.o pipe.o quest.o \
     ranger.o recycle.o redit-ilab.o remort.o bsave.o scan.o skills.o\
     smith.o social-edit.o special.o stats.o string.o tables.o update.o \
     freeze.o warfare.o  grant.o wizlist.o marry.o forget.o clan.o \
     buildutil.o buffer_util.o simsave.o breath.o tflag.o grep.o vshift.o \
     tattoo.o religion.o playback.o mob_stats.o \
     mt19937ar.o lua_scripting.o olc_opcode.o obj_prog.o \
     olc_apcode.o area_prog.o protocol.o timer.o olc_rpcode.o room_prog.o \
     lua_arclib.o lua_main.o mudconfig.o \
     lsqlite3.o
O_FILES = $(addprefix $(OBJDIR)/,$(O_FILE_NAMES))

CU_TEST_C = tests/CuTest.c
CU_TEST_O = $(OBJDIR)/CuTest.o

UNITTEST_C_FILES = $(wildcard tests/UNIT_*.c)
UNITTEST_O_FILE_NAMES = $(patsubst tests/%.c,%.o,$(UNITTEST_C_FILES)) 
UNITTEST_O_FILES = $(addprefix $(OBJDIR)/,$(UNITTEST_O_FILE_NAMES))
UNITTEST_O = $(OBJDIR)/UnitTests.o

LIVETEST_C_FILES = $(wildcard tests/LIVE_*.c)
LIVETEST_O_FILE_NAMES = $(patsubst tests/%.c,%.o,$(LIVETEST_C_FILES))
LIVETEST_O_FILES = $(addprefix $(OBJDIR)/,$(LIVETEST_O_FILE_NAMES))
LIVETEST_O = $(OBJDIR)/LiveTests.o


all: aeaea

aeaea: $(O_FILES)
	rm -f aeaea 
	$(CC) -o aeaea $(O_FILES) $(L_FLAGS)

tester: C_FLAGS += -DTESTER
tester: aeaea

builder: C_FLAGS += -DBUILDER
builder: aeaea

remort: C_FLAGS += -DREMORT 
remort: aeaea

remort_tester: C_FLAGS += -DREMORT -DTESTER
remort_tester: aeaea

osx: C_FLAGS = -I/usr/local/include -I/usr/local/Cellar/lua51/5.1.5_1/include/lua-5.1 -ggdb -w
osx: L_FLAGS = -L/usr/local/lib -ldl -llua5.1
osx: aeaea

# Compile live tests. The binary needs to be run rom area directory as with normal startup
livetest: C_FLAGS += -DLIVETEST
livetest: _live_tests $(O_FILES) $(LIVETEST_O_FILES) $(CU_TEST_O)
	rm -f aeaea_livetest
	$(CC) -o aeaea_livetest \
		$(O_FILES) \
		$(LIVETEST_O_FILES) \
		$(LIVETEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)

# Compile with unit tests and run immediately
unittest: C_FLAGS += -DUNITTEST
unittest: _unit_tests $(O_FILES) $(UNITTEST_O_FILES) $(CU_TEST_O)
	rm -f aeaea_unittest
	$(CC) -o aeaea_unittest \
		$(O_FILES) \
		$(UNITTEST_O_FILES) \
		$(UNITTEST_O) \
		$(CU_TEST_O) \
		$(L_FLAGS)
	./aeaea_unittest

_live_tests: $(OBJDIR)
	./tests/make-live-tests.sh > ./tests/LiveTests.c
	$(CC) -c $(C_FLAGS) -o $(OBJDIR)/LiveTests.o ./tests/LiveTests.c

_unit_tests: $(OBJDIR)
	./tests/make-unit-tests.sh > ./tests/UnitTests.c
	$(CC) -c $(C_FLAGS) -o $(OBJDIR)/UnitTests.o ./tests/UnitTests.c


$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.c merc.h | $(OBJDIR)
	$(CC) -c $(C_FLAGS) -o $@ $<


clean:
	rm -f $(OBJDIR)/*.o
