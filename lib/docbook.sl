% File:          docbook.sl      -*- SLang -*-
%
% Author:        Guido Gonzato, <ggonza@tin.it>
% 
% Version:       1.0.0.
% 
% Description:   this mode is designed to facilitate the editing of
%                Docbook SGML files. It supports a large subset of Docbook
%                3.1, as described in the LDP Author Guide 
%                <http://www.linuxdoc.org/LDP/LDP-Author-Guide/> and
%                "Docbook: The Definive Guide" <http://www.docbook.org/tdg/>
%                Complete enough for writing Linux HOWTOs, manuals, and much
%                more...
%                For Jed 0.99.13+.
%
% Installation:  if opening a document with .sgml extension doesn't toggle
%                docbook mode on, then insert these lines in your .jedrc:
%
%                  autoload ("docbook_mode", "docbook");
%                  add_mode_for_extension ("docbook", "sgml");
%                     
% Last updated:	 24 May 2001

WRAP_INDENTS = 1; % you really want this

custom_variable ("SGML_INDENT", 2);
custom_variable ("Sgml_Compile_PS_Cmd", "db2ps");
custom_variable ("Sgml_Compile_Pdf_Cmd", "db2pdf");
custom_variable ("Sgml_Compile_Html_Cmd", "db2html");
custom_variable ("Sgml_View_PS_Cmd", "gv");
custom_variable ("Sgml_View_Pdf_Cmd", "gv");
custom_variable ("Sgml_View_Html_Cmd", "netscape");

private variable NO_PUSH_SPOT = 0;
private variable PUSH_SPOT    = 1;
private variable NO_POP_SPOT  = 0;
private variable POP_SPOT     = 1;
% buffer-local variables
variable DOC_TYPE  = "DOC_TYPE";  % "a" = article, "b" = book
variable SECT_TYPE = "SECT_TYPE"; % "#" = sect#, "s" = section

% ----- %

!if (is_defined ("tex_ldots")) % read tex_insert_quote
  () = evalfile ("texcom");

private define sgml_insert_pair_around_region (left, right)
{
  check_region (1);
  exchange_point_and_mark ();
  insert (left);
  exchange_point_and_mark ();
  insert (right);
  pop_spot ();
  pop_mark_0 ();
}

define sgml_skip_tag ()		% ^C^F
{
  go_right_1 ();
  () = fsearch_char ('<');
}

define sgml_bskip_tag ()	% ^C^B
{
  () = bsearch_char ('<');
}  

private define sgml_insert_tags (tag1, tag2, do_push_spot, do_pop_spot)
{
  % if a region is defined, insert the tags around it
  if (markp () ) {
    sgml_insert_pair_around_region (tag1, tag2);
    return;
  }
  insert (tag1);
  if (do_push_spot)
    push_spot ();
  insert (tag2);
  if (do_pop_spot)
    pop_spot ();
}

define sgml_insert_tag (tag, do_push_spot, do_pop_spot)
{
  variable tag1, tag2;
  tag1 = sprintf ("<%s>", tag);
  tag2 = sprintf ("</%s>", tag);
  sgml_insert_tags (tag1, tag2, do_push_spot, do_pop_spot);
}

private define sgml_insert_tag_with_newline (tag, do_push_spot, do_pop_spot)
{
  variable tag1, tag2;
  tag1 = sprintf ("<%s>", tag);
  tag2 = sprintf ("</%s>\n", tag);
  sgml_insert_tags (tag1, tag2, do_push_spot, do_pop_spot);
}

private define sgml_paragraph_separator ()
{
  variable cs = CASE_SEARCH;
  bol_skip_white ();
  CASE_SEARCH = 0;
  eolp () or ffind ("<para>") or ffind ("</para>");
  CASE_SEARCH = cs;
}

% let's start

% Section stuff

define sgml_para (do_push_spot) % ^CP
{
  variable col = what_column () - 1;
  if (markp () ) {
    sgml_insert_pair_around_region ("<para>\n", "</para>\n");
    return;
  }
  insert ("<para>\n");
  insert_spaces (col + SGML_INDENT);
  if (do_push_spot)
    push_spot ();
  insert ("\n");
  insert_spaces (col);
  insert ("</para>");
}

define sgml_title (tag, do_push_spot)
{
  sgml_insert_tag_with_newline (tag, do_push_spot, NO_POP_SPOT);
}

private define sgml_section_skel (what_tag, do_push_spot)
{
  variable col = what_column () - 1;
  vinsert ("<%s>\n", what_tag);
  insert_spaces (col);
  sgml_title ("title", do_push_spot);
  insert ("\n");
  insert_spaces (col);
  sgml_para (NO_PUSH_SPOT);
  insert ("\n\n");
  insert_spaces (col);
  vinsert ("</%s>", what_tag);
  if (do_push_spot)
    pop_spot ();
}

define sgml_chapter (do_push_spot)
{
  sgml_section_skel ("chapter", do_push_spot);
}

define sgml_sidebar (do_push_spot)
{
  sgml_section_skel ("sidebar", do_push_spot);
}

define sgml_appendix (do_push_spot)
{
  sgml_section_skel ("appendix", do_push_spot);
}

define sgml_epigraph ()
{
  variable col = what_column () - 1;
  insert ("<epigraph>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<attribution>");
  push_spot ();
  insert ("</attribution>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_para (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</epigraph>\n");
  pop_spot ();
}

define sgml_section () % ^CS
{
  variable col = what_column () - 1;
  if ("#" == get_blocal_var (SECT_TYPE)) {
    beep ();
    error ("`section' not allowed in this environment!");
    % !!! what about <section> and <sectX> in Book doctype?
    return ();
  }
  set_blocal_var ("s", SECT_TYPE);
  sgml_section_skel ("section", PUSH_SPOT);
}

