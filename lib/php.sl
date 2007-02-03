%   file     : php.sl
%   author   : Mikael hultgren <micke@yeah.nu>
%   version  : 1.4-1
%
%   $Id: php.sl,v 1.190 2001/10/03 13:27:08 child Exp $
%
%   D e s c r i p t i o n
%   ---------------------
%   
%   This is a mode for editing php files in jed, hence the name phpmode :)
%    
%   The reason for this mode is that the only mode i
%   could find for editing php files under jed was one 
%   i found on dotfiles made by Eric Thelin. 
%   But it didn't work as i wanted, so i grabbed what i
%   could from it, and started from cmode as a template.
%   
%   At the moment it does keyword highlighting and proper
%   indenting, plus a slew of other functionality.
%   
%   -------------------------------------------------------------------------------------------
%   PHP-mode variables:
%   -------------------------------------------------------------------------------------------
%   variable PHP_INDENT      = 4;       % Amount of space to indent within block.
%   variable PHP_BRACE       = 0;       % Amount of space to indent brace
%   variable PHP_BRA_NEWLINE = 0;       % If non-zero, insert a newline first before inserting 
%                                       % a '{'.
%   variable PHP_CONTINUED_OFFSET = 2;  % This variable controls the indentation of statements
%                                       % that are continued onto the next line.
%   variable PHP_Colon_Offset = 1;      % Controls the indentation of case inside switch statements.
%   variable PHP_Class_Offset = 4;      % Controls the amount of indenting inside the class,
%                                       % doesn't apply to the braces
%   variable PHP_Switch_Offset = 0      % Controls the ammount of extra indention inside switch statements.                                    
%   variable PHP_KET_NEWLINE = 0;       % If non-zero, insert a newline first before inserting 
%                                       % a '}'.
%   variable PHP_Autoinsert_Comments = 1;
%  --------------------------------------------------------------------------------------------
%   
%   T h a n k s  g o  o u t  t o
%   ----------------------------
%   
%    o Eric thelin <eric at thelin.org> for his phpmode that got me started.
%    o David <dstegbauer at post.cz>  who pointed out that php isn't in fact a
%      case sensitive language when it comes to
%      functions ;)
%    o Abraham vd Merwe <abz at blio.net> for his relentless bug-reporting,
%      feature suggestions and beta-tester.
%      Without him my to-do list would be
%      considerable shorter ;)
%    o cmode.sl in jed, without that this mode wouldn't be
%      nearly as feature rich as it is now.
%    o latex.sl for tips on how to do things.
%

% Set all variables to a default value so people who forget to add
% them to their .jedrc doesnt get a error.
%
custom_variable( "PHP_INDENT", 4 );
custom_variable( "PHP_BRACE", 0 );
custom_variable( "PHP_BRA_NEWLINE", 0 );
custom_variable( "PHP_CONTINUED_OFFSET", 2 );
custom_variable( "PHP_Colon_Offset", 1 );
custom_variable( "PHP_Class_Offset", 4 );
custom_variable( "PHP_Switch_Offset", 0 );
custom_variable( "PHP_KET_NEWLINE", 0 );
custom_variable( "PHP_Autoinsert_Comments", 0 );


private define php_is_comment( ) %{{{
{
	push_spot( );
	bol_skip_white( );
	0;
	if( looking_at( "//" ) or looking_at( "#" ))
	{
		pop( );
		what_column( );
	}
	pop_spot( );
}
%}}}
private define php_parse_to_point( ) %{{{
{
	parse_to_point( )
	  or php_is_comment( );
}
%}}}
private variable PHPmode_Fill_Chars = "";
define php_paragraph_sep( ) %{{{
{
	if( strlen( PHPmode_Fill_Chars )) 
	  return 0;
	push_spot( );
	bol_skip_white( );
	if( looking_at( "* " ) or looking_at( "// " ) or looking_at( "# " ))
	{
		go_right( 2 );
		skip_white( );
		if( looking_at( "@ " )) 
		  eol( );
	}
	
	eolp( ) or ( -2 != parse_to_point( ) );
	pop_spot( );
}
%}}}
define php_format_paragraph( ) %{{{
{
	variable n, dwrap;
	
	PHPmode_Fill_Chars = "";
	if( php_paragraph_sep( ) ) 
	  return;
	push_spot( ); 
	push_spot( ); 
	push_spot( );
	
	while( not( php_paragraph_sep( ) ))
	{
		!if( up_1( ) ) 
		  break;
	}
	if( php_paragraph_sep( ) ) 
	  go_down_1( );
	push_mark( );
	pop_spot( );
	
	while( not( php_paragraph_sep( ) ))
	{
		!if( down_1( ) ) 
		  break;
	}
	
	if( php_paragraph_sep( ) ) 
	  go_up_1( );
	
	narrow( );
	pop_spot( );
	bol( );
	push_mark( );
	skip_white( );
	if( looking_at( "* " )) 
	  go_right( 2 );
	else if( looking_at( "// " )) 
	  go_right( 3 );
	else if( looking_at( "# " )) 
	  go_right( 2 );
	
	PHPmode_Fill_Chars = bufsubstr( );
	dwrap = what_column( );
	bob( );
	do 
	{
		bol_trim( );
		if( looking_at( "* " )) 
		  deln( 2 );
		else if( looking_at( "// " )) 
		  deln( 3 );
		else if( looking_at( "# " )) 
		  deln( 2 );
	}
	
	while( down_1( ) );
	WRAP -= dwrap;
	call( "format_paragraph" );
	WRAP += dwrap;
	bob( );
	do 
	{
		insert( PHPmode_Fill_Chars );
	}
	while( down_1( ) );
	
	bol( );
	go_right( strlen( PHPmode_Fill_Chars ));
	
	skip_white( );
	if( looking_at( "*/" ))
	{
		push_mark( );
		bol_skip_white( );
		del_region( );
	}
	
	PHPmode_Fill_Chars = "";
	widen( );
	pop_spot( );
}
%}}}
define php_in_block( ) %{{{
{
	variable begin = 0, end = 0;	
	variable delim_start = "<?";
	variable delim_end   = "?>";
	variable delim_ASP_start = "<%";
	variable delim_ASP_end   = "%>";
	variable test;
	
	push_spot( );
	if( bolp( ) )
	{
		if( looking_at( delim_start ) 
			or looking_at( delim_end ) 
			or looking_at( delim_ASP_start ) 
			or looking_at( delim_ASP_end ))
		{
			pop_spot( );
			return( 1 );
		}
	}
	
	if( looking_at( ">" ))
	{
		go_left( 1 );
		if( looking_at( "?" ) or looking_at( "%" ))
		{
			pop_spot( );
			return( 1 );
		}
	}
	
	forever
	{
		if( bsearch( delim_start ) or bsearch( delim_ASP_start ))
		{
			if( php_parse_to_point( ) == 0 )
			{
				begin = what_line( );
				break;
			}
			continue;
		} else {
			break;
		}
	}
	
	pop_spot( );
	
	push_spot( );
	forever
	{
		if( bsearch( delim_end ) or bsearch( delim_ASP_end ))
		{
			if( php_parse_to_point( ) == 0 )
			{
				end = what_line( );
				break;
			}
			continue;
		} else {
			break;
		}
	}
	
	pop_spot( );
	
	if( end < begin )
	{
		return( 1 );
	}
	return( 0 );
}
%}}}

