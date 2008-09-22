%
%  file : html.sl
%
%  Original Author : Raikanta Sahu, rsahu@mail.unm.edu
%  Substantial additions by Jim Knoble.

%  Modified by John E. Davis for incorporation into JED.
%% 
%% Modified by Michael D Johnson to conform with the standards recommendation 
%% on capitalization.
%% 
%  Modified by Kees Serier
%  All lower case except for DOCTYPE (like all W3C code), html: 2 3.2 4 XHTML
%  Definition lists complete, tables, euro symbol and more.

% 1 => html_mode wraps, like text_mode
% 0 => html_mode doesn't wrap, like no_mode

define html_paragraph_separator ()
{
   bol_skip_white ();
   eolp () or ffind_char ('>') or ffind_char ('<');
}

% Movement function (JM)
%!%+
%\function{html_skip_tag}
%\synopsis{html_skip_tag}
%\description
% skip forward past html tag
%!%-
define html_skip_tag()
{
   !if (fsearch_char ('>')) return;
   go_right_1 ();
}

%!%+
%\function{html_bskip_tag}
%\synopsis{html_bskip_tag}
%\description
% skip backward past html tag
%!%-
define html_bskip_tag()
{
   () = bsearch_char ('<');
}

%!%+
%\function{html_mark_next_tag}
%\synopsis{html_mark_next_tag}
%\description
% mark the next html tag forward
%!%-
define html_mark_next_tag()
{
   variable taglng = 1;
   
   !if (fsearch_char ('>')) return;
   go_right(taglng);
   set_mark_cmd ();
   go_left(taglng);
   () = find_matching_delimiter (0);
}

%!%+
%\function{html_mark_prev_tag}
%\synopsis{html_mark_prev_tag}
%\description
% mark the previous html tag
%!%-
define html_mark_prev_tag()
{
   !if (bsearch_char ('<')) return;
   set_mark_cmd ();
   () = find_matching_delimiter(0);
   go_right_1 ();
   exchange_point_and_mark();
}


%
% First define some useful functions
%

define html_insert_pair_around_region (lfttag, rgttag)
{
   % make sure mark is before point;
   % 1 => push spot first
   check_region(1);
   
   % put tags on appropriate sides of region,
   % then return to where we were
   exchange_point_and_mark();
   insert(lfttag);
   exchange_point_and_mark();
   insert(rgttag);
   pop_spot();
   pop_mark_0 ();
}

define html_insert_move (str)
{
   variable len;
   variable beg, end;
   
   len = is_substr (str, "@");
   !if (len) return;
   len--;
   if (markp ())
     {
	beg = substr (str, 1, len);
	end = substr (str, len + 2, strlen (str));
	html_insert_pair_around_region (beg, end);
	return;
     }
   
   push_spot ();
   insert (str);
   pop_spot ();
   go_right (len);
   del ();
}

define html_simple_insert (str)
{
   html_insert_move (sprintf ("<%s>@</%s>", str, str));
}

define html_insert_with_newline (str)
{
   html_insert_move (sprintf ("<%s>\n@</%s>\n", str, str));
}

define html_insert_with_2newlines (str)
{
   html_insert_move (sprintf ("<%s>\n@\n</%s>\n", str, str));
}

define html_form ()
{
   html_insert_move ("<form action=\"\" method=\"\">\n@\n</form>");   
}

define html_input ()
{
   insert ("<input type=\"\" name=\"\" value=\"\">");
}

define html_input_text ()
{
   insert ("<input type=\"text\" name=\"\" value=\"\">");
}

define html_input_password ()
{
   insert ("<input type=\"password\" name=\"\" value=\"\">");
}

define html_input_checkbox ()
{
   insert ("<input type=\"checkbox\" name=\"\" value=\"\">");
}

define html_input_radio ()
{
   insert ("<input type=\"radio\" name=\"\" value=\"\">");
}

define html_input_submit ()
{
   insert ("<input type=\"submit\" value=\"OK\">");
}

define html_input_reset ()
{
   insert ("<input type=\"reset\"  value=\"Clear\">");
}

define html_input_hidden ()
{
   insert ("<input type=\"hidden\" name=\"\" value=\"\">");
}

