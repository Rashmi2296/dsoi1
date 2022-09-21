#!/bin/ksh
########################################################################
#
# setupMQSeries.ksh - Create MQ Series objects for DSOI
#
# Synopsis :
#  setupMQSeries.ksh [ BaseDirectory ]
#
#   Note: BaseDirectory is an optional parameter used to support
#         installation in a non-production environment.  It is not
#         required for the production environment.
#
########################################################################


####################################
# Set the locale character type
####################################

export LC_CTYPE=`locale -a | grep -i 'en.*us.*iso'`


####################################
# Get command line options
####################################

if [ $# -eq 1 ]; then
	DSOI_BASE=$1
else
	DSOI_BASE=/opt/DSOI
fi

echo "The DSOI base directory is set to '$DSOI_BASE'"

MQS_LOG_FILE=$DSOI_BASE/logfiles/DSOI_MQSetup.log
MQS_HOME=/opt/mqm
MQSINI_FILE=/var/mqm/mqs.ini


####################################
# Set the queue manager name
####################################

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
# Ask if the current queue manager
# should be deleted
####################################

echo " "
echo " "
echo "You need to decide if the current queue manager:"
echo " "
echo "  A) should be retained and reconfigured"
echo "  B) should be deleted, recreated and reconfigured"
echo "     Note: If you choose option B: "
echo "             1) If messages exist in any of the current"
echo "                queues they will be deleted."
echo "             2) After the new queue manager has been"
echo "                created and configured, all channels on"
echo "                remote systems that are connected to this"
echo "                queue manager will need to reset their"
echo "                sequence numbers to 1."
echo "  Q) quit MQSeries setup"
echo " "
echo "Do you want option A, B or Q (Q is the default)? \c"
read QMGR_OPTION

if [[ $QMGR_OPTION = [aA] ]]; then
	FROM_SCRATCH=N
	echo; echo "The queue manager $QMGR_NAME retained and reconfigured"
elif [[ $QMGR_OPTION = [Bb] ]]; then
	FROM_SCRATCH=Y
	echo; echo "The queue manager $QMGR_NAME will be deleted, recreated and reconfigured"
else
	echo; echo "MQ setup aborted by user"
	exit 0
fi


####################################
# Look for the MQSeries object file
####################################

if [ -r $DSOI_BASE ]; then
	## MQOBJ_FILE=$DSOI_BASE/etc/$QMGR_NAME.obj

	## if [ -r $MQOBJ_FILE ]; then
	##	echo; echo "The DSOI MQSeries setup file '$MQOBJ_FILE' will be used"
	## else
	##	echo; echo "The DSOI MQSeries setup file '$MQOBJ_FILE' was not found" >&2
    	##	exit 1
	## fi
	echo; echo "The Object file is not needed, not going to reload file " >&2
else
	echo; echo "The DSOI base directory '$DSOI_BASE' was not found" >&2
	exit 1
fi


####################################
# Look for the MQSeries object file
####################################

if [ -r $DSOI_BASE ]; then
        ## MQOBJ_FILE=$DSOI_BASE/etc/${QMGR_NAME}.obj

        ## if [ -r $MQOBJ_FILE ]; then
        ##        echo "The DSOI MQSeries setup file '$MQOBJ_FILE' will be used"
        ## else
        ##        echo; echo "The DSOI MQSeries setup file '$MQOBJ_FILE' was not found" >&2
	##	  exit 1
        ## fi
	echo; echo "The Object file is not needed, not going to reload file " >&2
else
        echo; echo "The DSOI base directory '$DSOI_BASE' was not found" >&2
        exit 1
fi


####################################
# Delete the queue manager
# ...if requested to do so
####################################

if [[ $FROM_SCRATCH = 'Y' ]]; then
	echo "Deleting Queue Manager $QMGR_NAME..."
	dltmqm $QMGR_NAME > $MQS_LOG_FILE 2>&1
	RETURN_CODE=$?
    
	if [[ $RETURN_CODE -eq 0 ]]; then
		echo "Queue Manager $QMGR_NAME deleted\n"
	elif [[ $RETURN_CODE -eq 16 ]]; then
		echo "Queue Manager $QMGR_NAME does not exist\n"
	elif [[ $RETURN_CODE -eq 5 ]]; then
		echo; echo "$QMGR_NAME currently running, setup aborted" >&2
		exit 1
	else
		echo; echo "Could not delete queue manager $QMGR_NAME, RC = $RETURN_CODE, setup aborted" >&2
		exit 1
	fi

else
	echo "Reconfiguring DSOI Queue Manager '$QMGR_NAME'..."
fi


####################################
# Create the queue manager,
# as well as default and system MQ objects
####################################

if [[ -d /var/mqm/qmgrs/$QMGR_NAME ]]; then
	echo "$QMGR_NAME found"
else
	echo "Creating default Queue Manager $QMGR_NAME..."
	#crtmqm -c "Queue Manager for DSOI" $QMGR_NAME >> $MQS_LOG_FILE 2>&1
	crtmqm -c "Queue Manager for DSOI" -q $QMGR_NAME >> $MQS_LOG_FILE 2>&1
	RETURN_CODE=$?
    
	if [[ $RETURN_CODE -eq 0 ]]; then
		echo "Queue Manager $QMGR_NAME created\n"
	else
		echo; echo "Could not create queue manager $QMGR_NAME, RC = $RETURN_CODE, setup aborted" >&2
		exit 1
	fi
fi


####################################
# Start the queue manager
####################################

echo "Starting Queue Manager $QMGR_NAME..."

strmqm $QMGR_NAME >> $MQS_LOG_FILE 2>&1
RETURN_CODE=$?

if [[ $RETURN_CODE -eq 0 ]]; then
	echo "Queue Manager $QMGR_NAME started\n"
else
	echo; echo "Could not start queue manager $QMGR_NAME, RC = $RETURN_CODE, setup aborted" >&2
	exit 1
fi


####################################
# Create the MQSeries default objects if MQ 2.x
####################################

if [ -r /opt/mqm/README ]; then
	echo "MQSeries Version 2"
	echo "Creating default objects for Queue Manager $QMGR_NAME..."
	runmqsc $QMGR_NAME < /opt/mqm/samp/amqscoma.tst > $DSOI_BASE/logfiles/amqscoma.out 2>&1
	RETURN_CODE=$?

	if [[ $RETURN_CODE -eq 0 ]]; then
		echo "Default objects for Queue Manager $QMGR_NAME created"; echo
	else
		echo "Could not create default objects for queue manager $QMGR_NAME, RC = $RETURN_CODE"
		echo "Setup aborted"
		exit 1
	fi
fi


####################################
# Create the DSOI MQSeries objects
####################################

## echo "Creating DSOI objects for Queue Manager $QMGR_NAME..."
## runmqsc $QMGR_NAME < $MQOBJ_FILE > $DSOI_BASE/logfiles/${QMGR_NAME}.log 2>&1
## RETURN_CODE=$?

## if [[ $RETURN_CODE -eq 0 ]]; then
##	echo "DSOI objects for Queue Manager $QMGR_NAME created\n"
## else
	# Continue with setup becuase qmgr needs to shutdown

##	echo; echo "Could not create one or more DSOI objects for queue manager $QMGR_NAME, RC = $RETURN_CODE." >&2
##	echo "Please review logfile $DSOI_BASE/logfiles/${QMGR_NAME}.log and contact development." >&2
##	echo "Continuing with MQSeries setup..." >&2
## fi
echo; echo "The Object file does not have to be loaded,it was done in a previous release" >&2


####################################
# Shutdown the queue manager
####################################

echo "Stopping Queue Manager $QMGR_NAME..."
echo "(executing shutdown script $DSOI_BASE/scripts/shutdownMQSeries.ksh)"

$DSOI_BASE/scripts/shutdownMQSeries.ksh
RETURN_CODE=$?

if [[ $RETURN_CODE -ne 0 ]]; then
	echo; echo "Shutdown script failed with returnCode = $RETURN_CODE" >> $MQS_LOG_FILE 2>&1
	echo "Please execute shutdown script manually as user mqm" >> $MQS_LOG_FILE 2>&1
	exit 1
fi


echo; echo "MQSeries setup successful"; echo
exit 0
