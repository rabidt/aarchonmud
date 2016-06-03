AARCHON_INC ?= /home/m256ada/include
AARCHON_LIB ?= /home/m256ada/lib

CC      = gcc
PROF    = -I$(AARCHON_INC) -L$(AARCHON_LIB) -O2
MKTIME	:= \""$(shell date)"\"
BRANCH	:= \""$(shell hg branch)"\"
PARENT	:= \""$(shell hg summary | grep parent | sed 's/parent: //')"\"

C_FLAGS =  -ggdb -rdynamic -m32 -Wall $(PROF) -DMKTIME=$(MKTIME) -DBRANCH=$(BRANCH) -DPARENT=$(PARENT)
L_FLAGS =  $(PROF) -m32 -llua -ldl -lcrypt -lm -lsqlite3

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     alchemy.o alias.o auth.o ban.o board.o buffer.o clanwar.o comm.o const.o crafting.o db.o db2.o \
     enchant.o effects.o fight.o fight2.o flags.o handler.o healer.o hunt.o \
     interp.o lookup.o magic.o magic2.o mem.o mob_cmds.o mob_prog.o \
     nanny.o olc.o olc_act.o olc_mpcode.o olc_save.o penalty.o pipe.o quest.o \
     ranger.o recycle.o redit-ilab.o remort.o bsave.o scan.o skills.o\
     smith.o social-edit.o special.o stats.o string.o tables.o update.o \
     freeze.o warfare.o  grant.o wizlist.o marry.o forget.o clan.o \
     buildutil.o buffer_util.o simsave.o breath.o tflag.o grep.o vshift.o \
     tattoo.o religion.o playback.o mob_stats.o \
     mt19937ar.o lua_scripting.o olc_opcode.o obj_prog.o\
     olc_apcode.o area_prog.o protocol.o timer.o olc_rpcode.o room_prog.o\
     lua_arclib.o lua_main.o mudconfig.o\
     lsqlite3.o

aeaea:  
tester: C_FLAGS += -DTESTER
builder: C_FLAGS += -DBUILDER
remort: C_FLAGS += -DREMORT 
remort_tester: C_FLAGS += -DREMORT -DTESTER
osx: C_FLAGS = -I/usr/local/include -I/usr/local/Cellar/lua51/5.1.5_1/include/lua-5.1 -ggdb -w
osx: L_FLAGS = -L/usr/local/lib -ldl -llua5.1

aeaea tester builder remort remort_tester osx: $(O_FILES)
	rm -f aeaea 
	$(CC) -o aeaea $(O_FILES) $(L_FLAGS)

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<

clean:
	rm *.o