define html_select ()
{
   html_insert_move ("<select name=\"@\" size=\"\">\n@\n</select>");
}

define html_text_area ()
{
   html_insert_move ("<textarea name=\"@\"></textarea>");
}

%
%  Make comment
%
define html_comment ()
{
   html_insert_move ("<!-- @ -->");
}

%
% insert Horizontal rule  TJO
%
define html_horiz_rule ()
{
   insert("\n<hr>\n") ;
}

define html_heading (c)
{
   html_insert_move (sprintf ("<h%c>@</h%c>", c, c));
}

define html_insert_eol (str)
{
   eol ();
   vinsert ("<%s>", str);
}

define html_insert_bol (str)
{
   bol ();
   vinsert ("<%s>", str);
}

% insert at cursor
define html_insert_here (str)
{
   vinsert ("<%s>", str);
}

%
% Make markers for an image
%
define html_image ()
{
   html_insert_move ("<img src=\"@\" alt=\"\">");
}

%
% main entry point into the html mode
% commands available to keystrokes in html mode
%

define html_quoted_insert ()
{
   variable ch;
   
   !if (input_pending (5)) flush ("`-");
   ch = getkey ();
   switch (ch)
     {
      case '\r':
	insert ("<br>\n");
     }
     {
      case '&':
	insert ("&amp;");
     }
     {
      case '>':
	insert ("&gt;");
     }
     {
      case '<':
	insert ("&lt;");
     }
     {
      case ' ':
	insert ("&nbsp;");
     }
     {
      case 'e':
	insert ("&euro;");
     }
     {
	% default:  The other special characters should be added.
	insert_char (ch);		       
     }
}

% Support for HTML paragraphs.  MDJ 04/06/98
define html_par_insert()
{
   insert("\n\n<p>\n");
}

define html_read_key (hlp)
{
   variable key;
   !if (input_pending (3)) flush (hlp);
   tolower (getkey ());
}

