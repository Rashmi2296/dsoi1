#! /bin/ksh
#############################################################
#
# installDSOI.ksh: A shell script to install DSOI
#
# Environment variables:
#   DSOI_BASE: The link to the current DSOI release. Default: /opt/DSOI
#   DSOIREL_DIR: Where to install DSOI. Default: /opt/DSOIrel
#
# Usage: installDSOI.ksh <tarfile>
#
#############################################################

if [ `whoami` != "dsoidmn" ]; then
	echo "You must be 'dsoidmn' to use this script" >&2
	echo "Please logon as 'dsoidmn'"
	exit 1
fi

trap "" 0 1 2 3 15

# if DSOI_BASE is not set, use /opt/DSOI as default
: ${DSOI_BASE:=/opt/DSOI}

# if DSOIREL_DIR is not set, use /opt/DSOIrel as default
: ${DSOIREL_DIR:=/opt/DSOIrel}

# Initialize all shell variables
INSTALLROOT=$DSOIREL_DIR
MYNAME=`basename $0`
USAGE="Usage: $MYNAME <tarfile>"
LOGFILE=installDSOI.log

# check command line argument.
if [ $# -ne 1 ]; then
	echo $USAGE
	exit 2
fi

tarFile=$1
installDir=`basename $tarFile | sed "s/\.t.*//"`

#check INSTALLROOT
if [ ! -d $INSTALLROOT ]; then
	echo "Install root directory $INSTALLROOT not found!" >&2
	exit 2
fi

chmod 775 $INSTALLROOT

# check tar file
if expr "$tarFile" : "[^/].*" >/dev/null 2>&1
then
	tarFile=$PWD/$tarFile
fi

if [ ! -r $tarFile ]; then
	echo "Unable to open file $tarFile, check file permissions" >&2
	exit 2
fi

# Now install the tar file
echo; echo "Installing $installDir..."
echo "The tar file is set to `basename $tarFile`"
umask 022

cd $INSTALLROOT

if [[ -d $installDir ]]; then
	print; print "Saving existing directory $installDir as $installDir.old..."
	rm -rf $installDir.old
	mv $installDir $installDir.old
fi

mkdir -p $installDir
cd $installDir

zcat $tarFile | tar xvf - > $LOGFILE 2>&1

if [ $? -ne 0 ]; then
	echo "Install failed (check tar file), exiting..." >&2
	exit 2
fi

grep "cannot create" $INSTALLROOT/$installDir/$LOGFILE > /dev/null 2>&1

if [[ $? = 0 ]]; then
	echo "DSOI load failed; could not create one or more files" >&2; echo
	echo "Please review log file $INSTALLROOT/$installDir/$LOGFILE"; echo
	exit 1
fi

cd $INSTALLROOT

# create DSOI_current link
DSOI_CURRENT=$DSOIREL_DIR/DSOI_current
rm -f $DSOI_CURRENT 2> /dev/null
ln -s "$INSTALLROOT/$installDir" $DSOI_CURRENT

if [[ $? != 0 ]]; then
	echo "Unable to link primary directory to new release" >&2; echo
	exit 1
fi

# create the logfiles directory
mkdir -m 777 $DSOI_BASE/logfiles
mkdir -m 777 $DSOI_BASE/logfiles/alarm
mkdir -m 777 $DSOI_BASE/logfiles/report
mkdir -m 777 $DSOI_BASE/logfiles/noncritical

# modify startup scripts to change ownership and set group id
echo; echo "Setting permissions..."
DSOIREPORT_EXEC="$DSOI_BASE/bin/DSOIReport"
SCRIPT_FILES="$DSOI_BASE/scripts/startupDSOI.ksh"

ksh -s <<! >/dev/null 2>&1
newgrp mqm <<EOF
chgrp mqm $SCRIPT_FILES $DSOIREPORT_EXEC
chmod g+s $SCRIPT_FILES $DSOIREPORT_EXEC
EOF
!


echo; echo "DSOI install successful"; echo

exit 0
