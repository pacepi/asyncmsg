NNLIB=-L../target/lib -lnanomsg -lanl -lpthread
AMINC=-I../target/inc
AMLIB=-L../target/lib -lasyncmsg -lanl -lpthread

all : nn am

nn :
	@echo "make nn ..."
	rm -rf nn_set nn_get
	gcc nn_set.c $(NNLIB) -g -static -o nn_set
	gcc nn_get.c $(NNLIB) -g -static -o nn_get

am :
	@echo "make am..."
	rm -rf am_set am_get
	gcc am_set.c $(AMLIB) $(AMINC) -g -static -o am_set
	gcc am_get.c $(AMLIB) $(AMINC) -g -static -o am_get

clean :
	@rm -rf nn_set nn_get am_set am_get
