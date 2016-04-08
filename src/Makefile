#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif


ifeq (X$(inproc), Xy)
	EXTERFLAG=-DINPROC
endif


TARGET=asyncmsg
LIBRARY=libasyncmsg.a


INC_DIR=-I$(AM_BUILD_TOP)/target/inc
LIB_DIR=-L$(AM_BUILD_TOP)/target/lib
LIBS=-liniparser -lnanomsg -ldl -lanl -lpthread -lrt
CFLAGS= -O2 -Wall -rdynamic -Wformat-security -g -fPIC
ARFLAGS=-rv


rebuild : clean all


all : $(TARGET) $(LIBRARY) release


%.o: %.c
	$(SILENCE)$(AM_CC) $(CFLAGS) $(EXTERFLAG) -c $< $(INC_DIR) $(LIB_DIR) $(LIBS)

$(TARGET) : am.o am_input.o am_output.o am_log.o
	$(SILENCE)$(AM_CC) $(CFLAGS) -o $@ $^ $(INC_DIR) $(LIB_DIR) $(LIBS)


$(LIBRARY) : asyncmsg.o
	$(SILENCE)cp $(AM_BUILD_TOP)/target/lib/libnanomsg.a $(LIBRARY)
	$(SILENCE)$(AM_AR) q $(LIBRARY) asyncmsg.o


clean :
	$(SILENCE)rm -rf *.o
	$(SILENCE)rm -rf $(TARGET) $(LIBRARY)


release :
	$(SILENCE)cp $(TARGET) $(AM_BUILD_TOP)/target/bin
	$(SILENCE)cp $(LIBRARY) $(AM_BUILD_TOP)/target/lib
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/inc/asyncmsg
	$(SILENCE)cp asyncmsg.h $(AM_BUILD_TOP)/target/inc/asyncmsg
