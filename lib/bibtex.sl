% BIBTeX mode for JED
% 
% Version: 1.9
% Original Author:  Carsten Tinggaard Nielsen, tinggard@iesd.auc.dk
% Update: 2001-04-24
% -----------------------------------------------------------------------
% History:
% 1.9 24 Apr 2001
%   Added user defined default values for fields.
% 1.8 16 Oct 1999
%   removed local_setkey statements from bibtex_mode function.
%   Added '&' character to mode menu. --JED
% 1.7 15 October 1999
%   * fixed bug where OPT would be removed from e.g. ".. optical.." in title 
%     with bibtex_removeOPT and consequently bibtex_clean_entry
%   * JED recommended self_insert_cmd thingy for keypress " to normal "
%   * more highlight keywords
%   Charl P. Botha <cpbotha@ieee.org>
% 1.6 30 September 1999
%   added syntax table
%   Charl P. Botha <cpbotha@ieee.org>
% 1.5 26 September 1999
%   added mode menu for bibtex mode
%   Charl P. Botha <cpbotha@ieee.org>
% 1.4 29 May 1995
%   fixed bug refering to bibtex_insert_quote: using tex_insert_quote
%   added usage of LaTeX font commands: ^C^F 
%     as suggested by Franz-Josef Knelangen
% 1.3 16 May 1995
%   next/prev field can now position correct in "" and "{}"
%   changed def of next entry to ESC N / Meta N
%   changed def of prev entry to ESC P / Meta P
%     ESC p and ESC P does the same job now.
% 1.2 11 May 1995
%   from fjk@ruf.uni-freiburg.de (Franz-Josef Knelangen):
%     next field ^n
%     prev field ^p
%     next entry ESC n
%     prev entry ESC p
% 1.1 10 May 1995
%   Optimized code after suggestions by John E. Davis
% 1.0 08 May 1995 
%   Public release to comp.editors,comp.text.tex,alt.lang.s-lang
% -----------------------------------------------------------------------
% 
% When bibtex mode is loaded, 'bibtex_mode_hook' is called.
% This hook will allow users to customize the mode.
% So, in your jed.rc /.jedrc file,
% add something like:
%   define bibtex_mode_hook () {
%      local_setkey ("bibtex_Article", "^C^E^A");
%   }
% which binds the function to Ctrl-C Ctrl-E Ctrl-A
% 
% For customization, the 'bibtex_item_hook' is called everytime a
% template is entered into the buffer.
% To add your own entry, 
% then add something like (in your jed.rc /.jedrc file):
%   autoload ("bib_field", "bibtex.sl");
%   define bibtex_item_hook () {
%      bib_field("myentry");
%   }
% Your additions will be placed before the note/annote field.
% The variable bibtex_item_name holds current name like Article, Manual etc.

%-------------------------------------------------------------------------
% Load the common definitions if not already loaded.  This also defines
% the TeX-Mode syntax table
require ("latex");

%
% For customization, the 'bib_field_default_hook' is called everytime a
% new field is inserted. It takes the name of the field and returns the
% default value (or the empty string). Simply add to your jed.rc/.jedrc:
%    define bibtex_field_default_hook (fieldstr) {
%       switch (fieldstr)
%         {case "isbn"      : "3-934601-";}
%         {case "location"  : "Jena";}
%         {case "publisher" : "IKS Garamond";}
%         {case "address"   : "Wildenbruchstr. 15, 07745 Jena, Germany";}
%         {"";}
%    }
%

%-------------------------------------------------------------------------
variable bib_OPT_flag = 0;  % if true then insert OPT in the field names
variable bib_indent = 16;   % amount of whitespace before '='
variable bibtex_item_name = Null_String; % name of current item

define bib_set_OPT () { bib_OPT_flag = 1; }
define bib_unset_OPT () { bib_OPT_flag = 0; }

