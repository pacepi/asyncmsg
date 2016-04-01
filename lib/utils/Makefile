#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif

LIBUTILS=libutils.a
CCFLAGS= -O2 -Wall -rdynamic -Wformat-security -g
ARFLAGS=-rv 


%.o: %.c
	$(SILENCE)$(AM_CC) -c $<


$(LIBUTILS) : rbtree.o
	$(SILENCE)$(AM_AR) $(ARFLAGS) $@ $^


all : $(libutils)


clean :
	rm -rf $(LIBUTILS)
	rm -rf *.o
	rm -rf $(LIBUTILS)
