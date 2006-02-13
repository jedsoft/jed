% mailalias.sl	-*- mode: slang; mode: fold -*-
%
% expand any aliases in the To, Cc, Bcc, From, Reply-To: lines
%
% - use Pine-style (TAB delimited) aliases
% - use UCB Mail (space delimited) aliases
%
% Pine (.addressbook) style:
% alias-name[TAB]Family Name(s), First Name(s)[TAB]e-mail-address
% alias-name[TAB]space for name[TAB]e-mail-address
%
% UCB Mail (.mailrc) style
% alias alias-name e-mail-address
% group alias-name address1 address2 ...
%
% The mail entry point: mailalias_expand ();
% relies on the X-Mailer: field to indicate the end of the header
%
% add to your mail_hook()
%
% define mail_hook ()
% {
%    local_setkey ("mailalias_expand", "^C^E");
%    ...
% }
%

require ("mailutils");

%{{{ Public Variables
%!% where to find the [TAB] delimited mail alias list
custom_variable ("Mailaliases", dircat (getenv ("HOME"), ".addressbook"));

%!% where to find the [SPACE] delimited mail alias list
!if (is_defined ("Mailrc"))
{
   variable Mailrc = getenv ("MAILRC");
   if (Mailrc == NULL)
     Mailrc = dircat (getenv ("HOME"), ".mailrc");
}
%}}}

%{{{ Private Variables
private variable Mailalias_buf = " *mailalias*";
%}}}
%{{{ Private Functions
%{{{ forward reference
define mailalias_xAlias();
%}}}

% take a comma-delimited list of aliases and return a list of fully
% expanded names and addresses
define mailalias_xList (raw,space_to_comma)	%{{{
{
   variable str, list = "";

   if (strlen (Mailrc))
     {
	if (space_to_comma)
	  raw = strtrans (raw, " ", ",");
     }

   raw = strcompress (raw, ",");
   foreach (strchop (raw, ',', 0))
     {
	str = mailalias_xAlias (strtrim (()));
	if (strlen (str))
	  {
	     if (strlen (list))
	       list += ",\n\t";

	     list += str;
	  }
     }
   strtrim (list);		% return on the stack
}
%}}}

% expands alias => "Full Name" <user@machine.domain>
% quotes any names that contain '@' to <name@machine>
% for a [TAB] delimited field
% This function is executed in the buffer containing the aliases
define mailalias_xAlias (alias)	%{{{
{
   variable name, addr = Null_String;
   variable name0, name1;

   !if (strlen (alias)) return addr;
   if ((is_substr (alias, "@") or is_substr (alias, "<")))
     {
	% "<user@machine.domain>"
	if (is_substr (alias, "<")) alias; else "<" + alias + ">";
	return;
     }

   bob ();
   !if (right (bol_fsearch (alias + "\t"))) return alias;

   % hand-rolled functions, regular expressions are too inconsistent
   % and do not work across lines
   USER_BLOCK0
     {
	push_mark ();		% extract a field
	skip_chars ("^\t\n");	% next field
	bufsubstr ();		% return the field on the stack
	go_right (1);		% skip TAB char
     }

   name = X_USER_BLOCK0;	% fullname or surname

   name0 = extract_element (name, 0, ',');
   name1 = extract_element (name, 1, ',');
   if (name1 == NULL) name1 = "";
   name = strtrim (strtrim (name1) + " " + strtrim (name0));

   % "quote" name in case it contains one of `,.@'
   name = "\"" + name + "\" ";

   if (looking_at_char ('('))	% distribution list (with parentheses)
     {
	name = Null_String;	% ignore name of distribution list?
	go_right (1);
	push_mark ();
	!if (fsearch_char (')')) skip_chars ("^\t \n");	% messed-up entry?
	addr = mailalias_xList (bufsubstr (), 1);
     }
   else
     {
	addr = X_USER_BLOCK0;
	addr = "<" + addr + ">";
     }
   name + addr;		% "Full Name" <user@machine.domain>
}
%}}}

% expands alias => user@machine.domain
% for a [SPACE] delimited field
% This function is executed in the buffer containing the aliases
define mailrc_xAlias (alias)	%{{{
{
   variable name, addr;
   variable n;

   alias = strtrim (alias);
   
   if (not(strlen(alias)) 
       or is_substr (alias, char ('@')))
     return alias;

   % look for "alias name"
   name = "alias " + alias + " ";
   
   bob ();

   % Convert all tabs to spaces
   replace ("\t", " ");
   
   n = bol_fsearch (name);
   !if (n)
     {
	% look for "group name"
	name = "group " + alias + " ";
	n = bol_fsearch (name);
	!if (n) return alias;
     }
   
   go_right (n);
   
   addr = "";

   % hand-rolled functions, regular expressions are too inconsistent
   forever
     {
	skip_white ();
	if (eolp ()) break;
	push_mark ();
	skip_chars ("^ \n");	% next address
	name = bufsubstr ();	% address
	if (strlen (addr)) addr += ",\n\t";
	addr += name;
     }
   addr;
}
%}}}

% look for the addresses that follow FIELD and are before the next one.
define mailalias_xField (field)	%{{{
{
   variable addr, buf = whatbuf ();

   addr = mailutils_get_keyword_value (field);
   if (addr == NULL)
     return;

   setbuf (Mailalias_buf);
   addr = "\t" + mailalias_xList (addr, 0);
   sw2buf (buf);
   mailutils_set_keyword_value (field, addr);
}
%}}}
%}}}

%!% expands "To, Cc, Bcc" mail header fields using the aliases/addresses
%!% that are defined in Mailaliases
define mailalias_expand ()	%{{{
{
   %variable buf = "*mail*";
   variable buf = whatbuf ();
   %if (strcmp (buf, whatbuf)) error ("Mail mode only");

   push_spot ();
   setbuf (Mailalias_buf);
   if (insert_file (Mailaliases) > 0)
     {
	set_buffer_modified_flag (0);   % mark unmodified
	sw2buf (buf);

	% order isn't important
	foreach (["From:", "Reply-To:", "Bcc:", "Cc:", "To:"])
	  {
	     variable header = ();
	     mailalias_xField (header);
	  }
     }
   else
     {
	sw2buf (buf);
     }
   delbuf (Mailalias_buf);
   pop_spot ();
}
%}}}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%{{{ check that everything is okay, unload as appropriate

#ifexists stat_file
% If the files exist, they may be empty.  Here is a quick check for that.
foreach ([&Mailaliases, &Mailrc])
{
   $1 = ();
   $2 = stat_file (@$1);
   if ($2 != NULL)
     {
	if ($2.st_size == 0)
	  @$1 = "";
     }
}
#endif
%verror (Mailaliases);

% ~/.addressbook is okay, un-load Mailrc stuff
if (1 == file_status (Mailaliases))
{
   eval (".()mailrc_xAlias");
   Mailrc = Null_String;
}
% ~/.mailrc is okay, then un-load addressbook stuff & over-load definition
else if (1 == file_status (Mailrc))
{
   eval (".(mailrc_xAlias) mailalias_xAlias");
   Mailaliases = Mailrc;
}
% nothing seems to have worked, so get rid of everything
else
{
   eval (".()mailrc_xAlias");
   eval (".()mailalias_xAlias");
   eval (".()mailalias_xList");
   eval (".()mailalias_xField");
   eval (".()mailalias_expand");
   Mailaliases = Null_String;
}
%}}}
