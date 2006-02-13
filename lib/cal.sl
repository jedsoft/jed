%
%  calender for JED
%
%  It was written to test a mixture of S-Lang RPN and infix notation.
%
%  It pops up a buffer like:

%     Jun 1993		      Jul 1993		       Aug 1993
% S  M Tu  W Th  F  S	  S  M Tu  W Th  F  S	   S  M Tu  W Th  F  S
%       1  2  3  4  5 		      1  *  3 	   1  2  3  4  5  6  7 
% 6  7  8  9 10 11 12 	  4  5  6  7  8  9 10 	   8  9 10 11 12 13 14 
%13 14 15 16 17 18 19 	 11 12 13 14 15 16 17 	  15 16 17 18 19 20 21 
%20 21 22 23 24 25 26 	 18 19 20 21 22 23 24 	  22 23 24 25 26 27 28 
%27 28 29 30 		 25 26 27 28 29 30 31 	  29 30 31 
%
%  The asterisk denotes the current day.  
%  The actual computational part of the code presented here is a 
%  translation of cal.el included with the GNU Emacs distribution.
%  (suitably modified to work with 16 bit integers)
%  
%  Changes:
%  2000-09-30 by Eero Tamminen:
%  - month can be given either as a localized month name or a number
%  - configurability, month / day names & prompt can be changed
%  - day of the week, on which day week starts, can be configured
%    (US week starts on Sunday, european on Monday)
%  - some readability cleanup & more comments
%----------------------------------------------------------------------

% country specific customization

variable CalDays, CalMonths, CalStartWeek, CalPrompt;

public define calendar_months(months) { CalMonths = months; }
public define calendar_days(days) { CalDays = days; }
public define calendar_start_week(day) { CalStartWeek = day; }
public define calendar_prompt(prompt) { CalPrompt = prompt; }

% set US defaults

% each month name may be at max. 15 characters
calendar_months(["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]);
% each day is two characters, separated with space
calendar_days(" S  M Tu  W Th  F  S");
% sunday=0, monday=1...
calendar_start_week(0);
% string with month & year
calendar_prompt("Month Year:");


% return nonzero if yearnum is a leap year
private define cal_leap_year_p (year)
{
   return ((not(year mod 4) and (year mod 100))
    or (not (year mod 400)));
}

% calculate nth day of year for given date
private define cal_day_number(month, day, year)
{
   variable d;
   d = 31 * ( month - 1 ) + day;
   if (month > 2)
     {
	d = d - (month * 4 + 23) / 10;
	if (cal_leap_year_p (year)) d++;
     } 
   return d;
}

% calculate day of week for given date
private define cal_day_of_week(month, day, year)
{
   variable c, delta, n, a, b;
   
   n = cal_day_number(month, day, year);
   --year;
   
   a = n + year + year/4;
   c = year/100 * 3; b = 0;
   if (c mod 4) b = 1;

   return (a - (b + c/4) - CalStartWeek) mod 7;
}

% output given month to buffer
private define cal_make_month (indent, month, year, day, highlight)
{
   variable month_name, first, nm, ny, max, i, istr;

   % get days in month
   first = cal_day_of_week(month, 1, year);
   nm = month + 1;
   ny = year;
   if (nm == 13)
     max = 31;
   else
     max = cal_day_number(nm, 1, ny) - cal_day_number(month, 1, year);
   
   ++indent;
   bob();

   % output month/year line
   month_name = CalMonths[month - 1];
   goto_column(indent + (strlen(CalDays) - strlen(month_name) - 5) / 2);
   insert(month_name); insert_single_space(); insert(string(year));
   !if (down_1 ()) newline();
   
   % output days line
   goto_column(indent);
   insert(CalDays);
   !if (down_1()) newline ();
   
   % output day numbers in 7 columns
   goto_column(first * 3 + indent);
   for (i = 1; i <= max; ++i)
     { 
	if (first == 7)
	  {
	     !if (down_1()) {
		eol(); newline ();
	     }
	     goto_column(indent);
	     first = 0;
	  }
	
	% highlight current day
	if ((day == i) and highlight)
	  {
	     if (day < 10)
	       insert (" * ");
	     else
	       insert ("** ");
	  }
	else vinsert ("%2d ", i);
	++first;
     } 
}


% return current (year, month day) as integers (from date string)
private define cal_get_date()
{
   variable t, n, m, months, month, month_name, day, year;

   t = time ();
   month_name = extract_element(t, 1, ' ');

   % have to be same as time() returns
   months = "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec";
   
   for (m = 0; m < 12; ++m)
     { 
	month = extract_element(months, m, ' ');
	!if (strcmp(month_name, month)) {
	   month = m + 1;
	   break;
	}
     }

   day = extract_element(t, 2, ' ');

   n = 0;
   % Some systems display the time as: Tue Jul 06 16:31:18 1993
   % while others use:                 Tue Jul  6 16:31:18 1993
   if (strlen(day) == 0)
     { 
	day = extract_element(t, 3, ' ');
	n = 1;
     } 
   year = extract_element(t, 4 + n, ' ');
   
   return integer(year), month, integer(strtrim_beg(day, "0"));
}

% convert month number or localized name string into integer
private define cal_convert_month (month_name)
{
   variable m;
   month_name = strlow(month_name);
   
   m = where (month_name == array_map (String_Type, &strlow, CalMonths));
   if (length(m))
     return m[0] + 1;

   % presume it's an integer
   return integer(month_name);
}

% output three month calendar into separate buffer
public define calendar ()
{
   variable t, month, year, nlines, wlines, obuf, default;
   variable this_day, this_month, this_year;

   run_mode_hooks("calendar_mode_hook");

   obuf = whatbuf;

   % ask user for month / year

   (this_year, this_month, this_day) = cal_get_date();
   default = sprintf ("%s %d", CalMonths[this_month-1], this_year);
   
   t = strtrim (read_mini (CalPrompt, default, Null_String));
   
   month = cal_convert_month(extract_element(t, 0, ' '));
   year = integer(extract_element(t, 1, ' '));

   pop2buf("*calendar*"); set_readonly(0); erase_buffer();

   % output three months

   --month; if (month == 0) { month = 12; --year; }
   cal_make_month (0, month, year, this_day,
		   ((month == this_month) and (year == this_year)));
   
   ++month; if (month == 13) { month = 1; ++year; }
   cal_make_month (25, month, year, this_day,
		   ((month == this_month) and (year == this_year)));
   
   ++month;  if (month == 13) { month = 1; ++year; }
   cal_make_month (50, month, year, this_day,
		   ((month == this_month) and (year == this_year)));
   
   % fix window size

   if (nwindows == 2)
     {
	eob();  bskip_chars("\n\t ");
	nlines = what_line() - window_info('r');
	
	if (nlines > 0)
	  {
	     loop (nlines) {call("enlarge_window");}
	  }
	else
	  {
	     call("other_window");
	     loop(- nlines) {call("enlarge_window");}
	     call("other_window");
	  } 
	 bob();
     } 
     
   set_readonly(1); set_buffer_modified_flag(0);
   bob(); pop2buf(obuf);

   %  what the heck, give current time
   message(time);
}
