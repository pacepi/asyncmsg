#read variables from command line and environment variables
ifeq (X$(showcommand), Xy)
	SILENCE=
else
	SILENCE=@
endif

.PHONY : iniparser nanomsg utils
all : iniparser nanomsg utils


iniparser :
	@echo "iniparser unpackage ..."
	$(SILENCE)tar -xzf iniparser.tar.gz
	
	@echo "iniparser build ..."
	$(SILENCE)make -C iniparser default

	@echo "iniparser release ..."
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/inc/iniparser
	$(SILENCE)cp iniparser/src/*.h $(AM_BUILD_TOP)/target/inc/iniparser
	$(SILENCE)cp iniparser/libiniparser.a $(AM_BUILD_TOP)/target/lib
	@echo "iniparser done ..."


nanomsg :
	@echo "nanomsg unpackage ..."
	$(SILENCE)tar -xzf nanomsg.tar.gz
	
	@echo "nanomsg build ..."
	$(SILENCE)cd nanomsg && sh autogen.sh && ./configure && make -j && cd -
	#$(SILENCE)make -C iniparser default

	@echo "nanomsg release ..."
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/inc/nanomsg
	$(SILENCE)cp nanomsg/src/*.h $(AM_BUILD_TOP)/target/inc/nanomsg
	$(SILENCE)cp nanomsg/.libs/libnanomsg.a $(AM_BUILD_TOP)/target/lib
	@echo "nanomsg done ..."


utils :
	@echo "utils build ..."
	$(SILENCE)make -C utils
	$(SILENCE)mkdir -p $(AM_BUILD_TOP)/target/inc/utils
	$(SILENCE)cp utils/*.h $(AM_BUILD_TOP)/target/inc/utils
	$(SILENCE)cp utils/libutils.a $(AM_BUILD_TOP)/target/lib
	@echo "utils done ..."

clean :
	$(SILENCE)rm -rf iniparser
	$(SILENCE)rm -rf nanomsg
	$(SILENCE)make -C ./utils clean showcommand=$(showcommand)

