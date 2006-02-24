define paste ()
{
   flush ("Ready for data...");
   
   if (0 == input_pending (300))
     {
	message ("You have taken too much time");
	return;
     }
   variable as = _test_buffer_flag (2);
   ERROR_BLOCK
     {
	if (as) _set_buffer_flag (2);
     }
   _unset_buffer_flag (2);

   flush ("Pasting in progress...");
   while (input_pending (10))
     {
	variable ch = _getkey ();
	if (ch == '\r')
	  {
	     ch = '\n';
	  }
	insert_byte (ch);
	update (0);
     }
   EXECUTE_ERROR_BLOCK;
   message ("Done");
}
