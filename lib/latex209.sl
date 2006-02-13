% latex209.sl
%
% AUC-TeX style LaTeX209-mode (v0.3) by Kevin Humphreys <kwh@cogsci.ed.ac.uk>
%
% For JED (v0.97.9b) by John E. Davis <davis@space.mit.edu>
%
% Based on AUC-TeX (v7.3) by Per Abrahamsen <auc_tex_mgr@iesd.auc.dk>


% Loaded from latex.sl if LaTeX_default_documentstyle is non-null
% (If you don't use LaTeX209 you don't need this file)


% CHANGES HISTORY:
% Modified for integration into the main jed distribution.
% --- v0.2-0.3
% added call to read_with_completion for documentstyle
% --- v0.1-0.2
% added insertions for letter style
% call tex_embrace for font specifier insertion


define latex_insert_document_env ()
{
   variable class = read_with_completion(LaTeX_classes, 
					 "Enter Document Class:", 
					 LaTeX_default_documentstyle, 
					 Null_String, 's');
   variable options = read_mini ("Enter Style Options:", LaTeX_default_options, Null_String);
   if (strlen (options)) options = sprintf("[%s]", options);
	
   bob ();
   vinsert ("\\\\documentstyle%s{%s}\n\n", options, class);
   insert ("\\begin{document}\n\n\n\n\\end{document}\n"); go_up(3);
   !if (strcmp(class, "letter")) latex_insert_letter_args();
}



% Font Selection - LaTeX209

define tex_insert_font (pre, post, arg) 
{
   if (arg == -1)  tex_embrace(pre, post);
   else % if prefix argument
     {  push_spot();
        bsearch("{\\"); pop();
	delete_word(); del ();
	insert (pre);
	() = fsearch_char ('}'); del ();
	go_left(2); if (looking_at("\\/")) del (); else go_right(2);
	insert(post);
	pop_spot (); 
     }  
}


define tex_delete_font () 
{
   push_spot();
   () = bsearch("{\\");
   delete_word(); del ();
   () = fsearch_char ('}'); del ();
   go_left(2); if (looking_at("\\/")) deln(2);
   pop_spot ();  
}

	
define tex_font () 
{
   variable arg = prefix_argument(-1);
   switch (getkey())
     { case 2  : tex_insert_font ("{\\bf ", "}",    arg);} % C-b
     { case 3  : tex_insert_font ("{\\sc ", "}",    arg);} % C-c
     { case 5  : tex_insert_font ("{\\em ", "\\/}", arg);} % C-e
     { case 9  : tex_insert_font ("{\\it ", "\\/}", arg);} % C-i
     { case 18 : tex_insert_font ("{\\rm ", "}",    arg);} % C-r
     { case 19 : tex_insert_font ("{\\sl ", "\\/}", arg);} % C-s
     { case 20 : tex_insert_font ("{\\tt ", "}",    arg);} % C-t
     { case 4  : tex_delete_font ();} % C-d
}



