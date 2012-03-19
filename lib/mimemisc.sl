require ("base64");

private define gather (sp, s)
{
   @sp = @sp + s;
}

define mime_base64_decode (str)
{
   variable s = "";
   variable b = _base64_decoder_new (&gather, &s);
   _base64_decoder_accumulate (b, str);
   _base64_decoder_close (b);
   return s;
}

define mime_base64_encode (str)
{
   variable s = "";
   variable b = _base64_encoder_new (&gather, &s);
   _base64_encoder_accumulate (b, str);
   _base64_decoder_close (b);
   return str;
}

require ("iconv");
define mime_iconv (to, from, str)
{
   variable save = str;
   try
     {
	variable f = iconv_open (to, from);
	str = iconv (f, str);
	iconv_close (f);
     }
   catch AnyError: str = save;
   return str;
}
