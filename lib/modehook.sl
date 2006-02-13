% modehook.sl	-*- SLang -*-

%!%+
%\variable{Enable_Mode_Hook_Eval}
%\synopsis{Control the use of "eval" from a file's mode statement}
%\description
% When a file is read into a buffer, the editor will look for a line near the
% top of the file containing \exmp{-*- TEXT -*-}, where \exmp{TEXT} usually 
% specifies the mode to be applied to the buffer, e.g.,
%#v+
%    /* -*- C -*- */
%#v-
% For this reason, such a line is referred to as the files mode statement.
% In addition, an arbitrary \slang expression may be executed by
% the mode statement through the use of \var{eval} and \var{evalfile}, e.g.,
%#v+
%   /* -*- mode: C; eval: set_buffer_no_backup; -*- */
%#v-
% This example would cause c-mode to be assigned to the buffer, and backups
% for the file turned-off.  Obviously this poses a security risk, since it 
% permits the evaluation of arbitrary \slang code.  
% 
% The \var{Enable_Mode_Hook_Eval} variable may be used to control how 
% \var{eval}, and other potentially risky constructs are handled by the file's
% mode statement.  If its value is 0, such statements will not get executed.
% If the value of \var{Enable_Mode_Hook_Eval} is \NULL, then the editor will
% query the user about whether to execute the statement, otherwise such 
% statements will get executed.  The default value is \NULL, i.e., to 
% query the user.
%\seealso{modeline_hook2, eval, evalfile, set_mode}
%!%-
custom_variable ("Enable_Mode_Hook_Eval", NULL);

private define check_eval (checked)
{
   if (@checked != -1)
     return @checked;
   
   if (Enable_Mode_Hook_Eval == NULL)
     {
	sw2buf (whatbuf ());
	update (1);
	@checked = get_yes_no ("Allow execution of file's mode statement");
     }
   else @checked = Enable_Mode_Hook_Eval == 1;
   
   return @checked;
}

%\function{modeline_hook2}
%\synopsis{modeline_hook2}
%\description
% check for the following mode statements:
%#v+
% -*- mode: NAME -*-		set mode to NAME
% -*- evalfile: FILE -*-	evaluate file FILE
% -*- eval: expression -*-    evaluate expression
% -*- VAR: VALUE -*-		set VAR = VALUE
%#v-
% these statements may be combined:
%#v+
% -*- mode: NAME; evalfile: FILE; VAR: VALUE -*-
%#v-
define modeline_hook2 ()
{
   variable keyword, value, mode = 0, tag = "-*-", modestr;
   
   if (BATCH)
     return 0;

   bob ();
   !if (fsearch (tag)) return 0;

   variable checked = -1;

   while (ffind (tag))
     {
	go_right (3);
#iffalse
	if (looking_at ("END -*-")) break;
#endif

	push_spot ();
	skip_white (); 
	!if (ffind (tag), pop_spot ()) break;	% closing tag exists?

	forever
	  {
	     skip_chars (" \t;");
	     push_mark ();
	     !if (ffind_char (':'))
	       {
		  pop_mark_0 ();
		  break;
	       }
	     keyword = bufsubstr ();	     
	     go_right_1 ();

	     push_mark ();
	     do
	       {
		  skip_chars ("^-;\n");
		  if (looking_at_char (';') or looking_at (tag))
		    break;
	       }
	     while (right (1));
	     value = strtrim (bufsubstr ());
	     
	     push_spot ();
	     
	     ERROR_BLOCK
	       {
		  pop_spot ();
	       }
	     % error (sprintf ("keyword <%s> value <%s>", keyword, value));
	     switch (keyword)
	       { case "mode":
		  modestr = "_mode";
		  value = strlow (strtrans (value, "-", "_"));
		  !if (is_substr (value, modestr)) 
		    value += modestr;
		  if (value == "c++_mode")
		    value = "c_mode";
		  if (is_defined(value))
		    {
		       eval (value);
		       mode++;
		    }
	       }
	       { case "evalfile":
		  if (check_eval (&checked))
		    evalfile (value);
	       }
	       { case "eval" :
		  if (check_eval (&checked))
		    eval (value);
	       }
	       { (is_defined (keyword) < 0) and strlen(value): % set a value
		  if (check_eval (&checked))
		    eval (keyword + " = " + value);
	       }
	     
	     pop_spot ();
	  }
	go_down_1 ();
     }
   mode;
}
