#!/bin/ksh
################################################################################
#
# getDSOI.ksh - Retrieve the latest tar file and install scripts from a remote
#               host
#
# Usage: getDSOI.ksh
#
################################################################################

if [[ `whoami` != "dsoidmn" ]]; then
    print "You must be 'dsoidmn' to use this script" >&2
    print "Please logon as 'dsoidmn'"; print
    exit 1
fi


# setup

DSOI_DIR=/opt/DSOI
DSOIREL_DIR=/opt/DSOIrel
ERROR_LOG=/tmp/`basename $0`.$$

clear

while true; do
	print "Are you retrieving installation files for:"; print
	print "\t1) Integration test or system test"
	print "\t2) End-to-end test or production"
	print "\t3) Other (specify remote host and directory)"
	print "\tQ) Quit"; print
	print "Please select an option: \c"

	read OPTION

	case $OPTION in
		1)
			REMOTE_HOST=styx
			LOCAL_DIR=$DSOIREL_DIR/tar_files
			REMOTE_TARFILE_DIR=/usr/projects/tarfiles/DSOI
			REMOTE_SCRIPTS_DIR=
			break
			;;

		2)
			REMOTE_HOST=hpdnt496
			LOCAL_DIR=$DSOIREL_DIR/tar_files
			REMOTE_TARFILE_DIR=$LOCAL_DIR
			REMOTE_SCRIPTS_DIR=$DSOI_DIR/scripts
			break
			;;

		3)
			print
			print "Please enter remote host name: \c"
			read REMOTE_HOST
			print "Please enter remote tarfile directory: \c"
			read REMOTE_TARFILE_DIR
			print "Please enter remote scripts directory: \c"
			read REMOTE_SCRIPTS_DIR
			LOCAL_DIR=$DSOIREL_DIR/tar_files
			break
			;;

		[qQ])
			print; exit 0
			;;

		*)
			print "Invalid option: '$OPTION'"; print
			;;
	esac
done

print


# change directories on local host

if [[ -d $LOCAL_DIR ]]; then
	cd $LOCAL_DIR
else
	print "Creating directory $LOCAL_DIR..."
	mkdir -m 755 $LOCAL_DIR

	if [[ $? != 0 ]]; then
		print "Unable to create directory $LOCAL_DIR" >&2; print
		exit 1
	else
		cd $LOCAL_DIR
	fi
fi


# figure out which tar file and install scripts to get

TARFILE=`remsh $REMOTE_HOST cd $REMOTE_TARFILE_DIR \; ls -1t | grep .tar.Z | sed 1!d`

if [[ -z $TARFILE ]]; then
	print "Unable to read tar file from $REMOTE_HOST:$REMOTE_TARFILE_DIR" >&2; print
	exit 1
else
	print "The tarfile name is set to $TARFILE"; print
fi

if [[ ! -z $REMOTE_SCRIPTS_DIR ]]; then
	set -A SCRIPTS installDSOI.ksh
else
	# for integration and system test: getting tarfile from SCM machine, and DSOI scripts do not exist on that machine

	set -A SCRIPTS
fi

remsh $REMOTE_HOST cd $REMOTE_SCRIPTS_DIR \; ls ${SCRIPTS[*]} > /dev/null 2> $ERROR_LOG

if [[ -s $ERROR_LOG ]]; then
	for FILE in ${SCRIPTS[*]}; do
		if [[ ! -x $DSOIREL_DIR/$FILE ]]; then
			print "Install script $DSOIREL_DIR/$FILE not found,"
			print "please contact DSOI development to get the file."; print
			exit 1
		fi
	done

	set -A SCRIPTS
fi


# get the tar file and install scripts from remote host

print "Getting file $REMOTE_TARFILE_DIR/$TARFILE from $REMOTE_HOST..."
rcp $REMOTE_HOST:$REMOTE_TARFILE_DIR/$TARFILE $LOCAL_DIR

if [[ $? != 0 ]]; then
	print "Error copying file $TARFILE from $REMOTE_HOST:$REMOTE_TARFILE_DIR" >&2; print
	exit 1
else
	chmod 755 $TARFILE
fi

for FILE in ${SCRIPTS[*]}; do
	print "Getting file $REMOTE_SCRIPTS_DIR/$FILE from $REMOTE_HOST..."
	rcp $REMOTE_HOST:$REMOTE_SCRIPTS_DIR/$FILE $DSOIREL_DIR

	if [[ $? != 0 ]]; then
		print "Error copying file $FILE from $REMOTE_HOST:$REMOTE_SCRIPTS_DIR" >&2; print
		exit 1
	else
		chmod 755 $DSOIREL_DIR/$FILE
	fi
done

print; print "Remote copy successful"
print "Tarfile is in $LOCAL_DIR"
print "Install scripts are in $DSOIREL_DIR"; print
exit 0
