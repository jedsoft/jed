% Interface to VMS Help

variable VMSHelp_Topic_Len = 0;
variable VMS_Help_Library = "SYS$HELP:HELPLIB.HLB"; 
% change as necessary - routines work with any .HLB file.

define vms_help ()
{
   variable curr_buf, helptopic;
   
   if (VMSHelp_Topic_Len) return;

   curr_buf = whatbuf();
   
   ERROR_BLOCK 
     {
	pop_mark_0 ();
	sw2buf(curr_buf);
	VMSHelp_Topic_Len = -0;
     }
   

   helptopic = read_mini("VMS Help Topic:", Null_String, Null_String);
   

   sw2buf ("*VMS-Help*");
   % set_readonly (0);
   erase_buffer ();

   push_mark();           % This mark is popped later
   VMSHelp_Topic_Len = -1;
   vms_get_help (VMS_Help_Library, helptopic);
   
   EXECUTE_ERROR_BLOCK;
}

variable VMSHelp_This_Topic = Null_String;
define vms_help_grab_topic ()
{
   variable word = "-/_@=:0-9a-zA-Z\277-\326\330-\336\340-\366\370-\376";
   bskip_chars (word);
   push_mark();
   skip_chars (word);
   VMSHelp_This_Topic = bufsubstr ();
   strlen (VMSHelp_This_Topic);
}

define vms_help_newtopic (prompt)
{
   variable use_call, fun, ch, topic, msg;
   setbuf ("*VMS-Help*");
   pop_mark_1 ();
   % set_readonly (1);
   
   VMSHelp_This_Topic = Null_String;
   if (VMSHelp_Topic_Len)
     {
	msg = sprintf("Hit RET for '%s', PgDn/PgUp,  ", prompt);
	recenter (1);
	forever 
	  {
	     ERROR_BLOCK
	       {
		  _clear_error ();
	       }
	     
	     message (msg);
	     update_sans_update_hook (1);
	     
	     ch = getkey ();
	     if (ch == '\r') break;
	     if (ch == '.')
	       {
		  if (vms_help_grab_topic ()) break;
		  continue;
	       }
	     
	     if (ch == 127)
	       {
		  use_call = 1;
		  fun = "page_up";
	       }
	     else 
	       {
		  ungetkey (ch);
		  (use_call, fun) = get_key_binding ();
		  if (typeof (fun) == Ref_Type)
		    {
		       (@fun)();
		       continue;
		    }
		  if (fun == NULL) fun = "";
	       }
	     
	     if (fun == "self_insert_cmd")
	       {
		  if (ch == ' ') fun = "page_down"; 
		  else
		    {
		       ungetkey(ch);
		       break;
		    }
	       }

	     if (use_call) call (fun); else eval (fun);
	  }
     }
   
   re_fsearch ("^[\t ]*\\cAdditional information available:"); pop();
   topic = strtrim(read_mini(prompt, VMSHelp_This_Topic, Null_String));
   % set_readonly (0);

   VMSHelp_Topic_Len = strlen (topic);
   if (VMSHelp_Topic_Len) eob();
   push_mark();
   topic;
}


