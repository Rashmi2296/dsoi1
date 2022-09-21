#!/bin/ksh
########################################################################
#                                                                      #
# shutdownMQSeries.ksh: Shut down Queue Manager and Channels for DSOI  #
#                                                                      #
# Synopsis:                                                            #
#  shutdownMQSeries.ksh [DSOI base directory]                          #
#                                                                      #
########################################################################

MQS_HOME=/opt/mqm
MQS_BIN=/usr/bin
DAY=`date +%m%d`
MQSINI_FILE=/var/mqm/mqs.ini


####################################
# Ignore these signals
####################################

trap "" 0 1 2 3 15


####################################
# Get command line options
####################################

if [ $# -eq 1 ]; then
	DSOI_BASE=$1
else
	DSOI_BASE=/opt/DSOI
fi


############################################
# Look for the shutdown log directory
###########################################

if [ -r $DSOI_BASE ]; then
	DSOI_SHUTDOWN_FILE=$DSOI_BASE/logfiles/DSOI_${DAY}_shutdown.log
	echo "The shutdown log file is '$DSOI_SHUTDOWN_FILE'"
else
	echo "The DSOI base directory '$DSOI_BASE' was not found" >&2
	exit 1
fi


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


####################################
# Add MQSeries bin directory to PATH
####################################

export PATH=${PATH}:${MQS_BIN}


####################################
# Stop any leftover runmqsc processes
####################################

# RUNMQSC_PROCESSES=`ps -ef| grep runmqsc | grep -v grep |wc -l`

# if [[ $RUNMQSC_PROCESSES -gt 0 ]]; then
#	kill -9 `ps -ef |grep "runmqsc" | grep -v grep | awk '{print $2}'`
# fi


####################################
# Stop the Queue Manager
####################################

if [[ `ps -ef | grep amq | grep $QMGR_NAME | wc -l` -gt 0 ]]; then
	# qmgr is running and needs to be shut down

	if [ -r $DSOI_SHUTDOWN_FILE ]; then
		# there was a previous shutdown attempt (evidenced by shutdown log)

		echo; echo "Requesting immediate shutdown for Queue Manager $QMGR_NAME..." | tee -a $DSOI_SHUTDOWN_FILE
		$MQS_BIN/endmqm -i $QMGR_NAME

#		until [[ `ps -ef | grep amq | grep $QMGR_NAME | wc -l` -eq 0 ]]; do
#			continue
#		done

		echo; echo "Queue manager $QMGR_NAME shutdown complete" | tee -a $DSOI_SHUTDOWN_FILE

	else
		echo; echo "Requesting shutdown for Queue Manager $QMGR_NAME..." | tee $DSOI_SHUTDOWN_FILE
		$MQS_BIN/endmqm $QMGR_NAME
	
		# wait a while for the queue manager to shut down; force the shut down if it takes too long
	
		ELAPSED_TIME=0
		INTERVAL=5	# 5 seconds
		LIMIT=300	# 5 minutes
	
		sleep 60
#		until [[ `ps -ef | grep amq | grep $QMGR_NAME | wc -l` -eq 0  ||  $ELAPSED_TIME -ge $LIMIT ]]; do
#			sleep $INTERVAL
#			(( ELAPSED_TIME += INTERVAL ))
#		done
	
#		if [[ $ELAPSED_TIME -ge $LIMIT ]]; then
#			# still running; force the shutdown
#	
#			echo; echo "Requesting immediate shutdown for $QMGR_NAME..." | tee -a $DSOI_SHUTDOWN_FILE
#			$MQS_BIN/endmqm -i $QMGR_NAME
	
#			until [[ `ps -ef | grep amq | grep $QMGR_NAME | wc -l` -eq 0 ]]; do
#				continue
#			done
#		fi
	
		$MQS_BIN/endmqm -i $QMGR_NAME
		echo; echo "Queue manager $QMGR_NAME shutdown complete" | tee -a $DSOI_SHUTDOWN_FILE
	fi

else
	echo; echo "Queue manager $QMGR_NAME is not running; shutdown complete" | tee -a $DSOI_SHUTDOWN_FILE
fi

# kill -9 `ps -ef | grep "runmqtrm" | grep -v grep | grep $QMGR_NAME | awk '{print $2}'`
kill -9 `ps -ef | grep "shutdownMQSeries" | grep -v grep | awk '{print $2}'`
$MQS_BIN/endmqm -i $QMGR_NAME

####################################
# Stop any runmqchi processes
####################################
RUNMQCHI_PROCESSES=`ps -ef | grep runmqchi | grep -v grep | grep $QMGR_NAME | wc -l`

if [[ $RUNMQCHI_PROCESSES -gt 0 ]]; then
     kill -9 `ps -ef | grep "runmqchi" | grep -v grep | grep $QMGR_NAME | awk '{print $2}'`
fi
if [[ $QMGR_NAME = "DOTSEROQM1" ]]; then
	RUNMQCHI_PROCESSES=`ps -ef | grep runmqchi | grep -v grep | grep DOTSEROQM3 | wc -l`

	if [[ $RUNMQCHI_PROCESSES -gt 0 ]]; then
      		kill -9 `ps -ef | grep "runmqchi" | grep -v grep | grep DOTSEROQM3 | awk '{print $2}'`
	fi
fi

if [[ $QMGR_NAME = "SUBLIMEQM2" ]]; then
	RUNMQCHI_PROCESSES=`ps -ef | grep runmqchi | grep -v grep | grep SUBLIMEQM1 | wc -l`

	if [[ $RUNMQCHI_PROCESSES -gt 0 ]]; then
      		kill -9 `ps -ef | grep "runmqchi" | grep -v grep | grep SUBLIMEQM1 | awk '{print $2}'`
	fi
fi

if [[ $QMGR_NAME = "DOTSEROQM1" ]]; then
     echo "Do not remove shared memory/semaphores for DOTSEROQM1" >&2
     exit 0
fi
if [[ $QMGR_NAME = "KIDFROSTQM1" ]]; then
     echo "Do not remove shared memory/semaphores for KIDFROSTQM1" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMP96JQM" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMP96JQM" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMP97JQM" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMP97JQM" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMD73IQM2" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMD73IQM2" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMT70IQM" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMT70IQM" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMT74IQM2" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMT74IQM2" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMT78IQM" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMT78IQM" >&2
     exit 0
fi
if [[ $QMGR_NAME = "SUOMT00KQM" ]]; then
     echo "Do not remove shared memory/semaphores for SUOMT00KQM" >&2
     exit 0
fi
if [[ $QMGR_NAME = "HPOMT793QM2" ]]; then
     echo "Do not remove shared memory/semaphores for HPOMT793QM2" >&2
     exit 0
fi
exit 0
####################################
# Remove all shared memory and semaphores belonging to user
####################################

# give MQ a change to exit gracefully before we hammer it
sleep 10

TMP_FILE=/tmp/ipcrm.tmp
USERNAME=`whoami`
set -A IPC m s

echo; echo "Removing $USERNAME's shared memory and semaphores..." | tee -a $DSOI_SHUTDOWN_FILE

for ipc in ${IPC[*]}; do
	if [[ -z `ipcs | grep ^$ipc | grep $USERNAME` ]]; then
		print "No ipc facilities of type $ipc for user $USERNAME"
		continue
	fi

	# construct the ipcrm statement with an argument list of ipc facilities to be removed
	# (sed does not seem to like variable substitutions for file names)

	ipcs | grep ^$ipc | grep $USERNAME | awk '{print $1" "$2;}' | sed -e '1i\
ipcrm' -e 'w /tmp/ipcrm.tmp' | sed -e '1!s/^.*/-&/' -e 'w /tmp/ipcrm.tmp' >&-

	ex - +%j\|wq $TMP_FILE

	# execute the ipcrm command

	chmod 744 $TMP_FILE
	cat $TMP_FILE
	$TMP_FILE
	rm -f $TMP_FILE
	print
done
/opt/mqm/bin/amqiclen -x

exit 0
