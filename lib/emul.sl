% These functions allow jed to work with older versions of the slang library

#ifnexists strtrans
% This emulation is not at all complete.  It can only be used with one
% one character transformations.
define strtrans (a, b, c)
{
   str_replace_all (a, b, c);
}
#endif