define html_doctype ()
{
   variable key2;
   key2 = html_read_key ("HTML version: 2  3.2  4  Xhtml");
   switch (key2)
     {case '2': 
	insert ("<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");}
     {case '3': 
	insert ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 FINAL//EN\">\n");}
     {case '4': 
	insert ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.0 FINAL//EN\">\n");}
     {case 'x' or case 'X': 
	insert ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">\n");}
     { beep (); }
}

define html_template ()
{
   html_doctype ();
   html_insert_move ("<html>\n\n<head>\n<title>@</title>\n</head>\n\n<body>\n</body>\n\n</html>") ;
}


define html_keymap_a ()
{
   variable name = "<a name=\"@\"></a>";
   variable href = "<a href=\"@\"></a>";
   
   switch (html_read_key ("Href  Name"))
     { case 'h': href; }
     { case 'n': name; }
     {
	beep (); return;
     }
   html_insert_move (());
}

define html_keymap_d ()
{
   variable key;
   key = html_read_key ("dL  dT  dD");
   switch (key)
     { case 'l': html_insert_with_newline ("dl"); }
     { case 't': insert ("<dt>"); }
     { case 'd': insert ("<dd>"); }
     { beep (); }
}

define html_keymap_f ()
{
   
   switch (html_read_key ("txtArea Chkbox Form Hidden Input Option Passw Radio Select Text Xreset Ysubmit"))
     {case 'a': html_text_area (); }
     {case 'c': html_input_checkbox (); }
     {case 'f': html_form (); }
     {case 'h': html_input_hidden (); }
     {case 'i': html_input (); }
     {case 'o': html_insert_bol("option"); }
     {case 'p': html_input_password (); }
     {case 'r': html_input_radio (); }
     {case 's': html_select (); }
     {case 't': html_input_text (); }
     {case 'x': html_input_reset (); }
     {case 'y': html_input_submit (); }
     {
	% default
	beep ();
     }
}

define html_keymap_h ()
{
   variable key;
   key = html_read_key ("h1  h2  h3  h4  h5  h6  templAte Doctype  Head  Body  htmL  Title");
   switch (key)
     % { case 'd': html_insert_bol ("doc"); }
     { case 'd': html_doctype (); }
     { case 'h': html_insert_with_newline ("head"); }
     { case 'b': html_insert_with_newline ("body"); }
     { case 'l': html_insert_with_newline ("html"); }
     { case 't': html_insert_with_newline ("title"); }
     { case 'a': html_template (); }
     { (key <= '6') and (key >= '1') : html_heading (key);}
     { beep (); }
}

define html_keymap_i ()
{
   html_image ();
}

define html_keymap_l ()
{
   switch (html_read_key ("Dir Li Menu Ordered Un-ordered"))
     { case 'd': html_insert_with_newline ("dir"); }
     { case 'l': html_insert_here ("li"); }
     { case 'm': html_insert_with_newline ("menu"); }
     { case 'o': html_insert_with_newline ("ol"); }
     { case 'u': html_insert_with_newline ("ul"); }
     {
	% default 
	beep ();
     }
}

define html_keymap_p ()
{
   switch (html_read_key ("Break Hrule Par blockQuote pRe"))
     { case 'b': html_insert_eol ("br"); }
     { case 'h': html_horiz_rule (); }
     { case 'p': insert ("<p>\n"); }
     { case 'q': html_insert_with_newline ("blockquote"); }
     { case 'r': html_insert_with_newline ("pre"); }
     {
	beep ();
     }
}

define html_keymap_s ()
{
   switch (html_read_key ("Address Bold Cite Emph Font Ital Kbd cOde Samp Tt Uline Var"))
     { case 'a': "address"; }
     { case 'b': "b"; }
     { case 'c': "cite"; }
     { case 'e': "em"; }
     { case 'f': "font"; }
     { case 'i': "i"; }
     { case 'k': "kbd"; }
     { case 'o': "code"; }
     { case 's': "samp"; }
     { case 't': "tt"; }
     { case 'u': "u"; }
     { case 'v': "var"; }
     {
	beep (); return;
     }
   html_simple_insert (());
}

define html_keymap_t ()
{
   switch (html_read_key ("Table Row Header Data"))
     { case 't': html_insert_with_newline ("table"); }
     { case 'r': html_insert_with_newline ("tr"); }
     { case 'h': html_simple_insert ("th"); }
     { case 'd': html_simple_insert ("td"); }
     {
	beep (); return;
     }
}

define html_keymap ()
{
   variable key = html_read_key ("Anchors  Dfnlists  Forms  Headings  Images  Lists  Pstyles  cStyles Tables");
   switch (key)
     { case 2: html_bskip_tag (); }	   %  ^B
     { case 6: html_skip_tag (); }	   %  ^F
     { case 14: html_mark_next_tag (); }   %  ^N
     { case 16: html_mark_prev_tag (); }   %  ^P
     { case 'c': html_comment (); }
     { case 'a': html_keymap_a (); }
     { case 'd': html_keymap_d (); }
     { case 'f': html_keymap_f (); }
     { case 'h': html_keymap_h (); }
     { case 'i': html_keymap_i (); }
     { case 'l': html_keymap_l (); }
     { case 'p': html_keymap_p (); }
     { case 's': html_keymap_s (); }
     { case 't': html_keymap_t (); }
     {
	ungetkey (key);
	html_quoted_insert ();
     }
   flush ("");
}

$1 = "html";
!if (keymap_p ($1)) make_keymap ($1);
if (_Reserved_Key_Prefix != NULL)
{
   undefinekey (_Reserved_Key_Prefix, $1);
   definekey("html_keymap", _Reserved_Key_Prefix, $1);
}

undefinekey ("\e;", $1);
definekey ("html_comment",   "\e;",  $1);
definekey ("html_quoted_insert",   "`",  $1);

definekey("html_par_insert", "\e^M", $1); % Neater paragraph insert (MDJ 04/06/98)


create_syntax_table ($1);
define_syntax ("<", ">", '(', $1);     %  make these guys blink match
define_syntax ("<>", '<', $1);
%define_syntax ("<!-", "-->", '%', $1);   % Some broken html files require this
define_syntax ("<!--", "-->", '%', $1);
define_syntax ("A-Za-z&", 'w', $1);
define_syntax ('#', '#', $1);

#ifdef HAS_DFA_SYNTAX
% The highlighting copes with comments, "&eth;" type things, and <argh> type
% HTML tags. An unrecognised &..; construct or an incomplete <...> construct
% is flagged in delimiter colour.
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
   dfa_enable_highlight_cache ("html.dfa", name);
   dfa_define_highlight_rule ("<!.*-[ \t]*>", "Qcomment", name);
   dfa_define_highlight_rule ("^([^\\-]|-+[^>])*-+[ \t]*>", "Qcomment", name);
   dfa_define_highlight_rule ("<!.*", "comment", name);
   dfa_define_highlight_rule ("<([^>\"]|\"[^\"]*\")*>", "keyword", name);
   dfa_define_highlight_rule ("<([^>\"]|\"[^\"]*\")*(\"[^\"]*)?$", "delimiter", name);
   dfa_define_highlight_rule ("&#[0-9]+;", "keyword1", name);
   dfa_define_highlight_rule ("&[A-Za-z]+;", "Kdelimiter", name);
   dfa_define_highlight_rule (".", "normal", name);
   dfa_build_highlight_table (name);
}
dfa_set_init_callback (&setup_dfa_callback, "html");
%%% DFA_CACHE_END %%%
#endif

