#!/bin/ksh
################################################################################
#
# loadDatarefq.ksh: Populate the DSOI reference queue, QL_DSOI_DATAREFQ
#                   with data from the routeInfo and npa_split files.
#
# Usage: loadDatarefq.ksh <QueueManager>
#
################################################################################

if [[ `whoami` != "dsoidmn" ]]; then
	print "You must be 'dsoidmn' to use this script" >&2
	print "Please logon as 'dsoidmn'"
	exit 1
fi

if [[ -z $1 ]]; then
	print "Usage '`basename $0` QueueMangerName'" >&2
	print "You must specify a QueueManagerName"
	print "e.g. 'loadDatarefq.ksh DOTSEROQM1'"
	exit 1
fi

# ------------------------------------------------------
#   Check to see if the Qmanager is running
# - - - - - - ------------------------------ - - - - - -

if [[ -z "`ps -ef | grep $1 | grep amqzxma0`" ]]; then
	print "Qmanager $1 is not running" >&2
	print "Cannot load QL_DSOI_DATAREFQ"
	exit 1

else
	RESULTS_FILE=/tmp/`basename $0`.$$.results

	print; print "** CLEARING ** QL_DSOI_DATAREFQ..."; print

	runmqsc -e $1 <<-EOF | tee $RESULTS_FILE
		clear ql(QL_DSOI_DATAREFQ)
	EOF

	print; print "** RUNNING ** DSOIDRefLoader to LOAD QL_DSOI_DATAREFQ..."
	print

	/opt/DSOI/bin/DSOIDRefLoader QL_DSOI_DATAREFQ $1 /opt/DSOI/etc

	print; print "** DISPLAYING ** current depth of QL_DSOI_DATAREFQ..."
	print

	runmqsc -e $1 <<-EOF | tee -a $RESULTS_FILE
		dis q(QL_DSOI_DATAREFQ) curdepth
	EOF

	print

	# verify that all commands were correct and processed okay

	set -A SYNTAX_ERRORS `grep "syntax error" $RESULTS_FILE | awk '{print $1;}'`
	SUM=0
	TOTAL=0

	for ERROR in ${SYNTAX_ERRORS[*]}; do
		[[ $ERROR = [0-9]* ]] && (( SUM += ERROR ))
	done

	if (( $SUM != 0 )); then
		print "$SUM MQSeries commands have a syntax error" >&2
		(( TOTAL += SUM ))
	fi

	set -A UNPROCESSED `grep processed $RESULTS_FILE | awk '{print $1;}'`
	SUM=0

	for ERROR in ${UNPROCESSED[*]}; do
		[[ $ERROR = [0-9]* ]] && (( SUM += ERROR ))
	done

	if (( $SUM != 0 )); then
		print "$SUM MQSeries commands cannot be processed" >&2
		(( TOTAL += SUM ))
	fi

	if (( $TOTAL != 0 )); then 
		print "Procedure failed; DATAREFQ was not loaded!"; print
		exit 1
	fi
fi

print "DATAREFQ loaded successfully if:"
print "\tNo commands have a syntax error"
print "\tAll valid MQSC commands were processed"; print

rm -f $RESULTS_FILE
exit 0