define php_top_of_function( ) %{{{
{
	push_spot( );
	variable current,end,start_brace;
	current = what_line;
	!if( re_bsearch( "function[ \t]+[a-zA-Z_0-9]+[ \t]?\(.*\)") )
	{
		error( "Cant find top of function" );
	}
	
	% Check to see if were in a comment
	if( php_parse_to_point( ) )
	{
		pop_spot( );
		error( "Cant find top of function" );
	}
	
	!if( fsearch( "{") )
	{
		error( "Missing beginning brace of function." );
	}
	start_brace = what_line;
	if( start_brace > current )
	{
		pop_spot( );
		error( "Missing beginning brace of function." );
	}
	find_matching_delimiter( '{' );
	end = what_line;
	if( end < current )
	{
		pop_spot( );
		error( "Not in function" );
	}
	find_matching_delimiter( '}' );
}
%}}}
define php_end_of_function( ) %{{{
{
	!if( bolp( ) and looking_at_char( '{' ))
	  php_top_of_function( );
	call( "goto_match" );
}
%}}}
define php_mark_function( ) %{{{
{
	php_end_of_function( );
	push_visible_mark( );
	eol( );
	exchange_point_and_mark( );
	php_top_of_function( );
	bol_skip_white( );
	if( looking_at( "{") )
	{
		go_up( 1 );
	}
	bol( );
}
%}}}
define php_mark_matching( ) %{{{
{
	push_spot( );
	if( find_matching_delimiter( 0 ))
	{
		% Found one
		pop_spot( );
		push_visible_mark( );
		find_matching_delimiter( 0 );		
		exchange_point_and_mark( );
	} else {
		pop_spot( );
	}
}
%}}}
define php_bskip_over_comment( ) %{{{
{
	forever 
	{	
		bskip_chars (" \t\n");
		if( bobp( ) )
		  return;
		
		push_mark( );
		while( up_1( ) )
		{	
			go_down_1( );
			break;
		}
		
		bol_skip_white( );
		
		if( looking_at( "<?" ) or looking_at( "?>" ) or looking_at( "<%" ) or looking_at( "%>" ))
		{
			pop_mark_0( );
			continue;
		}
		pop_mark_1( );
		
		!if( blooking_at( "*/" ))
		{
			push_mark( );
			variable found = 0;
			forever
			{
				if( bfind( "//" ) or bfind( "#" ))
				{
					found = 1;
				} else {
					break;
				}
			}
			
			if( 0 == parse_to_point( ) and found != 0 )
			{
				% Not in a comment or string
				pop_mark_0( );
				continue;
			}
			
			bol( );
			!if( bobp( ) )
			{
				if( looking_at( "<?" ) or looking_at( "?>" ) or looking_at( "<%" ) or looking_at( "%>" ))
				{
					pop_mark_0( );
					continue;
				}
			}			
			pop_mark_1( );
			break;
		}
		!if( bsearch( "/*" )) break;
	}
}
%}}}
private define php_looking_at( token ) %{{{
{
	variable cse = CASE_SEARCH, ret = 0;
	CASE_SEARCH = 1;
	
	if( looking_at( token ))
	{
		push_spot( );
		go_right( strlen( token ));
		_get_point( );
		skip_chars( "\t :({" );
		ret = ( _get_point( ) - ( )) or eolp( );
		pop_spot( );
	}
	CASE_SEARCH = cse;
	ret;
}
%}}}
private define php_indent_to( n ) %{{{
{
	bol( );
	% Force a reindent if the line does not contain tabs followed by spaces.
	skip_chars( "\t" );
	skip_chars( " " );
	
	if( ( what_column != n )
		or ( _get_point( ) != ( skip_white( ), _get_point( ))))
	{
		bol_trim( );
		n--;
		whitespace( n );
	}
}
%}}}
private define php_indent_continued_comment( col ) %{{{
{
	push_spot( );
	col++;			       %  add 1 so the we indent under * in /*
	php_indent_to( col );
	
	if( looking_at( "*" )
		or not( eolp( ) ))
	  pop_spot( );
	else
	{
		insert( "* " );
		pop_spot( );
		
		if( what_column( ) <= col )
		{
			goto_column( col + 2 );
		}
	}
}
%}}}
private define php_mode_if_bol_skip_white( ) %{{{
{
	push_mark( );
	bskip_white( );
	1;
	if( bolp( ) )
	{
		pop( );
		skip_white( );
		0;
	}
	pop_mark( ( ) );		       %  take argument from stack
}
%}}}
%#iftrue
% Return true if the spot is inside of a class definition
% Takes the opening brace of the enclosing block as an
% argument.
private define inside_class( bra ) %{{{
{
	push_spot( );
	EXIT_BLOCK
	{
		pop_spot( );
	}
	
	goto_user_mark( bra );
	
	% Assume that class is at the beginning of a line.  We may want to
	% change this assumption later.
	while( re_bsearch( "^\\c[ \t]*\\<class\\>" ))
	{
		if( 0 == parse_to_point( ) )
		{
			while( fsearch( "{" ))
			{
				if( 0 != parse_to_point( ) )
				{
					go_right_1( );
					continue;
				}
				
				if ( bra == create_user_mark( ) )
				  return 1;
				break;
			}
			return 0;
		}
		
		!if( left( 1 ))
		  break;
	}
	
	return 0;
} %}}}
%#endif
define php_indent_line( ) %{{{
{	
	variable val, col, extra_indent = 0;
	variable prep_line = 0;
	variable match_char, match_indent, this_char, match_line;
	variable match_mark;
	variable is_continuation = 0;
	
	% Check whetever we are in a php block or not
	if( php_in_block( ) )
	{
		push_spot( );
		bol_skip_white( );
		
		% Store the character we are standing on
		this_char = what_char( );
		if( -2 == parse_to_point( ) )
		{
			% In a c comment.  Indent it at level of matching /* string
			( ) = bsearch( "/*" );
			col = what_column( );
			pop_spot( );
			php_indent_continued_comment( col );
			php_mode_if_bol_skip_white( );
			return;
		}
		
		EXIT_BLOCK
		{
			php_mode_if_bol_skip_white( );
		}
		
		if( orelse
		  { php_looking_at( "case" ) }
			{ php_looking_at( "default" ) }
			)
		{
			if( ffind_char( ':' ))
			{
				extra_indent -= PHP_INDENT;
				extra_indent += PHP_Colon_Offset;
				%message(string(extra_indent));
			}
			bol( );
		} else {
			forever
			{
				php_bskip_over_comment( );
				!if( orelse
				   { blooking_at( ";" ) }
					 { blooking_at( "{" ) }
					 { blooking_at( "}" ) }
					 { blooking_at( ")," ) }
					 { blooking_at( "}," ) }
					 { blooking_at( ":" ) }
					 { bobp( ) }
					 )
				{	
					% This needs to be here to make sure were still in the phpblock
					if( php_in_block( ) )
					{
						% message("hej2");
						if( is_continuation )
						{
							% message("hej");
							extra_indent += PHP_CONTINUED_OFFSET;
						} 
						else 
						{
							% message("hej3");
							push_spot( );
							bol_skip_white( );
							% fsearch( "{" );
							% !if( blooking_at( ")" )
							extra_indent += PHP_CONTINUED_OFFSET;
							pop_spot( );
						}
					
						% extra_indent += PHP_CONTINUED_OFFSET;					
						is_continuation++;
						% is_continuation++;
					}
				}
				
				!if( blooking_at( ")" ))
				  break;
				push_mark( );
				go_left_1( );
				if( 1 != find_matching_delimiter( ')' ))
				{
					pop_mark_1( );
					break;
				}
				
				php_bskip_over_comment( );
				
				push_spot( );
				if( ( 1 == find_matching_delimiter( ')' )), pop_spot( ) )
				{
					pop_mark_1( );
					break;
				}
				
				pop_mark_0( );
				bol ( );
			}
		}
		
		val = find_matching_delimiter( ')' );
		match_mark = create_user_mark( );
		
		match_char = what_char( );
		match_line = what_line( );
		
		if( ( val < 0 ) and looking_at( "/*" ))
		  val = -2;
		else if( val == 1 )
		{
			go_right( 1 );
			skip_white( );
		}
		
		col = what_column( );

		bol_skip_white( );
		match_indent = what_column( );
		if( what_line( ) < prep_line )
		{
			match_char = 0;
		}
		
		pop_spot( );
		
		switch( val )
		{
		 case 0:			       %  mismatch
			if( match_char == '{' )
			{
				push_spot( );
				goto_user_mark( match_mark );
				
				bskip_chars( "\n\t " );				
				if( blooking_at( ")" ))
				{
					variable same_line = ( what_line == match_line );
					
					go_left_1( );
					if( 1 == find_matching_delimiter( ')' ))
					{
						bol_skip_white( );
						
						if( same_line )
						  match_indent = what_column( );
						
						% NOTE: This needs work.
						if( ( this_char != '}' )
							and looking_at( "switch" ))
						  match_indent += PHP_Switch_Offset;
					}
				}
				
				pop_spot( );
				col = match_indent;
#ifexists PHP_Class_Offset
				if( this_char == '}' )
				  col += PHP_INDENT;
				else if( inside_class( match_mark ))
				  col += PHP_Class_Offset;
				else
				  col += PHP_INDENT;
#else
				col += PHP_INDENT;
#endif
			} else if( match_char == '[' ) {
				push_spot( );
				php_indent_to( col + 1 );
				pop_spot( );
				return;
			} else {
				push_spot( );
				bol_skip_white( );
				if( looking_at_char( '{' ))
				  extra_indent = PHP_BRACE;
				extra_indent++;
				php_indent_to( extra_indent );
				pop_spot( );				
				return;
			}
		}
		{
		case 1:
			extra_indent = 0;	       %  match found
		}
		{
		case -2:			       %  inside comment
			if( this_char != '\\' ) 
			  col++;
			php_indent_continued_comment( col );
			return;
		}
		{
		 case 2:
			push_spot_bol( );
			trim( );
			pop_spot( );
			return;
		}
		switch( this_char )
		{
		 case '}':
			col -= PHP_INDENT;
		}
		{
		 case '{':			
			col += PHP_BRACE;
			if( is_continuation )
			  col -= PHP_CONTINUED_OFFSET;
			col += extra_indent;
		}
		{
			col += extra_indent;
		}
		
		push_spot( );
		php_indent_to( col );
		pop_spot( );
	}
}
%}}}
define php_indent_region_or_line( ) %{{{
{
	!if( is_visible_mark )
        {	
		if( php_in_block( ) )
		  php_indent_line( );
		else
		  insert( "\t" );
	}
	else
	{
		variable now,start,stop;
		check_region( 1 );
		stop = what_line( );
		pop_mark_1( );
		start = what_line( );
		push_mark( );
		forever
		{
			now = what_line( );
			if( now >= stop )
			  break;
			php_indent_line( );
			down_1( );
		}
		pop_spot( );
	}
}
%}}}
define php_indent_buffer( ) %{{{
{
	variable col, max_line;
	push_spot( );
	eob( );
	max_line = what_line( );
	bob( );
	do
	{
		bol_skip_white( );
		indent_line( );
	} while( down_1( ) );
	
	trim_buffer( );
	flush( sprintf( "processed %d/%d lines.", what_line( ), max_line ));
	pop_spot( );
}
%}}}
define php_newline_and_indent( ) %{{{
{	
	variable PhpCcComment = "//";
	variable PhpBashComment = "#";
	
	if( bolp ( ) )
	{
		newline( );
		php_indent_line( );
		return;
	}
	
	variable col;
	variable PhpCcComment_len = strlen( PhpCcComment );
	variable PhpBashComment_len = strlen( PhpBashComment );
	
	if( PHP_Autoinsert_Comments )
	{
		col = what_column( );
		push_spot_bol( );
		if( looking_at( PhpCcComment ))
		{
			push_mark( );
			go_right( PhpCcComment_len );
			skip_white( );
			PhpCcComment = bufsubstr( );
			pop_spot( );
			newline( );
			if( col > PhpCcComment_len ) insert( PhpCcComment );
			return;
		} else if( looking_at( PhpBashComment )) {
			push_mark( );
			go_right( PhpBashComment_len );
			skip_white( );
			PhpBashComment = bufsubstr( );
			pop_spot( );
			newline( );
			if( col > PhpBashComment_len ) insert( PhpBashComment );
			return;
		}		  
		pop_spot( );
	}
	
	col = php_is_comment( );
	newline( );
	if( col )
	{
		php_indent_to( col );
		insert( "" );
	}
	else php_indent_line( );
}
%}}}
define php_insert_bra( ) %{{{
{
	if( php_parse_to_point( ) )
	  insert_char( '{' );
	else {
		push_spot( );
		php_bskip_over_comment( 0 );
		if( blooking_at( "," ), pop_spot( ) )
		{
			insert_char( '{' );
		} else { 
			push_spot( );
			skip_white( );
			if( eolp( ) )
			{
				bskip_white( );
				if( not( bolp( ) ) and PHP_BRA_NEWLINE, pop_spot( ) ) 
				  newline( );
				push_spot( );
				bskip_white( );
				bolp( );	       %  on stack
				pop_spot( );
				insert_char( '{' );
				if( ( ) ) 
				  php_indent_line( );   %  off stack
				eol( );
				if( PHP_BRA_NEWLINE ) 
				  php_newline_and_indent( );
			} else  {
				pop_spot( );
				insert_char( '{' );
			}
		}
	}
}
%}}}
define php_insert_ket( ) %{{{
{
	variable status = php_parse_to_point( );
	variable line = what_line( );
	
	push_spot( );
	skip_white( );
	push_spot( );
	if( status 
		or not( eolp( ) )
		or( 1 == find_matching_delimiter( '}' )) and( line == what_line( ) ))
		%or (bol_skip_white ( ), looking_at_char ('{')), pop_spot ( ))
	{
		pop_spot( );
		pop_spot( );
		if( PHP_KET_NEWLINE )
		{
			insert( "\n}" );
			php_indent_line( );
		}
		else
		  insert( "}" );
		blink_match( );
		return;
	}
	
	pop_spot( );
	bskip_white( );
	if( bolp( ), pop_spot( ) )
	{
		insert_char( '}' );
		trim( );
	} else {
		eol( );
		if( PHP_KET_NEWLINE )
		  insert( "\n}" );
		else
		  insert( "}" );
	}
	php_indent_line( );
	eol( ); 
	blink_match( );
	if( PHP_BRA_NEWLINE )
	  php_newline_and_indent( );
}
%}}}
define php_insert_colon( ) %{{{
{
	insert_char( ':' );
	!if( php_parse_to_point( ) )
	  php_indent_line( );
}
%}}}
define php_getname( tellstring ) %{{{
{
	variable gname = read_mini( tellstring, Null_String, Null_String );
	return gname;
}
%}}}
define php_ins_tn( str ) %{{{
{
	insert( str );
	php_indent_line( );
	insert( "\n" );
}
%}}}
define php_insert_function( ) %{{{
{
	variable name = php_getname( "function:" );
	php_ins_tn( sprintf( "function %s ( )", name ));
	php_ins_tn( "{" );
	php_ins_tn( "" );	
	php_ins_tn( "}" );
	bsearch( ")" );	
}
%}}}
define php_insert_class( ) %{{{
{
	variable name = php_getname( "class:" );
	php_ins_tn(sprintf( "class %s", name ));
	php_ins_tn( "{" );
	php_ins_tn( "" );	
	php_ins_tn( "}" );	
}
%}}}
define php_insert_tab( ) %{{{
{
	insert( "\t" );
}
%}}}
private define php_init_menu( menu ) %{{{
{
	menu_append_item( menu, "&Top of function", "php_top_of_function" );
	menu_append_item( menu, "&End of function", "php_end_of_function" );
	menu_append_item( menu, "&Mark function", "php_mark_function" );
	menu_append_item( menu, "&Mark matching", "php_mark_matching" );
	menu_append_separator( menu );
	menu_append_item( menu, "&Indent buffer", "php_indent_buffer" );
	menu_append_separator( menu );
	menu_append_item( menu, "&Insert class", "php_insert_class" );
	menu_append_item( menu, "&Insert function", "php_insert_function" );
	menu_append_item( menu, "&Insert brace", "php_insert_bra" );
	menu_append_item( menu, "&Insert ket", "php_insert_ket" );
	menu_append_item( menu, "&Insert colon", "php_insert_colon" );
	menu_append_separator( menu );
	menu_append_item( menu, "&Format paragraph", "php_format_paragraph" );
	menu_append_item( menu, "&Goto Match", "goto_match" );
	menu_append_item( menu, "&Insert TAB", "php_insert_tab" );
}
%}}}
$1 = "PHP";

!if( keymap_p( $1 )) 
  make_keymap( $1 ); %{{{
definekey( "indent_line", "\t", $1 );
definekey( "php_top_of_function", "\e^A", $1 );
definekey( "php_end_of_function", "\e^E", $1 );
definekey( "php_mark_function", "\e^H", $1 );
definekey( "php_mark_matching", "\e^M", $1 );
definekey( "php_insert_bra", "{", $1 );
definekey( "php_insert_ket", "}", $1 );
definekey( "php_insert_colon", ":", $1 );
definekey( "php_format_paragraph", "\eq", $1 );
definekey( "php_newline_and_indent", "\r", $1 );

definekey_reserved( "php_indent_region", "^R", $1 );
definekey_reserved( "php_indent_buffer", "^B", $1 );
definekey_reserved( "php_insert_class", "^C", $1 );
definekey_reserved( "php_insert_function", "^F", $1 );
definekey_reserved( "php_insert_tab","^I", $1 );
%}}}

% Now create and initialize the syntax tables. %{{{
create_syntax_table( $1 );
define_syntax( "/*", "*/", '%', $1 );          % comments
define_syntax( "#", "", '%', $1 );             % comments
define_syntax( "//", "", '%', $1 );            % comments
%define_syntax ("<>", '<', $1);
define_syntax( "([{", ")]}", '(', $1 );        % parentheses
define_syntax( '"', '"', $1 );                 % strings
define_syntax( '\'', '\'', $1 );               % strings
define_syntax( '\\', '\\', $1 );               % escape character
define_syntax( "0-9a-zA-Z_", 'w', $1 );        % words
define_syntax( "-+0-9a-fA-F.xXL", '0', $1 );   % numbers
define_syntax( ",;.:", ',', $1 );              % delimiters
define_syntax( "+-*/%=.&|^~<>!?@`", '+', $1 ); % operators
set_syntax_flags( $1, 0x05 );
%}}}

#ifdef HAS_DFA_SYNTAX %{{{
%%% DFA_CACHE_BEGIN %%%
private define setup_dfa_callback( name )
{
	dfa_enable_highlight_cache( "php.dfa", name );
	dfa_define_highlight_rule( "<%", "Qpreprocess", name );          % Asp style start tag
	dfa_define_highlight_rule( "%>", "Qpreprocess", name );          % Asp style end tag
	dfa_define_highlight_rule( "<\\?|<\\?php", "preprocess", name ); % Php style start tag 
	dfa_define_highlight_rule( "\\?>", "Qpreprocess", name ); % Php style end tag
	dfa_define_highlight_rule ("<!\\-\\-.*\\-\\-[ \t]*>", "Qcomment", name); % HTML comments
	dfa_define_highlight_rule ("<!\\-\\-", "comment", name); % HTML comments
	dfa_define_highlight_rule ("\\-\\-[ \t]*>", "comment", name); % HTML comments
	dfa_define_highlight_rule( "//.*", "comment", name );            % C++ style comment
	dfa_define_highlight_rule( "#.*", "comment", name );             % Shell style comment
	dfa_define_highlight_rule( "/\\*.*\\*/", "Qcomment", name );     % C style comment
	dfa_define_highlight_rule( "^([^/]|/[^\\*])*\\*/", "Qcomment", name ); % C style comment
	dfa_define_highlight_rule( "/\\*.*", "comment", name );          % C style comment
	dfa_define_highlight_rule( "^[ \t]*\\*+([ \t].*)?$", "comment", name ); % C style comment
	dfa_define_highlight_rule( "[A-Za-z_\\$][A-Za-z_0-9\\$]*", "Knormal", name );
	dfa_define_highlight_rule( "[ \t]+", "normal", name );
	dfa_define_highlight_rule( "[0-9]+(\\.[0-9][LlUu]*)?([Ee][\\+\\-]?[0-9]*)?","number", name );
	dfa_define_highlight_rule( "0[xX][0-9A-Fa-f]*[LlUu]*", "number", name );
	dfa_define_highlight_rule( "[\\(\\[{}\\]\\),;\\.:]", "delimiter", name );
	dfa_define_highlight_rule( "[%@\\?\\.\\-\\+/&\\*=<>\\|!~\\^]", "operator", name );
	dfa_define_highlight_rule( "\"([^\"\\\\]|\\\\.)*\"", "string", name );
	dfa_define_highlight_rule( "\"([^\"\\\\]|\\\\.)*\\\\?$", "string", name );
	dfa_define_highlight_rule( "'([^'\\\\]|\\\\.)*'", "string", name );
	dfa_define_highlight_rule( "'([^'\\\\]|\\\\.)*\\\\?$", "string", name );
	dfa_build_highlight_table( name );
}
dfa_set_init_callback( &setup_dfa_callback, $1 );
%%% DFA_CACHE_END %%%
#endif
%}}}

