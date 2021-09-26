# ---------------------------------------------------------------------------
# MAKE FILE SM:
# $@: nome del targhet
# $<: il primo nome nella lista di dipendenze
# lista di dipendenze: cosa mi serve per realizzare il targhet
# il makefile si costruisce a partire dalla prima regola definita e dalle sue dipendenze
# ---------------------------------------------------------------------------

CC			=  gcc
AR          =  ar
CFLAGS	   += -g -std=c99
ARFLAGS     =  rvs
INCLUDES	= -I.
LDFLAGS 	= -L.
LIBS        = -lutils -lsm -lpthread
FILE 		= config.txt

# aggiungere qui altri targets
TARGETS = prova

.PHONY: all clean cleanall test1 test2 leaks

%: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


all		: $(TARGETS)

$(TARGETS): prova.c datastruct.h libutils.a libsm.a

libsm.a : libsm.o libsm.h
	$(AR) $(ARFLAGS) $@ $<

libutils.a: libutils.o libutils.h
	$(AR) $(ARFLAGS) $@ $<

test1	:
	./$(TARGETS) $(FILE) & sleep 15
	killall -3 $(TARGETS)
	chmod +x analisi.sh
	./analisi.sh logfile.txt
test2	:
	./$(TARGETS) $(FILE) & sleep 25
	killall -1 $(TARGETS)
	chmod +x analisi.sh
	./analisi.sh logfile.txt

leaks	:
	valgrind --leak-check=full --track-origins=yes ./$(TARGETS) $(FILE)

clean	:
	rm -f $(TARGETS)

cleanall: clean
	\rm -f *.o *~ *.a *.gch
	chmod -x analisi.sh