define bib_field (fieldstr)  {
   variable sl;
   sl = bib_indent - strlen(fieldstr);
   insert(",\n  ");
   if (bib_OPT_flag) {
      insert("OPT");
      sl -= 3;
   }
   vinsert ("%s =", fieldstr);
   insert_spaces(sl);
   
   sl = "";
   variable hook = __get_reference ("bibtex_field_default_hook");
   if (hook != NULL)
     sl = @hook (fieldstr);

   if (fieldstr == "title")
     vinsert("\"{%s}\"", sl);
   else
     vinsert("\"%s\"", sl);
}

define bib_journal () { bib_field("journal"); }
define bib_year () { bib_field("year"); }
define bib_volume () { bib_field("volume"); }
define bib_number () { bib_field("number"); }
define bib_pages () { bib_field("pages"); }
define bib_month () { bib_field("month"); }
define bib_editor () { bib_field("editor"); }
define bib_publisher () { bib_field("publisher"); }
define bib_series () { bib_field("series"); }
define bib_address () { bib_field("address"); }
define bib_edition () { bib_field("edition"); }
define bib_howpublished () { bib_field("howpublished"); }
define bib_booktitle () { bib_field("booktitle"); }
define bib_organization () { bib_field("organization"); }
define bib_institution () { bib_field("institution"); }
define bib_school () { bib_field("school"); }
define bib_type () { bib_field("type"); }
define bib_chapter () { bib_field("chapter"); }
define bib_note () { bib_field("note"); }
define bib_author () { bib_field("author"); }

define bib_item (itemstr, use_author)  {
   vinsert("@%s{", itemstr);
   bibtex_item_name = itemstr;
   bib_unset_OPT();
   if (use_author) { bib_author();}
   bib_field("title");
}

define bib_itemend (use_note)  {
   bib_field("location");
   bib_field("isbn");
   bib_field("keywords");
   run_mode_hooks("bibtex_item_hook");
   if (use_note) { bib_note(); }
   bib_field("annote");
   insert("\n}\n\n");
   pop(bsearch("{,"));
   go_right_1 ();
}

define bib_cref_and_key() {
   bib_set_OPT();
   bib_field("crossref");
   bib_field("key");
}

define bibtex_Article ()  {
   bib_item("Article", 1);
   bib_journal();
   bib_year();
   bib_cref_and_key();
   bib_volume();
   bib_number();
   bib_pages();
   bib_month();
   bib_itemend(1);
}

define bibtex_Book ()  {
   bib_item("Book", 1);
   bib_publisher();
   bib_year();
   bib_cref_and_key();
   bib_editor();
   bib_volume();
   bib_number();
   bib_series();
   bib_address();
   bib_edition();
   bib_month();
   bib_itemend(1);
}

define bibtex_preamble () {
   insert("@Preamble\{\}\n");
   go_left(2);
}

define bibtex_string () {
   insert("@string\{ = \"\"\}\n");
   go_left(7);
}

define bibtex_Unpublished () {
   bib_item("Unpublished", 1);
   bib_note();
   bib_cref_and_key();
   bib_year();
   bib_month();
   bib_itemend(0);
}

define bibtex_TechReport () {
   bib_item("TechReport", 1);
   bib_institution();
   bib_year();
   bib_cref_and_key();
   bib_type();
   bib_number();
   bib_address();
   bib_month();
   bib_itemend(1);
}

define bibtex_PhdThesis () {
   bib_item("PhdThesis", 1);
   bib_school();
   bib_year();
   bib_cref_and_key();
   bib_address();
   bib_month();
   bib_type();
   bib_itemend(1);
}

define bibtex_Proceedings () {
   bib_item("Proceedings", 0);
   bib_year();
   bib_cref_and_key();
   bib_editor();
   bib_volume();
   bib_series();
   bib_publisher();
   bib_organization();
   bib_address();
   bib_month();
   bib_itemend(1);
}

define bibtex_Misc () {
   bib_item("Misc", 0);
   bib_cref_and_key();
   bib_author();
   bib_howpublished();
   bib_year();
   bib_month();
   bib_itemend(1);
}