% Type 0 keywords (keywords and constants) %{{{
() = define_keywords_n ($1,
"asdoifor",
2,0);

() = define_keywords_n ($1,
"andfornewvarxor",
3,0);

() = define_keywords_n ($1,
"argcargvcaseelsetrue",
4,0);

() = define_keywords_n ($1,
"breakclassfalsewhile",
5,0);

() = define_keywords_n ($1,
"elseifglobalphp_osswitch",
6,0);

() = define_keywords_n ($1, strcat(
"defaulte_errore_parseextendsforeach",
"globals"
), 7,0);

() = define_keywords_n ($1, strcat(
"continuee_notice__file__function",
"__line__php_self"
),8,0);

() = define_keywords_n ($1,
"e_warning",
9,0);

() = define_keywords_n ($1,
"php_version",
11,0);

() = define_keywords_n ($1,
"php_errormsg",
12,0);

() = define_keywords_n ($1,
"http_get_vars",
13,0);

() = define_keywords_n ($1,
"http_post_vars",
14,0);

() = define_keywords_n ($1,
"http_cookie_vars",
16,0);
%}}}

% Type 1 keywords (functions) %{{{
() = define_keywords_n ($1,
"dlpi",
2,1);

() = define_keywords_n ($1, strcat(
"abschrcosdieendexpkeylogmaxmd5minordpos",
"powsintan"
),3,1);

() = define_keywords_n ($1, strcat(
"acosasinatanbindceilchopcopydateeach",
"echoeregevalexecexitfeoffilejoinleaklink",
"listmailmsqlnextpackprevrandreadsortsqrt",
"stattimetrim"
),4,1);

() = define_keywords_n ($1, strcat(
"arrayasortatan2bcaddbcdivbcmodbcmul",
"bcpowbcsubchdirchgrpchmodchownclosecount",
"crc32cryptemptyeregifgetcfgetsflockfloor",
"flushfopenfputsfreadfseekfstatftellgzeof",
"hw_cphw_mviconvissetksortlog10lstatltrim",
"mhashmkdirnl2brpopenprintrangeresetrmdir",
"roundrsortrtrimsleepsplitsrandstrtrtouch",
"umaskunsetusortwrite"
),5,1);

() = define_keywords_n ($1, strcat(
"arsortassertbccompbcsqrtbindecbzopen",
"bzreadchrootdblistdecbindechexdecoctdefine",
"deletefclosefflushfgetssfscanffwritegetcwd",
"getenvgmdategmp_orgzfilegzgetcgzgetsgzopen",
"gzputsgzreadgzseekgztellheaderhebrevhexdec",
"hw_whoifx_dointvalis_diris_intkrsortlisten",
"mktimeoctdecora_dopclosepg_ttyprintfputenv",
"recoderenamerewindsizeofsocketsplitisscanf",
"strchrstrcmpstrlenstrposstrrevstrspnstrstr",
"strtokstrvalsubstrsyslogsystemuasortuksort",
"uniqidunlinkunpackusleepxmldoc"
),6,1);

() = define_keywords_n ($1, strcat(
"bcscalebin2hexbzclosebzerrnobzerror",
"bzflushbzwritecom_getcompactcom_setconnect",
"currentdbmopendefineddeg2raddirnameexplode",
"extractfgetcsvfileproftp_getftp_putftp_pwd",
"getdategetmxrrgettextgettypegmp_absgmp_add",
"gmp_andgmp_cmpgmp_divgmp_gcdgmp_modgmp_mul",
"gmp_neggmp_powgmp_subgmp_xorgzclosegzgetss",
"gzwritehebrevchw_infohw_rootimagesximagesy",
"implodeincludeini_getini_setip2longis_bool",
"is_fileis_linkis_longis_nullis_reallong2ip",
"mt_randnatsortodbc_doopendiropenlogpdf_arc",
"pdf_newpg_execpg_hostpg_portphpinfoprint_r",
"rad2degreaddirrequiresem_getsettypeshuffle",
"snmpgetsnmpsetsoundexsprintfstrcollstrcspn",
"stristrstrncmpstr_padstrrchrstrrposswffill",
"swffontswftextsymlinktempnamtmpfileucfirst",
"ucwordsvirtualxmltreeyp_next"
),7,1);

() = define_keywords_n ($1, strcat(
"basenamebzerrstrclosedircloselog",
"com_loadconstantcpdf_arcdba_opendba_sync",
"dbmclosedbmfetchdbx_sortdgettextfdf_open",
"fdf_savefilesizefiletypefloatvalftp_cdup",
"ftp_fgetftp_fputftp_mdtmftp_pasvftp_quit",
"ftp_siteftp_sizegetmypidgetmyuidgmmktime",
"gmp_factgmp_initgmp_powmgmp_signgmp_sqrt",
"gzencodegzrewindhw_closehw_errorhw_mapid",
"imagearcimagegifimagepngimap_uidin_array",
"ircg_msgis_arrayis_floatjdtounixldap_add",
"linkinfomt_srandob_startocierrorocifetch",
"ocilogonociparseora_bindora_execora_open",
"passthrupathinfopdf_arcnpdf_clippdf_fill",
"pdf_openpdf_rectpdf_savepdf_showpdf_skew",
"pg_closepg_tracereadfilereadlinereadlink",
"realpathsnmpwalkstrerrorstrftimeswfmorph",
"swfmovieswfshapeudm_findunixtojdvar_dump",
"wordwrapxslt_runyaz_hitsyaz_scanyaz_sort",
"yaz_waityp_firstyp_matchyp_orderzip_open",
"zip_read"
),8,1);

() = define_keywords_n ($1, strcat(
"array_maparray_padarray_poparray_sum",
"checkdatecpdf_clipcpdf_fillcpdf_opencpdf_rect",
"cpdf_savecpdf_showcpdf_textcurl_execcurl_init",
"dba_closedba_fetchdba_popendbmdeletedbmexists",
"dbminsertdbx_closedbx_errordbx_querydcgettext",
"doublevalerror_logfdf_closefileatimefilectime",
"filegroupfileinodefilemtimefileownerfileperms",
"fpassthrufsockopenftp_chdirftp_loginftp_mkdir",
"ftp_nlistftp_rmdirftruncatefunctionsget_class",
"getrusagegmp_div_qgmp_div_rgmp_scan0gmp_scan1",
"gzdeflategzinflatehw_insdochw_unlockicap_open",
"ifx_closeifx_errorifx_queryimagecharimagecopy",
"imagefillimagejpegimagelineimagewbmpimap_8bit",
"imap_bodyimap_mailimap_openimap_pingimap_sort",
"imap_utf8ini_alteriptcparseircg_joinircg_kick",
"ircg_nickircg_partis_doubleis_objectis_scalar",
"is_stringlcg_valueldap_bindldap_listldap_read",
"localtimemb_strcutmb_strlenmb_strposmb_substr",
"mcal_openmetaphonemicrotimeocicancelocicommit",
"ocilogoffocinlogonociplogonociresultodbc_exec",
"ora_closeora_errorora_fetchora_logonora_parse",
"parse_strparse_urlpdf_closepdf_scalepg_dbname",
"pg_loopenpg_loreadpg_resultphp_unamepreg_grep",
"quotemetarewinddirserializesetcookiesetlocale",
"strnatcmpstrtotimeswfactionswfbitmapswfbutton",
"swf_orthoswf_scaleswfspriteudm_errnoudm_error",
"urldecodeurlencodexml_parsexslt_freeyaz_close",
"yaz_errnoyaz_erroryaz_rangeyp_masterzip_close"
),9,1);

() = define_keywords_n ($1, strcat(
"addslashesarray_diffarray_flip",
"array_keysarray_pusharray_randarray_walk",
"aspell_newbzcompresscheckdnsrrcom_addref",
"com_invokecpdf_closecpdf_scalecurl_close",
"dba_deletedba_existsdba_insertdbase_open",
"dbase_packdbmnextkeydbmreplacedbplus_add",
"dbplus_aqldbplus_sqldbplus_tclezmlm_hash",
"fdf_createfdf_set_apfrenchtojdftp_delete",
"ftp_renamegetlastmodgetmyinodegetrandmax",
"gmp_clrbitgmp_div_qrgmp_gcdextgmp_intval",
"gmp_invertgmp_jacobigmp_randomgmp_setbit",
"gmp_sqrtrmgmp_strvalgmstrftimegzcompress",
"gzpassthruhw_connecthw_gettexthw_inscoll",
"icap_closeimagetypesimap_checkimap_close",
"imap_msgnoircg_topicircg_whoisis_integer",
"is_numericjdtofrenchjdtojewishjdtojulian",
"jewishtojdjuliantojdldap_closeldap_errno",
"ldap_errorlocaleconvmb_strrposmcal_close",
"mcal_popenmcrypt_cbcmcrypt_cfbmcrypt_ecb",
"mcrypt_ofbmsql_closemsql_errormsql_query",
"ocicollmaxociexecuteociloadlobocinumcols",
"ocisavelobodbc_closeodbc_errorora_commit",
"ora_logoffora_plogonpdf_circlepdf_concat",
"pdf_deletepdf_linetopdf_movetopdf_rotate",
"pdf_strokepfpro_initpfsockopenpg_connect",
"pg_loclosepg_lowritepg_numrowspg_options",
"pg_untracephpcreditsphpversionposix_kill",
"preg_matchpreg_quotepreg_splitpspell_new",
"readgzfilesem_removesession_idshm_attach",
"shm_detachshmop_openshmop_readshmop_size",
"shm_removestrcasecmpstrip_tagsstr_repeat",
"strtolowerstrtoupperswf_lookatswf_nextid",
"swf_ortho2swf_rotatetextdomainuser_error",
"xmldocfilexpath_evalxslt_errnoxslt_error",
"yaz_recordyaz_searchyaz_syntax"
),10,1);

() = define_keywords_n ($1, strcat(
"addcslashesapache_notearray_merge",
"array_shiftarray_slicechunk_splitcom_propget",
"com_propputcom_propsetcom_releasecount_chars",
"cpdf_circlecpdf_linetocpdf_movetocpdf_rotate",
"cpdf_strokectype_alnumctype_alphactype_cntrl",
"ctype_digitctype_graphctype_lowerctype_print",
"ctype_punctctype_spacectype_uppercurl_setopt",
"dba_nextkeydba_replacedbase_closedbmfirstkey",
"dbplus_currdbplus_finddbplus_infodbplus_last",
"dbplus_nextdbplus_opendbplus_prevdbplus_rzap",
"dbplus_undodbx_comparedbx_connectdebugger_on",
"domxml_rooteaster_dateeaster_daysfbsql_close",
"fbsql_errnofbsql_errorfbsql_queryfdf_set_opt",
"file_existsftp_connectftp_rawlistftp_systype",
"get_browserget_cfg_vargmp_hamdisthw_children",
"hw_edittexthw_errormsghw_identifyhw_pconnect",
"ibase_closeibase_queryibase_transicap_snooze",
"ifx_connectifx_prepareimagecharupimagecreate",
"imagepsbboximagepstextimagestringimap_alerts",
"imap_appendimap_base64imap_binaryimap_delete",
"imap_errorsimap_headerimap_qprintimap_reopen",
"imap_searchimap_statusini_get_allini_restore",
"ircg_noticeis_readableis_resourceis_writable",
"jddayofweekjdmonthnameldap_deleteldap_dn2ufn",
"ldap_get_dnldap_modifyldap_renameldap_search",
"ldap_unbindlevenshteinmb_languagemb_strwidth",
"mcal_reopenmcal_snoozemhash_countmsql_dbname",
"msql_dropdbmsql_resultmssql_closemssql_query",
"mysql_closemysql_errnomysql_errormysql_query",
"natcasesortocicollsizeocicolltrimocifreedesc",
"ocirollbackocirowcountodbc_commitodbc_cursor",
"odbc_resultodbc_tablesora_numcolsora_numrows",
"pdf_curvetopdf_endpathpdf_restorepdf_setdash",
"pdf_setflatpdf_setfontpdf_setgraypdf_show_xy",
"pg_end_copypg_fieldnumpg_locreatepg_loexport",
"pg_loimportpg_lounlinkpg_pconnectpg_put_line",
"posix_timesposix_unamerecode_filesem_acquire",
"sem_releasesesam_queryshm_get_varshmop_close",
"shmop_writeshm_put_varshow_sourcesnmpwalkoid",
"sql_regcasestrncasecmpstr_replaceswfgradient",
"swf_setfontunserializeutf8_decodeutf8_encode",
"xslt_createyaz_addinfoyaz_connectyaz_element",
"yaz_present"
),11,1);

() = define_keywords_n ($1, strcat(
"array_filterarray_reducearray_search",
"array_splicearray_uniquearray_valuesascii2ebcdic",
"aspell_checkbase_convertbzdecompressclass_exists",
"cpdf_curvetocpdf_newpathcpdf_restorecpdf_rlineto",
"cpdf_rmovetocpdf_setdashcpdf_setflatcpdf_setgray",
"cpdf_show_xyctype_xdigitcurl_versiondba_firstkey",
"dba_optimizedbase_createdbplus_chdirdbplus_close",
"dbplus_errnodbplus_firstdbplus_flushdbplus_rkeys",
"dbplus_ropendebugger_offebcdic2asciiereg_replace",
"fbsql_commitfbsql_resultfdf_get_filefdf_set_file",
"func_get_arggetimagesizegettimeofdaygmp_divexact",
"gmp_legendregmp_popcountgzuncompressheaders_sent",
"htmlentitieshw_getobjecthw_getremoteibase_commit",
"ibase_errmsgifx_errormsgifx_get_blobifx_get_char",
"ifx_getsqlcaifx_num_rowsifx_pconnectimagecolorat",
"imagedestroyimageellipseimagepolygonimagesettile",
"imagettfbboximagettftextimap_expungeimap_headers",
"imap_num_msginclude_onceingres_closeingres_query",
"is_writeableldap_compareldap_connectldap_err2str",
"ldap_mod_addldap_mod_delmb_parse_strmb_send_mail",
"mcal_expungemsql_connectmsql_drop_dbmsql_listdbs",
"msql_numrowsmsql_regcasemssql_resultmysql_result",
"ob_end_cleanob_end_flushob_gzhandlerocifetchinto",
"ocinewcursorodbc_binmodeodbc_columnsodbc_connect",
"odbc_executeodbc_prepareold_functionopenssl_open",
"openssl_sealopenssl_signora_commitonora_rollback",
"ovrimos_execpdf_add_notepdf_end_pagepdf_findfont",
"pdf_get_fontpdf_open_gifpdf_open_pdipdf_open_png",
"pdf_setcolorpdf_set_fontpdf_set_infopg_cmdtuples",
"pg_fetch_rowpg_fieldnamepg_fieldsizepg_fieldtype",
"pg_loreadallpg_numfieldsposix_getcwdposix_getgid",
"posix_getpidposix_getsidposix_getuidposix_isatty",
"posix_mkfifoposix_setgidposix_setsidposix_setuid",
"preg_replaceprinter_listprinter_openpspell_check",
"rawurldecoderawurlencoderequire_oncesesam_commit",
"session_nameshmop_deletesimilar_textstripslashes",
"substr_countswf_addcolorswf_endshapeswf_fontsize",
"swf_getframeswf_mulcolorswf_openfileswf_posround",
"swf_setframeswf_shapearcswftextfieldswf_viewport",
"sybase_closesybase_queryudm_cat_listudm_cat_path",
"udm_free_resxslt_openlogxslt_processyaz_ccl_conf",
"yaz_databasezend_version"
),12,1);

() = define_keywords_n ($1, strcat(
"array_reversearray_unshiftbase64_decode",
"base64_encodecpdf_end_textcpdf_finalizecpdf_set_font",
"dbplus_rquerydbplus_updatediskfreespaceeregi_replace",
"fbsql_connectfbsql_drop_dbfbsql_stop_dbfdf_get_value",
"fdf_set_flagsfdf_set_valuefunc_get_argsfunc_num_args",
"getallheadersgethostbyaddrgethostbynameget_meta_tags",
"getservbynamegetservbyportgregoriantojdhw_getanchors",
"hw_getandlockhw_getparentsibase_connectibase_execute",
"ibase_prepareibase_timefmtifx_copy_blobifx_fetch_row",
"ifx_free_blobifx_free_charimagecolorsetimageloadfont",
"imagesetbrushimagesetpixelimagesetstyleimagestringup",
"imap_undeleteingres_commitircg_pconnectis_executable",
"jdtogregorianmb_http_inputmb_strimwidthmethod_exists",
"msql_createdbmsql_fieldlenmsql_list_dbsmsql_num_rows",
"msql_pconnectmsql_selectdbmssql_connectmt_getrandmax",
"mysql_connectmysql_db_namemysql_drop_dbnumber_format",
"ob_get_lengthocibindbynameocicollassignocicolumnname",
"ocicolumnsizeocicolumntypeocifreecursorodbc_errormsg",
"odbc_num_rowsodbc_pconnectodbc_rollbackora_commitoff",
"ora_errorcodeora_getcolumnovrimos_closepdf_closepath",
"pdf_close_pdipdf_get_valuepdf_open_filepdf_open_jpeg",
"pdf_open_tiffpdf_setmatrixpdf_set_valuepdf_translate",
"pfpro_cleanuppfpro_processpfpro_versionpg_freeresult",
"pg_getlastoidphp_logo_guidphp_sapi_nameposix_ctermid",
"posix_getegidposix_geteuidposix_getpgidposix_getpgrp",
"posix_getppidposix_setpgidposix_ttynameprinter_abort",
"printer_closeprinter_writereadline_inforecode_string",
"sesam_connectsesam_execimmsession_startsession_unset",
"stripcslashesstrnatcasecmpswf_closefileswf_endbutton",
"swf_endsymbolswf_fontslantswf_polarviewswf_popmatrix",
"swf_showframeswf_textwidthswf_translatesybase_result",
"trigger_errorwddx_add_varsxslt_closelogyaz_ccl_parse",
"yaz_itemorder"
),13,1);

() = define_keywords_n ($1, strcat(
"accept_connectaspell_suggest",
"assert_optionsbindtextdomaincall_user_func",
"clearstatcachecpdf_closepathcpdf_page_init",
"cpdf_set_titlecpdf_translatecybercash_decr",
"cybercash_encrdbplus_errcodedbplus_getlock",
"dbplus_lockreldbplus_rchpermdbplus_rcreate",
"dbplus_resolvedbplus_rrenamedbplus_runlink",
"dbplus_saveposdbplus_tremovedomxml_dumpmem",
"escapeshellargescapeshellcmdfbsql_db_query",
"fbsql_list_dbsfbsql_num_rowsfbsql_pconnect",
"fbsql_rollbackfbsql_start_dbfbsql_warnings",
"fdf_get_statusfdf_set_statusget_class_vars",
"gethostbynamelgetprotobynamegmp_prob_prime",
"highlight_filehw_childrenobjhw_docbyanchor",
"hw_getusernameibase_pconnectibase_rollback",
"ifx_fieldtypesifx_nullformatifx_num_fields",
"imagecopymergeimagefilledarcimagefontwidth",
"imageinterlaceimagerectangleimap_fetchbody",
"imap_get_quotaimap_mail_copyimap_mail_move",
"imap_set_quotaimap_subscribeingres_connect",
"is_subclass_ofmb_http_outputmcrypt_decrypt",
"mcrypt_encryptmcrypt_genericmsql_create_db",
"msql_data_seekmsql_fetch_rowmsql_fieldname",
"msql_fieldtypemsql_numfieldsmsql_select_db",
"msql_tablenamemssql_num_rowsmssql_pconnect",
"mysql_db_querymysql_list_dbsmysql_num_rows",
"mysql_pconnectocicollgetelemocicolumnscale",
"ocisavelobfileocisetprefetchodbc_close_all",
"odbc_fetch_rowodbc_field_lenodbc_field_num",
"odbc_setoptionopenssl_verifyora_columnname",
"ora_columnsizeora_columntypeora_fetch_into",
"ovrimos_commitovrimos_cursorovrimos_result",
"pdf_begin_pagepdf_get_bufferpdf_open_ccitt",
"pdf_open_imagepdf_setlinecappdf_show_boxed",
"pg_fetch_arraypg_fieldisnullpg_fieldprtlen",
"posix_getgrgidposix_getgrnamposix_getlogin",
"posix_getpwnamposix_getpwuidpreg_match_all",
"pspell_suggestread_exif_datasesam_errormsg",
"sesam_rollbacksesam_seek_rowsession_decode",
"session_encodeset_time_limitshm_remove_var",
"substr_replaceswf_actionplayswf_actionstop",
"swf_definefontswf_definelineswf_definepoly",
"swf_definerectswf_definetextswfdisplayitem",
"swf_labelframeswf_pushmatrixswf_startshape",
"sybase_connectudm_free_agentxml_set_object",
"xslt_transformzend_logo_guidzip_entry_name",
"zip_entry_openzip_entry_read"
),14,1);

() = define_keywords_n ($1, strcat(
"array_intersectarray_multisort",
"cpdf_begin_textcpdf_setlinecapcreate_function",
"dbase_numfieldsdbplus_freelockdbplus_rcrtlike",
"dbplus_setindexdbplus_unselectdbplus_xlockrel",
"domxml_add_rootdomxml_childrenerror_reporting",
"fbsql_create_dbfbsql_data_seekfbsql_db_status",
"fbsql_fetch_rowfbsql_field_lenfbsql_insert_id",
"fbsql_select_dbfbsql_tablenamefunction_exists",
"get_object_varshw_array2objrechw_deleteobject",
"hw_getchildcollhw_insertobjecthw_modifyobject",
"hw_new_documenthw_objrec2arrayhw_pipedocument",
"ibase_fetch_rowifx_create_blobifx_create_char",
"ifx_free_resultifx_update_blobifx_update_char",
"ifxus_free_slobifxus_open_slobifxus_read_slob",
"ifxus_seek_slobifxus_tell_slobimagecolorexact",
"imagedashedlineimagefontheightimagepsfreefont",
"imagepsloadfontimap_headerinfoimap_last_error",
"imap_num_recentingres_num_rowsingres_pconnect",
"ingres_rollbackircg_disconnectircg_ignore_add",
"ircg_ignore_delldap_explode_dnldap_get_option",
"ldap_get_valuesldap_next_entryldap_set_option",
"mb_convert_kanamb_detect_ordermcal_date_valid",
"mcal_event_initmcal_time_validmsql_fieldflags",
"msql_field_seekmsql_fieldtablemsql_freeresult",
"msql_listfieldsmsql_listtablesmsql_num_fields",
"mssql_data_seekmssql_fetch_rowmssql_select_db",
"mysql_create_dbmysql_data_seekmysql_fetch_row",
"mysql_field_lenmysql_insert_idmysql_select_db",
"mysql_tablenameob_get_contentsocicolumnisnull",
"ocidefinebynameodbc_autocommitodbc_fetch_into",
"odbc_field_nameodbc_field_typeodbc_num_fields",
"odbc_proceduresodbc_result_allodbc_statistics",
"ovrimos_connectovrimos_executeovrimos_prepare",
"pdf_add_outlinepdf_add_pdflinkpdf_add_weblink",
"pdf_attach_filepdf_close_imagepdf_end_pattern",
"pdf_fill_strokepdf_place_imagepdf_set_leading",
"pdf_setlinejoinpdf_setpolydashpdf_setrgbcolor",
"pdf_stringwidthpg_errormessagepg_fetch_object",
"posix_getgroupsposix_getrlimitprinter_end_doc",
"sesam_fetch_rowsession_destroyset_file_buffer",
"swf_enddoactionswf_getfontinfoswf_oncondition",
"swf_perspectiveswf_placeobjectswf_shapelineto",
"swf_shapemovetoswf_startbuttonswf_startsymbol",
"sybase_num_rowssybase_pconnectudm_alloc_agent",
"udm_api_versionwddx_packet_endxml_parser_free",
"yaz_scan_resultzip_entry_close"
),15,1);

() = define_keywords_n ($1, strcat(
"aspell_check_rawcall_user_method",
"cpdf_add_outlinecpdf_fill_strokecpdf_import_jpeg",
"cpdf_set_creatorcpdf_set_leadingcpdf_setlinejoin",
"cpdf_setrgbcolorcpdf_set_subjectcpdf_stringwidth",
"cybermut_testmacdbase_add_recorddbase_get_record",
"dbase_numrecordsdbplus_getuniquedbplus_rcrtexact",
"dbplus_rsecindexdbplus_unlockreldisk_total_space",
"domxml_new_childextension_loadedfbsql_autocommit",
"fbsql_field_namefbsql_field_seekfbsql_field_type",
"fbsql_num_fieldsfdf_set_encodingfilepro_retrieve",
"filepro_rowcountget_current_userget_defined_vars",
"get_parent_classgetprotobynumberhighlight_string",
"htmlspecialcharshw_document_sizehw_free_document",
"hw_getanchorsobjhw_getparentsobjhw_incollections",
"ibase_field_infoibase_free_queryibase_num_fields",
"icap_fetch_eventicap_list_alarmsicap_list_events",
"icap_store_eventifxus_close_slobifxus_write_slob",
"imagecolorstotalimagecopyresizedimagepsslantfont",
"imap_fetchheaderimap_listmailboximap_scanmailbox",
"imap_unsubscribeimap_utf7_decodeimap_utf7_encode",
"ingres_fetch_rowircg_html_encodeircg_set_current",
"is_uploaded_fileldap_first_entryldap_free_result",
"ldap_get_entriesldap_mod_replacemcal_day_of_week",
"mcal_day_of_yearmcal_fetch_eventmcal_list_alarms",
"mcal_list_eventsmcal_store_eventmcrypt_create_iv",
"mdecrypt_genericmhash_keygen_s2kmsql_fetch_array",
"msql_fetch_fieldmsql_free_resultmsql_list_fields",
"msql_list_tablesmssql_field_namemssql_field_seek",
"mssql_field_typemssql_num_fieldsmysql_field_name",
"mysql_field_seekmysql_field_typemysql_num_fields",
"ob_iconv_handlerocicolumntyperawocifreestatement",
"ociinternaldebugocinewcollectionocinewdescriptor",
"ociserverversionocistatementtypeodbc_field_scale",
"odbc_foreignkeysodbc_free_resultodbc_gettypeinfo",
"odbc_longreadlenodbc_primarykeysopenssl_free_key",
"ovrimos_num_rowsovrimos_rollbackpdf_add_bookmark",
"pdf_end_templatepdf_get_fontnamepdf_get_fontsize",
"pdf_initgraphicspdf_set_durationpdf_setgray_fill",
"pdf_setlinewidthpdf_set_text_posprinter_draw_bmp",
"printer_draw_pieprinter_end_pagesesam_diagnostic",
"sesam_disconnectsesam_field_namesesam_num_fields",
"session_registerswf_actiongeturlswf_definebitmap",
"swf_fonttrackingswf_modifyobjectswf_removeobject",
"swf_shapecurvetoswf_shapefilloffsybase_data_seek",
"sybase_fetch_rowsybase_select_dbwddx_deserialize",
"xml_error_string"
),16,1);

() = define_keywords_n ($1, strcat(
"apache_lookup_uriconnection_status",
"cpdf_save_to_filecpdf_setgray_fillcpdf_set_keywords",
"cpdf_setlinewidthcpdf_set_text_posdbplus_freerlocks",
"dbplus_restoreposdbplus_xunlockreldomxml_attributes",
"domxml_new_xmldocfbsql_change_userfbsql_fetch_array",
"fbsql_fetch_assocfbsql_fetch_fieldfbsql_field_flags",
"fbsql_field_tablefbsql_free_resultfbsql_list_fields",
"fbsql_list_tablesfbsql_next_resultfilepro_fieldname",
"filepro_fieldtypeget_class_methodsget_resource_type",
"hw_docbyanchorobjhw_insertdocumentibase_free_result",
"icap_delete_eventifx_affected_rowsifx_byteasvarchar",
"ifx_textasvarcharifxus_create_slobignore_user_abort",
"imagecolorclosestimagecolorresolveimagefilltoborder",
"imagegammacorrectimagepsencodefontimagepsextendfont",
"imagesetthicknessimap_getmailboxesimap_mail_compose",
"imap_setflag_fullingres_autocommitingres_field_name",
"ingres_field_typeingres_num_fieldsircg_channel_mode",
"mb_output_handlermcal_append_eventmcal_date_compare",
"mcal_delete_eventmcal_is_leap_yearmcrypt_list_modes",
"msql_fetch_objectmssql_fetch_arraymssql_fetch_field",
"mssql_free_resultmssql_next_resultmysql_change_user",
"mysql_fetch_arraymysql_fetch_assocmysql_fetch_field",
"mysql_field_flagsmysql_field_tablemysql_free_result",
"mysql_list_fieldsmysql_list_tablesob_implicit_flush",
"ocicollassignelemocifetchstatementocifreecollection",
"ociwritelobtofileopenssl_x509_freeopenssl_x509_read",
"ovrimos_fetch_rowovrimos_field_lenovrimos_field_num",
"pdf_add_locallinkpdf_add_thumbnailpdf_begin_pattern",
"pdf_continue_textpdf_get_parameterpdf_get_pdi_value",
"pdf_makespotcolorpdf_open_pdi_pagepdf_setmiterlimit",
"pdf_set_parameterpdf_set_text_risepfpro_process_raw",
"printer_create_dcprinter_delete_dcprinter_draw_line",
"printer_draw_textprinter_start_docpspell_new_config",
"sesam_fetch_arraysesam_field_arraysesam_free_result",
"session_save_pathset_error_handlersocket_get_status",
"swf_getbitmapinfoswf_shapecurveto3swf_startdoaction",
"sybase_field_seeksybase_num_fieldsudm_get_doc_count",
"udm_get_res_fieldudm_get_res_paramwddx_packet_start",
"xml_parser_createxpath_new_contextxslt_fetch_result"
),17,1);

() = define_keywords_n ($1, strcat(
"array_count_valuesconnection_aborted",
"connection_timeoutconvert_cyr_stringcpdf_continue_text",
"cpdf_finalize_pagecpdf_output_buffercpdf_setmiterlimit",
"cpdf_set_text_risedbplus_undopreparefbsql_fetch_object",
"filepro_fieldcountfilepro_fieldwidthget_included_files",
"get_required_filesgmp_perfect_squarehw_getchildcollobj",
"hw_getchilddoccollhw_getsrcbydestobjhw_output_document",
"ibase_fetch_objecticonv_get_encodingiconv_set_encoding",
"ifx_htmltbl_resultimagealphablendingimagecolorallocate",
"imagecopymergegrayimagecopyresampledimagecreatefromgif",
"imagecreatefrompngimagefilledellipseimagefilledpolygon",
"imap_createmailboximap_deletemailboximap_getsubscribed",
"imap_renamemailboxingres_fetch_arrayingres_field_scale",
"ircg_is_conn_aliveldap_count_entriesmb_detect_encoding",
"mcal_days_in_monthmcal_event_set_endmcrypt_generic_end",
"mcrypt_get_iv_sizemcrypt_module_openmove_uploaded_file",
"msql_affected_rowsmssql_fetch_objectmssql_field_length",
"mysql_fetch_objectocicolumnprecisionopenssl_pkcs7_sign",
"openssl_x509_parseovrimos_fetch_intoovrimos_field_name",
"ovrimos_field_typeovrimos_num_fieldsovrimos_result_all",
"pdf_add_annotationpdf_add_launchlinkpdf_begin_template",
"pdf_close_pdi_pagepdf_place_pdi_pagepdf_setgray_stroke",
"pg_client_encodingprinter_create_penprinter_delete_pen",
"printer_draw_chordprinter_get_optionprinter_select_pen",
"printer_set_optionprinter_start_pagepspell_config_mode",
"pspell_config_replsesam_fetch_resultsession_unregister",
"socket_set_timeoutswf_shapefillsolidswf_shapelinesolid",
"sybase_fetch_arraysybase_fetch_fieldsybase_free_result",
"xml_get_error_codezip_entry_filesize"
),18,1);

() = define_keywords_n ($1, strcat(
"cpdf_add_annotationcpdf_setgray_stroke",
"dbase_delete_recorddbplus_freealllocksfbsql_affected_rows",
"fbsql_fetch_lengthsfdf_next_field_nameget_extension_funcs",
"hw_document_bodytaghw_document_contenthw_getobjectbyquery",
"ifx_blobinfile_modeifx_fieldpropertiesimagecolorsforindex",
"imagecreatefromjpegimagecreatefromwbmpimap_clearflag_full",
"imap_fetch_overviewimap_fetchstructureimap_listsubscribed",
"imap_mailboxmsginfoingres_fetch_objectingres_field_length",
"ldap_get_attributesldap_get_values_lenldap_next_attribute",
"mb_convert_encodingmcrypt_generic_initmcrypt_get_key_size",
"mhash_get_hash_namemysql_affected_rowsmysql_escape_string",
"mysql_fetch_lengthsmysql_get_host_infoodbc_specialcolumns",
"ovrimos_free_resultovrimos_longreadlenpdf_get_image_width",
"pdf_open_image_filepdf_set_border_dashpdf_set_text_matrix",
"printer_create_fontprinter_delete_fontprinter_draw_elipse",
"printer_select_fontpspell_new_personalsesam_affected_rows",
"session_module_namesession_write_closesocket_set_blocking",
"swf_actiongotoframeswf_actiongotolabelswf_actionnextframe",
"swf_actionprevframeswf_actionsettargetswf_addbuttonrecord",
"sybase_fetch_objectudm_set_agent_paramwddx_serialize_vars"
),19,1);

() = define_keywords_n ($1, strcat(
"call_user_func_array",
"cpdf_set_text_matrixdbase_replace_record",
"domxml_get_attributedomxml_set_attribute",
"get_declared_classesget_magic_quotes_gpc",
"hw_getremotechildrenimagecolordeallocate",
"imagecolorexactalphaimagecreatetruecolor",
"imagefilledrectangleircg_fetch_error_msg",
"ldap_first_attributemb_convert_variables",
"mb_decode_mimeheadermb_encode_mimeheader",
"mb_internal_encodingmcal_create_calendar",
"mcal_delete_calendarmcal_event_set_alarm",
"mcal_event_set_classmcal_event_set_start",
"mcal_event_set_titlemcal_next_recurrence",
"mcal_rename_calendarmcrypt_enc_self_test",
"mhash_get_block_sizemysql_get_proto_info",
"odbc_field_precisionodbc_tableprivileges",
"openssl_error_stringopenssl_pkcs7_verify",
"pdf_closepath_strokepdf_get_image_height",
"pdf_set_border_colorpdf_set_border_style",
"pdf_set_char_spacingpdf_setrgbcolor_fill",
"pdf_set_word_spacingprinter_create_brush",
"printer_delete_brushprinter_select_brush",
"pspell_clear_sessionpspell_config_create",
"pspell_config_ignorepspell_save_wordlist",
"readline_add_historysesam_settransaction",
"snmp_get_quick_printsnmp_set_quick_print",
"sybase_affected_rowsudm_add_search_limit",
"udm_free_ispell_dataudm_load_ispell_data",
"wddx_serialize_valuexslt_set_sax_handler"
),20,1);

() = define_keywords_n ($1, strcat(
"array_merge_recursive",
"cpdf_closepath_strokecpdf_set_char_spacing",
"cpdf_set_current_pagecpdf_setrgbcolor_fill",
"cpdf_set_word_spacingget_defined_constants",
"get_defined_functionsget_loaded_extensions",
"hw_getchilddoccollobjimagecolortransparent",
"imagecreatefromstringingres_field_nullable",
"mcrypt_get_block_sizemysql_get_client_info",
"mysql_get_server_infoodbc_columnprivileges",
"odbc_procedurecolumnsopenssl_get_publickey",
"openssl_pkcs7_decryptopenssl_pkcs7_encrypt",
"pdf_get_pdi_parameterpdf_open_memory_image",
"pdf_set_horiz_scalingpreg_replace_callback",
"pspell_add_to_sessionreadline_list_history",
"readline_read_historyrestore_error_handler",
"session_cache_limitersession_is_registered",
"xml_parse_into_structxml_parser_get_option",
"xml_parser_set_optionyp_get_default_domain"
),21,1);

() = define_keywords_n ($1, strcat(
"call_user_method_array",
"cpdf_set_horiz_scalinghw_document_attributes",
"hw_document_setcontenthw_getobjectbyqueryobj",
"imagecolorclosestalphaimagecolorresolvealpha",
"ingres_field_precisionmb_preferred_mime_name",
"mcrypt_enc_get_iv_sizemcrypt_get_cipher_name",
"mcrypt_list_algorithmsmssql_get_last_message",
"mysql_unbuffered_queryopenssl_get_privatekey",
"pdf_setrgbcolor_strokepdf_set_text_rendering",
"pg_set_client_encodingprinter_draw_rectangle",
"printer_draw_roundrectpspell_add_to_personal",
"pspell_config_personalreadline_clear_history",
"readline_write_historyregister_tick_function",
"satellite_exception_idswf_actionwaitforframe"
),22,1);

() = define_keywords_n ($1, strcat(
"cpdf_place_inline_image",
"cpdf_set_page_animationcpdf_setrgbcolor_stroke",
"cpdf_set_text_renderingcybercash_base64_decode",
"cybercash_base64_encodecybermut_creerreponsecm",
"dbplus_setindexbynumberdefine_syslog_variables",
"fbsql_database_passwordhw_getobjectbyquerycoll",
"imagetruecolortopaletteimap_mime_header_decode",
"java_last_exception_getmb_decode_numericentity",
"mb_encode_numericentitymb_substitute_character",
"mcal_event_set_categorymcrypt_enc_get_key_size",
"mcrypt_module_self_testpspell_config_save_repl",
"quoted_printable_decodeswf_actiontogglequality",
"swf_shapefillbitmapclipswf_shapefillbitmaptile",
"sybase_get_last_messageudm_clear_search_limits",
"xml_set_default_handlerxml_set_element_handler"
),23,1);

() = define_keywords_n ($1, strcat(
"get_magic_quotes_runtime",
"mcal_event_add_attributemcrypt_enc_is_block_mode",
"mssql_min_error_severitypspell_store_replacement",
"session_set_save_handlerset_magic_quotes_runtime",
"unregister_tick_functionxslt_output_endtransform",
"zip_entry_compressedsize"
),24,1);

() = define_keywords_n ($1, strcat(
"fdf_set_javascript_action",
"imap_rfc822_parse_adrlistimap_rfc822_parse_headers",
"imap_rfc822_write_addressjava_last_exception_clear",
"mcal_event_set_recur_nonemcrypt_enc_get_block_size",
"mcrypt_enc_get_modes_nameopenssl_x509_checkpurpose",
"pdf_closepath_fill_strokepspell_config_runtogether",
"satellite_exception_valuesession_get_cookie_params",
"session_set_cookie_paramssybase_min_error_severity"
),25,1);

() = define_keywords_n ($1, strcat(
"cpdf_closepath_fill_stroke",
"cybermut_creerformulairecmfdf_set_submit_form_action",
"get_html_translation_tablehw_getobjectbyquerycollobj",
"mcal_event_set_descriptionmcal_event_set_recur_daily",
"mssql_min_message_severityprinter_logical_fontheight",
"register_shutdown_functionsatellite_caught_exception",
"sybase_min_client_severitysybase_min_server_severity",
"xml_get_current_byte_indexxslt_output_begintransform"
),26,1);

() = define_keywords_n ($1, strcat(
"dbase_get_record_with_names",
"ircg_lookup_format_messagesmcal_event_set_recur_weekly",
"mcal_event_set_recur_yearlymcrypt_module_is_block_mode",
"sybase_min_message_severityxml_get_current_line_number",
"zip_entry_compressionmethod"
),27,1);

() = define_keywords_n ($1,
"readline_completion_function",
28,1);

() = define_keywords_n ($1, strcat(
"ircg_register_format_messages",
"mcrypt_enc_is_block_algorithmxml_get_current_column_number",
"xml_set_notation_decl_handler"
),29,1);

() = define_keywords_n ($1, strcat(
"mcrypt_enc_get_algorithms_name",
"xml_set_character_data_handler"
),30,1);

() = define_keywords_n ($1, strcat(
"cpdf_global_set_document_limits",
"mcal_fetch_current_stream_eventmcrypt_module_get_algo_key_size"
),31,1);

() = define_keywords_n ($1,
"mcrypt_module_is_block_algorithm",
32,1);

() = define_keywords_n ($1, strcat(
"mcal_event_set_recur_monthly_mday",
"mcal_event_set_recur_monthly_wdaymcrypt_module_get_algo_block_size"
),33,1);

() = define_keywords_n ($1, strcat(
"mcrypt_enc_get_supported_key_sizes",
"mcrypt_enc_is_block_algorithm_mode"
),34,1);

() = define_keywords_n ($1,
"xml_set_external_entity_ref_handler",
35,1);

() = define_keywords_n ($1,
"xml_set_unparsed_entity_decl_handler",
36,1);

() = define_keywords_n ($1,
"mcrypt_module_is_block_algorithm_mode",
37,1);

() = define_keywords_n ($1,
"xml_set_processing_instruction_handler",
38,1);

() = define_keywords_n ($1,
"mcrypt_module_get_algo_supported_key_sizes",
42,1);
%}}}

