% this -*- SLang -*- file defines syntax/indentation for Perl programs
% ---------------------------------------------------------------------------
%
% Changes:
% Original author: Lars Marowsky-Bree <lmb@pointer.in-minden.de>
%
% 2001-01-10  Mark Olesen <mark.olesen@gmx.net>
% - modified a few keywords, added a Perl specific keymap
% - indentation mode improved and now includes a POD mode
% - added begin/end/mark/prev/next 'chunk' (sub or pod)
%
% 2002-01-27  Mark Olesen <mark.olesen@gmx.net>
% - fixed indentation for [] array entries and references
% - indentation for closing ')' or ']' lines up with opening '(', '['
% - improved indentation for code blocks within parentheses
% - improved indentation for 'keyword => { ...' and '$ref = {' construct
% - perl_indent_region() now handles '<<HERE' documents.
% - perl_indent_region() now indents the current line if no region is marked
% - added separate 'perl_indent_buffer()'
% - added new keywords for perl 5.6.0
% - simple keyword extraction utility (embedded Perl code in SLang file)
% - disabled DFA syntax highlighting until someone verifies it *completely*
%
% 2002-01-31  Mark Olesen <mark.olesen@gmx.net>
% - indentation for "=>(", "=>[" and "=> {" constructs
%   and for "= (", "= [" and "={" constructs use 'Perl_Indent' are are
%   not lined up! This helps reduce panning for large tables.
% - The multi-line contents of '()' are indented by 1
% - The multi-line contents of '[]' are indented by 'Perl_Continued_Offset'
%
% 2002-02-17  Mark Olesen <mark.olesen@gmx.net>
% - The contents of '([{' are no longer indented to match the
%   opening parentheses.  Instead they receive a uniform 'Perl_Indent'.
%   This matches the results of perltidy(1) with '-i=4 -ci=2' much better,
%   the code tends not to run off the end of lines as quickly,
%   and the results are much more consistent (less monkeying with the
%   code-layout to get the indent to work correctly).
% - lines starting with 'grep', 'and', etc and lines following them
%   are now corectly indented.
%   Even when the previous line ended with '}'.
%   We can continue our lines with the perlstyle(1) suggested 'and' / 'or'.
% - perl_parse_to_point() now returns -3 if inside of pod.
% - lines starting with '#' in the first column are left alone
% - The older versions are commented out with 'PERL_INDENT_V?'
%
% 2002-02-24  Mark Olesen <mark.olesen@gmx.net>
% - perl_mark_matching is a bit more generous with whitespace when
%   attempting a match
% - the brain-dead PERL_INDENT_V0 (version 0) is no longer included
% - included the '$@%' sigils as word characters. The '%' as modulus
%   operator will be incorrect, but we have many more hases ...
%
% 2002-03-02  Mark Olesen <mark.olesen@gmx.net>
% - added 'perl_exec' and 'perl_check' functions
% - since I've never seen a perl program with folding marks,
%   I changed the folding marks from the '#{{{' and '#}}}' to
%   the slightly more useful '=pod' and '=cut'.  
%   Thus you can fold your documentation out of the way.
% - added interface to the 'perltidy' (sourceforge) program.
%   This is *extremely* handy, especially if you are a bit sloppy about
%   spacing around brackets etc.
%
% ---------------------------------------------------------------------------
%
% NB: no automatic brace '{' handling!  otherwise constructs such as
%   '@foo = @{$arrayref}', and '%foo = %{$hashref}' are a nightmare!
%
% - Because of how the internal syntax highlighting is implemented
%   (ask John about details), it's only possible to either
%   '=pod' or '=head' in the highlighting, but not both.
%
%   Many (most) modules simply use '=head', but if you're mixing code
%   and documentation a lot, the '=pod' directive is extremely handy.
%   Although perl understands it when you start a pod chunk with '=item',
%   the editor would not, and for this case prefixing it with '=pod'
%   would trigger the same thing, but prefixing it with '=head' would
%   definitely be wrong.  See perlpod(1) for more details.
%
%   The mode tries to guess which style is being used.
%   When only '/^=head/' and no '/^=pod/' is found, the '=head' tag will
%   be used.  Otherwise '=pod' will be used. 
%   If you change your coding style a lot, an M-x 'perl_mode' may help.
%
%   NB: this only affects the *syntax highlighting* ... 
%   indentation should be fine.
%
%   NB: since the syntax highlighting matches /=pod/ and not /^=pod/,
%   these patterns within the code will cause serious confusion with 
%   the highlighting!
%   Consider using /^=(pod|head|cut)/ instead
%
% ---------------------------------------------------------------------------
% BUGS / MIS-FEATURES:
% - supressing indentation in '<<HERE' documents is not handled.
%   It's not easily implemented without a significant speed penalty.
%   'perl_indent_region()' gives a hint of how annoying it can be,
%   but try it with backtracking!
% - you can easily write correct perl that will completely confuse
%   the editor.  Here are a few examples:
%     1. s)foo)bar)g;
%     2. s"foo"bar"g;
%     3. local ($") = ":";
%     4. $gid  = $(;
%     5. $egid = $);
%   You can normally avoid #1 and #2. For the others, you need to add some
%   extra perl comments to help the editor get back into shape.
%     eg: local ($") = ":"; #for-editor"); # okay
%         $gid = $(; #) for-editor; # okay
%   Similar tricks can help with the $', $`, $(, $), $[, and $] variables.
%   For the $) and $] variables, you need to be even a bit trickier
%   and use a preceeding comment
%     eg:  #( for-editor 
%             $egid  = $);
%   fortunately, there's now $^V instead of $]
%
% BUGS?
% - could someone *please* check the DFA syntax highlighting?
% - the rules for 'm//', 's///' and 'tr///' are too simple to be useful
%   eg, 's{/+}{/}g' or 's|/+|/|g' are not handled
% - the rules with character classes '[...]' seem to have an inordinate
%   number of backslashes.  In 'standard' regexps, the meta-characters
%   have no meaning in this context and need not be backslash-quoted,
%   the same goes for '-' at the start/end of an '[...]' character class
%   (see 'ed' and 'perlre' manpages)
%
% ---------------------------------------------------------------------------
%{{{ default values for Perl custom variables
% override these default values in ~/.jedrc
%!%+
%\variable{Perl_Indent}
%\synopsis{Perl_Indent}
%\usage{Integer Perl_Indent = 4;}
%\description
% This value determines the number of columns the current line is indented
% past the previous line containing an opening \exmp{'\{'} character.
% eg.,
%#v+
%  if (test)
%  {
%      block_statement();
%  }
%  else
%  {
%      block_statement();
%  }
%#v-
%\seealso{C_INDENT, Perl_Continued_Offset}
%!%-
custom_variable("Perl_Indent", 4);
% %!%+
%\variable{Perl_Continued_Offset}
%\synopsis{Perl_Continued_Offset}
%\usage{Integer Perl_Continued_Offset = 2;}
%\description
% This variable controls the indentation of statements that are continued
% onto the next line as in the following example:
%#v+
%  print
%    "hallo ",
%    " world\n";
%#v-
%\seealso{C_CONTINUED_OFFSET, Perl_Indent}
%!%-
custom_variable("Perl_Continued_Offset", 2);
%!%+
%\variable{Perl_Expert_Flags}
%\synopsis{Perl_Expert_Flags}
%\usage{String Perl_Expert_Flags = "-w";}
%\description
% Extra (or 'expert') command-line options (switches) for running Perl.
% eg, \var{'-I'} or \var{'-M'}.
% You only need these if you know why you need these.
%
% Warnings are *always* enabled, regardless of what you try here.
% If your code doesn't run with \var{'-w'}, re-write it or you're an
% expert and know which sections of code should have disabled warnings.
%!%-
custom_variable("Perl_Flags", Null_String);
%}}}
% some constant strings for tracking POD documentation
private variable pod_beg = "=pod", pod_too = "=head", pod_end = "=cut";
%{{{ create/initialize key map
$1 = "perl";
!if (keymap_p ($1)) make_keymap($1);
definekey("indent_line",     "\t",   $1);
definekey("perl_beg_chunk",  "\e^A", $1);
definekey("perl_end_chunk",  "\e^E", $1);
definekey("perl_mark_chunk", "\e^H", $1);
definekey("perl_next_chunk", "\e^N", $1);
definekey("perl_prev_chunk", "\e^P", $1);
definekey("perltidy",        "\e^T", $1);
definekey("perl_mark_matching", "\e^M", $1);
definekey("perl_format_paragraph", "\eq", $1);
definekey("newline_and_indent", "\r", $1);
% some people may like this: 
%  definekey(".\"\\\\n\" insert", "^J", $1);

definekey("perl_indent_region", "\e\t", $1);	% override 'move-to-tab'
%}}} key map
%{{{ create/initialize syntax tables
$1 = "perl";
create_syntax_table($1);
define_syntax("#", "", '%', $1);		% single line comment
define_syntax(pod_beg, pod_end, '%', $1);	% multi-line comment (POD)
define_syntax("([{", ")]}", '(', $1);		% matching brackets
define_syntax('\'', '"', $1);			% string
define_syntax('"', '"', $1);			% string
define_syntax('\\', '\\', $1);			% backslash escaping
define_syntax("$%0-9@A-Z_a-z", 'w', $1);	% words plus sigils
define_syntax("-+.0-9_xa-fA-F", '0', $1);	% numbers
define_syntax(",.:;?", ',', $1);		% punctuation
define_syntax("!&*+-/<=>`^|~", '+', $1);	% operators - leave %-sigil alone
set_syntax_flags ($1, 0x10|0x80);
%}}} syntax table

