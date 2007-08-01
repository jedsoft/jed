% this -*- SLang -*- file defines syntax/indentation for Perl programs
% ------------------------------------------------------------------------
%
% NB: no automatic brace '{' handling!  otherwise constructs such as
%   '@foo = @{$arrayref}', and '%foo = %{$hashref}' are a nightmare!
%
% - Because of how the internal syntax highlighting is implemented
%   (ask John about details), it's only possible to use either
%   '=pod' or '=head' in the highlighting, but not both.
%
%   Many (most) modules simply use '=head', but if you're mixing code
%   and documentation a lot, the '=pod' directive is extremely handy.
%   Although perl understands it when you start a pod chunk with any
%   '=foo' tag, the editor does not.
%   For this case use '=pod' as an introducer to trigger pod.
%   NB: you couldn't use '=head' here!
%   See perlpod(1) for more details.
%
%   The mode tries to guess which style is being used.
%   When only '/^=head/' and no '/^=pod/' is found, the '=head' tag will
%   be used.  Otherwise '=pod' will be used.
%   For intermixed coding styles, a periodic M-x 'perl_mode' helps.
%
%   NB: this only affects the *syntax highlighting* ... 
%   indentation should be fine.
%
%   NB: since the syntax highlighting matches /=pod/ and not /^=pod/,
%   these patterns within the code will cause spurious highlighting!
%   Consider using something like /^=(pod|head|cut)/ instead.
%
% ------------------------------------------------------------------------
% BUGS / MIS-FEATURES:
% - suppressed indentation in '<<HERE' documents is not handled.
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
% - outdented labels is not yet supported
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
% - my recommendation - forget DFA syntax highlighting for perl code.
%
% ------------------------------------------------------------------------
%
% Changes:
% Original author: Lars Marowsky-Bree <lmb@pointer.in-minden.de>
%
% 2001-01-10  Mark Olesen <mark dot olesen at gmx dot net>
% - modified a few keywords, added a Perl specific keymap
% - indentation mode improved and now includes a POD mode
% - added begin/end/mark/prev/next 'chunk' (sub or pod)
%
% 2002-01-27  Mark Olesen <mark dot olesen at gmx dot net>
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
% 2002-01-31  Mark Olesen <mark dot olesen at gmx dot net>
% - indentation for "=>(", "=>[" and "=> {" constructs
%   and for "= (", "= [" and "={" constructs use 'Perl_Indent' are are
%   not lined up! This helps reduce panning for large tables.
% - The multi-line contents of '()' are indented by 1
% - The multi-line contents of '[]' are indented by 'Perl_Continued_Offset'
%
% 2002-02-17  Mark Olesen <mark dot olesen at gmx dot net>
% - The contents of '([{' are no longer indented to match the
%   opening parentheses.  Instead they receive a uniform 'Perl_Indent'.
%   This matches the results of perltidy(1) with '-i=4 -ci=2' much better,
%   the code tends not to run off the end of lines as quickly,
%   and the results are much more consistent (less monkeying with the
%   code-layout to get the indent to work correctly).
% - lines starting with 'grep', 'and', etc and lines following them
%   are now corectly indented.
%   Even when the previous line ended with '}'.
%   We can continue our lines with the perlstyle(1) suggested 'and' / 'or'
% - perl_parse_to_point() now returns -3 if inside of pod.
% - lines starting with '#' in the first column are left alone
% - The older versions are commented out with 'PERL_INDENT_V\d+'
%
% 2002-02-24  Mark Olesen <mark dot olesen at gmx dot net>
% - perl_mark_matching is a bit more generous with whitespace when
%   attempting a match
% - the brain-dead PERL_INDENT_V0 (version 0) is no longer included
% - included the '$@%' sigils as word characters. The '%' as modulus
%   operator will be incorrect, but we have many more hashes ...
%
% 2002-03-02  Mark Olesen <mark dot olesen at gmx dot net>
% - added 'perl_exec' and 'perl_check' functions
% - since I've never seen a perl program with folding marks,
%   I changed the folding marks from the '#{{{' and '#}}}' to
%   the slightly more useful '=pod' and '=cut'.  
%   Thus you can fold your documentation out of the way.
% - added interface to the 'perltidy' (sourceforge) program.
%   This is *extremely* handy, especially if you are a bit sloppy about
%   spacing around brackets etc.
%
% 2002-08-20  Mark Olesen <mark dot olesen at gmx dot net>
% - minor S-Lang improvements
% - somewhat improved documentation
%
% 2002-10-21  Mark Olesen <mark dot olesen at gmx dot net>
% - split off some functions into perlxtra.sl
%
% 2006-08-28  Mark Olesen
% - use '#<embed>' .. '#</embed>' construct
% - cosmetics
% 
% 2006-11-11 JED
% - Use "require" instead of evalfile for loading perlxtra.
% 
% 2007-06-20 JED
% - Added find_matching_brace_ignore_fold_marks to search for a
%   matching delimiter ignoring fold marks.
%
% ------------------------------------------------------------------------
% ------------------------------------------------------------------------
% forget autoloading - always just add in perl 'extras'
% we also get 'Perl_Indent' from there
%% autoload ("perltidy",	"perlxtra");
%% autoload ("perl_exec",	"perlxtra");
%% autoload ("perl_info",	"perlxtra");
%% autoload ("perl_check",	"perlxtra");
%% autoload ("perl_help",	"perlxtra");
%% autoload ("perldoc",	"perlxtra");
require ("perlxtra");