define bibtex_MastersThesis () {
   bib_item("MastersThesis", 1);
   bib_school();
   bib_year();
   bib_cref_and_key();
   bib_address();
   bib_month();
   bib_type();
   bib_itemend(1);
}

define bibtex_Manual () {
   bib_item("Manual", 0);
   bib_cref_and_key();
   bib_author();
   bib_organization();
   bib_address();
   bib_edition();
   bib_year();
   bib_month();
   bib_itemend(1);
}

define bibtex_InProceedings () {
   bib_item("InProceedings", 1);
   bib_cref_and_key();
   bib_editor();
   bib_volume();
   bib_number();
   bib_series();
   bib_pages();
   bib_booktitle();
   bib_year();
   bib_organization();
   bib_publisher();
   bib_address();
   bib_month();
   bib_itemend(1);
}

define bibtex_InCollection () {
   bib_item("InCollection", 1);
   bib_cref_and_key();
   bib_booktitle();
   bib_publisher();
   bib_year();
   bib_editor();
   bib_volume();
   bib_number();
   bib_series();
   bib_type();
   bib_chapter();
   bib_pages();
   bib_address();
   bib_edition();
   bib_month();
   bib_itemend(1);
}

define bibtex_InBook () {
   bib_item("InBook", 1);
   bib_chapter();
   bib_cref_and_key();
   bib_publisher();
   bib_year();
   bib_editor();
   bib_pages();
   bib_volume();
   bib_number();
   bib_series();
   bib_address();
   bib_edition();
   bib_type();
   bib_month();
   bib_itemend(1);
}

define SearchInThisLine(str) {
   % return 1 if str is found at current line
   variable thisline = what_line();
   variable res = 0;
   
   bol();   
   if (fsearch(str))
     if (thisline == what_line())
       res = 1;
   !if (res) {
      goto_line(thisline);
      bol();
   }
   return res;
}

variable bibtex_remove_value = 0; % 0:nothing removed 1:OPT 2:killed

define bibtex_removeOPT () {
   % remove the string OPT from the current line
   % if there is no text in the entry then the line is deleted
   
   % ensure beginning of current line
   bol();
   bibtex_remove_value = 0;
   % if current line has OPT then go on
   if (SearchInThisLine(" OPT")) {
      % first make sure that the OPT is before the = !
      % get current column
      variable thiscolumn = what_column();
      % get position of equal sign
      variable r = SearchInThisLine(" =");
      variable esigncolumn = what_column();
      % if there is an equal sign and the OPT is before the =, we can nuke it
      if (r and (thiscolumn < esigncolumn)) {
	 % go back to where we were just after the initial OPT search
	 go_left(esigncolumn - thiscolumn);
	 % replace OPT with spaces after =
	 go_right_1 ();
	 deln(3);
	 if (SearchInThisLine(" =")) {
	    go_right(2);
	    insert_spaces(3);
	 }
	 bibtex_remove_value = 1;
	 % if the line has no information then kill it
	 if (SearchInThisLine("\"\"")) {
	    delete_line();
	    bibtex_remove_value = 2;
	 }
      }
   }
}

define bibtex_clean_entry ()  {
% remove lines where field starts with OPT and ends with ""
% remove lines if the contains no information
% if lines has OPT and "<anything>" then OPT is removed and
% line is adjusted accordingly
% the stopmark is "}" at the first position in a line or eobp().
   () = bsearch_char ('@');
   go_down_1();
   while (looking_at_char('}') == 0) {
      bibtex_removeOPT();
      if (bibtex_remove_value == 1)
	go_down(1); % removed OPT in line with content
      !if (bibtex_remove_value) {
	 % did nothing, check for empty line
	 if (SearchInThisLine("\"\""))
	   delete_line();
	 else
	   go_down_1();
      }
      bol();
   } 
   % there must not be a comma after the last entry
   go_left(2);
   if (looking_at_char(','))
     del();
   bol();
   go_down(3);
}

define bibtex_no(whatdir, whatstr) 
{
   vmessage ("There is no %s %s", whatdir, whatstr);
}

define bibtex_no_next(whatstr) { bibtex_no("next", whatstr); }

