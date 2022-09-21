#######################################################
##  FILE:  Makefile
##
##  PURPOSE: To generate the commands to create the 
## 		DSOIMQ archive library.  It also
##		allows the user to clean all .o files and
##		remove the executable in the $INSTALLDIR directory.
##
##  RELATED HEADER FILES:
##
##  REVISION HISTORY:
##  version	date	  author	 description
##  ------------------------------------------------------
##  7.00.00 12/23/99      rfuruta     initial development
##  7.01.01 05/10/00      bxcox       Upgrade to aCC compiler
##
##		CONFIDENTIAL
##  Disclose and Distribute Solely To Employees Of 
##  U S West and Its Affiliates Having A Need To Know
###########################################################

.SUFFIXES:          ## reset all default suffixes to not be used
.SUFFIXES: .C .o .H ##suffix rules for .C .o. and .H files only

SHELL	      = /usr/bin/ksh
OOCC          = /opt/SUNWspro/bin/CC
CFLAGS	      = +p -g
INCLUDEPATH   = -I../inc -I/opt/mqm/inc
DEST	      = lib
EXTHDRS	      =
INSTALL_LIB   = ../lib
LIBS	      = /opt/mqm/lib
ARCHIVE	      = libDSOIMQ.a
#SHLIB         = libDSOIMQ.sl
MAKEFILE      = Makefile
MQDIR         = /opt/mqm/lib  ##include the MQSeries library
MQLIBS        = mqm           ##include the MQSeries directory

SRCS	      = DSOIerrorHandler.C \
		DSOIreceiveMQ.C \
		DSOItoolbox.C \
		DSOIconnectMQ.C \
		DSOIsendMQ.C \
                DSOIconstants.C

OBJS	      = DSOIerrorHandler.o \
		DSOIreceiveMQ.o \
		DSOItoolbox.o \
		DSOIconnectMQ.o \
		DSOIsendMQ.o \
                DSOIconstants.o

.C.o:
	@echo
	@echo "******************************************"
	@echo " Compiling $< "
	@echo "******************************************"
	${OOCC} ${CFLAGS} ${INCLUDEPATH} -c -o $@ $<

all: $(ARCHIVE) install_dir make_install
#all: $(ARCHIVE) $(SHLIB) install_dir make_install

$(ARCHIVE): $(OBJS) 
	@echo
	@echo "******************************************"
	@echo " Creating archive library: $@ "
	@echo "******************************************"
	/usr/ccs/bin/ar rv $@ $(OBJS)

#$(SHLIB): $(OBJS)
#	@echo
#	@echo "******************************************"
#	@echo " Creating shared library: $@ "
#	@echo "******************************************"
#	${OOCC} -b -o $@ $(OBJS)

install:	
		make all

debug: 
	echo "Compiling for debug "
	${MAKE} "CFLAGS=+p +z -z +DAportable -DDEBUG -g0"

purify:
	${MAKE} "CFLAGS=+p +z -z +DAportable -DDEBUG -g0"

quantify:
	${MAKE}

clean:
	rm -f $(OBJS) core

clobber:
	rm -f $(OBJS) ../lib/${ARCHIVE} core 
#clobber:
#	rm -f $(OBJS) ../lib/${ARCHIVE} ../lib/${SHLIB} core 

depend:
	@mkmf -f $(MAKEFILE) ROOT=$(ROOT)

print:
	@$(PRINT) $(HDRS) $(SRCS)

install_dir:
	cd ..; \
	if [ ! -d $(DEST) ]; then \
	echo "Creating directory: $(DEST)" ; \
	mkdir -m 755 $(DEST); \
	fi ; \
	cd -

make_install: 
	mv $(ARCHIVE) ../$(DEST) ; \
#	mv $(ARCHIVE) $(SHLIB) ../$(DEST) ; \
	chmod 755 ../$(DEST)/*

