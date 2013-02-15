CC      = gcc
PROF	= -O
NOCRYPT =
C_FLAGS =  -ggdb -w -Wall $(PROF) $(NOCRYPT)
L_FLAGS =  $(PROF)

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
     websvr.o alchemy.o alias.o auth.o ban.o bit.o board.o buffer.o clanwar.o comm.o const.o crafting.o db.o db2.o \
     enchant.o effects.o fight.o fight2.o flags.o ftp.o handler.o healer.o hunt.o \
     interp.o lookup.o magic.o magic2.o mem.o mob_cmds.o mob_prog.o music.o \
     nanny.o olc.o olc_act.o olc_mpcode.o olc_save.o passive.o penalty.o pipe.o quest.o \
     ranger.o recycle.o redit-ilab.o remort.o bsave.o scan.o skills.o\
     smith.o social-edit.o song.o special.o stats.o string.o tables.o update.o \
     freeze.o warfare.o  grant.o wizlist.o marry.o forget.o clan.o \
     buildutil.o buffer_util.o simsave.o breath.o tflag.o grep.o vshift.o \
     tattoo.o religion.o playback.o leaderboard.o 

aeaea: 
tester: C_FLAGS=-ggdb -w -Wall $(PROF) $(NOCRYPT) -DTESTER
builder: C_FLAGS =  -ggdb -w -Wall $(PROF) $(NOCRYPT) -DBUILDER
remort: C_FLAGS =  -ggdb -w -Wall $(PROF) $(NOCRYPT) -DREMORT 
remort_tester: C_FLAGS =  -ggdb -w -Wall $(PROF) $(NOCRYPT) -DREMORT -DTESTER

aeaea tester builder remort remort_tester: $(O_FILES)
	rm -f aeaea 
	$(CC) $(L_FLAGS) -o aeaea $(O_FILES) -lcrypt -lm

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<

clean:
	rm *.o
