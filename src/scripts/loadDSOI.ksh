#! /bin/ksh
################################################################################
#
# loadDSOI.ksh: A shell script to install DSOI in one step. Works by
#               calling all other install scripts in sequence.
#
# Usage: loadDSOI.ksh
#
################################################################################

if [[ `whoami` != "dsoidmn" ]]; then
	print "You must be 'dsoidmn' to use this script" >&2
	print "Please logon as 'dsoidmn'"
	exit 1
fi

trap "" 0 1 2 3 15


# if DSOI_BASE is not set, use /opt/DSOI as default

: ${DSOI_BASE:=/opt/DSOI}


# if DSOIREL_DIR is not set, use /opt/DSOIrel as default

: ${DSOIREL_DIR:=/opt/DSOIrel}


# Initialize all shell variables

SCRIPTS_DIR=$DSOI_BASE/scripts
REMOTE_HOST=`hostname`
USER=`whoami`
set -A RHOSTS_FILES ~dsoidmn/.rhosts ~mqm/.rhosts
REMOTE_TARFILE_DIR=$DSOIREL_DIR/tar_files
MQSINI_FILE=/var/mqm/mqs.ini
ERROR_LOG=/tmp/`basename $0`.$$


# Set the queue manager name

HOSTNAME=`hostname`

clear
print "Installing DSOI on $HOSTNAME..."; print

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
		print "A DSOI MQSeries configuration is not defined for $HOSTNAME"
		print "Select a QUEUE MANAGER on $HOSTNAME..."

		if [[ -r $MQSINI_FILE ]]; then
			set -A QMGRS `grep Name= $MQSINI_FILE | sort -u | cut -d = -f2`
		else
			print "Unable to determine avaliable queue managers from file $MQSINI_FILE" >&2; print
			exit 1
		fi

		print "\t${QMGRS[*]}"; print

		for i in ${QMGRS[*]}; do
			QMGR_NAME=`print $i | cut -d = -f2`
			print "Would you like $QMGR_NAME? [y/n] \c"
			read ANSWER

			if [[ $ANSWER = [yY] ]]; then
				print
				export QMGR_NAME
				break
			else
				print
			fi
		done

		if [[ $ANSWER != [yY] ]]; then
			print "A queue manager has not been selected. Goodbye" >&2; print
			exit 1
		fi
		;;
esac

print "The queue manager name is set to $QMGR_NAME"


# check if valid .rhosts file exists for users dsoidmn and mqm

for FILE in ${RHOSTS_FILES[*]}; do
	if [[ ! -r $FILE ]]; then
		print "File $FILE must exist and contain the following entry:" >&2
		print "$REMOTE_HOST $USER" >&2; print
		exit 1
	else
		grep $REMOTE_HOST $FILE | grep -q $USER

		if [[ $? != 0 ]]; then
			print "File $FILE must exist and contain the following entry:" >&2
			print "$REMOTE_HOST $USER" >&2; print
			exit 1
		fi
	fi
done


#-------------------------------------------------------------------------------

function print_step_header {
	# display information to the user at the beginning of each step in the install process

	: ${step:=1}
	(( step > 1 )) && clear

	print
	print - "-------------------------------------------"
	print   "   Executing $FILE..."
	print   "   (step $step of 5)"
	print - "-------------------------------------------"
	print

	(( step += 1 ))
}

function print_step_footer {
	# continue with the next step or quit, per user

	print; print "Press ENTER to continue the install, or Q to quit: \c"
	read ANSWER

	if [[ $ANSWER = [qQ] ]]; then
		print "Goodbye"; print
		exit 0
	fi
}

function process_errors {
	if [[ -s $ERROR_LOG ]]; then
		# process any errors from the remsh'd script, and terminate

		print; print "Contents of error log:"
		cat $ERROR_LOG
		print; print "Unable to execute remote command, install aborted" >&2; print
		rm -f $ERROR_LOG
		exit 1
	else
		# no errors
		print_step_footer
	fi
}


#-------------------------------------------------------------------------------

# execute each install script in sequence

# getDSOI.ksh 
FILE=getDSOI.ksh 
print_step_header

