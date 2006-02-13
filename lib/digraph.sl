% digraph.sl	-*- Slang -*-
%
% This is digraph.sl. It allows for easy input of accented characters and
% other 8-bit characters. It is an alternative to the mutekeys.sl package.
% I wrote it because I don't like mutekeys. I added a lot of characters not
% found in mutekeys.sl.
%
% Usage: put this file in $JED_ROOT/lib and add a line
%
%   autoload ("digraph_cmd",  "digraph");  setkey ("digraph_cmd", "\ek");
%
% to your ~/.jedrc file, restart jed so that this setkey takes effect before
% any other keymaps are defined.  The digraphs are now available by typing
% Esc-K 'accent' 'letter'.
%
% Thanks to John Davis for reducing the number of case statements in the
% original code and for adding the user-friendly interface (don't type too
% fast, otherwise you can't read it).
%
% DONE: This does not work yet under MSDOS and OS2 . Any volunteers?
%
% Ronald Rietman, 1995/03/05
%%%
% - added MSDOS, OS/2 digraphs
% - changed key for British Pounds from $ to # (pounds) so that it occurs in
%   the same place as a UK keyboard
% - added European-style double quotes
% - moved inverted punctuation marks to use a period accent and added DOS
%   line-drawing characters to the same.  Letters for lines correspond to
%   curses ACS (Alternate Character Set), except 'P' invented to == `Plus'
%
% Mark Olesen	16 Aug 1995

define digraph_cmd ()
{
   variable i, accent, letters;
#iffalse
   EXIT_BLOCK
     {
	message (Null_String);
     }
#endif
   switch (get_mini_response ("Enter an accent character: [/`'^\"~,.]"))
     { case '/':
#ifdef IBMPC_SYSTEM
	"aAeEs#yc";
	"\d134\d143\d145\d146\d225\d156\d157\d155";
#else
	"aAdDeEoOstT#yc";
	"\d229\d197\d240\d208\d230\d198\d248\d216\d223\d254\d222\d163\d165\d162";
#endif
     }
     { case '"':
#ifdef IBMPC_SYSTEM
	"`aAeioOsuUy'";
       %"\d174\d132\d142\d137\d139\d148\d153\d129\d225\d154\d152\d175";
	"\d174\d132\d142\d137\d139\d148\d153\d225\d129\d154\d152\d175";
#else
	"`aAeEiIoOsuU'";
        "\d171\d228\d196\d235\d203\d239\d207\d246\d214\d223\d252\d220\d187";
#endif
     }
     { case '\'':
#ifdef IBMPC_SYSTEM
	"aeEiou'";
	"\d160\d130\d144\d161\d162\d163\d175";
#else
	"aAeEiIoOuU'";
        "\d225\d193\d233\d201\d237\d205\d243\d211\d250\d218\d187";
#endif
     }
     { case '`':
#ifdef IBMPC_SYSTEM
	"`aeiou";
	"\d174\d133\d138\d141\d149\d151";
#else
	"`aAeEiIoOuU";
	"\d171\d224\d192\d232\d200\d236\d204\d242\d210\d249\d217";
#endif
     }
     { case '^':
#ifdef IBMPC_SYSTEM
	"aeiou";
	"\d131\d136\d140\d147\d150";
#else
	"aAeEiIoOuU";
	"\d226\d194\d234\d202\d238\d206\d244\d212\d251\d219";
#endif
     }
     { case '~':
#ifdef IBMPC_SYSTEM
	"nN";
	"\d164\d165";
#else
	"aAnNoO";
	"\d227\d195\d241\d209\d245\d213";
#endif
     }
     { case ',':
	"cC";
#ifdef IBMPC_SYSTEM
	"\d135\d128";
#else
	"\d231\d199";
#endif
     }
     { case '.':
#ifdef IBMPC_SYSTEM
	"!?aAjJkKlLmMpPqQtTuUvVwWxX";
	"\d173\d168\d177\d178\d217\d188\d191\d187\d218\d201\d192\d200\d197\d206\d196\d205\d195\d204\d180\d185\d194\d203\d193\d202\d179\d186";
#else
	"!?1/";
	"\d161\d191\d161\d191";
#endif
     }
     { return; }	% default

   (letters, accent) = ();
   i = is_substr (letters, 
		  char (get_mini_response (sprintf ("Enter [%s] to get [%s]",
						    letters, accent))));

   !if (i)
     {
	beep ();
	return;
     }
   
   i--;
   insert_char (accent[i]);
}
