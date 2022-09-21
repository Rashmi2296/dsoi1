#######################################################
##  FILE:  Makefile
##
##  PURPOSE: To generate the commands to create the 
## 		routeRequestMQmain executable.  It also
##		allows the user to clean all .o files and
##		remove the executable in the $INSTALLDIR directory.
##
##  RELATED HEADER FILES:
##
##  REVISION HISTORY:
##  version	date	  author	 description
##  ------------------------------------------------------
##  1.00.00 06/05/97  lparks     original check in
##  2.00.00 10/03/97  jalee3     check in for phase II DSOI central
##  7.01.01 05/10/00  bxcox      Upgrade to aCC compiler
##
##		CONFIDENTIAL
##  Disclose and Distribute Solely To Employees Of 
##  U S West and Its Affiliates Having A Need To Know
###########################################################

.SUFFIXES:	    ## reset all default suffixes to not be used
.SUFFIXES: .C .o .H ##suffix rules for .C .o. and .H files only

SHELL       = /bin/ksh
OOCC        = /opt/SUNWspro/bin/CC
CFLAGS      = +p -g
INCLUDEPATH = -I../inc -I../../inc -I../../log/inc -I/opt/mqm/inc
MQLIB       = mqm		##include the MQSeries library
MQDIR       = /opt/mqm/lib	##include the MQSeries directory
DSOIMQDIR   = ../../lib
DSOIMQLIB   = DSOIMQ
# PURIFY      = /opt/purify/purify
PURIFY      = /opt/rational/releases/purify.hp.2002.05.00/purify
QUANTIFY    = /opt/quantify/quantify
OBJECTPATH  = ../objects
EXECNAME    = DSOIRequest 
INSTALLDIR  = ../../bin

OBJECTS     = DSOImainRequest.o \
              DSOImessageRequest.o \
              DSOIlogMQ.o

$(EXECNAME): $(OBJECTS)
	@echo
	@echo "*****************************************"
	@echo " Linking $@ "
	@echo "*****************************************"
	${OOCC} $(CFLAGS) -o $(EXECNAME) $(OBJECTS) -L${MQDIR} -l${MQLIB} -L$(DSOIMQDIR) -l$(DSOIMQLIB) 

.C.o: 
	@echo
	@echo "*****************************************"
	@echo " Compiling $< "
	@echo "*****************************************"
	${OOCC} ${CFLAGS} ${INCLUDEPATH} -c -o $@ $<

install:
	make links; \
	make $(EXECNAME); \
	echo "Copying $(EXECNAME) "; \
        cp ${EXECNAME} ${INSTALLDIR}; \
	cd ${INSTALLDIR}; \
	chmod 755 ${EXECNAME}; \
	chown ${LOGNAME}:users ${EXECNAME}

debug:  
	${MAKE} "CFLAGS=+p -z +DAportable -DDEBUG -g0"
	
purify:
	${PURIFY} -always-use-cache-dir -cache-dir=/u/mqm/tmp ${OOCC} $(CFLAGS) -DDEBUG -g0 -o $(EXECNAME) $(OBJECTS) -L$(MQDIR) -l${MQLIB} -L${DSOIMQDIR} -l${DSOIMQLIB}

quantify:
	${QUANTIFY} -cache-dir=${HOME}/tmp ${OOCC} $(CFLAGS) -o $(EXECNAME) $(OBJECTS) -L$(MQDIR) -l${MQLIB} -L${DSOIMQDIR} -l${DSOIMQLIB}

clean  :		# clean only when specified    
	rm -f $(OBJECTS) core

clobber:	# clobber only when specified on command line
	rm -f $(OBJECTS) core ; \
	rm -f ${INSTALLDIR}/${EXECNAME} ; \
	rm -rf ${INSTALLDIR}

links:
	if [ ! -L DSOIlogMQ.C ]; then \
		ln -s ../../log/src/DSOIlogMQ.C DSOIlogMQ.C ; \
	fi