define bibtex_no_prev(whatstr) { bibtex_no("previous", whatstr); }

define bibtex_next_entry() {
   % jump to the next entry: Article, Manual
   if (fsearch_char ('@')) { % first char of entry
      () = fsearch_char ('{'); % point at label
      go_right_1 ();
   }
   else
     bibtex_no_next("entry");
}

define bibtex_prev_entry() {
   % jump to the previous entry: Article, Manual
   if (bsearch_char ('}')){      % last char of entry
      () = bsearch_char ('@');   % first char of entry
      () = fsearch_char ('{');   % point at label
      go_right_1 ();
   }
   else
     bibtex_no_prev("entry");
}

define bibtex_go_into_field() {
   % A std. field is of the form ""
   % the title field is of the form "{}"
   % if the field is of the title form "{}" the
   % posiiton the cursor between the braces.
   go_right(2);
   if (looking_at_char ('{'))
     go_right_1 ();
}

define bibtex_next_field() {
   % jump to the next field: author, title
   if (fsearch(" \""))
     bibtex_go_into_field();
   else
     bibtex_no_next("field");
}

define bibtex_prev_field() {
   % jump to the previous field: author,title
   bol();
   if (bsearch(" \""))
     bibtex_go_into_field();
   else
     bibtex_no_prev("field");
}

private variable bibtexName = "BibTeX";
private variable bibtexModeName = bibtexName + "-Mode";

!if (keymap_p(bibtexModeName))
{
   make_keymap (bibtexModeName);
   definekey ("tex_insert_quote", "\"", bibtexModeName);
   definekey ("tex_insert_quote", "'", bibtexModeName);
   definekey ("tex_blink_dollar", "$", bibtexModeName);

   % The next two conflict with other, non-emacs, bindings --JED
   %definekey ("bibtex_next_field", "^N",	bibtexModeName);
   %definekey ("bibtex_prev_field", "^P",	bibtexModeName);

   definekey ("bibtex_next_entry", "\eN",	bibtexModeName);
   definekey ("bibtex_prev_entry", "\eP",	bibtexModeName);
   definekey("self_insert_cmd", "\"", bibtexModeName);

   % These are the keys that have a ^C prefix
   definekey_reserved ("tex_font", "^F",	bibtexModeName);
   definekey_reserved ("bibtex_Article", "a",	bibtexModeName);
   definekey_reserved ("bibtex_Book", "b",	bibtexModeName);
   definekey_reserved ("bibtex_Unpublished", "u",	bibtexModeName);
   definekey_reserved ("bibtex_string", "s",	bibtexModeName);
   definekey_reserved ("bibtex_TechReport", "t",	bibtexModeName);
   definekey_reserved ("bibtex_PhdThesis", "T",	bibtexModeName);
   definekey_reserved ("bibtex_preamble", "P",	bibtexModeName);
   definekey_reserved ("bibtex_Proceedings", "p",	bibtexModeName);
   definekey_reserved ("bibtex_Misc", "M",	bibtexModeName);
   definekey_reserved ("bibtex_MastersThesis", "m",	bibtexModeName);
   definekey_reserved ("bibtex_Manual", "^M",	bibtexModeName);
   definekey_reserved ("bibtex_InProceedings", "I",	bibtexModeName);
   definekey_reserved ("bibtex_InCollection", "i",	bibtexModeName);
   definekey_reserved ("bibtex_InBook", "B",	bibtexModeName);
   definekey_reserved ("bibtex_removeOPT", "^O",	bibtexModeName);
   definekey_reserved ("bibtex_clean_entry", "^C",	bibtexModeName);
}

