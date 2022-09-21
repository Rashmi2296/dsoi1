#######################################################
##  FILE:  Makefile
##
##  PURPOSE: To generate the commands to create the 
## 		DSOI application executables.  It also
##		allows the user to clean all .o files and
##		remove the executable in the $INSTALLDIR directory.
##
##  RELATED HEADER FILES:
##
##  REVISION HISTORY:
##  version	date	  author	 description
##  ------------------------------------------------------
##  2.00.00 10/03/97  jalee3     DSOI 2.00 check in
##  7.00.00 01/17/00  rfuruta    DSOI 7.00 check in
##  7.00.02 04/26/00  bxcox      1. Reinstate correct permissions for
##                                  docs/cgi-bin/* and scripts/*
##                               2. Add rule to include checkQM in SCM tarfile
##  7.01.01 05/10/00  bxcox      Upgrade to aCC compiler
##  7.01.01 09/15/00  rfuruta    CR3452, add DSOIcleanUpQ
##  8.00.00 11/14/00  rfuruta    CR4767, remove 4P stuff
##  8.03.00 07/12/01  bxcox      Remove docs dir (web-based Report Utility is
##                               not used anymore).
##  9.04.00 02/12/04  rfuruta    CR8911,  add expryInfo
## 10.04.00 02/01/07  rfuruta    CR10999, migrate DSOI to Solaris
##
##		CONFIDENTIAL
##  Disclose and Distribute Solely To Employees Of 
##  U S West and Its Affiliates Having A Need To Know
###########################################################

SHELL             = /bin/ksh
IMAGE             = ../images
IMAGE_BIN_DIR     = $(IMAGE)/bin
#IMAGE_DOCS_DIR    = $(IMAGE)/docs
IMAGE_ETC_DIR     = $(IMAGE)/etc
IMAGE_LIB_DIR     = $(IMAGE)/lib
IMAGE_SCRIPTS_DIR = $(IMAGE)/scripts
BIN_DIR           = bin
#DOCS_DIR          = docs
ETC_DIR           = etc
LIB_DIR           = lib
SCRIPTS_DIR       = scripts
SRC_DIR           = src
DATAREF_DIR       = data_ref/src
LOG_DIR           = log/src
REPLY_DIR         = reply/src
REPORT_DIR        = report/src
REQUEST_DIR       = request/src
CLEANUP_DIR       = cleanUp/src


hdrs:

# DO NOT remove this rule.  It is for the SCM team to copy
# to an area that they require(image). If you  have a file,
# new executable, or directory that needs to be part of the tar file
# SCM releases then it is your responsibility to add the 
# file(s) or executable to this rule.
images:
	if [ ! -d $(IMAGE) ]; then \
	echo "creating $(IMAGE) "; \
	mkdir -m 777 $(IMAGE); \
	mkdir -m 777 $(IMAGE)/bin; \
	mkdir -m 777 $(IMAGE)/etc; \
	mkdir -m 777 $(IMAGE)/lib; \
	mkdir -m 777 $(IMAGE)/scripts; \
	fi; \
	cp $(BIN_DIR)/DSOIDRefLoader $(IMAGE_BIN_DIR)
	cp $(BIN_DIR)/DSOILog $(IMAGE_BIN_DIR)
	cp $(BIN_DIR)/DSOIReply $(IMAGE_BIN_DIR)
	cp $(BIN_DIR)/DSOIReport $(IMAGE_BIN_DIR)
	cp $(BIN_DIR)/DSOIRequest $(IMAGE_BIN_DIR)
	cp $(BIN_DIR)/DSOIcleanUpQ $(IMAGE_BIN_DIR)
	cp $(DATAREF_DIR)/npa_split $(IMAGE_ETC_DIR)
	cp $(DATAREF_DIR)/routeInfo $(IMAGE_ETC_DIR)
	cp $(DATAREF_DIR)/ownerInfo $(IMAGE_ETC_DIR)
	cp $(DATAREF_DIR)/rqstSopId $(IMAGE_ETC_DIR)
	cp $(DATAREF_DIR)/ParmFile $(IMAGE_ETC_DIR)
	cp $(DATAREF_DIR)/expryInfo $(IMAGE_ETC_DIR)
#	cp $(DOCS_DIR)/*config $(IMAGE_DOCS_DIR)
#	cp $(DOCS_DIR)/*js $(IMAGE_DOCS_DIR)
#	cp $(DOCS_DIR)/home.html $(IMAGE_DOCS_DIR)
#	cp $(DOCS_DIR)/cgi-bin/*pl $(IMAGE_DOCS_DIR)/cgi-bin
#	chmod 555 $(IMAGE_DOCS_DIR)/cgi-bin/*
#	cp $(DOCS_DIR)/images/*gif $(IMAGE_DOCS_DIR)/images
	cp $(ETC_DIR)/DSOI* $(IMAGE_ETC_DIR)
