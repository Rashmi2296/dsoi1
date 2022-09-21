#!/usr/local/bin/perl -w
#@(#)
################################################################################
##
## File Name:	  DSOIcommon.pl
##
## File Purpose:   
##
## Special Notes:  None
##
## Author(s):	  Joo Ahn Lee , Linda Parks
##
## Copyright (c) 1996 by U S WEST Communications.
## The material contained herein is the proprietary property
## of U S WEST Communications.  Disclosure of this material is
## strictly prohibited except where permission is granted in writing.
##
## Description:
##	 This program reads report files and extracts statistical data
##	 relating to the serive orders processed by DSOIREQUEST &
##	 DSOIREPLY.
##
##	 Syntactical notes:
##		variables with name all in capitals  - global constant
##		variables with name all in lowercase - local to subroutine
##		variables with name in camel case	- global variable
##		all subroutine names are in lowercase
##
## REVISION HISTORY:
##
##  Name: J. A. Lee
##  Current Rel: 02.00.00
##  Date: 10/03/97
##  CR #: 1998
##  Comments:   Initial coding
##
##  Name: J. A. Lee
##  Current Rel: 02.00.01
##  Date: 10/09/97
##  CR #: IFEdn02034
##  Comments:   Added archiving, added call to DSOIReport & modified statistics
##		processing
##
##  Name: Bryce Cox
##  Current Rel: 06.01.00
##  Date: 11/08/1999
##  CR #: PVCS 767
##  Comments:   Modified subroutine build_report_files to search the report 
##		directory and build @Report_Files from existing report files, 
##		rather than manufacture the array from 
##		$NUMBER_OF_HISTORICAL_DAYS with file names that may not exist.
##
##  Name: Bryce Cox
##  Current Rel: 07.00.00
##  Date: 03/07/2000
##  CR #: PVCS 3135
##  Comments:   Add logic to use DSOIRouteRequest and DSOIRouteReply report
##		log entries in statistical reports.
##
################################################################################

##################################
# Establish the name of the tool #
##################################

$Tool_Name = "DSOIReportUtil.pl";

#########################################
# Try to include the header definitions #
#########################################
eval('require \'flush.pl\'');

################################################################################
# SUBROUTINE ###################################################################
################################################################################