% syntax table and keywords for bibtex stuff added by <cpbotha@ieee.org>
create_syntax_table (bibtexName);
% comments
define_syntax ("%", "", '%', bibtexName);
% matching set of delimiters (for blinking)
define_syntax ("{", "}", '(', bibtexName);
% string (key values)
define_syntax ('"', '"', bibtexName);
% keywords
define_syntax ("0-9a-zA-Z_@", 'w', bibtexName);
% delimiter
define_syntax (",", ',', bibtexName);
% operator
define_syntax ("=", '+', bibtexName);
% keywords are case-insensitive
set_syntax_flags (bibtexName, 1);
() = define_keywords_n (bibtexName, "key", 3, 0);
() = define_keywords_n (bibtexName, "notetypeyear", 4, 0);
() = define_keywords_n (bibtexName, "monthpagestitle", 5, 0);
() = define_keywords_n (bibtexName, "authoreditornumberschoolseriesvolume", 6, 0);
() = define_keywords_n (bibtexName, "addresschaptereditionjournal", 7, 0);
() = define_keywords_n (bibtexName, "crossref", 8, 0);
() = define_keywords_n (bibtexName, "booktitlepublisher", 9, 0);
() = define_keywords_n (bibtexName, "institution", 11, 0);
() = define_keywords_n (bibtexName, "howpublishedorganization", 12, 0);
% now for the entry types
() = define_keywords_n (bibtexName, "@book@misc", 5, 1);
() = define_keywords_n (bibtexName, "@inbook@manual@string", 7, 1);
() = define_keywords_n (bibtexName, "@article", 8, 1);
() = define_keywords_n (bibtexName, "@preamble", 9, 1);
() = define_keywords_n (bibtexName, "@phdthesis", 10, 1);
() = define_keywords_n (bibtexName, "@techreport", 11, 1);
() = define_keywords_n (bibtexName, "@proceedings@unpublished", 12, 1);
() = define_keywords_n (bibtexName, "@incollection", 13, 1);
() = define_keywords_n (bibtexName, "@inproceedings@mastersthesis", 14, 1);

private define init_menu (menu) 
{
   menu_append_item (menu, "&Article", "bibtex_Article");
   menu_append_item (menu, "&Book", "bibtex_Book");
   menu_append_item (menu, "&Unpublished", "bibtex_Unpublished");
   menu_append_item (menu, "&TechReport", "bibtex_TechReport");
   menu_append_item (menu, "&PhdThesis", "bibtex_PhdThesis");
   menu_append_item (menu, "P&roceedings", "bibtex_Proceedings");
   menu_append_item (menu, "&Misc", "bibtex_Misc");
   menu_append_item (menu, "Ma&stersThesis", "bibtex_MastersThesis");
   menu_append_item (menu, "Ma&nual", "bibtex_Manual");
   menu_append_item (menu, "&InProceedings", "bibtex_InProceedings");
   menu_append_item (menu, "In&Collection", "bibtex_InCollection");
   menu_append_item (menu, "InBoo&k", "bibtex_InBook");
   menu_append_separator (menu);
   menu_append_item (menu, "Next &Field", "bibtex_next_field");
   menu_append_item (menu, "Pre&vious Field", "bibtex_prev_field");
   menu_append_item (menu, "Next &Entry", "bibtex_next_entry");
   menu_append_item (menu, "Previo&us Entry", "bibtex_prev_entry");
   menu_append_separator (menu);
   menu_append_item (menu, "Rem&ove OPT", "bibtex_removeOPT");
   menu_append_item (menu, "C&lean entry", "bibtex_clean_entry");
}


define bibtex_mode ()
{
   use_keymap (bibtexModeName);
   set_mode (bibtexName, 0x1 | 0x20);
   set_buffer_hook ("par_sep", "tex_paragraph_separator");
   set_buffer_hook ("wrap_hook", "tex_wrap_hook");
   TAB = 0; % pure spaces in this mode

   mode_set_mode_info (bibtexName, "init_mode_menu", &init_menu);
   use_syntax_table(bibtexName);

   run_mode_hooks ("bibtex_mode_hook");
}
%-----------------------------------------------------------%

define bibtex_info_find_node ()
{
   variable node;
   
   node = read_mini ("Node:", Null_String, Null_String);
   !if (strlen (node)) return;
   info_reader ();
   info_find_node ("(bibtex)top");
   info_find_node ("(bibtex)" + node);
}
