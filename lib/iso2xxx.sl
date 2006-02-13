% Thanks to torres@upf.es for the bulk of this
_debug_info = 1;
private variable ISO_Latin_Chars =
strcat ("\d225\d233\d237\d243\d250\d193\d201\d205\d211\d218", % "áéíóúÁÉÍÓÚ"
	 "\d224\d232\d236\d242\d249\d192\d200\d204\d210\d217", % "àèìòùÀÈÌÒÙ"
	 "\d226\d234\d238\d244\d251\d194\d202\d206\d212\d219", % "âêîôûÂÊÎÔÛ"
	 "\d228\d235\d239\d246\d252\d196\d203\d207\d214\d220", % "äëïöüÄËÏÖÜ"
	 "\d161\d191\d231\d199\d241\d209\d230\d198",           % "¡¿çÇñÑæÆ"
	 "\d248\d216\d229\d197\d223");                          % "øØåÅß"

private variable ISO_TeX_Chars =
  strcat ("\\'a,\\'e,\\'{\\i},\\'o,\\'u,\\'A,\\'E,\\'I,\\'O,\\'U,",
	   "\\`a,\\`e,\\`{\\i},\\`o,\\`u,\\`A,\\`E,\\`I,\\`O,\\`U,",
	   "\\^a,\\^e,\\^{\\i},\\^o,\\^u,\\^A,\\^E,\\^I,\\^O,\\^U,",
	   "\\\"a,\\\"e,\\\"{\\i},\\\"o,\\\"u,\\\"A,\\\"E,\\\"I,\\\"O,\\\"U,",
	   "!`,?`,\\c{c},\\c{C},\\~n,\\~N,{\\ae},{\\AE},",
	   "{\\o},{\\O},{\\aa},{\\AA},{\\ss}");

define iso2tex () 
{
   variable i, str_old, str_new;
   variable save_case_search = CASE_SEARCH;

   CASE_SEARCH = 1;
   push_spot ();
   bob ();
   _for (0, strlen (ISO_Latin_Chars) - 1, 1)
     {
	i = ();
	str_new = extract_element (ISO_TeX_Chars, i, ',');
	str_old = char (ISO_Latin_Chars[i]);
	
	%bob ();  --- not needed since replace does not move the point
	replace (str_old, str_new);
     }
   pop_spot();
   CASE_SEARCH = save_case_search;
}


define tex2iso () 
{
   variable i, str_old, str_new;
   variable save_case_search = CASE_SEARCH;
   
   CASE_SEARCH = 1;
   push_spot ();
   _for (0, strlen (ISO_Latin_Chars) - 1, 1)
     {
	i = ();
	str_old = extract_element (ISO_TeX_Chars, i, ',');
	str_new = char (ISO_Latin_Chars[i]);
	
	bob ();
	replace (str_old, str_new);
     }
   pop_spot();
   CASE_SEARCH = save_case_search;
}