################################################################################
##
##   SUBROUTINE: build_report_files
##
##   DESCRIPTION: Builds an array of all the filenames to be searched based on
##		  report files that exist in report directory.
##
##   ENTRY PARAMETERS:
##      $BASE_REPORT_FILENAME - global set in initialize_stuff
##      $NUMBER_OF_HISTORICAL_DAYS - global set in initialize_stuff
##
##   EXIT PARAMETERS: @Report_Files - filename array
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: closedir, grep, length, my, opendir, readdir, sort,
##		       sprintf, substr
##
################################################################################
sub
build_report_files
{
	my($report_dir,$i,@temp);

	# Isolate report file directory.
	$report_dir = substr($BASE_REPORT_FILENAME,0,length($BASE_REPORT_FILENAME) - length($REPORT_FILENAME_NO_DATESTAMP));

	# Open report file directory and read contents.
	opendir(dir, $report_dir);
	@Report_Files = grep { $REPORT_FILENAME_NO_DATESTAMP && -f "$report_dir/$_" } readdir(dir);

	# Sort array by filename.
	@Report_Files = sort @Report_Files;
	for ($i=0; $i <= $#Report_Files; $i++)
	{
		# Concatenate path and filename.
		$Report_Files[$i] = sprintf("%s%s",$report_dir,$Report_Files[$i]);

		# Reverse the sort:  descending from today's date to oldest date.
		$temp[$#Report_Files - $i] = $Report_Files[$i];
	}
	@Report_Files = @temp;

	# Close the report file directory.
	closedir dir;
}

################################################################################
##
##   SUBROUTINE: build_dates_requested
##
##   DESCRIPTION: Builds an array of dates between the start and end dates.
##
##   ENTRY PARAMETERS:
##      $start_date, $end_date - beginning and end dates
##
##   EXIT PARAMETERS: @requested_dates - array of dates
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: substr
##
################################################################################
sub
build_dates_requested
{
	# Declare local variables
	local($start_date, $end_date)=@_;
	local($next_date,@requested_dates);
	local($year,$month,$day);
	local($end_year,$end_month,$end_day);
	local($next_year,$next_month,$next_day);

	# Initialize local variables
	$next_date=0;
	@requested_dates={};
	$year=$month=$day=0;
	$end_year=$end_month=$end_day=0;
	$next_year=$next_month=$next_day=0;

	$next_date=$start_date;
	$next_year  = $year  = substr($start_date,0,4);
	$next_month = $month = substr($start_date,4,2);
	$next_day   = $day   = substr($start_date,6,2);
	$end_year  = substr($end_date,0,4);
	$end_month = substr($end_date,4,2);
	$end_day   = substr($end_date,6,2);

	$requested_dates[$0]=$next_date;
	for ($i=1; ($next_date < $end_date); $i++)
	{
		($next_year,$next_month,$next_day) = &compute_tomorrow_from($next_year,$next_month,$next_day);
		$next_date = sprintf("%4.4d%2.2d%2.2d",$next_year,$next_month,$next_day);
		$requested_dates[$i]=$next_date;
	}

	return @requested_dates;
}


################################################################################
##
##   SUBROUTINE: check_if_leap_year
##
##   DESCRIPTION: Determines if the input year is a leap year. Only able to
##      verify for 2000 - 2100 year range.
##
##   ENTRY PARAMETERS: $year - in four digits
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $leapYearFlag - 0 or 1; 1 if leap year
##
##   SUBROUTINES USED: None
##
################################################################################
sub
check_if_leap_year
{
	local($year) = @_;
	local($leapYearFlag,@leapYears,$i);

	$leapYearFlag=0;
	@leapYears = (2000,2004,2008,2012,2016,
				  2020,2024,2028,2032,2036,
				  2040,2044,2048,2052,2056,
				  2060,2064,2068,2072,2076,
				  2080,2084,2088,2092,2096,2100);

	for ($i=0; $i<$#leapYears; $i++)
	{
		if ($year == $leapYears[$i])
		{
			$leapYearFlag=1;
			last;
		}
	}

	return $leapYearFlag;
}


################################################################################
##
##   SUBROUTINE: compute_date
##
##   DESCRIPTION: Determines today's date; year is given in 4 digits, month and
##      day in 2 digits.
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS:
##      $Current_Year, $Current_Month and $Current_Day - globals
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: localtime
##
################################################################################
sub
compute_date
{
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);

	$year += 1900; #because year is returned as how many years since 1900
	$Current_Year  = $year;
	$Current_Month = $mon+1; #because months start with a zero(0)
	$Current_Day   = $mday;
}


################################################################################
##
##   SUBROUTINE: compute_day_difference
##
##   DESCRIPTION: Computes the difference in days between the input dates.
##
##   ENTRY PARAMETERS: $request_date, $reply_date
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $days_between_request_and_reply
##
##   SUBROUTINES USED: substr, compute_days_from_start_of_year
##
################################################################################
sub
compute_day_difference
{
	# Declare local variables
	local($request_date,$reply_date)=@_;
	local($days_between_request_and_reply);
	local($request_days,$reply_days);
	local($request_year,$reply_year);

	# Define local variables
	$days_between_request_and_reply=0;
	$request_days=$reply_days=0;
	$request_year=$reply_year=0;

	$request_year = substr($request_date,-4,4);
	$reply_year   = substr($reply_date,-4,4);

        $request_days=&compute_days_from_start_of_year($request_date);
        $reply_days=&compute_days_from_start_of_year($reply_date);

        if (($reply_year - $request_year) == 0)
        {
                $days_between_request_and_reply = $reply_days - $request_days;
        }
        else
        {
                if (&check_if_leap_year($request_year) == 0)
                {
                        # not a leap year
                        $request_days = 365 - $request_days;
                }
                elsif (&check_if_leap_year($request_year) == 1)
                {
                        # leap year
                        $request_days = 366 - $request_days;
                }

                # Assuming here that DSOIRequest occurred in year prior to DSOIReply
                $days_between_request_and_reply = $request_days + $reply_days;
        }

	return $days_between_request_and_reply;
}


################################################################################
##
##   SUBROUTINE: compute_days_from_start_of_year
##
##   DESCRIPTION: Computes the number of days since the beginning of the year.
##      Takes into account leap years between 2000 and 2100.
##
##   ENTRY PARAMETERS:
##      $date - format must be one of the following (mmddyyyy, mmddyy, mddyy,
##		mdyy); note that the format mmdyy is not supported and will
##		cause errors
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $days_between_request_and_reply
##
##   SUBROUTINES USED: length, substr, check_if_leap_year
##
################################################################################
sub
compute_days_from_start_of_year
{
	# Declare local variables
	local($date)=@_;
	local($date_month,$date_day,$date_year);
	local($days_from_start_of_year);
	local($leap_year_flag,$i);

	# Define local variables
	$days_from_year_beginning=0;
	$date_month=$date_day=$date_year=0;
	$days_from_start_of_year=0;
	$leap_year_flag=$i=0;

	if (length($date) == 8) # mmddyyyy
	{
		$date_month = substr($date,0,2);
		$date_day   = substr($date,2,2);
		$date_year  = substr($date,4,4);
	}
	elsif (length($date) == 6) # mmddyy
	{
		$date_month = substr($date,0,2);
		$date_day   = substr($date,2,2);
		$date_year  = substr($date,4,2);
	}
	elsif (length($date) == 5) # mddyy
	{
		$date_month = substr($date,0,1);
		$date_day   = substr($date,1,2);
		$date_year  = substr($date,3,2);
	}
	elsif (length($date) == 4) # mdyy
	{
		$date_month = substr($date,0,1);
		$date_day   = substr($date,1,1);
		$date_year  = substr($date,2,2);
	}

	if ($date_year < 100) { $date_year += 1900; }
	elsif (length($date_year) != 4) { $date_year += 2000; } #leave a four digit year alone

	if (&check_if_leap_year($date_year) == 0) { $leap_year_flag=0; } else { $leap_year_flag=1; }

	# Add days from months prior to current month
	$days_from_start_of_year=0;
	for ($i=1; $i<$date_month; $i++)
	{
		if ($i == 1) { $days_from_start_of_year += 31; next; }
		if ($i == 2)
		{
			if ($leap_year_flag == 0)
			{
				$days_from_start_of_year += 28; # Not a leap year
				next;
			}
			else
			{
				$days_from_start_of_year += 29; # Is a leap year
				next;
			}
		}
		if ($i == 3) { $days_from_start_of_year += 31; next; }
		if ($i == 4) { $days_from_start_of_year += 30; next; }
		if ($i == 5) { $days_from_start_of_year += 31; next; }
		if ($i == 6) { $days_from_start_of_year += 30; next; }
		if ($i == 7) { $days_from_start_of_year += 31; next; }
		if ($i == 8) { $days_from_start_of_year += 31; next; }
		if ($i == 9) { $days_from_start_of_year += 30; next; }
		if ($i == 10) { $days_from_start_of_year += 31; next; }
		if ($i == 11) { $days_from_start_of_year += 30; next; }
		if ($i == 12) { $days_from_start_of_year += 31; next; } # Should not hit this if
	}

	# Add days from current month
	$days_from_start_of_year += $date_day;

	return $days_from_start_of_year;
}

################################################################################
##
##   SUBROUTINE: compute_next_day
##
##   DESCRIPTION: Computes next day. Takes into account month breaks.
##
##   ENTRY PARAMETERS: $month, $day
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $day - incremented from input $day
##
##   SUBROUTINES USED: check_if_leap_year
##
################################################################################
sub
compute_next_day
{
	local($month, $day) = @_;

	$day += 1;

	if (($day == 32)    &&
	    (($month == 1)  ||
		 ($month == 3)  ||
		 ($month == 5)  ||
		 ($month == 7)  ||
		 ($month == 8)  ||
		 ($month == 10) ||
		 ($month == 12)))
	{
		$day = 1;
	}
	elsif (($day == 31)     &&
	       (($month == 4)   ||
		    ($month == 6)   ||
		    ($month == 9)   ||
		    ($month == 11)))
	{
		$day = 1;
	}
	elsif (($day == 30) &&
	       ($month == 2))
	{
		$day = 1;
	}
	elsif (($day == 29) &&
	       ($month == 2))
	{
		if ((&check_if_leap_year($Current_Year))==1)
		{
			$day = 29;
		}
		else
		{
			$day = 1;
		}
	}

	return $day;
}



################################################################################
##
##   SUBROUTINE: compute_previous_day
##
##   DESCRIPTION: Computes previous day. Takes into account month breaks.
##
##   ENTRY PARAMETERS: $month, $day
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $day - decrimented from input $day
##
##   SUBROUTINES USED: check_if_leap_year
##
################################################################################
sub
compute_previous_day
{
	local($month, $day) = @_;

	if (($month == 1) ||
		($month == 3) ||
		($month == 5) ||
		($month == 7) ||
		($month == 8) ||
		($month == 10) ||
		($month == 12))
	{
		$day = 31 - $day;
	}
	elsif (($month == 4) ||
		   ($month == 6) ||
		   ($month == 9) ||
		   ($month == 11))
	{
		$day = 30 - $day;
	}
	elsif ($month == 2)
	{
		if ((&check_if_leap_year($Current_Year))==1)
		{
			$day = 29 - $day;
		}
		else
		{
			$day = 28 - $day;
		}
	}

	return $day;
}


################################################################################
##
##   SUBROUTINE: compute_next_month
##
##   DESCRIPTION: Computes next month. Takes into account year breaks.
##
##   ENTRY PARAMETERS: $month
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $month - incremented from input $month value
##
##   SUBROUTINES USED: local
##
################################################################################
sub
compute_next_month
{
	local($month) = @_;
	if (($month+=1) == 13) { $month = 1; }

	return $month;
}


################################################################################
##
##   SUBROUTINE: compute_previous_month
##
##   DESCRIPTION: Computes previous month. Takes into account year breaks.
##
##   ENTRY PARAMETERS: $month
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $month - decrimented from input $month value
##
##   SUBROUTINES USED: local
##
################################################################################
sub
compute_previous_month
{
	local($month) = @_;
	if (($month-=1) == 0) { $month = 12; }

	return $month;
}

################################################################################
##
##   SUBROUTINE: compute_tomorrow_from
##
##   DESCRIPTION: Computes the next date with respect to the input variables.
##
##   ENTRY PARAMETERS:
##     $year - given as a 4 digit year
##     $month, $day - given in 2 digits each
##
##   EXIT PARAMETERS: 
##
##   RETURN VALUES:
##
##   SUBROUTINES USED: compute_next_month, compute_next_day
##
################################################################################
sub
compute_tomorrow_from
{
	local($year,$month,$day) = @_;

	if (($day = &compute_next_day($month,$day)) == 1)
	{
		if (($month = &compute_next_month($month)) == 1)
		{
			$year += 1;
		}
	}

	return $year,$month,$day;
}



################################################################################
##
##   SUBROUTINE: compute_yesterday_from
##
##   DESCRIPTION: Computes the previous date with respect to the input variables.
##
##   ENTRY PARAMETERS: $year, $month, $day
##
##   EXIT PARAMETERS: $Year, $Month, $Day - these global variables are set to
##                                          the calculated values
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: compute_previous_month, compute_previous_day
##
################################################################################
sub
compute_yesterday_from
{
	local($year,$month,$day) = @_;

	if (($day-=1) <= 0)
	{
		$month = &compute_previous_month($month);
		if ($month == 12) { $year -= 1; }
		$day = &compute_previous_day($month,$day);
	}


	# Set local to global variables
	$Year  = $year;
	$Month = $month;
	$Day   = $day;
}


################################################################################
##
##   SUBROUTINE: convert_to_seconds
##
##   DESCRIPTION: Converts time in "hhmmssssssss" format to seconds. The time is
##      expected to be given at the microsecond level.
##
##   ENTRY PARAMETERS:
##      $input_time - time in "hhmmssssssss" format
##      $MICROSECONDS_NORMALIZING_FACTOR - global set in initialize_stuff; should
##         be equal to 1e6
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $time_in_seconds - converted time
##
##   SUBROUTINES USED: substr
##
################################################################################
sub
convert_to_seconds
{
	local($input_time)=@_;
	local($time_in_seconds);
	$time_in_seconds=0;

	$time_in_seconds = substr($input_time,0,2)*3600 +
					   substr($input_time,2,2)*60 +
					   substr($input_time,4,2) +
					   substr($input_time,-6,6)/$MICROSECONDS_NORMALIZING_FACTOR;
	return $time_in_seconds;
}


################################################################################
##
##   SUBROUTINE: count_cummulative_number_of_service_orders_processed
##
##   DESCRIPTION: Computes statistics on the service order messages processed
##      by DSOI for the past 10 calendar days.
##
##   ENTRY PARAMETERS:
##      @Report_Files - global array populated in build_report_files()
##      $ERROR_File - set in initialize_stuff()
##      $TEMPORARY_FILE - set in initialize_stuff()
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: open, close, flush, system, unlink, process_statistics
##
################################################################################
sub
count_cummulative_number_of_service_orders_processed
{
	# Declare local variables
	local(@sorted_files);

	# Initialize local variables
	@sorted_files={};

	if ($Process_Flag eq "COMMANDLINE")
	{
		# Let user know of processing
		print "\nProcessing...\n";
	}

	# Open temporary file for statistics processing
	open(TEMP_FILE, ">".$TEMPORARY_FILE);

	# Iterate over the files to search
	$record_found=0;
	foreach $file (@Report_Files)
	{
		# Open file and process
		open(FILE, $file) || next;
		while (<FILE>)
		{
			# Print messages within time period into temporary file
			print TEMP_FILE $_;
		}
		close(FILE);
	}
	&flush(TEMP_FILE);
	close(TEMP_FILE);

	# Sort first by msgId
	$sorted_temporary_file=$TEMPORARY_FILE.".sorted";
	if (-r $TEMP_FILE) 
	{
		print "Sorry can't open file $TEMPORARY_FILE error:$!\n";
		return;
	}
	elsif (system("sort -k 8,8 -t~ $TEMPORARY_FILE -o $sorted_temporary_file > $ERROR_FILE 2>&1") != 0)
	{
		print "Unable to execute system \"sort\" for \"$TEMPORARY_FILE\" \n errno= $!\n";
		&print_sort_error_message();

	# orignal code - restore when bug is fixed
	# CR 3545: system(sort... ) call always generates an error on DSOI-UT1 (but the sort works)
	# return on error
		#return;

	# workaround - delete when bug is fixed
	# CR 3699 (DSOI 7.0): temporary workaround for CR 3545
	# close error file and continue since the sort worked
		unlink($ERROR_FILE);
	}
	else
	{
		unlink($ERROR_FILE);
	}

	# Process statistical information on report files
	&process_statistics($sorted_temporary_file);

	# Remove temporary file(s)
	unlink($TEMPORARY_FILE);
	unlink($sorted_temporary_file);
}


################################################################################
##
##   SUBROUTINE: count_number_of_service_orders_processed_by_time
##
##   DESCRIPTION: Computes statistics on service order messages processed by
##      DSOI for a specified time period.
##
##   ENTRY PARAMETERS:
##      $BASE_REPORT_FILENAME - global set in initialize_stuff()
##      $ERROR_File - set in initialize_stuff()
##      $TEMPORARY_FILE - global set in initialize_stuff()
##      HTML inputs:
##         $numb_days        - single or multiple
##         $numb_month_from  - starting month
##         $numb_day_from    - starting day
##         $numb_year_from   - starting year
##         $numb_month_to    - ending month
##         $numb_day_to      - ending day
##         $numb_year_to     - ending year
##         $numb_times       - all day or specific time period
##         $numb_hour_from   - starting hour
##         $numb_minute_from - starting minute
##         $numb_hour_to     - ending hour
##         $numb_minute_to   - ending minute
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: get_time_period_html, get_time_period, open, close,
##      flush, system, unlink, sprintf, print, process_statistics,
##      print_unable_to_process_for_time_period
##
################################################################################
sub
count_number_of_service_orders_processed_by_time
{
	if ($Process_Flag eq "HTML")
	{
		local($numb_days,$numb_month_from,$numb_day_from,$numb_year_from,$numb_month_to,$numb_day_to,$numb_year_to,
			$numb_times,$numb_hour_from,$numb_minute_from,$numb_hour_to,$numb_minute_to)=@_;
	}

	# Declare local variables
	local(@dates_requested,@files_to_search,@sorted_files);
	local($start_date,$end_date,$start_time,$end_time);
	local($record_found,$i);
	local($file,$sorted_temporary_file);

	# Initialize local variables
	@dates_requested=@files_to_search=@sorted_files={};
	$record_found=$i=0;
	$start_date=$end_date=$start_time=$end_time=0;
	$file=$sorted_temporary_file="";

	# Get requested time period
	if ($Process_Flag eq "HTML")
	{
		($start_date,$end_date,$start_time,$end_time)=
			&get_time_period_html($numb_days,$numb_month_from,$numb_day_from,$numb_year_from,
				$numb_month_to,$numb_day_to,$numb_year_to,
				$numb_times,$numb_hour_from,$numb_minute_from,
				$numb_hour_to,$numb_minute_to);
	}
	else
	{
		($start_date,$end_date,$start_time,$end_time)=&get_time_period();
		if($start_date == 0)
		{
			print "Sorry bad input\n";
			return 0;
		}

		# Let user know of processing
		print "\nProcessing...\n";
	}

	# build report filenames for dates requested
	@dates_requested = &build_dates_requested($start_date, $end_date);
	for ($i=0; $i<=$#dates_requested; $i++)
	{
		$files_to_search[$i]=sprintf("%s.%.6d",$BASE_REPORT_FILENAME,$dates_requested[$i]);
	}

	# Open temporary file for statistics processing
	open(TEMP_FILE, ">".$TEMPORARY_FILE);

	# Iterate over the files to search
	$record_found=0;
	foreach $file (@files_to_search)
	{
		# Open file and process
		open(FILE, $file) || next;
		while (<FILE>)
		{
			/getTime=([0-9]+)~+/ && ($get_time = $1);
			if (($get_time gt $start_time) &&
				($get_time lt $end_time)   &&
				(($_ ne "\n") && ($_ ne "~\n")))
			{
				# Print messages within time period into temporary file
				print TEMP_FILE $_;
				$record_found=1;
				$get_time=0;
			}
		}
		close(FILE);
	}
	&flush(TEMP_FILE);
	close(TEMP_FILE);

	# Continue processing if matching service order messages were found
	if ($record_found == 1)
	{
		# Sort temporary file before printing to screen and output file
		$sorted_temporary_file=$TEMPORARY_FILE.".sorted";
		if (-r $TEMP_FILE)
		{
			print "Sorry can't open file $TEMPORARY_FILE error:$!";
			return;
		}
		elsif	(system("sort -k 8,8 -t~ $TEMPORARY_FILE -o $sorted_temporary_file > $ERROR_FILE 2>&1") != 0)
		{
			print "Unable to execute system \"sort\" for \"$TEMPORARY_FILE\" \n errno=$!\n";
			&print_sort_error_message();
			return;
		}
		else
		{
			unlink($ERROR_FILE);
		}

		# Process statistics
		&process_statistics($sorted_temporary_file);
	}
	else
	{
		# Write that service order messages could not be found
		&print_unable_to_process_for_time_period($start_date,$end_date,$start_time,$end_time);;
	}

	# Remove temporary file(s)
	for ($i=0; $i<=$#sorted_files; $i++) { unlink $sorted_files[$i]; }
	unlink $TEMPORARY_FILE;
	unlink $sorted_temporary_file;
}


################################################################################
##
##   SUBROUTINE: get_time_period
##
##   DESCRIPTION: Queries user for date and time period of interest. 
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $start_date, $end_date, $start_time, $end_time
##
##   SUBROUTINES USED: print
##
################################################################################
sub
get_time_period
{
	# Declare & initialize local variables
	local($single_or_multiple,$allday_or_period);
	local($single_date,$start_date,$end_date,$start_time,$end_time);
	$single_or_multiple=$allday_or_period="";
	$single_date=$start_date=$end_date=$start_time=$end_time=0;

	# Get date period(s)
	print "For a single day or multiple days (s/m)? ";
	chop($single_or_multiple = <STDIN>);

	if ($single_or_multiple =~ /^s\b/i)
	{
		# Get date
		print "Please enter date of interest (yyyymmdd): ";
		chop($single_date = <STDIN>);
		$start_date = $single_date;
		$end_date = $start_date;
	}
	elsif ($single_or_multiple =~ /^m\b/i)
	{
		# Get starting day
		print "Starting date (yyyymmdd)? ";
		chop($start_date = <STDIN>);

		# Get ending day
		print "Ending date (yyyymmdd)? ";
		chop($end_date = <STDIN>);
	}
	else
	{
		print "Not an option: $single_or_multiple \n";
		return (0,0,0,0);
	}

	# Get time period(s)
	print "For all day or for period (a/p)? ";
	chop($allday_or_period = <STDIN>);

	if ($allday_or_period =~ /^a\b/i)
	{
		$start_time = 000000;
		$end_time = 235959;
	}
	elsif ($allday_or_period =~ /^p\b/i)
	{
		# Get starting time
		print "Starting time (hhmmss)? ";
		chop($start_time = <STDIN>);

		# Get ending time
		print "Ending time (hhmmss)? ";
		chop($end_time = <STDIN>);
	}
	else
	{
		print "Not an option: $allday_or_period \n";
		return (0,0,0,0);
	}

	return ($start_date,$end_date,$start_time,$end_time);
}


################################################################################
##
##   SUBROUTINE: get_time_period_html
##
##   DESCRIPTION: Queries user for date and time period of interest.
##
##   ENTRY PARAMETERS: 
##      $list_days        - single or multiple days
##      $list_month_from  - starting month
##      $list_day_from    - starting month
##      $list_year_from   - starting month
##      $list_month_to    - ending month
##      $list_day_to      - ending month
##      $list_year_to     - ending month
##      $list_times       - all day or specific time period
##      $list_hour_from   - starting hour
##      $list_minute_from - starting minute
##      $list_hour_to     - ending hour
##      $list_minute_to   - ending minute
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: $start_date, $end_date, $start_time, $end_time
##
##   SUBROUTINES USED: sprintf
##
################################################################################
sub
get_time_period_html
{
	local($list_days,$list_month_from,$list_day_from,$list_year_from,
		$list_month_to,$list_day_to,$list_year_to,
		$list_times,$list_hour_from,$list_minute_from,
		$list_hour_to,$list_minute_to)=@_;

	# Declare & initialize local variables
	local($allday_or_period);
	local($single_date,$start_date,$end_date,$start_time,$end_time);
	$allday_or_period="";
	$single_date=$start_date=$end_date=$start_time=$end_time=0;

	if ($list_days eq "single")
	{
		# Get date
		$start_date = sprintf("%4.4d%2.2d%2.2d",$list_year_from,$list_month_from,$list_day_from);
		$end_date = $start_date;
	}
	else
	{
		$start_date = sprintf("%4.4d%2.2d%2.2d",$list_year_from,$list_month_from,$list_day_from);
		$end_date = sprintf("%4.4d%2.2d%2.2d",$list_year_to,$list_month_to,$list_day_to);
	}

	if ($list_times eq "allday")
	{
		$start_time = 000000;
		$end_time = 235959;
	}
	else
	{
		$start_time = sprintf("%2.2d%2.2d00",$list_hour_from,$list_minute_from);
		$end_time = sprintf("%2.2d%2.2d59",$list_hour_to,$list_minute_to);
	}

	return ($start_date,$end_date,$start_time,$end_time);
}


################################################################################
##
##   SUBROUTINE: initialize_stuff
##
##   DESCRIPTION: Initializes global constants and variables
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS:
##	  $DEFAULT_QUEUE_MANAGER, $REPORT_DIRECTORY, $BASE_REPORT_FILENAME,
##	  $NUMBER_OF_HISTORICAL_DAYS, $TEMPORARY_FILE, $ARCHIVE_DIRECTORY,
##	  $STDOUT_TOP_PRINT, $STDOUT_80_TOP_PRINT, $STATISTICS_TOP_PRINT,
##	  $DSOIREQUEST_STRING, $DSOIREPLY_STRING
##	  $Archive_Flag, $Output_File
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: compute_date, build_report_files
##
################################################################################
sub
initialize_stuff
{
	if ($Process_Flag eq "HTML") { local($nfs_mount)=@_; }

	############################################
	# constant: base report filename

	if ($Process_Flag eq "HTML")
	{
		$REPORT_DIRECTORY="logfiles/report";
	}
	else
	{
		$REPORT_DIRECTORY="../logfiles/report";
	}

	############################################
	# constant: report filename without datestamp

	$REPORT_FILENAME_NO_DATESTAMP="DSOI_report.log";

	############################################
	# constant: archive directory

	$ARCHIVE_DIRECTORY = $REPORT_DIRECTORY."/archive";

	############################################
	# constant: base report filename

	if ($Process_Flag eq "HTML")
	{
		$BASE_REPORT_FILENAME="$nfs_mount/".$REPORT_DIRECTORY."/".$REPORT_FILENAME_NO_DATESTAMP;
	}
	else
	{
		$BASE_REPORT_FILENAME=$REPORT_DIRECTORY."/".$REPORT_FILENAME_NO_DATESTAMP;
	}

	############################################
	# constant: default queue manager name

	$DEFAULT_QUEUE_MANAGER="DOTSEROQM3";

	############################################
	# constant: strings in flat files for DSOIRequest/Reply

	$DSOIREQUEST_STRING = "DSOIRequest";
	$DSOIREPLY_STRING = "DSOIREPLY";

	############################################
	# constant: error file written to when system commands fail

	$ERROR_FILE = "/tmp/DSOIReportUtil.err".".".$$;
	open(ERROR_LOG, ">>".$ERROR_FILE); # opened for append; see quit() for matching close

	############################################
	# constant: base report filename

	$MICROSECONDS_NORMALIZING_FACTOR=1000000; # normalizing factor for microseconds

	############################################
	# constant: number of calendar days for which historical
	#		   data is kept

	$NUMBER_OF_HISTORICAL_DAYS=10;

	############################################
	# constant: temporary file used when doing statistical calculations

	if ($Process_Flag eq "HTML")
	{
		$TEMPORARY_FILE = "/tmp/DSOIReportUtil.temp".$$;
	}
	else
	{
		$TEMPORARY_FILE = ".DSOIReportUtil.temp".$$;
	}

	############################################
	# constants: hard-coded headers

    $STDOUT_TOP_PRINT = "

                         Service Order Message Listing

Date     SendTime GetTime  DSOI Appl   OrderNumber Put Appl         Put Appl Time Put Appl Date Msg Id                Msg Fragment
-------- -------- -------- ----------- ----------- ---------------- ------------- ------------- --------------------- ----------"; #end

	$STDOUT_80_TOP_PRINT = "

                         Service Order Message Listing

Date     SendTime GetTime  DSOI Appl   OrderNumber MsgId      Msg Fragment
-------- -------- -------- ----------- ----------- ---------- ------------------"; #end

	$STATISTICS_TOP_PRINT = "

                                DSOI Statistics

CompTrans DSOI+SOPAvgTime/Trans DSOIAvgTime/Trans SOPAvgTime/Trans IncompTrans
--------- --------------------- ----------------- ---------------- -----------"; #end

	############################################
	# constant: define usage

	$USAGE = "USAGE: $Tool_Name [-q <Queue_Manager_Name>] [-a]\n";

	############################################
	# compute today's date; call BEFORE building
	# report filenames

	&compute_date;

	############################################
	# variable: array of last 10 calendar report filenames

	&build_report_files;

	############################################
	# variable: indicator to archive report files

	$Archive_Flag=0; # 0 = doesn't archive, 1 = does

	############################################
	# variable: menu choice

	$Menu_Response="";

	############################################
	# variable: output directory

	$OUTPUT_DIR="output";

	############################################
	# variable: output filename; create AFTER call
	# to compute_date()

	$Output_File="$OUTPUT_DIR/DSOIReportUtil.out".".".$Current_Year.$Current_Month.$Current_Day.".".$$;

	############################################
	# variable: write to output file flag

	$Output_File_Flag=0; # 0 = doesn't write to file, 1 = does write

	############################################
	# variable: queue manager name - command-line parameter

	$Queue_Manager_Name="";

	############################################
	# variable: full or 80 char output to screen flag

	$Screen_Output_Flag=0; # 0 = full output, 1 = 80 chars

	return;
}


################################################################################
##
##   SUBROUTINE: list_records_by_time
##
##   DESCRIPTION: Lists service order given an order number.
##
##   ENTRY PARAMETERS:
##      @Report_Files - global array populated in build_report_files()
##      $ERROR_File - set in initialize_stuff()
##      $TEMPORARY_FILE - set in initialize_stuff()
##      HTML inputs:
##         $list_month_from  - starting month
##         $list_day_from    - starting day
##         $list_year_from   - starting year
##         $list_month_to    - ending month
##         $list_day_to      - ending month
##         $list_year_to     - ending month
##         $list_times       - flag for all day or specific time period
##         $list_hour_from   - starting hour
##         $list_minute_from - starting minute
##         $list_hour_to     - ending hour
##         $list_minute_to   - ending minute
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: get_time_period_html, get_time_period, open, close,
##      system, unlink, process_statistics, print_output, print_listing_remarks,
##      print_unable_to_process_for_time_period
##
################################################################################
sub
list_records_by_time
{
	if ($Process_Flag eq "HTML")
	{
		local($list_days,
			$list_month_from,$list_day_from,$list_year_from,
			$list_month_to,$list_day_to,$list_year_to,
			$list_times,$list_hour_from,$list_minute_from,$list_hour_to,$list_minute_to)=@_;
	}

	# Declare local variables
	local(@dates_requested,@files_to_search,@msgFrag);
	local($start_date,$end_date,$start_time,$end_time);
	local($file);
	local($record_found,$file_matched,$i);
	local($date,$sendTime,$getTime,$dsoiAppl,$putAppl,$putApplTime,$putApplDate,$msgId);
	local($order_type,$order_number);
	local($sorted_temporary_file);
	my(@fields);
	


	# Initialize local variables
	@dates_requested=@files_to_search=@msgFrag=@fields={};
	$start_date=$end_date=$start_time=$end_time=0;
	$file="";
	$record_found=$file_matched=$i=0;
	$dsoiAppl=$putAppl=$msgId="";
	$date=$sendTime=$getTime=$putApplTime=$putApplDate=0;
	$order_type="";
	$order_number=0;
	$sorted_temporary_file="";


	# Get requested time period
	if ($Process_Flag eq "HTML")
	{
		($start_date,$end_date,$start_time,$end_time)=
			&get_time_period_html($list_days,$list_month_from,$list_day_from,$list_year_from,
				$list_month_to,$list_day_to,$list_year_to,
				$list_times,$list_hour_from,$list_minute_from,
				$list_hour_to,$list_minute_to);
	}
	else
	{
		($start_date,$end_date,$start_time,$end_time)=&get_time_period();
		if($start_date==0) 
		{	
			print "Sorry bad input \n";
			return 0;
		}

		# Let user know of processing
		print "\nProcessing...\n";
	}

	# build report filenames for dates requested
	@dates_requested = &build_dates_requested($start_date, $end_date);

	for ($i=0; $i<=$#dates_requested; $i++)
	{
		$files_to_search[$i]=sprintf("%s.%.6d",$BASE_REPORT_FILENAME,$dates_requested[$i]);
	}

	# Open temporary file for statistics processing
	if(!open(TEMP_FILE, ">".$TEMPORARY_FILE))  
	{
		print "Sorry can't open file $TEMPORARY_FILE :$!\n";
		return (0);
	}

	# Iterate over the files to search
	$record_found=0;
	for ($i=0; $i<=$#files_to_search; $i++)
	{
		$file=$files_to_search[$i];
		$file_matched=0;
		foreach $report_file (@Report_Files)
		{
			if ($file eq $report_file) { $file_matched=1; last; } else { next; }
		}

		if ($file_matched==1)
		{
			open(SEARCH_FILE, $file) || next;
			while (<SEARCH_FILE>)
			{
				/getTime=([0-9]+)~+/ && ($get_time = $1);
				if (($get_time gt $start_time) &&
					($get_time lt $end_time)   &&
					(($_ ne "\n") && ($_ ne "~\n")))
				{
					# Write output to temporary file
					print TEMP_FILE $_;
					$get_time=0;
					$record_found=1;
				}
			}
			close(SEARCH_FILE);
		}
		next; # @files_to_search
	} # end of for ($i=0; $i<=$#files_to_search; $i++)

	# Close temporary file
	&flush(TEMP_FILE);
	close(TEMP_FILE);

	# Sort temporary file before printing to screen and output file
	$sorted_temporary_file=$TEMPORARY_FILE.".sorted";

	if (-R $TEMP_FILE) 
	{
		print "File $TEMPORARY_FILE does not exist : $! \n";
		return 0;
	}
	elsif	(system("sort  -t~ -k 8,8   $TEMPORARY_FILE -o $sorted_temporary_file > $ERROR_FILE 2>&1") != 0)
	{
		print "Unable to execute system \"sort\" for \"$TEMPORARY_FILE\" : $! \n";
		&print_sort_error_message();
		return 0;
	}
	else
	{
		unlink($ERROR_FILE);
	}
	
	# Continue processing if matching service order messages were found
	if ($record_found == 1)
	{
		if ($Process_Flag eq "HTML")
		{
			# Process statistics
			&process_statistics($sorted_temporary_file);
		}

		# Print output heading to screen (80 char or full) and output file (80 char or full)
		if($Screen_Output_Flag ==0) 
		{
			print OUTPUT_FILE $STDOUT_TOP_PRINT,"\n";  
			print $STDOUT_TOP_PRINT,"\n";  
		}
		else 
		{
			print OUTPUT_FILE $STDOUT_80_TOP_PRINT,"\n";
			print $STDOUT_80_TOP_PRINT,"\n";
		}
	
		# Write output to screen and output file
		if (!open(SORTED_FILE, "<".$sorted_temporary_file) )
		{
			print "Sorry can't open file $sorted_temporary_file :$!\n";
			return 0;
		}
		while (<SORTED_FILE>)
		{
			# Clear variables
			$dsoiAppl=$putAppl=$msgId="";
			$date=$sendTime=$getTime=$putApplTime=$putApplDate=0;
			$order_type="";
			$order_number=0;
	
			@fields = split(/~/,$_);
			($fields[0] =~ /Date=(.*)/) && ($date = $1);
			($fields[1] =~ /sendTime=(.*)/) && ($sendTime = $1);
			($fields[2] =~ /getTime=(.*)/) && ($getTime = $1);
			($fields[3] =~ /DsoiAppl=(.*)/) && ($dsoiAppl = $1);
			($fields[4] =~ /PutAppl=(.*)/) && ($putAppl = $1);
			($fields[5] =~ /PutApplTime=(.*)/) && ($putApplTime = $1);
			($fields[6] =~ /PutApplDate=(.*)/) && ($putApplDate = $1);
			($fields[7] =~ /MsgId=([0-9]+)/) && ($msgId = $1);
			@msgFrag = join("~",@fields[8..$#fields]);

			# Extract order type & number values 
			($msgFrag[0] =~ /OrderType=([a-zA-Z])~+/) && ($order_type = $1);
			($msgFrag[0] =~ /OrderNumber=([0-9]{0,10})~+/) && ($order_number = $1);
			&print_output(STDOUT, $date, $sendTime, $getTime, $dsoiAppl, $order_type,
				$order_number, $putAppl, $putApplTime, $putApplDate, $msgId, @msgFrag);
			if ($Output_File_Flag==1)
			{
				&print_output(OUTPUT_FILE, $date, $sendTime, $getTime, $dsoiAppl, $order_type,
					$order_number, $putAppl, $putApplTime, $putApplDate, $msgId, @msgFrag);
			}
		}
		close(SORTED_FILE);

		# Print remarks about listing
		&print_listing_remarks();

		if ($Process_Flag eq "COMMANDLINE")
		{
			# Process statistics on this data if requested
			print "\nWould you like statistics on this data (y/n)? ";
			chop($statistics_requested = <STDIN>);
			if ($Output_File_Flag==1) { print OUTPUT_FILE "\nWould you like statistics on this data (y/n)? $statistics_requested\n"; }
			if ($statistics_requested =~ /^y/i)
			{
				&process_statistics($sorted_temporary_file);
			}
			else
			{
				print "No statistics are provided.\n";
			}
		}
	} #end of if ($record_found == 1)
	else
	{
		# Write that service order messages could not be found
		&print_unable_to_process_for_time_period($start_date,$end_date,$start_time,$end_time);;
	}

	# Remove temporary files
	unlink $TEMPORARY_FILE;
	unlink $sorted_temporary_file;
}


################################################################################
##
##   SUBROUTINE: list_service_order_by_order_type_and_number
##
##   DESCRIPTION: Lists service order given an order type and order number.
##
##   ENTRY PARAMETERS:
##      @Report_Files - array populated in build_report_files()
##      $ERROR_File - set in initialize_stuff()
##      $TEMPORARY_FILE - global set in initialize_stuff()
##      HTML inputs:
##         $list_month_from  - starting month
##         $list_day_from    - starting day
##         $list_year_from   - starting year
##         $list_month_to    - ending month
##         $list_day_to      - ending month
##         $list_year_to     - ending month
##         $list_times       - flag for all day or specific time period
##         $list_hour_from   - starting hour
##         $list_minute_from - starting minute
##         $list_hour_to     - ending hour
##         $list_minute_to   - ending minute
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print, get_time_period_html, get_time_period, open,
##      close, flush, system, split, unlink, process_statistics, print_output,
##      print_listing_remarks, print_unable_to_process_for_time_period
##
################################################################################
sub
list_service_order_by_order_type_and_number
{
	if ($Process_Flag eq "HTML")
	{
		local($order_type_requested,$order_number_requested,
		      $list_days,$list_month_from,$list_day_from,$list_year_from,
		      $list_month_to,$list_day_to,$list_year_to,
		      $list_times,$list_hour_from,$list_minute_from,$list_hour_to,$list_minute_to)=@_;
	}
	else
	{
		local($order_type_requested,$order_number_requested);
		$order_type_requested="";
		$order_number_requested=0;
	}

	## Declare local variables
	local($start_date,$end_date,$start_time,$end_time);
	local(@dates_requested,@files_to_search);
	local($file,$record_found);
	local($date,$sendTime,$getTime,$dsoiAppl,$putAppl,$putApplTime,$putApplDate,$msgId,$msgFrag);
	local($order_type,$order_number);
	local($prev_msgId,$prev_order_type,$prev_order_number);
	local($temp_file_1,$temp_file_2,$sorted_temp_file_1,$sorted_temp_file_2);
	local($date,$sendTime,$getTime,$dsoiAppl,$putAppl,$putApplTime,$putApplDate,$msgId,$msgFrag);
	local($all_days_flag);
	my(@fields);
	## Declare local variables

	## Initialize local variables
	$start_date=$end_date=$start_time=$end_time=0;
	@dates_requested=@files_to_search=@fields={};
	$file=$record_found=0;
	$dsoiAppl=$putAppl=$msgId=$msgFrag=0;
	$date=$sendTime=$getTime=$putApplTime=$putApplDate=0;
	$order_type=$order_number=0;
	$prev_msgId=$prev_order_type=$prev_order_number=0;
	$temp_file_1=$temp_file_2=$sorted_temp_file_1=$sorted_temp_file_2=0;
	$date=$sendTime=$getTime=$dsoiAppl=$putAppl=$putApplTime=$putApplDate=$msgId=$msgFrag=0;
	$all_days_flag=0;
	## Initialize local variables

	if ($Process_Flag eq "HTML")
	{
		# Reformat order type & number, if necessary
		$order_type_requested =~ tr/a-z/A-Z/; #capitalize
		if ($Output_File_Flag==1)
		{
			print OUTPUT_FILE "\nService order type requested: $order_type_requested\n";
			print OUTPUT_FILE "Service order number requested: $order_number_requested\n";
		}

		if ($list_days !~ /^all/i)
		{
			($start_date,$end_date,$start_time,$end_time)=
				&get_time_period_html($list_days,$list_month_from,$list_day_from,$list_year_from,
					$list_month_to,$list_day_to,$list_year_to,
					$list_times,$list_hour_from,$list_minute_from,
					$list_hour_to,$list_minute_to);
		}
	} ##end of HTML
	else ##prompt user
	{
		# Get order type
		print "Please enter the service order type: ";
		chop($order_type_requested = <STDIN>);
		$order_type_requested =~ tr/a-z/A-Z/; #capitalize the request
		if ($Output_File_Flag==1) { print OUTPUT_FILE "\nPlease enter the service order type: $order_type_requested\n"; }
	
		# Get order number
		print "Please enter the service order number: ";
		chop($order_number_requested = <STDIN>);
		if ($Output_File_Flag==1) { print OUTPUT_FILE "Please enter the service order number: $order_number_requested\n"; }
	
		print "Search over all valid days (y/n)? ";
		chop($all_days_flag = <STDIN>);
		if ($Output_File_Flag==1) { print OUTPUT_FILE "Search over all valid days? $all_days_flag\n"; }

		if ($all_days_flag !~ /^y/i)
		{
			($start_date,$end_date,$start_time,$end_time)=&get_time_period();
			if($start_date==0)
			{ 
				print "Sorry bad input \n";
				return 0;
			}
		}

		# Let user know of processing
		print "\nProcessing...\n";
	} #end of else prompt user

	# Open first temporary file to hold data from @Report_Files
	$temp_file_1 = $TEMPORARY_FILE; #global set to /tmp/DSOIReportUtil.temp.$$
	if(!open(TEMP_FILE_1, ">$TEMPORARY_FILE") )
	{
		print  "Unable to open $TEMPORARY_FILE error:$!\n";
		return 0;
	}

	if ((($Process_Flag eq "HTML") && ($list_days =~ /^all/i)) ||
	    (($Process_Flag eq "COMMANDLINE") && ($all_days_flag =~ /^y/i)))
	{
		# Transfer data in @Report_Files to first temporary file
		foreach $file (@Report_Files)
		{
			open(REPORT_FILE, $file) || next;
			while (<REPORT_FILE>)
			{
				if (($_ ne "\n") && ($_ ne "~\n"))
				{
					# Write output to first temporary file
					print TEMP_FILE_1 $_;
				}
			}
			close(REPORT_FILE);
		}
	} #end of HTML
	else # specific time period
	{
		# build report filenames for dates requested
		@dates_requested = &build_dates_requested($start_date, $end_date);
		for ($i=0; $i<=$#dates_requested; $i++)
		{
			$files_to_search[$i]=sprintf("%s.%.6d",$BASE_REPORT_FILENAME,$dates_requested[$i]);
		}

		# Iterate over the files to search
		$record_found=0;
		for ($i=0; $i<=$#files_to_search; $i++)
		{
			$file=$files_to_search[$i];
			$file_matched=0;
			foreach $report_file (@Report_Files)
			{
				if ($file eq $report_file) { $file_matched=1; last; } else { next; }
			}
	
			if ($file_matched==1)
			{
				open(SEARCH_FILE, $file) || next;
				while (<SEARCH_FILE>)
				{
					/getTime=([0-9]+)~+/ && ($get_time = $1);
					if (($get_time gt $start_time) &&
						($get_time lt $end_time)   &&
						(($_ ne "\n") && ($_ ne "~\n")))
					{
						# Write output to temporary file
						print TEMP_FILE_1 $_;
	
						$record_found=1;
						$get_time =0;
					}
				}
				close(SEARCH_FILE);
			}
			next; # @files_to_search
		}
	} # end of else not HTML

	# Close first temporary file
	&flush(TEMP_FILE_1);
	close(TEMP_FILE_1);

	# Sort first temporary file by msgId
	$sorted_temp_file_1=$temp_file_1.".sorted";
	if (-r $TEMP_FILE_1) 
	{
		print "Sorry can't open file $temp_file_1, error: $! line = $__LINE__\n";
		return 0;
	}
	elsif (system("sort -k 8,8 -t~ $temp_file_1 -o $sorted_temp_file_1 > $ERROR_FILE 2>&1") != 0)
	{
		print "Unable to execute system \"sort\" for \"$temp_file_1\" \n error=$! \n";
		&print_sort_error_message();
		return 0;
	}
	else
	{
		unlink($ERROR_FILE);
	}

	# Open second temporary file for statistics processing
	# /tmp/DSOIReportUtil.temp.$$._2
	$temp_file_2 = $TEMPORARY_FILE."_2";
	if (!open(TEMP_FILE_2, ">$temp_file_2") )
	{
		print  "Unable to open $temp_file_2\n";
		return 0;
	}

	# Search first sorted temporary file by order type & number and by msgId; the msgId correlation is necessary
	# since some DSOIReply messages lack order type & number
	$record_found=0;
	if (open(SORTED_TEMP_FILE_1, $sorted_temp_file_1) != 0) #open for reading
	{
		while (<SORTED_TEMP_FILE_1>)
		{
			#initialize $order_type and $order_number
			$order_type="";
			$order_number=0;
			# Extract order type, order number and msgId
			/OrderType=([a-zA-Z])~+/ && ($order_type = $1);
			/OrderNumber=([0-9]{0,10})~+/ && ($order_number = $1);
			/MsgId=([0-9]+)~?/ && ($msgId = $1);


			if ( (($order_type eq $order_type_requested) && ($order_number == $order_number_requested)) ||
			     (($order_type eq "") && ($order_number == 0) && ($prev_order_type eq $order_type_requested) &&
			      ($prev_order_number == $order_number_requested) && ($msgId eq $prev_msgId)) )
			{
				if (($_ ne "\n") && ($_ ne "~\n"))
				{
					# Write output to temporay file
					print TEMP_FILE_2 $_;
					$record_found=1;
				}
			}

			# Store current as previous
			$prev_msgId        = $msgId;
			$prev_order_type   = $order_type;
			$prev_order_number = $order_number;
		}
		close(SORTED_TEMP_FILE_1);
	}

	# Close second temporary file
	&flush(TEMP_FILE_2);
	close(TEMP_FILE_2);

	# Continue processing if matching service order messages were found
	if ($record_found == 1)
	{
		# Resort second temporary file before printing to screen and output file
		$sorted_temp_file_2=$temp_file_2.".sorted";
		if(-r $TEMP_FILE_2) 
		{
			print "Sorry can't open file $temp_file_2, error: $! \n";
			return 0;
		}
		elsif (system("sort -k 8,8 -t~ $temp_file_2 -o $sorted_temp_file_2 > $ERROR_FILE 2>&1") != 0)
		{
			print "Unable to execute system \"sort\" for \"$sorted_temp_file_2\" \n error=$! \n";
			&print_sort_error_message();
		}
		else
		{
			unlink($ERROR_FILE);
		}

		if ($Process_Flag eq "HTML")
		{
			# Process statistics
			&process_statistics($sorted_temp_file_2);
		}

		# Print output heading to screen (80 char or full) and output file (full)
		if ($Screen_Output_Flag==0) { print $STDOUT_TOP_PRINT,"\n"; } else { print $STDOUT_80_TOP_PRINT,"\n"; }
		print OUTPUT_FILE $STDOUT_TOP_PRINT,"\n";

		# Write output to screen and output file
		open(SORTED_TEMP_FILE_2, "<".$sorted_temp_file_2);
		while (<SORTED_TEMP_FILE_2>)
		{
			# Clear variables
			$dsoiAppl=$putAppl=$msgId=$msgFrag="";
			$date=$sendTime=$getTime=$putApplTime=$putApplDate=0;
			$order_type="";
			$order_number=0;
	
			# Extract order type & number values
			/OrderType=([a-zA-Z])~?/ && ($order_type = $1);
			/OrderNumber=([0-9]{0,10})~?/ && ($order_number = $1);
	
			# Extract field values
			@fields= split(/~/,$_);
			($fields[0] =~ /Date=([0-9]+)/) && ($date = $1);
			($fields[1] =~ /sendTime=([0-9]+)/) && ($sendTime = $1);
			($fields[2] =~ /getTime=([0-9]+)/) && ($getTime = $1);
			($fields[3] =~ /DsoiAppl=(.*)/) && ($dsoiAppl = $1);
			($fields[4] =~ /PutAppl=(.*)/) && ($putAppl = $1);
			($fields[5] =~ /PutApplTime=([0-9]+)/) && ($putApplTime=$1);
			($fields[6] =~ /PutApplDate=([0-9]+)/) && ($putApplDate=$1);
			($fields[7] =~ /MsgId=([0-9]+)/) && ($msgId = $1);
			@msgFrag = join("~",@fields[8..$#fields]);
	
			&print_output(STDOUT, $date, $sendTime, $getTime, $dsoiAppl, $order_type,
				$order_number, $putAppl, $putApplTime, $putApplDate, $msgId, @msgFrag);
			if ($Output_File_Flag==1)
			{
				&print_output(OUTPUT_FILE, $date, $sendTime, $getTime, $dsoiAppl, $order_type,
					$order_number, $putAppl, $putApplTime, $putApplDate, $msgId, @msgFrag);
			}
		}
		close(SORTED_TEMP_FILE_2);

		# Print remarks about statistical data
		&print_listing_remarks();

		if ($Process_Flag eq "COMMANDLINE")
		{
			# Process statistics on this data if requested
			print "\nWould you like statistics on this data (y/n)? ";
			chop($statistics_requested = <STDIN>);
			if ($Output_File_Flag==1) { print OUTPUT_FILE "\nWould you like statistics on this data (y/n)? $statistics_requested\n"; }
			if ($statistics_requested =~ /^y/i)
			{
				&process_statistics($sorted_temp_file_2);
			}
			else
			{
				print "No statistics are provided.\n";
			}
		}
	}
	else
	{
		print "Unable to find service order #$order_number_requested of type \"$order_type_requested\"";
		if ($Output_File_Flag==1)
		{
			print OUTPUT_FILE "Unable to find service order #$order_number_requested of type \"$order_type_requested\"";
		}

		# Write that service order messages could not be found
		&print_unable_to_process_for_time_period($start_date,$end_date,$start_time,$end_time,"short");
	}

	# Remove temporary files
	unlink $temp_file_1;
	unlink $temp_file_2;
	unlink $sorted_temp_file_1;
	unlink $sorted_temp_file_2;
}


################################################################################
##
##   SUBROUTINE: print_help
##
##   DESCRIPTION: Prints detailed help text.
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print
##
################################################################################
sub
print_help
{

if ($Process_Flag eq "COMMANDLINE")
{
	print <<HelpText;
\n================================================================================
                                  Overview\n
The Distributed Service Order Interface (DSOI) application routes service order
messages between requesting applications and service order processors (SOPs).
The DSOIRequest process receives service order requests from requestors such as
ICADS, IFE, SOC or CodeTalker and routes them to the appropriate SOP. The DSOI-
Reply process then routes the service order messages processed by the SOP back
to the requestor.

This DSOIReport utility provides access to the service order messages processed
by DSOI and to DSOI performance. Only data for 10 calendar days, today and 9
days prior, can be accessed. The following functionality is available:

   * lists the services order messages processed for a specified time period
   * lists the service order messages processed for a specified order type and
     number
   * computes statistics on the service order messages processed for a specified
     time period
   * computes statistics on the service order messages processed for the past 10
     calendar days

The list functions generate the requested listing and statistics on that listing.
The statistics functions only provide statistics on the requested data.

The abbreviated headings for the statistical output translate to the following:

   * CompTrans = complete transactions
   * IncompTrans = incomplete transactions
   * DSOI+SOPAvgTime/Trans = average time for DSOI and SOP processing per
     transaction
   * DSOIAvgTime/Trans = average time for only DSOI processing per transaction
   * SOPAvgTime/Trans = average time for SOP processing per transaction

Since DSOIRequest routes service order messages to a SOP and DSOIReply routes
them back from a SOP, a DSOIRequest/Reply pair constitutes a DSOI transaction--
a successfully processed service order message. The message ids in the listings
can be used to correlate a transaction. An incomplete transaction is either a
DSOIRequest or DSOIReply without its matching pair. Usually, the orphaned
service order message will be a DSOIRequest without its DSOIReply counterpart.

================================================================================\n
HelpText
}
else
{
	print <<HelpText;
<H1><CENTER><FONT SIZE=+1>Overview</FONT></CENTER></H1>
The Distributed Service Order Interface (DSOI) application routes service order
messages between requesting applications and service order processors (SOPs).
The DSOIRequest process receives service order requests from requestors such as
ICADS, IFE, SOC or CodeTalker and routes them to the appropriate SOP. The DSOI-
Reply process then routes the service order messages processed by the SOP back
to the requestor.
<BR>
This DSOIReport utility provides access to the service order messages processed
by DSOI and to DSOI performance. Only data for 10 calendar days, today and 9
days prior, can be accessed. The following functionality is available for
service order messages processed by DSOI:
<UL>
<LI><A HREF="../dsoi_report_list_record.html" TARGET="body">lists messages for a specified time period</A>
<LI><A HREF="../dsoi_report_list_order.html" TARGET="body">lists messages for a specified order type and number</A>
<LI><A HREF="../dsoi_report_stat_record.html" TARGET="body">computes statistics on messages for a specified time period</A>
<LI><A HREF="DSOI_report.pl?menu_choice=cumm" TARGET="body">computes statistics on messages for the past 10 calendar days</A>
</UL>
The list functions generate the requested listing and statistics on that listing.
The statistics functions only provide statistics on the requested data.
<BR>
The abbreviated headings for the statistical output translate to the following:
<UL>
<LI>CompTrans = complete transactions
<LI>IncompTrans = incomplete transactions
<LI>DSOI+SOPAvgTime/Trans = average time for DSOI and SOP processing per transaction
<LI>DSOIAvgTime/Trans = average time for only DSOI processing per transaction
<LI>SOPAvgTime/Trans = average time for SOP processing per transaction
</UL>
Since DSOIRequest routes service order messages to a SOP and DSOIReply routes
them back from a SOP, a DSOIRequest/Reply pair constitutes a DSOI transaction--
a successfully processed service order message. The message ids in the listings
can be used to correlate a transaction. An incomplete transaction is either a
DSOIRequest or DSOIReply without its matching pair. Usually, the orphaned
service order message will be a DSOIRequest without its DSOIReply counterpart.
HelpText
}

} # sub print_help


################################################################################
##
##   SUBROUTINE: print_intro
##
##   DESCRIPTION: Prints introductory information.
##
##   ENTRY PARAMETERS: $Output_File - global set in initialize_stuff()
##
##   EXIT PARAMETERS:
##      $Screen_Output_Flag - global indicating full or 80 character screen
##                            output; 1 = full, 0 = 80 char
##      $Output_File - global indicating that output should be written to
##                     a flat file
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: local, print, chop, write_to_output_file
##
################################################################################
sub
print_intro
{
	local($screen_flag,$output_flag);

	# Display welcome message
	print "\n                   Welcome to $Tool_Name.\n\n";

	# Ask if screen output should be full or limited to 80 characters
	print "Would you like full output to the screen or limited to 80 characters (l/f)? ";
	chop($screen_flag = <STDIN>);
	if ($screen_flag =~ /^l/i) { $Screen_Output_Flag = 1; }

	# Ask if output should go to a flat file
	print "Would you like output sent to a flat file as well (y/n)? ";
	chop($output_flag = <STDIN>);
	if ($output_flag =~ /^y/i)
	{
		&write_to_output_file();
	}
}


################################################################################
##
##   SUBROUTINE: print_listing_remarks
##
##   DESCRIPTION: Prints comments clarifying the order of the listings.
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print
##
################################################################################
sub
print_listing_remarks
{
	printf "\nNote: The listing is sorted by the msgId so that corresponding DSOIRequest/\n";
	printf "Reply messages are paired together; consequently, the ordering may not correlate\n";
	printf "with SendTime/GetTime.\n";
}


################################################################################
##
##   SUBROUTINE: print_menu
##
##   DESCRIPTION: Displays menu
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: $Menu_Response - global with menu choice
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print, chop
##
################################################################################
sub
print_menu
{
print <<Menu;
\nPlease select from one of the following:
\n   (l)ist of service order messages processed by DSOI for a given time period
   (s)how service order messages given order type and number
   (n)umber of service order messages processed by DSOI for a given time period
   (c)ummulative number of service order messages processed by DSOI for past 10 calendar days

   (r)everse screen output length
   (w)rite output to a file
   (h)elp
   (q)uit
Menu

	chop($Menu_Response = <STDIN>);
	if ($Output_File_Flag==1) { print OUTPUT_FILE "\nPlease select from one of the following:... $Menu_Response\n"; }
}


################################################################################
##
##   SUBROUTINE: print_output
##
##   DESCRIPTION: Prints output either for 80 characters or all ("full").
##
##   ENTRY PARAMETERS:
##	    $Screen_Output_Flag
##	    $output
##	    $date
##	    $sendTime
##	    $getTime
##	    $dsoiAppl
##	    $order_type
##	    $order_number
##	    $putAppl
##	    $putApplTime
##	    $putApplDate
##	    $msgId
##	    $msgFrag
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: local, printf
##
################################################################################
sub
print_output
{
	local($output,$date,$sendTime,$getTime,$dsoiAppl,$order_type,$order_number,$putAppl,
		$putApplTime,$putApplDate,$msgId,$msgFrag)=@_;
	if ($Process_Flag eq "HTML")
	{
		if ($Screen_Output_Flag==0) # full
		{
			printf "%8.8s %8.8s %8.8s %-11.11s %-1.1s%-10.10s %16.16s %-13.13s %-13.13s %-21.21s %-94.94s\n",
				$date, $sendTime, $getTime, $dsoiAppl, $order_type, $order_number, $putAppl,
				$putApplTime, $putApplDate, substr($msgId,-21,21), $msgFrag;
		}
		else # 80 character
		{
			printf "%8.8s %8.8s %8.8s %-11.11s %-1.1s%-10.10s %-10.10s %19.19s\n",
				$date, $sendTime, $getTime, $dsoiAppl, $order_type, $order_number,
				substr($msgId,-10,10), $msgFrag;
		}
	}
	else
	{
		if ($Screen_Output_Flag==0) # full
		{
			printf $output "%8.8s %8.8s %8.8s %-11.11s %1.1s%-10.10s %-16.16s %-13.13s %-13.13s %-21.21s %-94.94s\n",
				$date, $sendTime, $getTime, $dsoiAppl, $order_type, $order_number, $putAppl,
				$putApplTime, $putApplDate, substr($msgId,-21,21), $msgFrag;
		}
		else # 80 character
		{
			printf $output "%8.8s %8.8s %8.8s %-11.11s %1.1s%-10.10s %-10.10s %19.19s\n",
				$date, $sendTime, $getTime, $dsoiAppl, $order_type, $order_number, 
				substr($msgId,-10,10), $msgFrag;
		}
	}
}

################################################################################
##
##   SUBROUTINE: print_sort_error_message
##
##   DESCRIPTION: Prints error message for failed sort
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print
##
################################################################################
sub
print_sort_error_message
{
	$current_dir = $ENV{'PWD'};
	print "\nError detected: UNIX \"sort\" command failed to complete - check \"$ERROR_FILE\" for details.\n";
	print ERROR_LOG "The UNIX \"sort\" command may have failed due to an anomaly in the report files.\n";
	print ERROR_LOG "They can be found in the directory \"$current_dir/$REPORT_DIRECTORY\".\n";
	print ERROR_LOG "Note that \"sort\" is not able to process a file with embedded nulls.\n\n";
	&flush(ERROR_LOG);
}



################################################################################
##
##   SUBROUTINE: print_output_statistics
##
##   DESCRIPTION: Prints statistics output
##
##   ENTRY PARAMETERS:
##      $output
##      $completeTransactions
##      $dsoiSopAvgTime
##      $dsoiAvgTime
##      $sopAvgTime
##      $incompleteTransactions
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: printf
##
################################################################################
sub
print_output_statistics
{
	local($output,$completeTransactions,$dsoiSopAvgTime,$dsoiAvgTime,$sopAvgTime,$incompleteTransactions)=@_;

	printf $output "%9.9s %21.10s %17.10s %16.10s %11.11s\n",
		$completeTransactions, $dsoiSopAvgTime, $dsoiAvgTime, $sopAvgTime, $incompleteTransactions;
}


################################################################################
##
##   SUBROUTINE: print_unable_to_process_for_time_period
##
##   DESCRIPTION: Prints error message indicating failure to find service order
##      transaction information for requested date/time period.
##
##   ENTRY PARAMETERS:
##      $start_date, $end_date - date period boundaries
##      $start_time, $end_time - time period boundaries
##      $Output_File_Flag - global specifying output to be written to a flat file
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print
##
################################################################################
sub
print_unable_to_process_for_time_period
{
	# Declare & initialize local variables
	local($start_date,$end_date,$start_time,$end_time,$length_flag)=@_;
	my(@reports);
	local($msg);
	$msg="";

	for ($i=0; $i <= $#Report_Files; $i++)
	{
		$Report_Files[$i] =~ (/DSOI_report\.log\.([0-9]*)/) && ($reports[$i]=$1);
	}
	# Write message for single day request
	if ($start_date == $end_date)
	{
		if ($start_time == $end_time) # all day
		{
			if ($length_flag eq "short")
			{
				$msg=" for $start_date.\n";
			}
			else
			{
				$msg="Unable to find service order messages for $start_date.\n";
			}
			print $msg;
			if ($Output_File_Flag==1) { print OUTPUT_FILE $msg; }
		}
		else # specified period
		{
			if ($length_flag eq "short")
			{
				$msg=" for $start_date for time period from $start_time to $end_time.\n";
			}
			else
			{
				$msg="\nUnable to find log files containing service order messages for $start_date for time period from $start_time to $end_time. \nDates of log files available: @reports.\n";
			}
			print $msg;
			if ($Output_File_Flag==1) { print OUTPUT_FILE $msg; }
		}
	}
	else # Write message for multi-day request
	{
		if ($start_time == $end_time) # all day
		{
			if ($length_flag eq "short")
			{
				$msg=" between $start_date and $end_date.\n";
			}
			else
			{
				$msg="Unable to find service order messages between $start_date and $end_date.\n";
			}
			print $msg;
			if ($Output_File_Flag==1) { print OUTPUT_FILE $msg; }
		}
		else # specified period
		{
			if ($length_flag eq "short")
			{
				$msg=" between $start_date and $end_date for time period from $start_time to $end_time.\n";
			}
			else
			{
				$msg="Unable to find service order messages between $start_date and $end_date for time period from $start_time to $end_time.\n";
			}
			print $msg;
			if ($Output_File_Flag==1) { print OUTPUT_FILE $msg; }
		}
	}
}


################################################################################
##
##   SUBROUTINE: print_unknown_menu_option
##
##   DESCRIPTION: Prints message indicating invalid menu choice.
##
##   ENTRY PARAMETERS: None
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print
##
################################################################################
sub
print_unknown_menu_option
{
print <<UnknownMenuOptionText;
\n>> Choice "$Menu_Response" is not a valid menu option. Please try again. <<
UnknownMenuOptionText
}


################################################################################
##
##   SUBROUTINE: process_cmd_line_args
##
##   DESCRIPTION: Checks that no "-" options are given.
##
##   ENTRY PARAMETERS: @ARGV - global arguments passed on the command line
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES:
##       0 -- bad/invalid arguments were specified on the command line
##       1 -- argument processing ok
##
##   SUBROUTINES USED: local
##
################################################################################
sub
process_cmd_line_args
{
	local($option);

	# $#ARGV starts at 0
	if ($#ARGV < 0)
	{
		return 0;
	}

	while ($ARGV[0] =~ /^-/)
	{
		$option = $ARGV[0];

		##############################################################
		# -a option is for archiving invalid report files
		##############################################################
		if ($option =~ /^-a/)
		{
			$Archive_Flag = 1;

			shift @ARGV;
			next;
		}
		elsif ($option =~ /^-q/)
		{
			shift @ARGV;
			$Queue_Manager_Name=$ARGV[0];

			shift @ARGV;
			next;
		}
		else
		{
		##############################################################
		# all other options are invalid
		##############################################################
			print STDERR "\nERROR: unknown option \"$ARGV[0]\"\n\n";
			return 0;
		}
	}
	return 1;
}


################################################################################
##
##   SUBROUTINE: process_statistics
##
##   DESCRIPTION: Processes statistics on data located in a file. First sorts
##      input file by msgId, then cycles through messages to find complete
##      transactions and orphaned messages. A complete transaction will include
##	report log entries from DSOIRouteRequest, DSOIRequest, DSOIReply and	##	maybe DSOIRouteReply depending if the service request required a trip	##	to 4P services or not.
##
##	Maintains running total of DSOI+4P+SOP, DSOI-only, 4P services only,
##	and SOP-only processing times; then computes average DSOI+4P+SOP,
##	DSOI-only, 4P services only, and SOP-only processing times.
##
##      When processing the times, hours and minutes are converted to seconds
##      before the averages are done.
##
##   ENTRY PARAMETERS: $file - input file(s) which must be already sorted
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: local, print, split, open, close, unlink,
##      convert_to_seconds, compute_day_difference, print_output_statistics
##
################################################################################
sub
process_statistics
{
	local(@input_sorted_files)=@_;

	# Declare local variables
	local($MSG_ID_IDX,$APPL_IDX,$GET_IDX,$SEND_IDX,$DATE_IDX);
	local($SECONDS_IN_24_HOURS);
	local($date,$sendTime,$getTime,$putApplTime,$putApplDate);
	local($dsoiAppl,$putAppl,$msgId,$msgFrag);
	local(@msg1,@msg2,@msg3);
	local($completeDSOISopTime,$completeSopTime);
	local($completeTransactions,$incompleteTransactions,$i);
	local($sorted_file);
	local($request_date,$reply_date,$days_between_request_and_reply);
	local($requestGetTime,$requestSendTime,$replyGetTime,$replySendTime);
	my(@fields);

	# Initialize local variables
	$MSG_ID_IDX=0;$APPL_IDX=1;$GET_IDX=2;$SEND_IDX=3,$DATE_IDX=4;
	$date=$sendTime=$getTime=$putApplTime=$putApplDate=0;
	$dsoiAppl=$putAppl=$msgId=$msgFrag="";
	@msg1=@msg2=@msg3=@fields={};
	$completeDSOISopTime=$completeSopTime=0;
	$completeTransactions=$incompleteTransactions=$i=0;
	$sorted_file="";
	$request_date=$reply_date=$days_between_request_and_reply=0;
	$requestGetTime=$requestSendTime=$replyGetTime=$replySendTime=0;
	$SECONDS_IN_24_HOURS=86400;	

	# Specify format for screen and output file, then print header(s)
	print $STATISTICS_TOP_PRINT,"\n";
	print OUTPUT_FILE $STATISTICS_TOP_PRINT,"\n";

	foreach $sorted_file (@input_sorted_files)
	{
		# Open sorted file, then process
		open(FILE, $sorted_file) || next;
		while (<FILE>)
		{
			if (($_ ne "\n") && ($_ ne "~\n"))
			{
				# Extract necessary field values
				@fields = split(/~/,$_);
				($fields[0] =~ /Date=(.*)/) && ($date = $1);
				($fields[1] =~ /sendTime=(.*)/) && ($sendTime = $1);
				($fields[2] =~ /getTime=(.*)/) && ($getTime = $1);
				($fields[3] =~ /DsoiAppl=(.*)/) && ($dsoiAppl = $1);
				($fields[4] =~ /PutAppl=(.*)/) && ($putAppl = $1);
				($fields[5] =~ /PutApplTime=(.*)/) && ($putApplTime = $1);
				($fields[6] =~ /PutApplDate=(.*)/) && ($putApplDate = $1);
				($fields[7] =~ /MsgId=([0-9]+)/) && ($msgId = $1);
				@msgFrag = join("~",@fields[8..$#fields]);

				# Save in order: $msgId, $dsoiAppl, $getTime & $sendTime values of the last three messages
				# Age values on each iteration: $msg1 = most current, $msg3 = least current
				$msg3[$MSG_ID_IDX]=$msg2[$MSG_ID_IDX];
				$msg2[$MSG_ID_IDX]=$msg1[$MSG_ID_IDX];
				$msg1[$MSG_ID_IDX]=$msgId;

				$msg3[$APPL_IDX]=$msg2[$APPL_IDX];
				$msg2[$APPL_IDX]=$msg1[$APPL_IDX];
				$msg1[$APPL_IDX]=$dsoiAppl;

				$msg3[$GET_IDX]=$msg2[$GET_IDX];
				$msg2[$GET_IDX]=$msg1[$GET_IDX];
				$msg1[$GET_IDX]=$getTime;

				$msg3[$SEND_IDX]=$msg2[$SEND_IDX];
				$msg2[$SEND_IDX]=$msg1[$SEND_IDX];
				$msg1[$SEND_IDX]=$sendTime;

				$msg3[$DATE_IDX]=$msg2[$DATE_IDX];
				$msg2[$DATE_IDX]=$msg1[$DATE_IDX];
				$msg1[$DATE_IDX]=$date;

				# Check for REQUEST/REPLY transaction pair
				if ($msg1[$MSG_ID_IDX] eq $msg2[$MSG_ID_IDX])
				{
					# Initialize various parameters
					$days_between_request_and_reply=0;

					# Increment complete transactions counter
					$completeTransactions++;

					# Compute DSOI+SOP transaction & SOP processing times
					#    DSOI+SOP transaction time would be (REPLY $sendTime - REQUEST $getTime)
					#    SOP processing times would be (REPLY $getTime - REQUEST $sendTime)
					if (($msg2[$APPL_IDX] =~ $DSOIREQUEST_STRING) &&
						($msg1[$APPL_IDX] =~ $DSOIREPLY_STRING))
					{
						# Increment complete transactions counter
						$completeTransactions++;

						# Convert getTime and sendTime to seconds
						$requestGetTime=&convert_to_seconds($msg2[$GET_IDX]);
						$requestSendTime=&convert_to_seconds($msg2[$SEND_IDX]);

						$replyGetTime=&convert_to_seconds($msg1[$GET_IDX]);
						$replySendTime=&convert_to_seconds($msg1[$SEND_IDX]);

						# DSOIRequest ($msg2) record is encountered before DSOIReply ($msg1)
						$request_date=$msg2[$DATE_IDX];
						($request_date =~ /Date=/) && ($request_date = $');
						$reply_date=$msg1[$DATE_IDX];
						($reply_date =~ /Date=/) && ($reply_date = $');
						$days_between_request_and_reply=&compute_day_difference($request_date,$reply_date);

						if ($days_between_request_and_reply != 0)
						{
							# 12311999 < 01012000 (for example)
							if ( ($request_date < $reply_date) || (substr($request_date,-4,4) < substr($reply_date,-4,4)) )
							{
								$replyGetTime  += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
								$replySendTime += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
							}
							else
							{
								# Should never enter here since request should always come before a reply
								$requestGetTime  += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
								$requestSendTime += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
							}
						}

						$transactionTime=($replySendTime-$requestGetTime);
						$sopTime=($replyGetTime-$requestSendTime);
					}
					elsif (($msg2[$APPL_IDX] =~ $DSOIREPLY_STRING) &&
						   ($msg1[$APPL_IDX] =~ $DSOIREQUEST_STRING))
					{
						# Increment complete transactions counter
						$completeTransactions++;

						# Convert getTime and sendTime to seconds
						$requestGetTime=&convert_to_seconds($msg1[$GET_IDX]);
						$requestSendTime=&convert_to_seconds($msg1[$SEND_IDX]);

						$replyGetTime=&convert_to_seconds($msg2[$GET_IDX]);
						$replySendTime=&convert_to_seconds($msg2[$SEND_IDX]);

						# DSOIReply ($msg2) record is encountered before DSOIRequest ($msg1)
						$request_date=$msg1[$DATE_IDX];
						($request_date =~ /Date=/) && ($request_date = $');
						$reply_date=$msg2[$DATE_IDX];
						($reply_date =~ /Date=/) && ($reply_date = $');
						$days_between_request_and_reply=&compute_day_difference($request_date,$reply_date);

						if ($days_between_request_and_reply != 0)
						{
							# 12311999 < 01012000 (for example)
							if ( ($request_date < $reply_date) || (substr($request_date,-4,4) < substr($reply_date,-4,4)) )
							{
								$replyGetTime  += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
								$replySendTime += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
							}
							else
							{
								# Should never enter here since request should always come before a reply
								$requestGetTime  += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
								$requestSendTime += $days_between_request_and_reply * $SECONDS_IN_24_HOURS;	
							}
						}

						$transactionTime=($replySendTime-$requestGetTime);
						$sopTime=($replyGetTime-$requestSendTime);
					}
					else # Zero times since there was no match
					{
						# Ideally this else block should never be executed since the msgId's matched

						$incompleteTransactions++;
						$transactionTime=0;
						$sopTime=0;
					}

					$completeDSOISopTime+=$transactionTime;
					$completeSopTime+=$sopTime;
				}
				elsif (($msg3[$MSG_ID_IDX] ne $msg2[$MSG_ID_IDX]) &&
					   ($msg2[$MSG_ID_IDX] ne $msg1[$MSG_ID_IDX]))
				{
					$incompleteTransactions++;
				}
			}
		}

		# Increment $incompleteTransactions if last service order is unmatched
		if (($msg1[$MSG_ID_IDX] ne $msg2[$MSG_ID_IDX]) &&
			($msg1[$MSG_ID_IDX] ne $msg3[$MSG_ID_IDX]))
		{
			$incompleteTransactions++;
		}

		# Close & remove sorted file
		close(FILE, $sorted_file);
	}

	# Compute statistics
	if ($completeTransactions==0)
	{
		$dsoiSopAvgTime=0;
		$sopAvgTime=0;
		$dsoiAvgTime=0;
	}
	else
	{
		$dsoiSopAvgTime = $completeDSOISopTime/$completeTransactions;
		$sopAvgTime	= $completeSopTime/$completeTransactions;
		$dsoiAvgTime    = $dsoiSopAvgTime-$sopAvgTime;
	}

	# Write statistics
	&print_output_statistics(STDOUT,$completeTransactions,$dsoiSopAvgTime,$dsoiAvgTime,$sopAvgTime,$incompleteTransactions);
	if ($Output_File_Flag==1)
	{
		&print_output_statistics(OUTPUT_FILE,$completeTransactions,$dsoiSopAvgTime,$dsoiAvgTime,$sopAvgTime,$incompleteTransactions);
	}

	if ($completeTransactions==0)
	{
		print <<NoStatisticsMessage;
The time statistics are "0" because there were no complete transaction pairs,
i.e. there were no matching DSOIREQUEST/REPLY pairs.
NoStatisticsMessage
	}
	else
	{
		# Print remarks about statistical data
		printf "\nNote: The statistical times are given in seconds.\n";
	}
}


################################################################################
##
##   SUBROUTINE: quit
##
##   DESCRIPTION: Prints exiting message, then exits.
##
##   ENTRY PARAMETERS:
##      $Output_File_Flag - global specifying output to be written to a flat file.
##      OUTPUT_FILE - file handle to output file
##
##   EXIT PARAMETERS: None
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print, close, exit
##
################################################################################
sub
quit
{
print <<ExitText;
\nExiting the DSOI Reporting Utility.
ExitText

	if ($Output_File_Flag==1)
	{
print <<ExitText2;
Output from this session has been written to file = "./$Output_File".\n
ExitText2
	}

	# close open files
	if ($Output_File_Flag==1) { close(OUTPUT_FILE); }
	close(ERROR_LOG);

	exit(0);
}


################################################################################
##
##   SUBROUTINE: reverse_screen_output_length
##
##   DESCRIPTION: Reverses screen output from full to 80 characters and vice
##      versa.
##
##   ENTRY PARAMETERS:
##      $Screen_Output_Flag - global variable indicating full screen output
##         if =1 or 80 character output if =0
##
##   EXIT PARAMETERS:
##      $Screen_Output_Flag
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: print
##
################################################################################
sub
reverse_screen_output_length
{
	if ($Screen_Output_Flag==0)
	{
		$Screen_Output_Flag=1;
		print "Output to screen will now be limited to 80 characters.\n";
	}
	elsif ($Screen_Output_Flag==1)
	{
		$Screen_Output_Flag=0;
		print "Output to screen will now be full.\n";
	}
}


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
	if (system($system_cmd) != 0)
	{
		print "Unable to execute DSOIReport with system command \"$system_cmd\"\n";
		print "Reported data may not be the most current.\n\n";
	}
	else
	{
		print "DSOIReport processing complete.\n\n";
	}
}

################################################################################
##
##   SUBROUTINE: write_to_output_file
##
##   DESCRIPTION: Causes output from further processing to be written to a flat
##      file.
##
##   ENTRY PARAMETERS: $Output_File - output file set in initialize_stuff()
##
##   EXIT PARAMETERS: $Output_File_Flag - global flag to signal that output
##      should be appended to a flat file specified in $Output_File.
##
##   RETURN VALUES: None
##
##   SUBROUTINES USED: local, system, print, open
##
################################################################################
sub
write_to_output_file
{
	# Declare local variables
	local($system_cmd);

	$Output_File_Flag = 1;

	# Create output directory, if needed
	$system_cmd="mkdir -p ".$OUTPUT_DIR;
	if (system($system_cmd) != 0)
	{
		print "Unable to make directory with system command \"$system_cmd\" \n errno=$! \n";
	}

	open(OUTPUT_FILE, ">>".$Output_File);
print <<WriteToOutputFile;
Output will be written to the file = "$Output_File".
WriteToOutputFile
}


1;