define sgml_simple_section ()
{
  sgml_section_skel ("simplesect", PUSH_SPOT);
}

define sgml_sect (do_push_spot, level_str) % ^CN
{
  variable level_int;
  variable ok = 0;

  if ( ("s" == get_blocal_var (SECT_TYPE) ) and
      ("a" == get_blocal_var (DOC_TYPE) ) ) {
    beep ();
    error ("`sect' not allowed in this environment!");
    return ();
  }
  set_blocal_var ("#", SECT_TYPE);
  !if (strcmp ("0", level_str) )
    while (0 == ok) {
      level_str = read_mini ("Section level (1..5)?", Null_String, "1");
      level_int = level_str [0] - '0';
      if ( (level_int > 0) and (level_int < 6) )
        ok = 1;
      !if (ok) {
        beep ();
        message ("Wrong value! Only 1..5 allowed. ");
      }
    }
  
  insert (sprintf ("<sect%s id=\"", level_str));
  if (do_push_spot)
    push_spot ();
  insert ("\">\n");
  sgml_title ("title", NO_PUSH_SPOT);
  insert ("\n");
  sgml_para (NO_PUSH_SPOT);
  insert (sprintf ("\n\n\n</sect%s>", level_str));
  if (do_push_spot)
    pop_spot ();
}

define sgml_formalpara ()
{
  sgml_section_skel ("formalpara", PUSH_SPOT);
}

% Header stuff

