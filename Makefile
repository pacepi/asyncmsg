#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif


rebuild : clean all


all : check prepare lib src conf


check :
ifeq (X$(AM_BUILD_TOP), X)
	@echo "please source build/setupenv firstly"
	@exit 1
endif


prepare : 
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/lib
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/bin
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/inc
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/conf


.PHONY : lib src conf test
lib :
	$(SILENCE)make -C $(AM_BUILD_TOP)/lib all showcommand=$(showcommand)


src :
	$(SILENCE)make -C $(AM_BUILD_TOP)/src all showcommand=$(showcommand) inproc=$(inproc)


test :
	$(SILENCE)make -C $(AM_BUILD_TOP)/test all showcommand=$(showcommand)


conf :
	$(SILENCE)cp $(AM_BUILD_TOP)/conf/*.ini $(AM_BUILD_TOP)/target/conf


clean :
	$(SILENCE)rm -rf target
	$(SILENCE)make -C $(AM_BUILD_TOP)/lib clean showcommand=$(showcommand)
	$(SILENCE)make -C $(AM_BUILD_TOP)/src clean showcommand=$(showcommand)
	$(SILENCE)make -C $(AM_BUILD_TOP)/test clean showcommand=$(showcommand)


astyle :
	$(SILENCE)git status | grep "\.c" | awk '{print $$ 2}' | xargs astyle
	$(SILENCE)find -name "*.c.orig" | xargs rm


help :
	@echo "Help    :"
	@echo "check   :  check building environment"
	@echo "lib     :  build all libraries"
	@echo "all     :  build all target and release to target"
	@echo "clean   :  clean all target"
	@echo "rebuild :  clean all and rebuild all"
	@echo "astyle  :  format cpp files by astyle"
	@echo "options :  showcommand=y : show building details"
	@echo "           inproc=y : use inproc between input and output"