% !if (is_defined("perldoc") and is_defined("Perl_Indent")) 
%  () = evalfile("perlxtra");

%{{{ default values for Perl custom variables
% override these default values in ~/.jedrc
%!%+
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
%
% The default value (2) corresponds to the default for \var{perltidy}
%
%\seealso{C_CONTINUED_OFFSET, Perl_Indent}
%!%-
custom_variable("Perl_Continued_Offset", 2);
%}}}

% some constant strings for tracking POD documentation
static variable
  pod_beg = "=pod",	% start pod documentation (the canonical way)
  pod_too = "=head",	% one of the alternative ways to start pod
  pod_end = "=cut";	% stop pod documentation

%{{{ create/initialize key map
$1 = "perl";
!if (keymap_p ($1)) make_keymap($1);
!if (is_defined("Win_Keys") ) {  % conflict with windows region copy
    definekey("perl_help",	"^C?", $1);
    definekey("perl_check",	"^Cc", $1);
    definekey("perl_exec",	"^Ce", $1);
    definekey("perl_info",	"^Ci", $1);
    definekey("perl_indent_region", "^C\t", $1);
    definekey("perltidy",	"^C^T", $1);
}

definekey("indent_line",     "\t",   $1);
definekey("perl_beg_chunk",  "\e^A", $1);
definekey("perl_end_chunk",  "\e^E", $1);
definekey("perl_mark_chunk", "\e^H", $1);
definekey("perl_next_chunk", "\e^N", $1);
definekey("perl_prev_chunk", "\e^P", $1);
definekey("perl_mark_matching", "\e^M", $1);
definekey("perltidy",		"\e^T", $1);
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
#ifdef HAS_DFA_SYNTAX_THAT_WORKS_FOR_PERL
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
% keywords
% ignore the functions that do special, pre-defined things including
%   `BEGIN', `CHECK', `INIT', `END', `AUTOLOAD', and
%   `DESTROY'--plus all functions mentioned in the perltie manpage.
% $1 = "perl";

%{{{ Type 0 keywords - automatically generated
() = define_keywords($1,
		     "doiflcmynoqqqrqwqxtruc",
		     2);
() = define_keywords($1,
    "abschrcosdieeofexpforhexintlogmapoctordourpopposrefsinsubtieusevec",
		     3);
() = define_keywords($1,
    "bindcarpchopdumpeachelseevalexecexitforkgetcglobgotogrepjoinkeyskilllastlinklocknextopenpackpipepushrandreadrecvredoseeksendsizesortsqrtstattelltiedtimewaitwarn",
		     4);
() = define_keywords($1,
    "alarmatan2blesschdirchmodchompchownclosecroakcryptelsiffcntlflagsflockindexioctllocallstatmkdirorderprintresetrmdirsemopshiftsleepsplitsrandstudytimesumaskundefuntieuntilutimewhilewrite",
		     5);
() = define_keywords($1,
    "acceptcallerchrootdeleteexistsfilenoformatgmtimeimportlengthlistenmsgctlmsggetmsgrcvmsgsndprintfrenamereturnrindexscalarselectsemctlsemgetshmctlshmgetsocketsplicesubstrsystemunlessunlinkunpackvaluesvector",
		     6);
() = define_keywords($1,
    "binmodeconfessconnectdefinedforeachgetpgrpgetppidlcfirstopendirpackagereaddirrequirereverseseekdirsetpgrpshmreadsprintfsymlinksyscallsysopensysreadsysseektelldirucfirstunshiftwaitpid",
		     7);
() = define_keywords($1,
    "closedircontinueendgrentendpwentformlinegetgrentgetgrgidgetgrnamgetlogingetpwentgetpwnamgetpwuidreadlinereadlinkreadpipesetgrentsetpwentshmwriteshutdownsyswritetruncate",
		     8);
() = define_keywords($1,
    "endnetentgetnetentlocaltimeprecisionprototypequotemetarewinddirsetnetentwantarray",
		     9);
() = define_keywords($1,
    "endhostentendserventgethostentgetserventgetsockoptsethostentsetserventsetsockoptsocketpair",
		     10);
() = define_keywords($1,
    "endprotoentgetpeernamegetprioritygetprotoentgetsocknamesetprioritysetprotoent",
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
%}}} Type 0 keywords - automatically generated

% Type 1 keywords - use for operator-like keywords
() = define_keywords_n($1, "eqgegtleltneor", 2, 1);
() = define_keywords_n($1, "andcmpnotxor", 3, 1);
%

% returns the same as the standard parse_to_point,
% except -3 if we're inside of pod
private define perl_parse_to_point()
{
    % debug->    EXIT_BLOCK { pop_spot(); dup(); vmessage("ptp=%d",()); (); }
    EXIT_BLOCK { pop_spot(); }

    push_spot();
    variable ptp = parse_to_point();

    % see if the current line starts with a /^=[a-z]+/ pod directive
    bol();
    if (looking_at_char('=')) {
	go_right_1();
	_get_point();	% leave on the stack
	skip_chars ("a-z");
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
static variable delimiter = "\t $%@([{:?}])";

%
% returns non-zero if looking at one of the tokens in the list
% NB: if you look for a token such as '||', it must be nicely delimited
% from its following neighbour. ie, write '|| foo()' instead of '||foo()'
%
private define perl_looking_at()
{
    variable cs = CASE_SEARCH, rc = 0;

    EXIT_BLOCK {
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
    
    EXIT_BLOCK {
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

private define find_matching_brace_ignore_fold_marks (endch);
private define find_matching_brace_ignore_fold_marks (endch)
{
   variable m = create_user_mark ();
   if (1 != find_matching_delimiter (endch))
     return 0;
   !if (blooking_at ("#{{") or blooking_at ("# {{"))
     return 1;
   go_left(2);
   if (1 == find_matching_brace_ignore_fold_marks (endch))
     return 1;

   goto_user_mark (m);
   return 0;
}

%!%+
%\function{perl_indent_line}
%\synopsis{Void perl_indent_line (Void)}
%\description
% indents the line
%\seealso{perl_indent_region, Perl_Indent, Perl_Continued_Offset, perl_mode}
%!%-
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
    
    % vmessage("ptp = %d", ptp);

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
	if ( orelse { blooking_at ("<=") } { blooking_at ("=>") } ) {
	    bol_skip_white();
	    col = what_column();
	    extra_indent = Perl_Indent;	
	}
#endif
	if (ch != endch) col += extra_indent;
	indent_ok++;	
    }

    goto_spot(); bol();	% (original position : start of line)

    % --------------------------------------------------------------------
    % take care of indentation for '{' blocks
    % --------------------------------------------------------------------
    endch = '}';
    if (ch == '{') indent_ok++;
    if (find_matching_brace_ignore_fold_marks (endch))
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

	if ( orelse { bfind_char ('(') } { bfind_char ('[') } ) {
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
	% vmessage("col '%c' = %d", endch, col);
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

    % vmessage("look = %c", what_char);

    bskip_white();
    ch = ';';		% default final character
    !if (bolp()) {
	go_left_1();
	if (perl_parse_to_point() >= -2) ch = what_char();
    }

    % vmessage("end char = %c", ch);
    
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

%!%+
%\function{perl_indent_region}
%\synopsis{Void perl_indent_region (Void)}
%\description
% indents each line in the region
%\seealso{perl_indent_line, perl_indent_buffer, perltidy, perl_mode}
%!%-
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
	if ( orelse { looking_at (pod_beg) } { looking_at (pod_too) } ) {
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
	    if (right (ffind ("<<"))) {
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
	vmessage("processed %d/%d lines.", line, nlines);
    } while (down_1() and (line < nlines));
    
    EXECUTE_ERROR_BLOCK;	% pop_spot and restore CASE_SEARCH
}

%!%+
%\function{perl_indent_buffer}
%\synopsis{Void perl_indent_buffer (Void)}
%\description
% indents the entire buffer (NB: using perltidy might be easier)
%\seealso{perl_indent_line, perl_indent_region, perltidy, perl_mode}
%!%-
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

static define perl_pn_chunk (dirfun)
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

%!%+
%\function{perl_beg_chunk}
%\synopsis{Void perl_beg_chunk (Void)}
%\description
% move to the beginning of a code chunk
%   (starting with 'sub' in the first column)
% or to the beginning of a POD chunk.
%\seealso{perl_end_chunk, perl_mark_chunk, perl_mode}
%!%-
define perl_beg_chunk()
{
    variable beg = pod_beg;
    variable ptp = perl_parse_to_point();
    if (ptp > -3) beg = "sub";	% not inside of pod
    eol(); 
    !if (bol_bsearch(beg)) error ("Top of '" + beg + "' not found.");
}

%!%+
%\function{perl_end_chunk}
%\synopsis{Void perl_end_chunk (Void)}
%\description
% move to the end of a code chunk or to the end of a POD chunk
%\seealso{perl_beg_chunk, perl_mark_chunk, perl_mode}
%!%-
define perl_end_chunk()
{
    perl_beg_chunk();
    variable ptp = perl_parse_to_point();
    if (ptp > -3) {		% not inside of pod
	if (fsearch_char('{')) call("goto_match");
    } else {
	bol();
	!if (bol_fsearch(pod_end)) error(pod_end + " not found");
    }
}

%!%+
%\function{perl_mark_chunk}
%\synopsis{Void perl_mark_chunk (Void)}
%\description
% marks the code/Pod code
%\seealso{perl_beg_chunk, perl_end_chunk, perl_mode}
%!%-
define perl_mark_chunk()
{
    perl_beg_chunk();
    push_visible_mark();
    perl_end_chunk();
    eol();
    exchange_point_and_mark();
}

%!%+
%\function{perl_mark_matching}
%\synopsis{Void perl_mark_matching (Void)}
%\description
% works mostly like find_matching_delimiter, except that it attempts
% to be extra smart if starting out on whitespace
%\seealso{find_matching_delimiter}
%!%-
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
    _get_point();	% leave on the stack
    bskip_chars(beg);
    if (() - _get_point()) {
	ch = what_char();
	X_USER_BLOCK0 (&go_right_1);
    } 

    % look forwards for an enclosing ")]}"
    skip_white ();
    _get_point();	% leave on the stack
    skip_chars(end);
    if (() - _get_point()) {
	go_left_1 ();	% backup again
	ch = what_char();
	X_USER_BLOCK0 (&skip_word_chars);	% actually serves as a no-op
    }
}

static define perl_init_menu (menu)
{
    menu_append_item(menu, "&Top of Function",	"perl_beg_chunk");
    menu_append_item(menu, "&End of Function",	"perl_end_chunk");
    menu_append_item(menu, "&Mark Function",	"perl_mark_chunk");
    menu_append_item(menu, "&Format Buffer",   "perl_indent_buffer");
}

static variable PerlMode_Comment    = "# ";
static variable PerlMode_CommentLen = 2;

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
static variable Perlmode_Fill_Chars = "";
%!%+
%\function{perl_paragraph_sep}
%\synopsis{Void perl_paragraph_sep (Void)}
%\description
% defines paragraphs for Perl mode
%\seealso{parse_to_point}
%!%-
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

%!%+
%\function{perl_format_paragraph}
%\synopsis{Void perl_format_paragraph (Void)}
%\description
% should format a comment paragraph in Perl mode, but not entirely stable?
%\seealso{perl_mode}
%!%-
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


%------------------------------------------------------------------------------
%!%+
%\function{perl_mode}
%\synopsis{Void perl_mode (Void)}
%\description
% This is a mode that is dedicated to editing Perl language files
% including a bimodal Pod/Perl indentation mode.
% The indentation style matches the results of perltidy(1) with
% '-ci=2 -i=4 -en=8' fairly closely, except some of the closing brackets.
%
% This seems to be missing, so you might want to add 
% add_mode_for_extension("perl", "pm");
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
%  perl_newline_and_indent    Ctrl-M (not bound)
%  perl_indent_buffer         (not bound)
%
%  perl_help                  Ctrl-C ?
%  perl_check                 Ctrl-C c
%  perl_indent_region	      Ctrl-TAB
%  perl_exec                  Ctrl-C e
%  perl_info                  Ctrl-C i
%  perltidy                   Ctrl-C Ctrl-T
%
%#v-
% Variables affecting this mode include:
%#v+
%  Perl_Continued_Offset
%  Perl_Flags
%  Perl_Indent
%#v-
% Hooks: \var{perl_mode_hook}
%\seealso{perldoc}
%!%-
define perl_mode ()	% <AUTOLOAD> <COMPLETE> <EXTS="pm">
{
    variable mode = "perl";

    set_mode(mode, 4);
    use_keymap(mode);
    use_syntax_table(mode);

    % 2001-02-19  Mark Olesen
    % - changed syntax highlighting to start with /^=head/ instead
    %   of the more correct but less common /^=pod/ as a comment.
    %   Note that this only affects the syntax highlighting ... the
    %   indentation continues to work as before and /^=cut/ is always the end

    push_spot_bob();
    variable beg = pod_beg;
    if ( (not (bol_fsearch (beg))) and (bol_fsearch (pod_too)) )
      beg = pod_too;
    define_syntax(beg, pod_end, '%', mode);
    pop_spot();

    set_buffer_hook("par_sep", "perl_paragraph_sep");
    set_buffer_hook("indent_hook", &perl_indent_line);
#iffalse
    mode_set_mode_info(mode, "fold_info", "#{{{\r#}}}\r\r");
#else	% more useful
    mode_set_mode_info(mode, "fold_info", "=pod\r=cut\r\r");
#endif
    mode_set_mode_info(mode, "init_mode_menu", &perl_init_menu);
    run_mode_hooks("perl_mode_hook");
}
% -------------------------------------------------------- [end of S-Lang]
% Run this code snippet thru 'perl -x' to extract the function names from
% 'perlfunc' or narrow to this region and run it through 'perl_exec'
%
% keywords 'if', 'else', etc. added by hand
%
% use '#%#' for S-Lang syntax highlighting of perl comments
% ------------------------------------------------------------------------
#<perl>
#!/usr/bin/perl -w	#%# for 'perl -x'
use strict;	        #%# even for small programs!

@ARGV = "perldoc -u perlfunc|";
while (<>) { last if /^=head2\s+Alphabetical/ }	# cue up

my %kw = map { $_ => length }
(
    #%# language elements + carp
    qw(
	if else elsif for foreach unless until while
	carp cluck croak confess
    ),
    #%# keywords
    map { /^=item\s+([a-z\d]+)/ } <>,
);

delete @kw{ grep { /^dbm/ } keys %kw };	#%# remove obsolete stuff

my @list;	#%# store sorted keywords by length
$list[$kw{$_}] .= $_  for sort keys %kw;

splice @list, 0, (my $n = 2);	#%# keywords are min 2 chars

my $tag = "Type 0 keywords - automatically generated";
print "\%{{{ $tag\n";

for (@list) {
  defined and length
      and print qq{()=define_keywords(\$1,\n    "$_",\n    $n);\n};
  $n++
}

print "\%}}} $tag\n";

__END__
#</perl>
% ---------------------------------------------------------- [end of Perl]