define sgml_revision ()
{
  variable col = what_column () - 1;
  insert ("<revision>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<revnumber>");
  push_spot (); % pop_spot () later
  insert ("</revnumber>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("date", NO_PUSH_SPOT, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("authorinitial", NO_PUSH_SPOT, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  insert ("<revmark>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</revmark>\n");
  insert_spaces (col);
  insert ("</revision>\n");
}

define sgml_revision_history ()
{
  variable col = what_column () - 1;
  insert ("<revhistory>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_revision (); % the spot is pushed
  insert_spaces (col);
  insert ("</revhistory>");
  pop_spot ();
}

% Author stuff

private define sgml_firstname (do_push_spot, do_pop_spot)
{
  sgml_insert_tag_with_newline ("firstname", do_push_spot, do_pop_spot);
}

private define sgml_surname (do_push_spot, do_pop_spot)
{
  sgml_insert_tag_with_newline ("surname", do_push_spot, do_pop_spot);
}

private define sgml_othername (do_push_spot, do_pop_spot)
{
  sgml_insert_tag_with_newline ("othername", do_push_spot, do_pop_spot);
}

private define sgml_affiliation (do_push_spot, do_pop_spot)
{
  variable col = what_column () - 1;
  insert ("<affiliation>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("shortaffil", do_push_spot, do_pop_spot);
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("jobtitle", do_push_spot, do_pop_spot);
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("orgname", do_push_spot, do_pop_spot);
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("orgdiv", do_push_spot, do_pop_spot);
  insert_spaces (col);
  insert ("</affiliation>\n");
}

private define sgml_honorific (do_push_spot, do_pop_spot)
{
  sgml_insert_tag_with_newline ("honorific", do_push_spot, do_pop_spot);
}

define sgml_author (do_push_spot, do_pop_spot)
{
  variable col = what_column () - 1;
  insert ("<author>\n");
  if (do_push_spot)
    push_spot ();
  insert_spaces (col + SGML_INDENT);
  sgml_honorific (do_push_spot, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_firstname (do_push_spot, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_othername (do_push_spot, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_surname (do_push_spot, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_affiliation (do_push_spot, NO_POP_SPOT);
  insert_spaces (col);
  insert ("</author>\n");
  if (do_pop_spot)
    pop_spot ();
}

define sgml_template ()
{
  variable col, ok, type, is_article = 0;

  col = what_column () - 1;
  ok = 0;
  while (0 == ok) {
    type = 
      read_mini ("Document type (Article, Book)?", Null_String, "a");
    if ( (type [0] == 'a') or (type [0] == 'b') )
      ok = 1;
    !if (ok) {
      beep ();
      message ("Wrong! 'a' or 'b'! ");
    }
  }
  if (type [0] == 'a') {
    set_blocal_var ("a", DOC_TYPE);
    type = "article";
    is_article = 1;
  }
  else {
    set_blocal_var ("b", DOC_TYPE);
    type = "book";
  }
  vinsert 
    ("<!DOCTYPE %s PUBLIC \"-//OASIS//DTD DocBook V3.1//EN\">", type);
  if (is_article)
    vinsert ("\n\n<%s lang=\"en\">\n<%s>\n\n", type, "artheader");
  else
    vinsert ("\n\n<%s lang=\"en\">\n<%s>\n\n", type, "bookinfo");
  
  insert_spaces (col + SGML_INDENT);
  sgml_title ("title", PUSH_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("subtitle", NO_PUSH_SPOT, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_author (NO_PUSH_SPOT, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  insert ("<address>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  sgml_insert_tag_with_newline ("email", NO_PUSH_SPOT, NO_POP_SPOT);
  insert_spaces (col + SGML_INDENT);
  insert ("</address>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_insert_tag_with_newline ("pubdate", NO_PUSH_SPOT, NO_POP_SPOT);
  insert ("\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<abstract>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  sgml_para (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</abstract>\n");
  
  if (is_article) {
    vinsert ("\n</%s>\n\n", "artheader");
    insert ("<!-- sections here... -->\n");
  }
  else {
    vinsert ("\n</%s>\n\n\n", "bookinfo");
    insert ("<preface>\n<title></title>\n\n");
    sgml_para (NO_PUSH_SPOT);
    insert ("\n</preface>\n\n");
    sgml_chapter (NO_PUSH_SPOT);
  }
  vinsert ("\n\n\n</%s>", type);
  pop_spot ();
}

define sgml_copyright ()
{
  variable col = what_column () - 1;
  insert_spaces (col);
  insert ("<copyright>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<year>");
  push_spot ();
  insert ("</year>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<holder></holder>\n");
  insert_spaces (col);
  insert ("</copyright>\n");
  pop_spot ();
}

% character styles

define sgml_directory ()
{
  insert ("<filename id=\"directory\">");
  push_spot ();
  insert ("</filename>");
  pop_spot ();
}

% keys

define sgml_keycombo (do_push_spot)
{
  insert ("<keycombo><keycap>");
  if (do_push_spot)
    push_spot ();
  insert ("</keycap><keycap></keycap></keycombo>");
  if (do_push_spot)
    pop_spot ();
}

% gui/menus

define sgml_menuchoice ()
{
  variable col = what_column () - 1;
  insert ("<menuchoice>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<shortcut>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  sgml_keycombo (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</shortcut>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<guimenu>");
  push_spot ();
  insert ("</guimenu>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<guimenuitem></guimenuitem>\n");
  insert_spaces (col);
  insert ("</menuchoice>\n");
  pop_spot ();
}

% computer-related

define sgml_arg_plain (do_push_spot, do_pop_spot)
{
  sgml_insert_tag ("arg", do_push_spot, do_pop_spot);
}

define sgml_arg (do_push_spot, do_pop_spot)
{
  variable ch, rep, choice, tag1, tag2, sep1, sep2;
  
  choice = "";
  rep = "";
  sep1 = " "; sep2 = " ";
  
  flush ("Choice:  Req  Plain (Enter=nothing) ");
  ch = tolower (getkey ());
  switch (ch)
    {case 'r': choice =  "choice=req"; }
    {case 'p': choice =  "req=plain"; }

  flush ("Rep:  Repeat (Enter=nothing) ");
  ch = tolower (getkey ());
  switch (ch)
    {case 'r': rep =  "rep=repeat"; }
  
  !if (strlen(choice))
    sep1= "";
  !if (strlen(rep))
    sep2= "";
  tag1 = sprintf ("<arg%s%s%s%s>", sep1, choice, sep2, rep );
  tag2 = "</arg>";
  sgml_insert_tags (tag1, tag2, do_push_spot, do_pop_spot);
}

private define sgml_replaceable (do_push_spot, do_pop_spot)
{
  sgml_insert_tag ("replaceable", do_push_spot, do_pop_spot);
}

define sgml_group ()
{
  variable col = what_column () - 1;
  insert ("<group>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_arg (PUSH_SPOT, NO_POP_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</group>\n");
  pop_spot ();
}

define sgml_cmdsynopsis ()
{
  variable col = what_column () - 1;
  insert ("<cmdsynopsis>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<command>");
  push_spot ();
  insert ("</command>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_arg (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</cmdsynopsis>\n");
  pop_spot ();
}

% environments, sort of

define sgml_blockquote ()
{
  variable col = what_column () - 1;
  insert ("<blockquote>\n");
  insert_spaces (col);
  insert ("<attribution>");
  push_spot ();
  insert ("</attribution>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_para (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</blockquote>");
  pop_spot ();
}

define sgml_env (what)
{
  variable col = what_column () - 1;
  vinsert ("<%s>\n", what);
  insert_spaces (col);
  !if (strcmp ("example", what)) {
    sgml_title ("title", PUSH_SPOT);
    insert_spaces (col + SGML_INDENT);
    sgml_para (NO_PUSH_SPOT);
    insert ("\n");
  }
  else {
    insert_spaces (col + SGML_INDENT);
    sgml_para (PUSH_SPOT);
    insert ("\n");
  }
  insert_spaces (col);
  vinsert ("</%s>\n", what);
  pop_spot ();
}

% links

define sgml_id ()
{
  insert ("id=\"");
  push_spot ();
  insert ("\"");
}

define sgml_anchor ()
{
  insert ("<anchor ");
  sgml_id (); % spot pushed
  insert (">");
  pop_spot ();
}

define sgml_url ()
{
  insert ("<ulink url=\"");
  push_spot ();
  insert ("\"><citetitle></citetitle></ulink>");
  pop_spot ();
}

define sgml_xref ()
{
  insert ("<xref linkend=\"");
  push_spot ();
  insert ("\">");
  pop_spot ();
}

% lists

define sgml_listitem (do_push_spot)
{
  variable col = what_column () - 1;
  insert ("<listitem>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_para (do_push_spot);
  insert ("\n");
  insert_spaces (col);
  insert ("</listitem>");
}

define sgml_itemized_list ()
{
  variable col = what_column () - 1;
  insert ("<itemizedlist>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_listitem (PUSH_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</itemizedlist>\n");
  pop_spot ();
}

define sgml_ordered_list ()
{
  variable col = what_column () - 1;
  insert ("<orderedlist>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<listitem>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  insert ("<para>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  push_spot ();
  insert ("\n");
  insert_spaces (col + 2 * SGML_INDENT);
  insert ("</para>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</listitem>\n");
  insert ("</orderedlist>\n");
  pop_spot ();
}

define sgml_varlistentry ()
{
  variable col = what_column () - 1;
  insert ("<varlistentry>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<term>");
  push_spot (); % we'll do pop_spot () later
  insert ("</term>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<listitem>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  sgml_para (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</listitem>\n");
  insert_spaces (col);
  insert ("</varlistentry>\n");
}

define sgml_variable_list ()
{
  variable col = what_column () - 1;
  insert ("<variablelist>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_varlistentry ();
  insert ("</variablelist>\n");
  pop_spot ();
}

define sgml_seglistitem (do_push_spot)
{
  variable col = what_column () - 1;
  insert ("<seglistitem>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<seg>");
  if (do_push_spot)
    push_spot ();
  insert ("</seg><seg></seg>\n");
  insert_spaces (col);
  insert ("</seglistitem>");
}

define sgml_segmented_list ()
{
  variable col = what_column () - 1;
  insert ("<segmentedlist>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_title ("title", push_spot);
  insert_spaces (col + SGML_INDENT);
  insert ("<segtitle></segtitle>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<segtitle></segtitle>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_seglistitem (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</segmentedlist>\n");
  pop_spot ();
}

define sgml_step (do_push_spot)
{
  variable col = what_column () - 1;
  insert ("<step>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_para (do_push_spot);
  insert ("\n");
  insert_spaces (col);
  insert ("</step>");
  if (1 == do_push_spot)
    pop_spot ();
}

define sgml_procedure ()
{
  variable col = what_column () - 1;
  insert ("<procedure>\n");
  insert_spaces (col + SGML_INDENT);
  sgml_title ("title", PUSH_SPOT);
  insert_spaces (col + SGML_INDENT);
  sgml_step (NO_PUSH_SPOT);
  insert ("\n");
  insert_spaces (col);
  insert ("</procedure>\n");
  pop_spot ();
}

% tables

define sgml_table_row (table_col_int, do_push_spot)
{
  variable i, col = what_column () - 1;
  
  insert ("<row>\n");
  for (i = 0; i < table_col_int; i++) {
    insert_spaces (col + SGML_INDENT);
    insert ("<entry>");
    if ( (do_push_spot) and (0 == i) )
      push_spot ();
    insert ("</entry>\n");
  }
  insert_spaces (col);
  insert ("</row>");
}

define sgml_align ()
{
  variable ch, align;
  
  flush 
    ("Align (Center cHar Justify Left Right)? ");
  ch = tolower (getkey ());
  switch (ch)
    {case 'c': align = "center"; }
    {case 'h': align = "char"; }
    {case 'j': align = "justify"; }
    {case 'l': align = "left"; }
    {case 'r': align = "right"; }
  
  vinsert ("align=\"%s\"", align);

}

% static
private variable table_columns = 4;

define sgml_table (informal_table)
{
  variable col = what_column () - 1;
  variable i, ch, frame, type_of_table, table_col_str, ok;
  
  if (informal_table)
    type_of_table = "informaltable";
  else
    type_of_table = "table";
  
  flush 
    ("Frame (All, Bottom, None, Sides, Top, tOpbot)? ");
  ch = tolower (getkey ());
  switch (ch)
    {case 'a': frame = "all"; }
    {case 'b': frame = "bottom"; }
    {case 'n': frame = "none"; }
    {case 's': frame = "sides"; }
    {case 't': frame = "top"; }
    {case 'o': frame = "topbot"; }
  
  ok = 0;
  while (0 == ok) {
    table_col_str = read_mini ("Columns?", Null_String, "4");
    table_columns = table_col_str [0] - '0';
    if ( (table_columns > 1) and (table_columns < 10) )
      ok = 1;
    !if (ok) {
      beep ();
      message ("Wrong value! ");
    }
  }
  
  vinsert ("<%s frame=\"%s\">\n", type_of_table, frame);
  !if (informal_table)
    sgml_title ("title", PUSH_SPOT);
  else
    push_spot ();
  insert_spaces (col + SGML_INDENT);
  insert (sprintf ("\n<tgroup cols=\"%s\">\n", table_col_str));
  for (i = 0; i < table_columns; i++) {
    insert_spaces (col + SGML_INDENT);
    vinsert 
      ("<colspec colname=\"col%s\" align=\"center\">\n", string (i+1) );
  }
  !if (informal_table) {
    insert_spaces (col + SGML_INDENT);
    insert ("<thead>\n");
    insert_spaces (col + SGML_INDENT);
    
    sgml_table_row (table_columns, NO_PUSH_SPOT);
    
    insert ("\n");
    insert_spaces (col + SGML_INDENT);
    insert ("</thead>\n");
    
    insert_spaces (col + SGML_INDENT);
    insert ("<tfoot>\n");
    insert_spaces (col + SGML_INDENT);
    insert ("<row>\n");
    for (i = 0; i < table_columns; i++) {
      insert_spaces (col + 2 * SGML_INDENT);
      insert ("<entry></entry>\n");
    }
    insert_spaces (col + SGML_INDENT);
    insert ("</row>\n");
    insert_spaces (col + SGML_INDENT);
    insert ("</tfoot>\n");
  }

  insert_spaces (col + SGML_INDENT);
  insert ("<tbody>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<row>\n");
  for (i = 0; i < table_columns; i++) {
    insert_spaces (col + 2 * SGML_INDENT);
    insert ("<entry></entry>\n");
  }
  insert_spaces (col + SGML_INDENT);
  insert ("</row>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</tbody>\n");

  insert_spaces (col + SGML_INDENT);
  insert ("</tgroup>\n");
  insert_spaces (col);
  vinsert ("</%s>\n", type_of_table);
  pop_spot ();
}

% figures

define sgml_figure ()
{
  variable i, col = what_column () - 1;
  insert ("<figure>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<title>");
  push_spot ();
  insert ("</title>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("<mediaobject>\n");
  for (i = 0; i < 2; i++) {
    insert_spaces (col + 2 * SGML_INDENT);
    insert ("<imageobject>\n");
    insert_spaces (col + 3 * SGML_INDENT);
    insert ("<imagedata fileref=\"\" format=\"\">\n");
    insert_spaces (col + 2 * SGML_INDENT);
    insert ("</imageobject>\n");
  }
  insert_spaces (col + 2 * SGML_INDENT);
  insert ("<textobject>\n");
  insert_spaces (col + 3 * SGML_INDENT);
  insert ("<phrase></phrase>\n");
  insert_spaces (col + 2 * SGML_INDENT);
  insert ("</textobject>\n");
  insert_spaces (col + SGML_INDENT);
  insert ("</mediaobject>\n");
  insert_spaces (col);
  insert ("</figure>\n");
  pop_spot ();

}

define insert_sgml_screen ()
{
  % this function is redundant, but I can't find a way to pass
  % Width="80" through menu_append_item
  sgml_insert_tags ("<screen width=\"80\">\n", "</screen>\n", 
		    PUSH_SPOT, NO_POP_SPOT);
}

define sgml_convert_to_ps ()
{
  variable mrk, cmd;
  mrk = create_user_mark;
  cmd = sprintf ("%s %s", Sgml_Compile_PS_Cmd, mrk.buffer_name);
  cmd = read_mini ("Convert command:", Null_String, cmd);
  compile (cmd);
}

define sgml_convert_to_pdf ()
{
  variable mrk, cmd;
  mrk = create_user_mark;
  cmd = sprintf ("%s %s", Sgml_Compile_Pdf_Cmd, mrk.buffer_name);
  cmd = read_mini ("Convert command:", Null_String, cmd);
  compile (cmd);
}

define sgml_convert_to_html ()
{
  variable mrk, cmd;
  mrk = create_user_mark;
  cmd = sprintf ("%s %s", Sgml_Compile_Html_Cmd, mrk.buffer_name);
  cmd = read_mini ("Convert command:", Null_String, cmd);
  compile (cmd);
}

define sgml_view (type)
{
  variable mrk, cmd, tmp;
  mrk = create_user_mark;
  tmp = mrk.buffer_name;
  (tmp, ) = strreplace (tmp, "sgml", type,  -1);
  switch (type)
    { case "ps": cmd = sprintf ("%s %s &", Sgml_View_PS_Cmd, tmp); }
    { case "pdf": cmd = sprintf ("%s %s &", Sgml_View_Pdf_Cmd, tmp); }
    { case "html": cmd = sprintf ("%s %s &", Sgml_View_Html_Cmd, tmp); }
  cmd = read_mini ("View with:", Null_String, cmd);
#ifdef UNIX
  () = system (cmd);
#else
  () = run_shell_cmd (cmd);
#endif
}

% let's finish

% defining keywords is not necessary, since all the highlighting is
% done by the second and third define_syntax (). Rough, but fairly nice.

$1 = "docbook";
create_syntax_table ($1);
define_syntax ("\"([{<", "\")]}>", '(', $1);
define_syntax ('<', '\\', $1);
define_syntax ('&', '\\', $1);
define_syntax ("0-9A-Za-z>/!", 'w', $1);
define_syntax ("<>", '<', $1);
define_syntax ("<!-", "-->", '%', $1);

% copied from html mode.

#ifdef HAS_DFA_SYNTAX
% The highlighting copes with comments, "&eth;" type things, and <argh> type
% HTML tags. An unrecognised &..; construct or an incomplete <...> construct
% is flagged in delimiter colour.
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback (name)
{
  dfa_enable_highlight_cache ("docbook.dfa", name);
  dfa_define_highlight_rule ("<!.*-[ \t]*>", "Qcomment", name);
  dfa_define_highlight_rule ("^([^\\-]|-+[^>])*-+[ \t]*>", "Qcomment", name);
  dfa_define_highlight_rule ("<!.*", "comment", name);
  dfa_define_highlight_rule ("<([^>\"]|\"[^\"]*\")*>", "keyword", name);
  dfa_define_highlight_rule ("<([^>\"]|\"[^\"]*\")*(\"[^\"]*)?$", "delimiter", 
			     name);
  dfa_define_highlight_rule ("&#[0-9]+;", "keyword1", name);
  dfa_define_highlight_rule ("&[A-Za-z]+;", "Kdelimiter", name);
  dfa_define_highlight_rule (".", "normal", name);
  dfa_build_highlight_table (name);
}
dfa_set_init_callback (&setup_dfa_callback, "docbook");
%%% DFA_CACHE_END %%%
#endif

private define init_menu (menu)
{
  % header
  menu_append_popup (menu, "&Header");
 
  variable m = sprintf ("%s.&Header", menu);
  menu_append_item (m, "<&ackno>", "sgml_insert_tag (\"ackno\", 1, 1)");
  menu_append_item (m, "<a&uthor>", "sgml_author (1, 1)");
  menu_append_item (m, "<&copyright>", "sgml_copyright ()");
  menu_append_item (m, "<rev&history>", "sgml_revision_history ()");
  menu_append_item (m, "<&revision>", "sgml_revision (); pop_spot ()");
  menu_append_item (m, "&Template", "sgml_template ()");
  % sections
  menu_append_popup (menu, "&Section");
  m = sprintf ("%s.&Section", menu);
  menu_append_item (m, "<&appendix>", "sgml_appendix (1)");
  menu_append_item (m, "<&chapter>", "sgml_chapter (1)");
  menu_append_item (m, "<&epigraph>", "sgml_epigraph ()");
  menu_append_item (m, "<&formalpara>", "sgml_formalpara ()");
  menu_append_item (m, "<si&mplesection>", "sgml_simple_section ()");
  menu_append_item (m, "&numbered section", "sgml_sect (1, \"0\")");
  menu_append_item (m, "<&section>", "sgml_section ()");
  menu_append_item (m, "<s&idebar>", "sgml_sidebar (1)");
  menu_append_item (m, "<&title>", 
		    "sgml_title (\"title\", 1); pop_spot ()");
  menu_append_item (m, "<s&ubtitle>", 
		    "sgml_title (\"subtitle\", 1); pop_spot ()");
  % character
  menu_append_popup (menu, "&Character");
  m = sprintf ("%s.&Character", menu);
  menu_append_item (m, "<&acronym>", "sgml_insert_tag (\"acronym\", 1, 1)");
  menu_append_item (m, "<&citation>", 
		    "sgml_insert_tag (\"citation\", 1, 1)");
  menu_append_item (m, "<&emphasis>", 
		    "sgml_insert_tag (\"emphasis\", 1, 1)");
  menu_append_item (m, "<&firstterm>", 
		    "sgml_insert_tag (\"firstterm\", 1, 1)");
  menu_append_item (m, "<foot&note>", 
		    "sgml_insert_tag (\"footnote\", 1, 1)");
  menu_append_item (m, "<&superscript>", 
		    "sgml_insert_tag (\"superscript\", 1, 1)");
  menu_append_item (m, "<s&ubscript>", 
		    "sgml_insert_tag (\"subscript\", 1, 1)");
  % computer
  menu_append_popup (menu, "C&omputer");
  m = sprintf ("%s.C&omputer", menu);
  menu_append_item (m, "<&application>", 
		    "sgml_insert_tag (\"application\", 1, 1)");
  menu_append_item (m, "<mouse&button>", 
		    "sgml_insert_tag (\"mousebutton\", 1, 1)");
  menu_append_item (m, "<&command>", "sgml_insert_tag (\"command\", 1, 1)");
  menu_append_item (m, "&directory", "sgml_directory ()");
  menu_append_item (m, "<&envar>", 
		    "sgml_insert_tag (\"envar\", 1, 1)");
  menu_append_item (m, "<&filename>", 
		    "sgml_insert_tag (\"filename\", 1, 1)");
  menu_append_item (m, "<fu&nction>", 
		    "sgml_insert_tag (\"function\", 1, 1)");
  menu_append_item (m, "<program&listing>", 
		    "sgml_insert_tag (\"programlisting\", 1, 1)");
  menu_append_item (m, "<computer&output>", 
		    "sgml_insert_tag (\"computeroutput\", 1, 1)");
  menu_append_item (m, "<cons&tant>", 
		    "sgml_insert_tag (\"constant\", 1, 1)");
  menu_append_item (m, "<&prompt>", "sgml_insert_tag (\"prompt\", 1, 1)");
  menu_append_item (m, "<&screen>", "insert_sgml_screen ()");
  menu_append_item (m, "<&userinput>", 
		    "sgml_insert_tag (\"userinput\", 1, 1)");
  menu_append_item (m, "<&varname>", 
		    "sgml_insert_tag (\"varname\", 1, 1)");
  % computer: popups
  menu_append_popup (m, "&Menu");
  menu_append_popup (m, "&Keys");
  menu_append_popup (m, "CmdS&ynopsis");
  % computer/menu
  m = sprintf ("%s.C&omputer.&Menu", menu);
  menu_append_item (m, "<&accel>", "sgml_insert_tag (\"accel\", 1, 1)");
  menu_append_item (m, "<menu&choice>", "sgml_menuchoice ()");
  menu_append_item (m, "<&guimenu>", "sgml_insert_tag (\"guimenu\", 1, 1)");
  menu_append_item (m, "<guimenu&item>", 
		    "sgml_insert_tag (\"guimenuitem\", 1, 1)");
  % computer/keys
  m = sprintf ("%s.C&omputer.&Keys", menu);
  menu_append_item (m, "<key&cap>", "sgml_insert_tag (\"keycap\", 1, 1)");
  menu_append_item (m, "<key&sym>", "sgml_insert_tag (\"keysym\", 1, 1)");
  menu_append_item (m, "<keyc&ode>", "sgml_insert_tag (\"keycode\", 1, 1)");
  menu_append_item (m, "<keyco&mbo>", "sgml_keycombo (1)");
  % computer/cmdsynopsis
  m = sprintf ("%s.C&omputer.CmdS&ynopsis", menu);
  menu_append_item (m, "<&arg>", "sgml_arg (1, 1)");
  menu_append_item (m, "<&break>", "insert (\"<sbr>\\n\")");
  menu_append_item (m, "<cmd&synopsis>", "sgml_cmdsynopsis ()");
  menu_append_item (m, "&plain <arg>", "sgml_arg_plain (1, 1)");
  menu_append_item (m, "<&group>", "sgml_group ()");
  menu_append_item (m, "<&replaceable>", 
		    "sgml_insert_tag (\"replaceable\", 1, 1)");
  % environment
  menu_append_popup (menu, "&Environment");
  m = sprintf ("%s.&Environment", menu);
  menu_append_item (m, "<&blockquote>", "sgml_blockquote ()");
  menu_append_item (m, "<&caution>", "sgml_env (\"caution\")");
  menu_append_item (m, "<&example>", "sgml_env (\"example\")");
  menu_append_item (m, "<&important>", "sgml_env (\"important\")");
  menu_append_item (m, "<&note>", "sgml_env (\"note\")");
  menu_append_item (m, "<&tip>", "sgml_env (\"tip\")");
  menu_append_item (m, "<&warning>", "sgml_env (\"warning\")");
  % links
  menu_append_popup (menu, "Lin&k");
  m = sprintf ("%s.Lin&k", menu);
  menu_append_item (m, "<&anchor>", "sgml_anchor ()");
  menu_append_item (m, "<&email>", "sgml_insert_tag (\"email\")");
  menu_append_item (m, "<&id>", "sgml_id (); pop_spot ()");
  menu_append_item (m, "<&url>", "sgml_url ()");
  menu_append_item (m, "<&xref>", "sgml_xref ()");
  % lists
  menu_append_popup (menu, "&List");
  m = sprintf ("%s.&List", menu);
  menu_append_item (m, "<&itemizedlist>", "sgml_itemized_list ()");
  menu_append_item (m, "<&listitem>", "sgml_listitem (1); pop_spot ()");
  menu_append_item (m, "<&orderedlist>", "sgml_ordered_list ()");
  menu_append_item (m, "<&segmentedlist>", "sgml_segmented_list ()");
  menu_append_item (m, "<&variablelist>", "sgml_variable_list ()");
  menu_append_item (m, "<varlist&entry>", 
		    "sgml_varlistentry (); pop_spot ()");
  menu_append_item (m, "<se&glistitem>", 
		    "sgml_seglistitem  (1); pop_spot ()");
  menu_append_item (m, "<&procedure>", "sgml_procedure ()");
  menu_append_item (m, "<s&tep>", "sgml_step (1)");
  % table
  menu_append_popup (menu, "&Table");
  m = sprintf ("%s.&Table", menu);
  menu_append_item (m, "<&align>", "sgml_align ()");
  menu_append_item (m, "<&informaltable>", "sgml_table (1)");
  menu_append_item (m, "<&table>", "sgml_table (0)");
  menu_append_item (m, "<&row>", 
		    "sgml_table_row (table_columns, 1, 1); pop_spot ()");
  % paragraph
  menu_append_item (menu, "<&para>", "sgml_para (1); pop_spot ()");
  % figure
  menu_append_item (menu, "<&figure>", "sgml_figure ()");
  % separator
  menu_append_separator (menu);
  % convert to...
  menu_append_item (menu, "Convert To &PostScript", "sgml_convert_to_ps ()");
  menu_append_item (menu, "Convert To P&DF", "sgml_convert_to_pdf ()");
  menu_append_item (menu, "Convert To &HTML", "sgml_convert_to_html ()");
  menu_append_item (menu, "&View PostScript", "sgml_view (\"ps\")");
  menu_append_item (menu, "View PDF", "sgml_view (\"ps\")");
  menu_append_item (menu, "V&iew HTML", "sgml_view (\"html\")");
}

$1 = "docbook";
!if (keymap_p ($1))
  make_keymap ($1);

  % various keymaps
definekey_reserved ("sgml_bskip_tag", "^B", $1);
definekey_reserved ("sgml_skip_tag", "^F", $1);
definekey_reserved ("sgml_para (1); pop_spot ()", "p", $1);
definekey_reserved ("sgml_section ()", "ss", $1);
definekey_reserved ("sgml_sect (1, \"0\"))", "sn", $1);
definekey_reserved ("insert (\"&dollar;\")", "$", $1);
definekey_reserved ("insert (\"&amp;\")", "&", $1);
definekey_reserved ("sgml_comment", ";", $1);
definekey_reserved ("insert (\"&hellip;\")", ".", $1);
definekey_reserved ("insert (\"&lt;\")", "<", $1);
definekey_reserved ("insert (\"&gt;\")", ">", $1);
definekey_reserved ("sgml_convert_to_ps ()",   "cp", $1);
definekey_reserved ("sgml_convert_to_pdf ()",  "cd", $1);
definekey_reserved ("sgml_convert_to_html ()", "ch", $1);
definekey_reserved ("sgml_convert_to_ps ()", "cp", $1);
definekey_reserved ("sgml_view (\"ps\")", "vp", $1);
definekey_reserved ("sgml_view (\"pdf\")", "vd", $1);
definekey_reserved ("sgml_view (\"html\")", "vh", $1);
% template
definekey_reserved ("sgml_insert_tag (\"ackno\", 1, 1)", "ha", $1);
definekey_reserved ("sgml_author (1, 1)", "hu", $1);
definekey_reserved ("sgml_copyright ()", "hc", $1);
definekey_reserved ("sgml_revision_history ()", "hh", $1);
definekey_reserved ("sgml_revision (); pop_spot ()", "hr", $1);
definekey_reserved ("sgml_template ()", "ht", $1);
% section
definekey_reserved ("sgml_appendix (1)", "sa", $1);
definekey_reserved ("sgml_chapter (1)", "sc", $1);
definekey_reserved ("sgml_epigraph ()", "se", $1);
definekey_reserved ("sgml_formalpara ()", "sf", $1);
definekey_reserved ("sgml_simple_section ()", "sm", $1);
definekey_reserved ("sgml_sect (1, \"0\")", "sn", $1);
definekey_reserved ("sgml_sidebar (1)", "si", $1);
definekey_reserved ("sgml_title (\"title\", 1); pop_spot ()", "st", $1);
definekey_reserved ("sgml_title (\"subtitle\", 1); pop_spot ()", "su", $1);
% character
definekey_reserved ("sgml_insert_tag (\"acronym\", 1, 1)", "ca", $1);
definekey_reserved ("sgml_insert_tag (\"citation\", 1, 1)", "cc", $1);
definekey_reserved ("sgml_insert_tag (\"emphasis\", 1, 1)", "ce", $1);
definekey_reserved ("sgml_insert_tag (\"firstterm\", 1, 1)", "cf", $1);
definekey_reserved ("sgml_insert_tag (\"footnote\", 1, 1)", "cn", $1);
definekey_reserved ("sgml_insert_tag (\"superscript\", 1, 1)", "cs", $1);
definekey_reserved ("sgml_insert_tag (\"subscript\", 1, 1)", "cu", $1);
% computer
definekey_reserved ("sgml_insert_tag (\"application\", 1, 1)", "oa", $1);
definekey_reserved ("sgml_insert_tag (\"mousebutton\", 1, 1)", "ob", $1);
definekey_reserved ("sgml_insert_tag (\"command\", 1, 1)", "oc", $1);
definekey_reserved ("sgml_directory ()", "od", $1);
definekey_reserved ("sgml_insert_tag (\"envar\", 1, 1)", "oe", $1);
definekey_reserved ("sgml_insert_tag (\"filename\", 1, 1)", "of", $1);
definekey_reserved ("sgml_insert_tag (\"function\", 1, 1)", "on", $1);
definekey_reserved ("sgml_insert_tag (\"programlisting\", 1, 1)", "ol", $1);
definekey_reserved ("sgml_insert_tag (\"computeroutput\", 1, 1)", "oo", $1);
definekey_reserved ("sgml_insert_tag (\"constant\", 1, 1)", "ot", $1);
definekey_reserved ("sgml_insert_tag (\"prompt\", 1, 1)", "op", $1);
definekey_reserved ("insert_sgml_screen ()", "os", $1);
definekey_reserved ("sgml_insert_tag (\"userinput\", 1, 1)", "ou", $1);
definekey_reserved ("sgml_insert_tag (\"varname\", 1, 1)", "ov", $1);
% environment
definekey_reserved ("sgml_blockquote ()", "eb", $1);
definekey_reserved ("sgml_env (\"caution\")", "ec", $1);
definekey_reserved ("sgml_env (\"example\")", "ee", $1);
definekey_reserved ("sgml_env (\"important\")", "ei", $1);
definekey_reserved ("sgml_env (\"note\")", "en", $1);
definekey_reserved ("sgml_env (\"tip\")", "et", $1);
definekey_reserved ("sgml_env (\"warning\")", "ew", $1);
% links
definekey_reserved ("sgml_anchor ()", "ka", $1);
definekey_reserved ("sgml_insert_tag (\"email\")", "ke", $1);
definekey_reserved ("sgml_id (); pop_spot ()", "ki", $1);
definekey_reserved ("sgml_url ()", "ku", $1);
definekey_reserved ("sgml_xref ()", "kx", $1);
% lists
definekey_reserved ("sgml_itemized_list ()", "li", $1);
definekey_reserved ("sgml_listitem (1); pop_spot ()", "ll", $1);
definekey_reserved ("sgml_ordered_list ()", "lo", $1);
definekey_reserved ("sgml_segmented_list ()", "ls", $1);
definekey_reserved ("sgml_variable_list ()", "lv", $1);
definekey_reserved ("sgml_varlistentry (); pop_spot ()", "le", $1);
definekey_reserved ("sgml_seglistitem  (1); pop_spot ()", "lg", $1);
definekey_reserved ("sgml_procedure ()", "lp", $1);
definekey_reserved ("sgml_step (1)", "lt", $1);
% table
definekey_reserved ("sgml_align ()", "ta", $1);
definekey_reserved ("sgml_table (1)", "ti", $1);
definekey_reserved ("sgml_table (0)", "tt", $1);
definekey_reserved ("sgml_table_row (table_columns, 1, 1); pop_spot ()",
		    "tr", $1);
definekey_reserved ("sgml_figure ()", "f", $1);
definekey ("tex_insert_quote", "\"", $1);
definekey ("tex_insert_quote", "'", $1);


%!%+
%\function{docbook_mode}
%\synopsis{docbook_mode}
%\usage{Void docbook_mode ();}
%\description
% This mode is designed to facilitate the editing of Docbook 3.1 SGML files.
% If a region is defined (i.e., if a mark is set), many SGML tags will
% insert around the region; e.g. '<emphasis>' and '</emphasis>'. Tags are
% inserted either using the Mode menu, or with a key combination resembling 
% the menu entry, e.g. ^Cce inserts <emphasis> (M&ode/&Character/<&emphasis>).
% Functions that affect this mode include (Emacs mode assumed - IDE mode
% uses ^Z instead of ^C):
%#v+
%  sgml_bskip_tag            ^C^B
%  sgml_skip_tag             ^C^F
%  sgml_para                 ^CP
%  sgml_section              ^CS
%  sgml_sect                 ^CN
%  insert &dollar;           ^C$
%  insert &amp;              ^C&
%  insert &hellip;           ^C.
%  insert &lt;               ^C<
%  insert &gt;               ^C>
%#v-
% Variables affecting this mode include:
%#v+
%  Variable                  Default value
%
%  SGML_INDENT               2
%  Sgml_Compile_PS_Cmd       "db2ps"
%  Sgml_Compile_Pdf_Cmd      "db2pdf"
%  Sgml_Compile_Html_Cmd     "db2html"
%  Sgml_View_PS_Cmd          "gv"
%  Sgml_View_Pdf_Cmd         "gv"
%  Sgml_View_Html_Cmd        "netscape"
%#v-
% To change the value of a variable, define that variable in .jedrc
% before loading docbook.sl. For example:
%#v+
%  variable SGML_INDENT = 3;
%#v-
% Hooks: \var{sgml_mode_hook}
%!%-
define docbook_mode ()
{
  variable mode = "docbook";

  set_mode (mode, 1); % wrap mode
  use_keymap (mode);
  set_syntax_flags (mode, 8);
  use_syntax_table (mode);

  set_buffer_hook ("par_sep", &sgml_paragraph_separator);
  !if (blocal_var_exists (DOC_TYPE))
     define_blocal_var (DOC_TYPE, "");
  !if (blocal_var_exists (SECT_TYPE))
     define_blocal_var (SECT_TYPE, "");
  push_spot ();
  % is the document type already defined?
  bob ();
  variable cs = CASE_SEARCH;
  CASE_SEARCH = 0;
  if (fsearch ("<book"))
    set_blocal_var ("b", DOC_TYPE);
  else if (fsearch ("<article"))
    set_blocal_var ("a", DOC_TYPE);
  else
    set_blocal_var ("*", DOC_TYPE);
  bob ();
  if (fsearch ("<section"))
    set_blocal_var ("s", SECT_TYPE);
  else if (fsearch ("<sect1"))
    set_blocal_var ("#", SECT_TYPE);
  else set_blocal_var ("*", SECT_TYPE);
  CASE_SEARCH = cs;
  pop_spot ();
  mode_set_mode_info (mode, "init_mode_menu", &init_menu);
  run_mode_hooks ("docbook_mode_hook");
}

% --- End of file docbook.sl ---
