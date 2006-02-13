% Mute (Dead or accent) keys for JED.
%
%   To use this package, put a line like the following in your jed.rc
%
%    mute_set_mute_keys ("`'^");
%
%   Here, the keys `, ', and ^ will be mute keys.  That is, pressing one of
%   these followed by the character to accent will insert an accented
%   character in the buffer.   This affects only the local keymap.
%
%   Valid Mute keys are:
%      ^, ~, ', `, \d168 (ISO Diaeresis), \d180 (ISO Acute), and ".
%

% To permit unicode characters > 255, it may be necessary to use an integer
% array of the character values instead of a string array.
define mute_insert_accent (ok_chars, maps_to)
{
   variable pos, ch;

   ch = maps_to[0];
   !if (input_pending (10))
     {
	vmessage ("%c-", ch);
	update_sans_update_hook (0);
     }

   ch = getkey ();
   pos = is_substr (ok_chars, char (ch));
   if (pos)
     {
	insert_char (maps_to[pos-1]);
     }
   else
     {
	insert_char (maps_to[0]);
	ungetkey (ch);
     }
}

define mute_set_mute_keys (str)
{
   variable i;

   _for (0, strlen(str) - 1, 1)
     {
	i = ();
	switch (str[i])
	  { case '"':	local_setkey ("mute_keymap_34", "\""); }
	  { case '\'':	local_setkey ("mute_keymap_39", "'"); }
	  { case '^':	local_setkey ("mute_keymap_94", "^"); }
	  { case '`':	local_setkey ("mute_keymap_96", "`"); }
	  { case '~':	local_setkey ("mute_keymap_126", "~"); }
	%% ISOLatin 1 diaeresis would be better, if included in keyboard
	  { case '\d168': local_setkey ("mute_keymap_168", "\d168"); }
	%% Asciitilde would be better to acute accent, if included in keyboard
	  { case '\d180': local_setkey ("mute_keymap_180", "\d180"); }
     }
}

define mute_keymap_39 () % ' map
{
#ifdef IBMPC_SYSTEM
   "'Eeaiou";
   "'\d144\d130\d160\d161\d162\d163";
#else
   "'AEIOUYaeiouy?!/1Cc";
   "'\d193\d201\d205\d211\d218\d221\d225\d233\d237\d243\d250\d253\d191\d161\d191\d161\d199\d231";
#endif
   mute_insert_accent ();
}

define mute_keymap_180 () % \d180 map
{
#ifdef IBMPC_SYSTEM
   "'Eeaiou";
   "'\d144\d130\d160\d161\d162\d163";
#else
   "\d180AEIOUYaeiouy";
   "\d180\d193\d201\d205\d211\d218\d221\d225\d233\d237\d243\d250\d253";
#endif
   mute_insert_accent ();
}

define mute_keymap_94 () % ^ map
{
#ifdef IBMPC_SYSTEM
   "^aeiou";
   "^\d131\d136\d140\d147\d150";
#else
   "^aeiou";
   "^\d226\d234\d238\d244\d251";
#endif
   mute_insert_accent ();
}

define mute_keymap_96 () % ` map
{
#ifdef IBMPC_SYSTEM
   "`aeiou";
   "`\d133\d138\d141\d149\d151";
#else
   "`AEIOUaeiou";   
   "`\d192\d200\d204\d210\d217\d224\d232\d236\d242\d249";
#endif
   mute_insert_accent ();
}

define mute_keymap_126 ()  % ~ map
{
#ifdef IBMPC_SYSTEM
   "~Nn";
   "~\d165\d164";
#else
   "~NnAOao";
   "~\d209\d241\d195\d213\d227\d245";
#endif
   mute_insert_accent ();
}

define mute_keymap_34 () % \" map
{
#ifdef IBMPC_SYSTEM
   "\"uaAeioyOU";
   "\"\d129\d132\d142\d137\d139\d148\d152\d153\d154";
#else
   "\"AEIOUaeiouys";   
   "\"\d196\d203\d207\d214\d220\d228\d235\d239\d246\d252\d255\d223";
#endif
   mute_insert_accent ();
}

define mute_keymap_168 () % \d168 map
{
#ifdef IBMPC_SYSTEM
   "\"uaAeioyOU";
   "\"\d129\d132\d142\d137\d139\d148\d152\d153\d154";
#else
   "\d168AEIOUaeiouy";   
   "\d168\d196\d203\d207\d214\d220\d228\d235\d239\d246\d252\d255";
#endif
   mute_insert_accent ();
}
