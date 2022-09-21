#!/bin/ksh
########################################################################
#                                                                      #
# startupMQSeries.ksh - Start Queue Manager and Channels for DSOI      #
#                                                                      #
# Synopsis :                                                           #
#  startupMQSeries.ksh                                                 #
#                                                                      #
########################################################################

MQS_HOME=/opt/mqm/bin
MQS_BIN=/usr/bin
MQSINI_FILE=/var/mqm/mqs.ini

####################################
# Ignore these signals
####################################
trap "" 0 1 2 3 15

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
# Add MQSeries bin directory to PATH
####################################
export PATH=${PATH}:${MQS_BIN}

####################################
# Start the Queue Manager
####################################
echo "Starting Queue Manager $QMGR_NAME..."
$MQS_BIN/strmqm $QMGR_NAME
RETURN_CODE=$?

if [[ $RETURN_CODE -eq 0 ]]; then
	echo "  Queue Manager $QMGR_NAME started\n"
else
	echo "  Could not start queue manager $QMGR_NAME, RC = $RETURN_CODE, startup aborted\n" >&2
	exit 1
fi

if [[ $QMGR_NAME = "DOTSEROQM1" ]]; then
	echo "Starting Queue Manager DOTSEROQM3..."
	$MQS_BIN/strmqm DOTSEROQM3
	RETURN_CODE=$?

	if [[ $RETURN_CODE -eq 0 ]]; then
		echo "  Queue Manager DOTSEROQM3 started\n"
	else
		echo "  Could not start queue manager DOTSEROQM3, RC = $RETURN_CODE, startup aborted\n" >&2
		exit 1
	fi
fi

if [[ $QMGR_NAME = "SUBLIMEQM2" ]]; then
	echo "Starting Queue Manager SUBLIMEQM1..."
	$MQS_BIN/strmqm SUBLIMEQM1
	RETURN_CODE=$?

	if [[ $RETURN_CODE -eq 0 ]]; then
		echo "  Queue Manager SUBLIMEQM1 started\n"
	else
		echo "  Could not start queue manager SUBLIMEQM1, RC = $RETURN_CODE, startup aborted\n" >&2
		exit 1
	fi
fi

####################################
# Stop any runmqchi processes
####################################
# RUNMQCHI_PROCESSES=`ps -ef | grep runmqchi | grep -v grep | grep $QMGR_NAME | wc -l`

# if [[ $RUNMQCHI_PROCESSES -gt 0 ]]; then
#	kill -9 `ps -ef | grep "runmqchi" | grep -v grep | grep $QMGR_NAME | awk '{print $2}'`
# fi

####################################
# Start the Channel initiator
####################################
echo "MQSeries will start Channel Initiator for Queue Manager $QMGR_NAME..."
## /opt/mqm/bin/runmqchi -m $QMGR_NAME &
## $MQS_HOME/runmqchi -q SYSTEM.CHANNEL.INITQ -m $QMGR_NAME &
## $MQS_HOME/runmqchi -m $QMGR_NAME &
## RETURN_CODE=$?
## if [[ $RETURN_CODE -eq 0 ]]; then
##        echo "Channel Initiator started for Queue Manager $QMGR_NAME..."
## else
##        echo "Could not start Channel Initiator for $QMGR_NAME, RC = $RETURN_CODE, startup aborted\n" >&2
## 	  exit 1
## fi

## $MQS_HOME/runmqchi  -m $QMGR_NAME &
## $MQS_BIN/runmqchi -q  QI_DSOI_CHL_INITQ -m $QMGR_NAME &
## $MQS_BIN/runmqchi -q  SYSTEM.CHANNEL.INITQ -m $QMGR_NAME &

## echo "Channels for Queue Manager $QMGR_NAME started"; echo

# bring the command prompt back after IBM does their thing
sleep 7; echo

exit 0
