%
%  Recall previous commands in MiniBuffer
%

% If -2, never store duplicates, use Least-Recently-Used strategy
% If -1, store only if not equal to last entered value
% If 0, never store duplicates
custom_variable ("Mini_Store_Duplicates", 1);

private variable Max_Num_Lines = 32;

private variable Mini_Previous_Lines;
private variable Mini_Last_Line;
private variable Mini_First_Line;
private variable Mini_Next_Line;

private define initialize ()
{
   Mini_Previous_Lines =  String_Type [Max_Num_Lines];
   Mini_Last_Line = 0;
   Mini_First_Line = 0;
   Mini_Next_Line = 0;

   Mini_Previous_Lines[[:]] = "";
}

initialize ();

private define mini_use_this_line ()
{
   erase_buffer ();
   insert (Mini_Previous_Lines[Mini_Next_Line]);
}


define next_mini_command ()
{
   variable line;

   if (Mini_Next_Line == Mini_Last_Line) 
     error ("End of list!");

   Mini_Next_Line++;
   % if (Mini_Next_Line == Max_Num_Lines)  Mini_Next_Line = 0;
   Mini_Next_Line = Mini_Next_Line mod Max_Num_Lines;
   mini_use_this_line ();
}

define prev_mini_command ()
{
   variable line;

   line = Mini_Next_Line;
   if (line == Mini_Last_Line)
     Mini_Previous_Lines [line] = line_as_string ();

   if (Mini_First_Line == Mini_Next_Line)
     error ("Top of list!");

   Mini_Next_Line = (Mini_Next_Line + Max_Num_Lines - 1) mod Max_Num_Lines;
   mini_use_this_line ();
}

private define store_line (s)
{
   if ((s == NULL) or (s == ""))
     return;

   Mini_Previous_Lines[Mini_Last_Line] = s;
   Mini_Last_Line++;
   Mini_Last_Line = Mini_Last_Line mod Max_Num_Lines;
   if (Mini_Last_Line == Mini_First_Line)
     {
	Mini_First_Line++;
	Mini_First_Line = Mini_First_Line mod Max_Num_Lines;
     }
   Mini_Next_Line = Mini_Last_Line;
   Mini_Previous_Lines [Mini_Last_Line] = "";
}

public define mini_exit_minibuffer ()
{
   Mini_Next_Line = Mini_Last_Line;

   bol_skip_white ();
   !if (eolp ())
     {
	variable line = line_as_string ();

	switch (Mini_Store_Duplicates)
	  {
	   case 0:		       %  never
	     if (any (line == Mini_Previous_Lines))
	       line = NULL;
	  }
	  {
	   case -1:		       %  sometimes
	     variable i = (Mini_Next_Line + Max_Num_Lines - 1) mod Max_Num_Lines;
	     if (Mini_Previous_Lines[i] == line)
	       line = NULL;
	  }
	  {
	   case -2:		       %  never, use LRU
             variable il, delta, la = Mini_Last_Line;
             if (la < Mini_First_Line) la = la + Max_Num_Lines;
             delta = 0;
             il = Mini_First_Line;
             while (il < la)
	       {
		  if (Mini_Previous_Lines[il mod Max_Num_Lines] == line) delta++;
		  if (delta)
		    {
		       if ((il + delta) > la) 
			 break;
		       Mini_Previous_Lines[il mod Max_Num_Lines] = Mini_Previous_Lines[(il + delta) mod Max_Num_Lines];
		    }
		  il++;
	       }
             if (delta)
	       {
		  Mini_Last_Line = (Mini_Last_Line + Max_Num_Lines - delta) mod Max_Num_Lines;
		  Mini_Next_Line = Mini_Last_Line;
	       }
	  }
	store_line (line);
     }
   call ("exit_mini");
}


public define mini_store_lines (lines)
{
   foreach (lines)
     store_line ();
}

public define mini_set_lines (lines)
{
   initialize ();
   mini_store_lines (lines);
}



public define mini_get_lines (num_p)
{
   variable n = Mini_Last_Line - Mini_First_Line;

   if (num_p != NULL)
     @num_p = Max_Num_Lines;

   if (n < 0)
     n += (Max_Num_Lines+1);
   
   variable lines = String_Type [n];
   
   n = Mini_Last_Line - Mini_First_Line;
   if (n < 0)
     {
	n = Max_Num_Lines - Mini_First_Line;
	lines[[0:n-1]] = Mini_Previous_Lines[[Mini_First_Line:]];
	lines[[n:]] = Mini_Previous_Lines[[0:Mini_Last_Line]];
	return lines;
     }
   
   return Mini_Previous_Lines[[0:n-1]];
}

% This function is called from site.sl AFTER jed.rc has been read but
% before other command line arguments have been parsed.
public define mini_init_minibuffer ()
{   
   variable mini = "Mini_Map";
   !if (keymap_p (mini))
     make_keymap (mini);

#ifdef IBMPC_SYSTEM
   definekey ("next_mini_command", "\eOr", mini);
   definekey ("next_mini_command", "\xE0P", mini);
   definekey ("prev_mini_command", "\xE0H", mini);
   definekey ("prev_mini_command", "\eOx", mini);
#else
   definekey ("next_mini_command", "\e[B", mini);
   definekey ("prev_mini_command", "\e[A", mini);
   definekey ("next_mini_command", "\eOB", mini);
   definekey ("prev_mini_command", "\eOA", mini);
#endif
   definekey ("mini_exit_minibuffer", "\r", mini);
   
   definekey ("exit_mini", "\e\r", mini);
   definekey ("mini_complete", "\t", mini);
   definekey ("mini_complete", " ", mini);

   % Now that we are initialized, remove this function
   eval (".() mini_init_minibuffer");
}

   
   