() = define_keywords ($1, "&gt&lt", 3);
() = define_keywords ($1, "&ETH&amp&eth", 4);
() = define_keywords ($1, strcat (
				  "&Auml&Euml&Iuml&Ouml&Uuml",
				  "&auml&euml&iuml&nbsp&ouml&quot&uuml&yuml"
				  ), 
		      5);

() = define_keywords ($1, strcat (
				  "&AElig&Acirc&Aring&Ecirc&Icirc&Ocirc&THORN&Ucirc&acirc",
				  "&aelig&aring&ecirc&icirc&ocirc&szlig&thorn&ucirc"
				  ), 
		      6);

() = define_keywords ($1, strcat (
				  "&Aacute&Agrave&Atilde&Ccedil&Eacute&Egrave&Iacute&Igrave",
				  "&Ntilde&Oacute&Ograve&Oslash&Otilde&Uacute&Ugrave&Yacute",
				  "&aacute&agrave&atilde&ccedil&eacute&egrave&iacute&igrave",
				  "&ntilde&oacute&ograve&oslash&otilde&uacute&ugrave&yacute"),
		      7);

%!%+
%\function{html_mode}
%\synopsis{html_mode}
%\usage{Void html_mode ();}
%\description
% \var{html_mode} is a mode designed for editing HTML files.  
% If a region is defined (i.e., if a mark is set), many HTML
% tags will insert around the region, e.g. '<B>' and '</B>'.
% 
% Keybindings begin with ^C and are grouped according to function:
%     ^CA...  Anchors (<A>...</A>)
%     ^CD...  Definition lists (<DL>...</DL>)
%     ^CF...  Forms (<form>...</form>)
%     ^CH...  Headings, document type, etc.
%     ^CI...  Images
%     ^CL...  Lists (<UL>...</UL>)
%     ^CP...  Paragraph styles, etc. (<P>, <BR>, <HR>, <ADDRESS>, etc.)
%     ^CS...  Character styles (<EM>, <STRONG>, <B>, <I>, etc.)
%     ^CT...  Tables
% Additionally, some special movement commands and miscellaneous
% characters are defined:
%     ^C^B    skip to beginning of prior HTML tag
%     ^C^F    skip to end of next HTML tag
%     ^C^N    mark next HTML tag from '<' to '>'
%     ^C^P    mark prior HTML tag from '<' to '>'
%     ^C&     insert HTML text for '&'
%     ^C>     insert HTML text for '>'
%     ^C<     insert HMTL text for '<'
%     ^C<enter> insert HMTL text for '<br>'
%     ^C<space> insert HMTL text for NonBreakableSPace
%     ^Ce     insert HMTL text for 'Eurosymbol'
%     ^CC     insert HTML comment (around region, if marked)
% 
% For a complete list of keybindings, use \var{describe_bindings}.
% 
% This function calls \var{html_mode_hook} if it exists.
%!%-
define html_mode ()
{
   variable html = "html";
   no_mode ();			       %  reset
   set_mode(html, 1);
   set_buffer_hook ("par_sep", "html_paragraph_separator");
   use_syntax_table (html);
   use_keymap (html);
   run_mode_hooks ("html_mode_hook");
}
