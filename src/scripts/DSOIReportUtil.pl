#!/usr/local/bin/perl
#@(#)
################################################################################
##
## File Name:      DSOIReportUtil.pl
##
## File Purpose:   Reads and extracts statistical data from report flat files.
##
## Special Notes:  None
##
## Author(s):      Joo Ahn Lee
##
## Copyright (c) 1996 by U S WEST Communications.
## The material contained herein is the proprietary property
## of U S WEST Communications.  Disclosure of this material is
## strictly prohibited except where permission is granted in writing.
##
## Description:
##     This program reads report files and extracts statistical data
##     relating to the serive orders processed by DSOIREQUEST &
##     DSOIREPLY.
##
##     Syntactical notes:
##        variables with name all in capitals  - global constant
##        variables with name all in lowercase - local to subroutine
##        variables with name in camel case    - global variable
##        all subroutine names are in lowercase
##
## REVISION HISTORY:
##
##  Name: J. A. Lee  Current Rel: 02.00.00
##  Date: 10/03/97
##  CR #: 1998
##  Comments: Initial coding
##
##  Name: J. A. Lee  Current Rel: 02.00.01
##  Date: 10/09/97
##  CR #: IFEdn02034
##  Comments: Added archiving, added call to DSOIReport & modified statistics
##            processing
##
################################################################################

##################################
# Establish the name of the tool #
##################################

$Tool_Name = "DSOIReportUtil.pl";

#######################
# Define process flag #
#######################

$Process_Flag = "COMMANDLINE";

#########################################
# Try to include the header definitions #
#########################################
eval('require \'flush.pl\'');
eval('require \'DSOIcommon.pl\'');

if ($@)
{
    $! = $EXIT_USAGE;
    die "$ToolName: Program Aborted\n";
}

############################################
# do initialization of variables and stuff #
############################################
&initialize_stuff();

######################################
# parse all of the command line args #
######################################
unless (&process_cmd_line_args)
{
    $! = $EXIT_USAGE;
    die "$USAGE\n";
}

################################
# do the rest of the real work #
################################
if ($Archive_Flag == 1)
{
	&archive;
}
else
{
	# Run DSOIReport to freshen report files
	&run_dsoireport;
	&main;
}

############
# finished #
############

exit($Error);





################################################################################
# SUBROUTINE ###################################################################
################################################################################

################################################################################
##
##   SUBROUTINE: run_dsoireport
##
##   DESCRIPTION: Runs DSOIReport to update report files.
##
##   ENTRY PARAMETERS: $Queue_Manager_Name - global command-line parameter
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: system
##
################################################################################
sub
run_dsoireport
{
	local($system_cmd);
	print "\nExecuting DSOIReport to update report files.\n";

	$system_cmd="cd ../bin; ./DSOIReport ".$Queue_Manager_Name;
	system($system_cmd);

	print "DSOIReport processing complete.\n\n";
}

