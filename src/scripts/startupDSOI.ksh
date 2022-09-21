#!/bin/ksh
################################################################################
#                                                                              #
# startupDSOI.ksh - Start Trigger Monitor for DSOI                             #
#                                                                              #
# Synopsis :                                                                   #
#  startupDSOI.ksh                                                             #
#  startupDSOI.ksh [ BaseDirectory ]                                           #
#                                                                              #
#   Note: BaseDirectory is an optional parameter used to support               #
#         installation in a non-production environment.  It is not             #
#         required for the production environment.                             #
#                                                                              #
################################################################################
export AMQ_SIGCHLD_SIGACTION=YES

MQS_HOME=/opt/mqm
MQS_BIN=/usr/bin
DAY_TIME=`date +%m%d%H%M`
MQSINI_FILE=/var/mqm/mqs.ini

####################################
# Get command line options
####################################

if [ $# -eq 1 ]; then
	DSOI_BASE=$1
else
	DSOI_BASE=/opt/DSOI
fi

echo "The DSOI base directory is set to '${DSOI_BASE}'"

############################################
# Look for the trigger monitor log directory 
###########################################
if [ -r ${DSOI_BASE} ]; then
	DSOI_TRIGGER_LOG_FILE=${DSOI_BASE}/logfiles/DSOI_${DAY_TIME}_MQTrigger.log
	echo "The trigger monitor log file is '${DSOI_TRIGGER_LOG_FILE}'"
else
	echo "The DSOI base directory '${DSOI_BASE}' was not found" >&2
        exit 1
fi

############################################
# Resolve relative path issues by executing
# from $DSOI_BASE/scripts directory
###########################################
cd $DSOI_BASE/scripts

####################################
# Set the queue manager name
####################################

# Set the queue manager name

HOSTNAME=`hostname`

case $HOSTNAME in

        dsoi-ne1)
                QMGR_NAME=DSOINE1QM
                ;;

        dsoi-ne2)
                QMGR_NAME=DSOINE2QM
                ;;

	hpdnt496)
		QMGR_NAME=KIDFROSTQM1
		;;

	kidfrost)
		QMGR_NAME=KIDFROSTQM1
		;;

	hpdnt490)
		QMGR_NAME=DOTSEROQM1
		;;

	meatloaf)
		QMGR_NAME=DOTSEROQM1
		;;

        suomp96j)
                QMGR_NAME=SUOMP96JQM
                ;;

        suomp97j)
                QMGR_NAME=SUOMP97JQM
                ;;

	sublime)
                QMGR_NAME=SUBLIMEQM2
                ;;

	suomd73i)
                QMGR_NAME=SUOMD73IQM2
                ;;

	suomt70i)
		QMGR_NAME=SUOMT70IQM
    		;;

        suomt74i)
		QMGR_NAME=SUOMT74IQM2
		;;

	suomt78i)
    		QMGR_NAME=SUOMT78IQM
   		;;

	suomt00k)
    		QMGR_NAME=SUOMT00KQM
   		;;

	hpomt793)
		QMGR_NAME=HPOMT793QM2
		;;

	*)
		echo "A DSOI MQSeries configuration is not defined for $HOSTNAME"
		echo "Select a QUEUE MANAGER on $HOSTNAME..."

		if [[ -r $MQSINI_FILE ]]; then
			set -A QMGRS `grep Name= $MQSINI_FILE | sort -u | cut -d = -f2`
		else
			echo "Unable to determine avaliable queue managers from file $MQSINI_FILE" >&2; echo
			exit 1
		fi

		echo "\t${QMGRS[*]}"; echo

		for i in ${QMGRS[*]}; do
			QMGR_NAME=`echo $i | cut -d = -f2`
			echo "Would you like $QMGR_NAME? [y/n] \c"
			read ANSWER

			if [[ $ANSWER = [yY] ]]; then
				echo
				export QMGR_NAME
				break
			else
				echo
			fi
		done

		if [[ $ANSWER != [yY] ]]; then
			echo "A queue manager has not been selected. Goodbye" >&2; echo
			exit 1
		fi
		;;
esac

echo "The queue manager name is set to $QMGR_NAME"

####################################
# Ignore these signals
####################################
trap "" 0 1 2 3 15

####################################
# Add MQSeries bin directory to PATH
####################################
export PATH=${PATH}:${MQS_BIN}

####################################
# Start the Trigger Monitor
####################################
echo; echo "Starting DSOI Trigger Monitor for Queue Manager $QMGR_NAME..."

## $MQS_BIN/runmqtrm -m $QMGR_NAME -q QI_DSOI_INIT > $DSOI_TRIGGER_LOG_FILE 2>&1 &
$MQS_BIN/runmqtrm -m $QMGR_NAME -q SYSTEM.DEFAULT.INITIATION.QUEUE > $DSOI_TRIGGER_LOG_FILE 2>&1 &

RETURN_CODE=$?
if [[ $RETURN_CODE -eq 0 ]]; then
        echo "DSOI Trigger Monitor started for Queue Manager $QMGR_NAME";  echo
else
        echo "Could not start Trigger Monitor for $QMGR_NAME, RC = $RETURN_CODE";  echo 
        exit 1
fi

if [[ $QMGR_NAME = "DOTSEROQM1" ]]; then
##        $MQS_BIN/runmqtrm -m DOTSEROQM3 -q QI_DSOI_INIT > $DSOI_TRIGGER_LOG_FILE 2>&1 &
          $MQS_BIN/runmqtrm -m DOTSEROQM3 -q SYSTEM.DEFAULT.INITIATION.QUEUE > $DSOI_TRIGGER_LOG_FILE 2>&1 &
	  RETURN_CODE=$?
	  if [[ $RETURN_CODE -eq 0 ]]; then
        	echo "DSOI Trigger Monitor started for Queue Manager DOTSEROQM3";  echo
	  else
        	echo "Could not start Trigger Monitor for DOTSEROQM3, RC = $RETURN_CODE";  echo 
          	exit 1
	  fi

fi

if [[ $QMGR_NAME = "SUBLIMEQM2" ]]; then
##        $MQS_BIN/runmqtrm -m SUBLIMEQM1 -q QI_DSOI_INIT > $DSOI_TRIGGER_LOG_FILE 2>&1 &
          $MQS_BIN/runmqtrm -m SUBLIMEQM1 -q SYSTEM.DEFAULT.INITIATION.QUEUE > $DSOI_TRIGGER_LOG_FILE 2>&1 &
	  RETURN_CODE=$?
	  if [[ $RETURN_CODE -eq 0 ]]; then
        	echo "DSOI Trigger Monitor started for Queue Manager SUBLIMEQM1";  echo
	  else
        	echo "Could not start Trigger Monitor for SUBLIMEQM1, RC = $RETURN_CODE";  echo 
          	exit 1
	  fi

fi



exit 0