%!%+
%\function{php_mode}
%\synopsis{php_mode}
%\usage{Void php_mode ();}
%\description
% This is a mode that is dedicated to faciliate the editing of PHP language files.
% It calls the function \var{php_mode_hook} if it is defined. It also manages
% to recognice whetever it is in a php block or in a html block, for those people
% that doesnt seperate function from form ;)
%
% Functions that affect this mode include:
%#v+
%  function:             default binding:
%  php_top_of_function        ESC Ctrl-A
%  php_end_of_function        ESC Ctrl-E
%  php_mark_function          ESC Ctrl-H
%  php_mark_matching          ESC Ctrl-M
%  php_indent_buffer          Ctrl-C Ctrl-B
%  php_insert_class           Ctrl-C Ctrl-C
%  php_insert_function        Ctrl-C Ctrl-F
%  php_insert_bra             {
%  php_insert_ket             }
%  php_insert_colon           :
%  php_format_paragraph       ESC q
%  indent_line                TAB
%  newline_and_indent         RETURN
%  goto_match                 Ctrl-\
%  php_insert_tab             Ctrl-C Ctrl-I
%#v-
% Variables affecting indentation include:
%#v+
% PHP_INDENT
% PHP_BRACE
% PHP_BRA_NEWLINE
% PHP_KET_NEWLINE
% PHP_Colon_Offset
% PHP_CONTINUED_OFFSET
% PHP_Class_Offset
% PHP_Autoinsert_Comments
% PHP_Switch_Offset
%#v-
% Hooks: \var{php_mode_hook}
%!%-
define php_mode( )
{
	variable kmap = "PHP";
	set_mode( kmap, 2 );
	use_keymap( kmap );
	use_syntax_table( kmap );
	set_buffer_hook( "par_sep", "php_paragraph_sep" );
	set_buffer_hook( "indent_hook", "php_indent_region_or_line" );
	set_buffer_hook( "newline_indent_hook", "php_newline_and_indent" ); 
	
	mode_set_mode_info( "PHP", "fold_info", "//{{{\r//}}}\r\r" );
	mode_set_mode_info( "PHP", "init_mode_menu", &php_init_menu );	
	run_mode_hooks( "php_mode_hook" );
}

provide( "php_mode" );