# 	cp $(ETC_DIR)/DOTSERO* $(IMAGE_ETC_DIR)
#	cp $(ETC_DIR)/KIDFROST* $(IMAGE_ETC_DIR)
#	cp $(ETC_DIR)/E2ENET7* $(IMAGE_ETC_DIR)
	cp $(ETC_DIR)/*cron $(IMAGE_ETC_DIR)
	cp $(LIB_DIR)/libDSOIMQ.a $(IMAGE_LIB_DIR)
#	cp $(LIB_DIR)/libDSOIMQ.sl $(IMAGE_LIB_DIR)
	cp $(SCRIPTS_DIR)/*.ksh $(IMAGE_SCRIPTS_DIR)
	cp $(SCRIPTS_DIR)/*.pl $(IMAGE_SCRIPTS_DIR)
	cp $(SCRIPTS_DIR)/checkQM $(IMAGE_SCRIPTS_DIR)
	cp $(SCRIPTS_DIR)/cleanUpHold $(IMAGE_SCRIPTS_DIR)
	chmod 555 $(IMAGE_SCRIPTS_DIR)/*

# If there is a new directory that has a Makefile that resides in it, 
# then update this rule with the new directory and Makefile.  
install:
	if [ ! -d $(BIN_DIR) ]; then \
	echo "creating $(BIN_DIR) "; \
	mkdir -m 777 $(BIN_DIR); \
	fi; \
	echo "Entering $(SRC_DIR)"; \
	cd $(SRC_DIR); \
	make install; \
	echo "Entering $(DATAREF_DIR)"; \
	cd ../$(DATAREF_DIR); \
	make install; \
	echo "Entering $(LOG_DIR)"; \
	cd ../../$(LOG_DIR); \
	make install; \
	echo "Entering $(REPLY_DIR)"; \
	cd ../../$(REPLY_DIR); \
	make install; \
	echo "Entering $(REPORT_DIR)"; \
	cd ../../$(REPORT_DIR); \
	make install; \
	echo "Entering $(REQUEST_DIR)"; \
	cd ../../$(REQUEST_DIR); \
	make install; \
	echo "Entering $(CLEANUP_DIR)"; \
	cd ../../$(CLEANUP_DIR); \
	make install

# Please leave this for SCM to use.
libs: install

libDSOIMQ.a:
	cd $(SRC_DIR); make install

#libDSOIMQ.sl:
#	cd $(SRC_DIR); make install

DSOIDRefLoader:
	cd $(DATAREF_DIR); make install

DSOILog:
	cd $(LOG_DIR); make install

DSOIReply:
	cd $(REPLY_DIR); make install

DSOIReport:
	cd $(REPORT_DIR); make install

DSOIRequest:
	cd $(REQUEST_DIR); make install

DSOIcleanUpQ:
	cd $(CLEANUP_DIR); make install

debug:
	cd $(SRC_DIR); make debug
	cd $(DATAREF_DIR); make debug
	cd $(LOG_DIR); make debug
	cd $(REPLY_DIR); make debug
	cd $(REPORT_DIR); make debug
	cd $(REQUEST_DIR); make debug
	cd $(CLEANUP_DIR); make debug

purify:
	cd $(SRC_DIR); make purify
	cd $(DATAREF_DIR); make purify
	cd $(LOG_DIR); make purify
	cd $(REPLY_DIR); make purify
	cd $(REPORT_DIR); make purify
	cd $(REQUEST_DIR); make purify
	cd $(CLEANUP_DIR); make purify

quantify:
	cd $(SRC_DIR); make quantify
	cd $(DATAREF_DIR); make quantify
	cd $(LOG_DIR); make quantify
	cd $(REPLY_DIR); make quantify
	cd $(REPORT_DIR); make quantify
	cd $(REQUEST_DIR); make quantify
	cd $(CLEANUP_DIR); make quantify

clean:
	cd $(SRC_DIR); make clean
	cd $(DATAREF_DIR); make clean
	cd $(LOG_DIR); make clean
	cd $(REPLY_DIR); make clean
	cd $(REPORT_DIR); make clean
	cd $(REQUEST_DIR); make clean
	cd $(CLEANUP_DIR); make clean

clobber:
	cd $(SRC_DIR); make clobber
	cd $(DATAREF_DIR); make clobber
	cd $(LOG_DIR); make clobber
	cd $(REPLY_DIR); make clobber
	cd $(REPORT_DIR); make clobber
	cd $(REQUEST_DIR); make clobber
	cd $(CLEANUP_DIR); make clobber