################################################################################
##
##   SUBROUTINE: archive
##
##   DESCRIPTION: Archives old report files and removes obsolete report files.
##      Report files upto 3 days past $NUMBER_OF_HISTORICAL_DAYS days are
##      compressed and moved into an archive directory. Report files between 3
##      and 7 days past $NUMBER_OF_HISTORICAL_DAYS days are removed from the
##      archive directory.
##
##   ENTRY PARAMETERS:
##      $NUMBER_OF_HISTORICAL_DAYS - global set in initialize_stuff()
##      $REPORT_FILENAME_NO_DATESTAMP - global set in initialize_stuff()
##      $Current_Year - global set in initialize_stuff()
##      $Current_Month - global set in initialize_stuff()
##      $Current_Day - global set in initialize_stuff()
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: local, system, compute_yesterday_from
##
################################################################################
sub
archive
{
	# Declare local variables
	local(@archive_files,@obsolete_files);
	local($system_cmd,$archive_filename_with_datestamp);
	local($i,$j,$k);

	# Initialize local variables
	@archive_files=@obsolete_files={};
	$system_cmd=$archive_filename_with_datestamp="";
	$i=$j=$k=0;

	# Build array of filenames 7 days past $NUMBER_OF_HISTORICAL_DAYS days
	$Year  = $Current_Year;
	$Month = $Current_Month;
	$Day   = $Current_Day;

	for ($i=0; $i<($NUMBER_OF_HISTORICAL_DAYS+7); $i++)
	{
		$archive_filename_with_datestamp=sprintf("%s.%.4d%.2d%.2d",
			$REPORT_FILENAME_NO_DATESTAMP,$Year,$Month,$Day);
		if (($i >= $NUMBER_OF_HISTORICAL_DAYS) &&
		    ($i < ($NUMBER_OF_HISTORICAL_DAYS+3)))
		{
			$archive_files[$j++]=$archive_filename_with_datestamp;
		}
		if (($i >= ($NUMBER_OF_HISTORICAL_DAYS+3)) &&
		    ($i < ($NUMBER_OF_HISTORICAL_DAYS+7)))
		{
			$obsolete_files[$k++]=$archive_filename_with_datestamp;
		}
		&compute_yesterday_from($Year,$Month,$Day);
	}

	# Create archive directory, if needed
	$system_cmd="mkdir -p ".$ARCHIVE_DIRECTORY;
	if (system($system_cmd) != 0)
	{
		print "Unable to make archive directory with system command \"$system_cmd\"\n";
	}

	# Notify user of archiving process
	print "Archiving out-of-date report files. Please wait.\n\n";

	# Compress & move archive files
	for ($j=0; $j<=$#archive_files; $j++)
	{
		$system_cmd=sprintf "compress $REPORT_DIRECTORY/$archive_files[$j]; mv $REPORT_DIRECTORY/$archive_files[$j].Z $ARCHIVE_DIRECTORY";
		print "$system_cmd ... ";
		if ((-r $REPORT_DIRECTORY."/".$archive_files[$j]) &&
		    (system($system_cmd) != 0))
		{
			print "Unable to compress archive files with system command \"$system_cmd\"\n";
		}
		print "Done\n";

		$system_cmd=sprintf "mv $REPORT_DIRECTORY/$archive_files[$j].sorted $ARCHIVE_DIRECTORY";
		print "$system_cmd ... ";
		if ((-r $REPORT_DIRECTORY."/".$archive_files[$j].sorted) &&
		    (system($system_cmd) != 0))
		{
			print "Unable to move archive files with system command \"$system_cmd\"\n";
		}
		print "Done\n";
	}

	# Remove obsolete report files from archive directory
	for ($k=0; $k<=$#obsolete_files; $k++)
	{
		$system_cmd=sprintf "rm -f $ARCHIVE_DIRECTORY/$obsolete_files[$k]*";
		print "$system_cmd ... ";
	    if (system($system_cmd) != 0)
		{
			print "Unable to remove obsolete files with system command \"$system_cmd\"\n";
		}
		print "Done\n";
	}
}


################################################################################
##
##   SUBROUTINE: main
##
##   DESCRIPTION: Executes main body of script
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED:
##      count_cummulative_number_of_service_orders_processed,
##      count_number_of_service_orders_processed_by_time,
##      list_records_by_time,
##      list_service_order_by_order_type_and_number,
##      print_help,
##      print_intro,
##      print_menu,
##      print_unknown_menu_option,
##      quit,
##      reverse_screen_output_length,
##      write_to_output_file
##
################################################################################
sub
main
{
	# print introduction
	&print_intro();

	# loop through menu choices
	while (1)
	{
		# display menu
		&print_menu();

		if ($Menu_Response =~ /^s/i)
		{
			&list_service_order_by_order_type_and_number();
		}
		elsif ($Menu_Response =~ /^l/i)
		{
			&list_records_by_time();
		}
		elsif ($Menu_Response =~ /^c/i)
		{
			&count_cummulative_number_of_service_orders_processed();
		}
		elsif ($Menu_Response =~ /^n/i)
		{
			&count_number_of_service_orders_processed_by_time();
		}
		elsif ($Menu_Response =~ /^h/i)
		{
			&print_help();
		}
		elsif ($Menu_Response =~ /^w/i)
		{
			&write_to_output_file();
		}
		elsif ($Menu_Response =~ /^r/i)
		{
			&reverse_screen_output_length();
		}
		elsif ($Menu_Response =~ /^q/i)
		{
			&quit();
		}
		else
		{
			&print_unknown_menu_option();
		}
	}
}
