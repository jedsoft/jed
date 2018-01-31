#% -*- mode: tm; mode: fold; eval: .0 =TAB -*-
#%{{{ Macros 

#i linuxdoc.tm

#d slang \bf{S-lang}
#d jed \bf{JED}
#d kw#1 \tt{$1}
#d exmp#1 \tt{$1}
#d var#1 \tt{$1}
#d ifun#1 \tt{$1}
#d dtype#1 \tt{$1}

#d ldots ...

#d function#1 \section{<bf>$1</bf>\label{$1}}<descrip>
#d variable#1 \section{<bf>$1</bf>\label{$1}}<descrip>
#% d function#1 <p><bf>$1</bf>\label{$1}<p><descrip>
#d synopsis#1 <tag> Synopsis </tag> $1
#d keywords#1 <tag> Keywords </tag> $1
#d usage#1 <tag> Usage </tag> <tt>$1</tt>
#d description <tag> Description </tag>
#d example <tag> Example </tag>
#d notes <tag> Notes </tag>
#d seealso#1 \ifarg{$1}{<tag> See Also </tag> <tt>$1</tt>}
#d documentstyle article
#d r#1 \ref{$1}{$1}
#d done </descrip><p>

#d -1 <tt>-1</tt>
#d 0 <tt>0</tt>
#d 1 <tt>1</tt>
#d 2 <tt>2</tt>
#d 3 <tt>3</tt>
#d 4 <tt>4</tt>
#d 5 <tt>5</tt>
#d NULL <tt>NULL</tt>
#d documentstyle book
#d section#1 \sect{$1}
#%}}}

\linuxdoc
\begin{\documentstyle}

\title Jed Intrinsic Function Reference Manual
\author John E. Davis, \tt{davis@space.mit.edu}
\date \__today__

\toc

\chapter{Movement Functions}
#i rtl/move.tm

\chapter{Insertion/Deletions Functions}
#i rtl/insdel.tm

\chapter{Search/Replace Functions}
#i rtl/search.tm

\chapter{Buffer Related Functions}
#i rtl/buffer.tm

\chapter{Abbreviation Functions}
#i rtl/abbrev.tm

\chapter{Buffer-Local Variable Functions}
#i rtl/blocal.tm

\chapter{Color Functions}
#i rtl/color.tm

\chapter{Loading and Evaluation of S-Lang Code Functions}
#i rtl/eval.tm

\chapter{File Related Functions}
#i rtl/file.tm

\chapter{Functions that work with Hidden Lines}
#i rtl/hidden.tm

\chapter{Functions dealing with hooks}
#i rtl/hooks.tm

\chapter{Informational Functions}
#i rtl/info.tm

\chapter{Keymaps and Key Input Functions}
#i rtl/keys.tm

\chapter{Mark and Spot Functions}
#i rtl/mark.tm

\chapter{Menu Functions}
#i rtl/menu.tm

\chapter{Message Functions}
#i rtl/message.tm

\chapter{Mini-Buffer Functions}
#i rtl/mini.tm

\chapter{Mouse Functions}
#i rtl/mouse.tm

\chapter{Subprocess Functions}
#i rtl/process.tm

\chapter{Rectangle Functions}
#i rtl/rect.tm

\chapter{Functions that Involve Regions}
#i rtl/region.tm

\chapter{Syntax Highlighting and Parsing Functions}
#i rtl/syntax.tm

\chapter{Terminal Functions}
#i rtl/terminal.tm

\chapter{User and Host Functions}
#i rtl/userinfo.tm

\chapter{Display and Window Functions}
#i rtl/window.tm

\chapter{Xjed-specific Functions}
#i rtl/xjed.tm

\chapter{Miscellaneous Functions}
#i rtl/misc.tm

\end{\documentstyle}