% with DFA, the =pod/=head/=cut block is not recognized
#ifdef HAS_DFA_SYNTAX_NOT_OK
%% #ifdef HAS_DFA_SYNTAX
%{{{ DFA syntax tables - have these be verified??
enable_highlight_cache("perl.dfa", $1);
define_highlight_rule("^#.*$", "comment", $1);
define_highlight_rule("[ \t;]#.*$", "comment", $1);
% normal => qr{([$%&@*]|\$#)\w+/};
% normal => qr{\$([-+_./,\"\'\`#*?\[(<>)\];!@:$%=~^|&]|\^[A-Z])}
define_highlight_rule("([\\$%&@\\*]|\\$#)[A-Za-z_0-9]+", "normal", $1);
define_highlight_rule("\\$([_\\./,\"\\\\#\\*\\?\\]\\[;!@:\\$<>\\(\\)" +
		      "%=\\-~\\^\\|&`'\\+]|\\^[A-Z])",
		      "normal", $1);
% define_highlight_rule("[A-Za-z_\\$][A-Za-z0-9_]*", "Knormal", $1);
define_highlight_rule("[A-Za-z_][A-Za-z0-9_]*", "Knormal", $1);
define_highlight_rule("[0-9]+(\\.[0-9]+)?([Ee][\\-\\+]?[0-9]+)?", "number", $1);
define_highlight_rule("0x[0-9A-Fa-f]+", "number", $1);
% strings: 
%   strange ... " abc \" def"; would appear to be invalid
define_highlight_rule("\"([^\"\\\\]|\\\\.)*\"", "string", $1);
define_highlight_rule("'([^'\\\\]|\\\\.)*'", "string", $1);
%
define_highlight_rule("[\\(\\[\\{\\<\\>\\}\\]\\),\\.:;\\?]", "delimiter", $1);
define_highlight_rule("[\\-\\+!%&\\*/=<>\\|~\\^]", "operator", $1);
define_highlight_rule("[A-Za-z0-9]", "keyword0", $1);
%
% unfortunately these rules are not robust enough and are also incomplete
% eg, 's{/+}{/}g' or 's|/+|/|g';
% 
%fixme? define_highlight_rule("m?/([^/\\\\]|\\\\.)*/[gio]*", "string", $1);
%fixme? define_highlight_rule("m/([^/\\\\]|\\\\.)*\\\\?$", "string", $1);
%fixme? define_highlight_rule("s/([^/\\\\]|\\\\.)*(/([^/\\\\]|\\\\.)*)?/[geio]*",
%fixme? 		      "string", $1);
%fixme? define_highlight_rule("s/([^/\\\\]|\\\\.)*(/([^/\\\\]|\\\\.)*)?\\\\?$",
%fixme? 		      "string", $1);
%fixme? define_highlight_rule("tr/([^/\\\\]|\\\\.)*(/([^/\\\\]|\\\\.)*)?/[cds]*",
%fixme? 		      "string", $1);
%fixme? define_highlight_rule("tr/([^/\\\\]|\\\\.)*(/([^/\\\\]|\\\\.)*)?\\\\?$",
%fixme? 		      "string", $1);
define_highlight_rule(".", "normal", $1);
build_highlight_table($1);
%}}} DFA syntax tables
#endif
%{{{ keywords
% Type 0 keywords
% ignore the functions that do special, pre-defined things including
%   `BEGIN', `CHECK', `INIT', `END', `AUTOLOAD', and
%   `DESTROY'--plus all functions mentioned in the perltie manpage.
% $1 = "perl";
() = define_keywords($1,
		     "doiflcmynoqqqrqwqxtruc",
		     2);
() = define_keywords($1,
		     "abschrcosdieeofexpforhexintlogmapoctordour" +
		     "popposrefsinsubtieusevec",
		     3);
() = define_keywords($1,
		     "bindcarpchopdumpeachelseevalexecexitfork" + 
		     "getcglobgotogrepjoinkeyskilllastlinklocknextopen"+
		     "packpipepushrandreadrecvredoseeksendsortsqrtstat" +
		     "telltiedtimewaitwarn",
		     4);
() = define_keywords($1,
		     "alarmatan2blesschdirchmodchompchownclosecroakcrypt" +
		     "elsiffcntlflockindexioctllocallstatmkdirprint" + 
		     "resetrmdirsemopshiftsleepsplitsrandstudytimes" + 
		     "umaskundefuntieuntilutimewhilewrite",
		     5);
() = define_keywords($1,
		     "acceptcallerchrootdeleteexistsfilenoformatgmtime" +
		     "importlengthlistenmsgctlmsggetmsgrcvmsgsnd" + 
		     "printfrenamereturnrindexscalarselectsemctl" +
		     "semgetshmctlshmgetsocketsplicesubstrsystem" + 
		     "unlessunlinkunpackvalues",
		     6);
() = define_keywords($1,
		     "binmodeconnectdefinedforeachgetpgrpgetppid" + 
		     "lcfirstopendirpackagereaddirrequirereverse" + 
		     "seekdirsetpgrpshmreadsprintfsymlink" +
		     "syscallsysopensysreadsysseek" +
		     "telldirucfirstunshiftwaitpid",
		     7);
() = define_keywords($1,
		     "closedircontinueendgrentendpwentformline" +
		     "getgrentgetgrgidgetgrnamgetlogingetpwent" +
		     "getpwnamgetpwuidreadlinereadlinkreadpipe" +
		     "setgrentsetpwentshmwriteshutdownsyswrite" + 
		     "truncate",
		     8);
() = define_keywords($1,
		     "endnetentgetnetentlocaltimeprototypequotemeta" +
		     "rewinddirsetnetentwantarray",
		     9);
() = define_keywords($1,
		     "endhostentendserventgethostentgetservent" +
		     "getsockoptsethostentsetserventsetsockoptsocketpair",
		     10);
() = define_keywords($1,
		     "endprotoentgetpeernamegetprioritygetprotoent" +
		     "getsocknamesetprioritysetprotoent",
		     11);
() = define_keywords($1,
		     "getnetbyaddrgetnetbyname",
		     12);
() = define_keywords($1,
		     "gethostbyaddrgethostbynamegetservbynamegetservbyport",
		     13);
() = define_keywords($1,
		     "getprotobyname",
		     14);
() = define_keywords($1,
		     "getprotobynumber",
		     16);

% Type 1 keywords - use for operator-like keywords
() = define_keywords_n($1, "eqgegtleltneor", 2, 1);
() = define_keywords_n($1, "andcmpnotxor", 3, 1);
%}}} keywords

% returns the same as the standard parse_to_point,
% except -3 if we're inside of pod
private define perl_parse_to_point()
{
    variable ptp = -2;
    
    push_spot();
    ptp = parse_to_point();

    % debug->    EXIT_BLOCK { pop_spot(); dup(); vmessage("ptp=%d",()); (); }
    EXIT_BLOCK { pop_spot(); }

    % see if the current line starts with a /^=[a-z]+/ pod directive
    bol();
    if (looking_at_char('=')) {
	go_right_1();
	_get_point();
	skip_word_chars();
	if (_get_point() - ()) return -3;		% pod!
    }

    variable here = what_line();

    if (andelse
	{re_bsearch("^=[a-z]")} 	% prev /^=[a-z]/ pod command
	  {bol_fsearch(pod_end)}	% next /^=cut/
	  {here < what_line()}		% are we in this region?
	)
      return -3;		% pod!

    % check for the start of a comment
    !if (ptp) {
	goto_spot();
	bol_skip_white();
	if (looking_at_char('#')) return -2;
    }

    return ptp;
}
 

% the usual delimiters that may occur before/after a keyword
private variable delimiter = "\t $%@([{:?}])";

%
% returns non-zero if looking at one of the tokens in the list
% NB: if you look for a token such as '||', it must be nicely delimited
% from its following neighbour. ie, write '|| foo()' instead of '||foo()'
%
private define perl_looking_at()
{
    variable cs = CASE_SEARCH, rc = 0;

    EXIT_BLOCK 
      {
	  pop_spot();
	  CASE_SEARCH = cs;
      }   
 
    CASE_SEARCH = 1;
    push_spot();
    
    foreach (__pop_args(_NARGS)) {
	variable token = ().value;
	variable len = strlen(token);

	goto_spot();
	if (looking_at(token)) {
	    go_right(len);
	    _get_point();	% leave on the stack
	    skip_chars(delimiter);
	    rc = (_get_point() - ()) or eolp();
	    if (rc) break;
	}
    }
    
    return rc;
}

%
% returns non-zero if looking at one of the tokens in the list
% NB: if you look for a token such as '||', it must be nicely delimited
% from its following neighbour. ie, write '|| foo()' instead of '||foo()'
%
private define perl_blooking_at()
{
    variable cs = CASE_SEARCH, rc = 0;
    
    EXIT_BLOCK 
      {
	  pop_spot();
	  CASE_SEARCH = cs;
      }   
 
    CASE_SEARCH = 1;
    push_spot();
    
    variable n = _get_point();	% check if there's room for the word
    foreach (__pop_args(_NARGS)) {
	variable token = ().value;
	variable len = strlen(token);
	
	if (n < len) continue;
	
	goto_spot();
	go_left(len);
	if (looking_at(token)) {
	    _get_point();	% leave on the stack
	    bskip_chars(delimiter);
	    rc = (() - _get_point()) or bolp();
	    if (rc) break;
	}
    }

    return rc;
}

public define perl_indent_line()
{
    variable ch;	% the first character
    variable col = 1;	% default to indent on first column
    variable indent_ok = 0;
    variable extra_indent = Perl_Indent;
    variable ptp;	% parse-to-point results

    push_spot();
    bol();
    ch  = what_char();		% the first character
    ptp = perl_parse_to_point();
    
    % flush( sprintf("ptp = %d", ptp) );

    % do not indent POD, but do trim blank lines to avoid problems
    %
    % don't indent a line that has '#' on column 1 ...
    % we probably want to keep our comment there
    if ( (ptp == -3) or ((ptp == -2) and (ch == '#')) ) {
	eol_trim();	% trim blank lines to avoid problems
	pop_spot();
	return;
    }

    skip_white();
    ch = what_char();		% the first non-blank character

    % on exit: restore position and indent to the prescribed 'col'
    EXIT_BLOCK {
	goto_spot();
	bol_skip_white();
	if ( what_column != col ) {
	    bol_trim();
	    col--;
	    whitespace(col);
	}
	goto_spot();
	bskip_white();
	bolp();				% leave on the stack
	pop_spot();
	if (()) bol_skip_white();	% (start of line)
    }

    variable rc, endch = ')';
    rc = find_matching_delimiter(endch);

    !if (rc) {	% enclosing '()' not found - retry with enclosing '[]'
	goto_spot(); bol();
	endch = ']';
	rc = find_matching_delimiter(endch);
    }
    if (rc == 1) {
#iftrue	% the latest version - uniform 'Perl_Indent'
	bol_skip_white();
	col = what_column();
#elifdef PERL_INDENT_V1    
	% -----------------------------------------------------------
	% inside '()' (or '[]')- indent to level of opening '(' + 1
	% A solitary closing ')' gets the same indent level as the '(
	% Ex:
	%  foo (bar
	%       baz(fum
	%           foz)
	%      )
	% -----------------------------------------------------------
	col = what_column();
	bskip_white();
	extra_indent = 1;	% bracket offset is 1
	% '<<= [' and '=> [' constructs are special
	if ( blooking_at("<=") or blooking_at("=>") ) {
	    bol_skip_white();
	    col = what_column();
	    extra_indent = Perl_Indent;	
	}
#endif
	if (ch != endch) col += extra_indent;
	indent_ok++;	
    }

    goto_spot(); bol();	% (original position : start of line)

    % ---------------------------------------------------------------------
    % take care of indentation for '{' blocks
    % ---------------------------------------------------------------------
    endch = '}';
    if (ch == '{') indent_ok++;
    if (andelse
        {find_matching_delimiter(endch) == 1}
	{not( blooking_at("#{{") ) }	% don't match '#{{{' fold
	)
    {	
	extra_indent = Perl_Indent;

	% ---------------------------------------------------------------
	% check for a '(' or '[' on the same line as the '{'
	% this covers many code-blocks such as:
	%
	% for my $var (grep {
	%                       defined $_ and /foobar/ and /
	%		     }
	%	         @list
	%             )
	% {
	%  	...
	% }
	%
	% but still could use a bit of work ...
	% ---------------------------------------------------------------

	% save indent level and position of the '{'
	col = what_column();
	bskip_white();
	push_spot();
	rc = 0;

	if (orelse {bfind(char('('))} {bfind(char('['))}) {
	    goto_spot();
	    rc = find_matching_delimiter(')');
	    !if (rc) {	% enclosing '()' not found
		goto_spot();	% retry with enclosing '[]'
		rc = find_matching_delimiter(']');
	    }
	}
	pop_spot();
	
	if (rc == 1) {	% matched from within an enclosing '()' or '[]'
	    indent_ok++;
	} else {
	    % check for '= {', '+{' or '=> {'  constructs
	    % and for functions taking references
	    _get_point ();	% leave on the stack
	    bskip_chars("+<=>");
	    if (() - _get_point ()) {
		indent_ok++;
	    }
	    else if (perl_blooking_at("bless")) {	% a naked 'bless'
		indent_ok++;
	    }
%%	    else if ( perl_blooking_at("grep", "map", "sort") ) {
%%		extra_indent = Perl_Continued_Offset;
%%	    }

	    bol_skip_white();
	    col = what_column();	% Indent level
	}

	if (ch != endch)
	  col += extra_indent;	% Indent to last '{' + extra
	else
	  indent_ok++;
	% flush(sprintf("col '%c' = %d", endch, col));
    }

    if (indent_ok) return;	% we're done

    % find out if we're on a continued line
    goto_spot();
  
    % Find previous non-comment line
    do {
	% Start of file, pretty hard to find context ;-)
	!if (up_1()) {
	    bol();
	    break;
	}
	bol_skip_white();
	!if (eolp()) go_right_1();
    } while (perl_parse_to_point() <= -2);
    eol();

    % Find last non-comment character
    while ( ptp = perl_parse_to_point(), (ptp <= -2) ) {
	!if (left(1)) break;
    }

    %% flush(sprintf("look  = %c", what_char));

    bskip_white();
    ch = ';';		% default final character
    !if (bolp()) {
	go_left_1();
	if (perl_parse_to_point() >= -2) ch = what_char();
    }

    % flush(sprintf("end char = %c", ch));
    
    extra_indent = Perl_Continued_Offset;
    if ( not(is_substr (";({}", char(ch))) ) {
	col += extra_indent;
#iftrue
    } else if ('}' == ch) {
	% started '{}' with grep/map/sort filter?
	() = find_matching_delimiter(ch);
	bskip_white();
	
	if (perl_blooking_at("grep", "map", "sort")) {
	    bol_skip_white();
	    col = what_column;
	}
	else {
	    % ended with '}', but continued with filter or logical operator?
	    goto_spot();	% (original position)
	    bol_skip_white();
	    
	    if (perl_looking_at("grep", "map", "sort",
				"and", "or", 
				"&&", "||",
				"?", ":" )) 
	      col += extra_indent;
	}
#endif
    }
}

public define perl_indent_region()
{
    !if (markp()) {
	perl_indent_line();	% do this line and get out
	return;
    }

    check_region(1);		% canonical region
    narrow();
    
    variable line = 0;
    variable nlines = what_line();	% number of lines
    bob();
    widen();

    variable cs = CASE_SEARCH; CASE_SEARCH = 1;
    ERROR_BLOCK { pop_spot(); CASE_SEARCH = cs; }

    do {
	line++;
	eol_trim(); bol();
	% skip the comment
	if ( looking_at(pod_beg) or looking_at(pod_too) ) {
	    while (down_1()) {
		line++;
		if (looking_at(pod_end)) break;
		eol_trim();
	    }
	    continue;
	}
	if (eolp()) continue;	% skip blank lines
	skip_white();

	% skip the comment
	if (looking_at_char('#')) {
	    indent_line();
	} else {
	    % try our best to avoid indenting '<<HERE_DOCUMENT' code
	    indent_line();	   
	    if (ffind("<<")) {
		go_right(2);
		push_mark();
		skip_chars("A-Z_a-z");
		variable junk = "\t ;";
		variable here = bufsubstr();
		variable len  = strlen(here);
		skip_chars(junk);
		if (len and (eolp() or looking_at_char('#'))) {
		    while (down_1()) {
			line++;
			if (looking_at(here)) {
			    go_right(len);
			    skip_chars(junk);
			    if (eolp() or looking_at_char('#')) break;
			}
			eol_trim();
		    }
		} else {
		    indent_line();
		}
	    }
	}
	flush(sprintf("processed %d/%d lines.", line, nlines));
    } while (down_1() and (line < nlines));
    
    EXECUTE_ERROR_BLOCK;	% pop_spot and restore CASE_SEARCH
}

public define perl_indent_buffer ()
{
    push_spot();
    bob(); push_mark(); eob();		% mark whole buffer
    perl_indent_region();
    pop_spot();
}

% provide a few simple routines to move thru
% and mark 'chunks' (WEB notation), where a 'chunk' may either
% be a text chunk (in this case '=pod/=cut')
% or a code chunk (in this case 'sub foo { ... }')

private define perl_pn_chunk (dirfun)
{
    push_mark();
    while (@dirfun()) {
	% not strictly true for /^=pod/ but useful for indented 'sub'
	bol_skip_white(); 
	if (looking_at(pod_beg) or looking_at("sub")) {
	    pop_mark_0();
	    return;
	}
    }
    pop_mark_1();
}

define perl_prev_chunk() { perl_pn_chunk(&up_1); }
define perl_next_chunk() { perl_pn_chunk(&down_1); }

define perl_beg_chunk()
{
    variable beg = pod_beg;
    variable ptp = perl_parse_to_point();
    if (ptp > -3) beg = "sub";	% not inside of pod
    eol(); 
    !if (bol_bsearch(beg)) error ("Top of '" + beg + "' not found.");
}

define perl_end_chunk()
{
    perl_beg_chunk();
    variable ptp = perl_parse_to_point();
    if (ptp > -3) {
	if (fsearch_char('{')) call("goto_match");
    } else {
	bol();
	!if (bol_fsearch(pod_end)) error(pod_end + " not found");
    }
}

define perl_mark_chunk()
{
    perl_beg_chunk();
    push_visible_mark();
    perl_end_chunk();
    eol();
    exchange_point_and_mark();
}

define perl_mark_matching()
{
    variable beg = "([{", end = ")]}";
    variable ch = what_char();
    
    ERROR_BLOCK { 
	pop_mark_1(); 
    }
   
    USER_BLOCK0 {
	variable fn = ();
	set_mark_cmd();		% we only want a single visible mark
	if (1 != find_matching_delimiter(ch))
	  error("matching delimiter not found");
	@fn();
	exchange_point_and_mark();
	return;
    }

    if (is_substr (beg, char(ch))) {
	X_USER_BLOCK0 (&go_right_1);
    } else if (is_substr(end, char(ch))) {
	X_USER_BLOCK0 (&skip_word_chars);	% actually serves as a no-op
    }

    % not on a beg/end character

    % look backwards for an enclosing "([{"
    bskip_white ();
    _get_point();
    bskip_chars(beg);
    if (() - _get_point()) {
	ch = what_char();
	X_USER_BLOCK0 (&go_right_1);
    } 

    % look forewards for an enclosing ")]}"
    skip_white ();
    _get_point();
    skip_chars(end);
    if (() - _get_point()) {
	go_left_1 ();	% backup again
	ch = what_char();
	X_USER_BLOCK0 (&skip_word_chars);	% actually serves as a no-op
    }
}

private define perl_init_menu (menu)
{
    menu_append_item(menu, "&Top of Function",	"perl_beg_chunk");
    menu_append_item(menu, "&End of Function",	"perl_end_chunk");
    menu_append_item(menu, "&Mark Function",	"perl_mark_chunk");
    menu_append_item(menu, "&Format Buffer",   "perl_indent_buffer");
}

private variable PerlMode_Comment    = "# ";
private variable PerlMode_CommentLen = 2;

%
% this is mostly really annoying when bound to '\r'
% especially bad for '<<HERE-DOCUMENTS'!
%
define perl_newline_and_indent()
{
    !if (bolp()) {
	push_spot();
	bol_skip_white();
	if (looking_at_char('#')) {
	    variable col = what_column();
	    col--;
	    pop_spot();
	    newline();
	    insert_spaces(col);
	    insert(PerlMode_Comment);
	    return;
	}
	pop_spot();
    }
    newline();
    indent_line();
}

% adapted from cmisc.sl - formats a comment in Perl mode
private variable Perlmode_Fill_Chars = "";
define perl_paragraph_sep()
{
    if (strlen(Perlmode_Fill_Chars)) return 0;
    push_spot();
    bol_skip_white();
    if (looking_at(PerlMode_Comment)) {
	go_right(PerlMode_CommentLen);
	skip_white();
    } else if ( looking_at("##") ) {
	eol();	% don't wrap this line
    }

    eolp() or (-2 != parse_to_point());
    pop_spot();
}

define perl_format_paragraph()
{
    variable dwrap;
    
    Perlmode_Fill_Chars = "";
    if (perl_paragraph_sep()) return;
    push_spot();
    while ( not(perl_paragraph_sep()) ) {
	!if (up_1()) break;
    }
    if (perl_paragraph_sep()) go_down_1();
    push_mark();
    goto_spot();
    
    while ( not(perl_paragraph_sep()) ) {
	!if (down_1()) break;
    }
    if (perl_paragraph_sep()) go_up_1();
    narrow();
    goto_spot();

    bol();
    push_mark();
    skip_white();
    if ( looking_at(PerlMode_Comment) ) go_right(PerlMode_CommentLen);

    Perlmode_Fill_Chars = bufsubstr();
    dwrap = what_column();
    bob();
    do {
	bol_trim();
	if ( looking_at(PerlMode_Comment) ) deln(PerlMode_CommentLen);
    } while (down_1());
    WRAP -= dwrap;
    call("format_paragraph");
    WRAP += dwrap;
    bob();
    do {
	insert(Perlmode_Fill_Chars);
    } while (down_1());

    Perlmode_Fill_Chars = "";
    widen();
    pop_spot();
}

private variable tmp_input    = "_tmp_jedperl_";
private variable shell_output = "*shell-output*";

%------------------------------------------------------------------------------
%!%+
%\function{perltidy}
%\synopsis{perltidy}
%\usage{Void perltidy();}
%\description
% This function runs the program perltidy on a region or the whole buffer.
% The style preferences should be set in the ~/.perltidyrc config file.
% The perltidy program must be installed!
%!%-
define perltidy ()
{
    variable cmd  = "perltidy -st -q ";
    variable line = what_line();	% we'll try to return here later
    
    variable file, dir, thisbuf;
    (file,dir,thisbuf,) = getbuf_info();
    if (change_default_dir(dir)) {
	error("cd '" + dir + "' failed");
    }

    variable use_tmp = markp();	% using a region implies using a tmp file
    % if there's no file attached
    !if (use_tmp) {
	!if (strlen(file)) use_tmp = 1;
	mark_buffer();
    }
    
    narrow();
    if (use_tmp) {
	% with a region implies using a tmp file
	file = tmp_input;

	% guess the start indentation level
	bob();
	bol_skip_white();
	cmd += sprintf("-sil=%d ", int(what_column() / Perl_Indent));
	mark_buffer();
	() = write_region_to_file(file);
    }

    sw2buf(shell_output);
    erase_buffer ();

    % clean-up function
    % unfortunately run_shell_cmd doesn't always signal an error!!
    ERROR_BLOCK {
	sw2buf(thisbuf);
	delbuf(shell_output);
	if (use_tmp) () = delete_file(file);
	widen();
	goto_line(line);
	bol();
	flush (Null_String);
    }
    flush (cmd + file);
    run_shell_cmd(cmd + file);		% leave return code on the stack
    set_buffer_modified_flag(0);	% mark as unchanged

    % handle errors from 'run_shell_cmd'
    if (()) error ("error running perltidy");
    
    % the command apparently worked
    % switch back to our original buffer and update everything
    sw2buf(thisbuf);
    mark_buffer();
    del_region();
    insbuf(shell_output);
    EXECUTE_ERROR_BLOCK;
}


% Run perl with some flags on current region if one is defined, otherwise
% on the whole buffer.
%
% Display output in *shell-output* buffer window.
%
% Error messages look like this:
% Missing right curly or square bracket at _tmp_jedperl_ line 7, at end of line
% 
% Thus we'll look for ' at FILENAME line '
private define do_perl (opts, prompt)
{
    variable args = Null_String;
    variable line = 0;		% line offset
    
    variable file, dir, thisbuf;
    (file,dir,thisbuf,) = getbuf_info();
    if (change_default_dir(dir)) {
	error("cd '" + dir + "' failed");
    }

    variable use_tmp = markp();	% using a region implies using a tmp file
    !if (use_tmp) {
	if (strlen(prompt))
	  args = read_mini( prompt, Null_String, Null_String );
	
	% if there's no file attached
	!if (strlen(file)) {
	    use_tmp = 1;	     
	    mark_buffer();
	}
    }

    if (use_tmp) {
	% with a region implies using a tmp file
	file = "_tmp_jedperl_";

	check_region(1);		% canonical region
	exchange_point_and_mark();	% goto start
	line = what_line();

	% force 'strict'; also has a line offset of 1 as a nice side-effect
	() = write_string_to_file( "use strict;\n", file );
	() = append_region_to_file(file);
	pop_spot();
    }

#ifdef OS2 UNIX
    args += " 2>&1";	% re-direct stderr as well
#endif 

    variable oldbuf = pop2buf_whatbuf(shell_output);
    erase_buffer (); 
    % in case our system command bombs out
    ERROR_BLOCK {
	if (use_tmp) () = delete_file(file);
    }
    
    run_shell_cmd(strjoin(["perl", opts, Perl_Flags, file, args], " "));
    set_buffer_modified_flag(0);	% mark as unchanged

    % report errors from 'run_shell_cmd'
    if (()) flush ("error running perl");

    EXECUTE_ERROR_BLOCK;
				      
    % try to restore any window that got replaced by the shell-output 
    %%     if (strlen(oldbuf) 
    %% 	and (oldbuf != shell_output)
    %% 	and (oldbuf != thisbuf) )
    %%       { 
    %% 	  splitwindow(); sw2buf(oldbuf); pop2buf(shell_output);
    %%       } 
    eob(); 
    
    % Move to the line in source that generated the error
    if ( right( bsearch( " at " + file + " line " ) ) ) {
	push_mark ();
	skip_chars ("0-9");
	line += integer(bufsubstr());
	%%	flush (sprintf ("goto line %d", line));
	pop2buf(thisbuf);
	goto_line(line);
	bol();
    }
}

define perl_exec()  { 
    do_perl("-w", "perl @ARGV");
}
define perl_check() {
    do_perl("-cTw", Null_String); % tainting on, no args
}

%------------------------------------------------------------------------------
%!%+
%\function{perl_mode}
%\synopsis{perl_mode}
%\usage{Void perl_mode();}
%\description
% This is a mode that is dedicated to editing Perl language files
% including a bimodal Pod/Perl indentation mode.
% The indentation style matches the results of perltidy(1) with
% '-ci=2 -i=4 -en=8' fairly closely, except some of the closing brackets.
%
% Functions that affect this mode include:
%#v+
%  function:             default binding:
%  indent_line                TAB
%  perl_beg_chunk             ESC Ctrl-A
%  perl_end_chunk             ESC Ctrl-E
%  perl_mark_chunk            ESC Ctrl-H
%  perl_mark_matching         ESC Ctrl-M
%  perl_next_chuck            ESC Ctrl-N
%  perl_prev_chunk            ESC Ctrl-P
%  perl_indent_region         ESC TAB
%  perl_indent_region         Ctrl-C TAB
%  perl_format_paragraph      ESC q
%  perl_newline_and_indent    not bound (Ctrl-M)
%  perl_indent_buffer         not bound
%
%  perl_exec                  not bound  (Ctrl-C Ctrl-C)
%  perl_check                 not bound  (Ctrl-C ?)
%  perltidy                   ESC Ctrl-T
%
%
%  perl_exec, perl_check, and perltidy work on a region or the whole buffer
%
%#v-
% Variables affecting this mode include:
%#v+
%  Perl_Indent
%  Perl_Continued_Offset
%#v-
% Hooks: \var{perl_mode_hook}
%!%-
define perl_mode()
{
    variable mode = "perl";

    set_mode(mode, 4);
    use_keymap(mode);
    use_syntax_table(mode);

    % 2001-02-19  Mark Olesen <mark.olesen@gmx.net>
    % - changed syntax highlighting to start with /^=head/ instead
    %   of the more correct but less common /^=pod/ as a comment.
    %   Note that this only affects the syntax highlighting ... the
    %   indentation continues to work as before and /^=cut/ is always the end

    push_spot_bob();
    variable beg = pod_beg;
    if ( (not(bol_fsearch(beg))) and (bol_fsearch(pod_too)) ) beg = pod_too;
    define_syntax(beg, pod_end, '%', mode);
    pop_spot();

    set_buffer_hook("par_sep", "perl_paragraph_sep");
    set_buffer_hook("indent_hook", &perl_indent_line);
#ifnfalse
    mode_set_mode_info(mode, "fold_info", "#{{{\r#}}}\r\r");
#else
    mode_set_mode_info(mode, "fold_info", "=pod\r=cut\r\r");
#endif
    mode_set_mode_info(mode, "init_mode_menu", &perl_init_menu);
    run_mode_hooks("perl_mode_hook");
}
% ------------------------------------------------------------- [end of S-Lang]
% Run this code snippet thru 'perl -x' to extract the function names from
% 'perlfunc' - keywords 'if', 'else', etc. added by hand
%
% use '##' so comments don't look like S-Lang pre-processor directives
% ----------------------------------------------------------------------------
#iffalse
#!perl -w
use strict;	# yes even for such a small program!

@ARGV = 'perldoc -u perlfunc|';
while (<>) { /^=head2\s+Alphabetical/ and last }	# cue up

my %kw = map { $_ => length } map { /^=item\s+([a-z\d]+)/ } <>;  # keywords

## standard keywords + carp/croak (which everyone always uses)
for (qw(if else elsif for foreach unless until while carp croak)) {
    $kw{$_} = length
}
delete @kw{ grep { /^dbm/ } keys %kw };	# obsolete

my @list;		# store sorted keywords by length
$list[$kw{$_}] .= $_  for ( sort keys %kw );

splice @list, 0, (my $n = 2);	# keywords with < 2 letters are useless

for (@list) {
  defined and length
    and print "() = define_keywords(\$1,\n  \"$_\",\n  $n);\n";
  $n++
}
__END__
#endif	% done processing perl
% --------------------------------------------------------------- [end of Perl]
