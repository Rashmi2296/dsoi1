#!/bin/ksh
########################################################################
#                                                                      #
# archiveDSOI.ksh - Archive out-of-date DSOI files under               #
#                   BaseDirectory/logfiles                             #
#                                                                      #
# Synopsis :                                                           #
#  archiveDSOI.ksh                                                     #
#  archiveDSOI.ksh [ BaseDirectory ]
#
#   Note: BaseDirectory is an optional parameter to be specified if
#         the directory tree for DSOI deviates from the production
#         environment. It is not required for the production
#         environment.
#
########################################################################

####################################
# Get command line options
####################################
if [ $# -eq 1 ]
then
        DSOI_BASE=$1
else
        DSOI_BASE=/opt/DSOI
fi
echo ${0}:" The DSOI base directory is set to '${DSOI_BASE}'"

################################################################
# Execute DSOILog's archiving functionality
# Resolve relative path issues by executing from $DSOI_BASE/bin
################################################################
cd $DSOI_BASE/bin
./DSOILog -a > /tmp/DSOILog.out 2>&1

####################################################################
# Execute DSOIReportUtil.pl's archiving functionality
# Resolve relative path issues by executing from $DSOI_BASE/scripts
####################################################################
cd $DSOI_BASE/scripts
./DSOIReportUtil.pl -a > /tmp/DSOIReportUtil.out 2>&1