if [[ -x $DSOIREL_DIR/$FILE ]]; then
	# execute the newest file

	if [[ $SCRIPTS_DIR/$FILE -nt $DSOIREL_DIR/$FILE ]]; then
		remsh $REMOTE_HOST $SCRIPTS_DIR/$FILE 2> $ERROR_LOG
	else
		remsh $REMOTE_HOST $DSOIREL_DIR/$FILE 2> $ERROR_LOG
	fi
else
	print "File $FILE not found" >&2
	print "Please contact DSOI development to get the file" >&2; print
	exit 1
fi

TARFILE=`ls -1t $REMOTE_TARFILE_DIR | grep .tar.Z | sed 1!d`

if [[ -z $TARFILE ]]; then
	print "Unable to read tar file from $REMOTE_HOST:$REMOTE_TARFILE_DIR" >&2; print
	exit 1
fi

process_errors


#---------------------------------------

# shutdownMQSeries.ksh
FILE=shutdownMQSeries.ksh
print_step_header

# The MQ command endmqm (called from within shutdownMQSeries.ksh) writes an informative message to stderr, so we can't trap errors (no 2>$ERROR_LOG)

if [[ -x $DSOIREL_DIR/$FILE ]]; then
	# execute the newest file


	if [[ $SCRIPTS_DIR/$FILE -nt $DSOIREL_DIR/$FILE ]]; then
		remsh $REMOTE_HOST -l mqm $SCRIPTS_DIR/$FILE
	else
		remsh $REMOTE_HOST -l mqm $DSOIREL_DIR/$FILE
	fi
else
	ps -ef | grep $QMGR_NAME | grep -q -v grep 2>&-
	
	if [[ $? = 0 ]]; then
		# queue manager is still running

		if [[ -x $SCRIPTS_DIR/$FILE ]]; then
			remsh $REMOTE_HOST -l mqm $SCRIPTS_DIR/$FILE
		else
			# the shutdown script is not in the DSOIREL_DIR or the SCRIPTS_DIR
	
			print "File $FILE not found" >&2
			print "Queue manager is still running, unable to proceed" >&2
			print "Please contact DSOI development to get the file" >&2; print
			exit 1
		fi
	else
		# might be a first-time install

		print "Queue manager is not running, please proceed"; print
	fi
fi

process_errors


#---------------------------------------

# installDSOI.ksh <tarfile>
FILE=installDSOI.ksh
print_step_header

if [[ -x $DSOIREL_DIR/$FILE ]]; then
	# execute the newest file

	# The crontab command (called from within installDSOI.ksh) writes an informative message to stderr, so we can't trap errors (no 2>$ERROR_LOG)

	if [[ $SCRIPTS_DIR/$FILE -nt $DSOIREL_DIR/$FILE ]]; then
		remsh $REMOTE_HOST $SCRIPTS_DIR/$FILE $REMOTE_TARFILE_DIR/$TARFILE
	else
		remsh $REMOTE_HOST $DSOIREL_DIR/$FILE $REMOTE_TARFILE_DIR/$TARFILE
	fi
else
	print "File $FILE not found" >&2
	print "Please contact DSOI development to get the file" >&2; print
	exit 1
fi

process_errors


#---------------------------------------

# startupMQSeries.ksh
FILE=startupMQSeries.ksh
print_step_header

# startupMQSeries.ksh does not return a command prompt when run via remsh; requires special handling (-n, &); runmqchi (called from within startupMQSeries.ksh) writes an informative message to stderr, so we can't trap errors (no 2>$ERROR_LOG)

remsh $REMOTE_HOST -l mqm -n $SCRIPTS_DIR/$FILE &

# allow time for the script to finish, then bring back the command prompt
sleep 20; print

process_errors


#---------------------------------------

# startupDSOI.ksh
FILE=startupDSOI.ksh
print_step_header

# startupDSOI.ksh does not return a command prompt when run via remsh; requires special handling (-n, &)

remsh $REMOTE_HOST -n $SCRIPTS_DIR/$FILE 2> $ERROR_LOG &

# allow time for the script to finish, then bring back the command prompt
sleep 10; print

process_errors


#---------------------------------------

print; print "DSOI install complete. Have a nice day!"; print
exit 0
