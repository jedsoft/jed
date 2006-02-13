% ltx-math.sl
%
% AUC-TeX style LaTeX-math-mode (v0.2) by Kevin Humphreys <kwh@cogsci.ed.ac.uk>
%
% For JED (v0.97.9b) by John E. Davis <davis@space.mit.edu>
%
% Based on AUC-TeX (v9.1i) by Per Abrahamsen <auc_tex_mgr@iesd.auc.dk>


% Autoloaded by latex_math_mode (C-c ~) in latex.sl


% CHANGES HISTORY:
% Modifed for use in main jed distribution
% --- v0.1-0.2
% reduce memory usage by using array (thanks to John E. Davis)


variable Latex_Math_Array = String_Type [128];
_for (0, 127, 1) {$1 = (); Latex_Math_Array[$1] = Null_String; }

Latex_Math_Array['a'] = "alpha";   
Latex_Math_Array['b'] = "beta";    
Latex_Math_Array['c'] = "mathcal{}";
Latex_Math_Array['d'] = "delta";   
Latex_Math_Array['e'] = "epsilon"; 
Latex_Math_Array['f'] = "phi";     
Latex_Math_Array['g'] = "gamma";   
Latex_Math_Array['h'] = "eta";     
Latex_Math_Array['k'] = "kappa";   
Latex_Math_Array['l'] = "lambda";  
Latex_Math_Array['m'] = "mu";      
Latex_Math_Array['N'] = "nabla";   
Latex_Math_Array['n'] = "nu";      
Latex_Math_Array['o'] = "omega";   
Latex_Math_Array['p'] = "pi";      
Latex_Math_Array['q'] = "theta";   
Latex_Math_Array['r'] = "rho";     
Latex_Math_Array['s'] = "sigma";   
Latex_Math_Array['t'] = "tau";     
Latex_Math_Array['u'] = "upsilon"; 
Latex_Math_Array['x'] = "chi";     
Latex_Math_Array['y'] = "psi";     
Latex_Math_Array['z'] = "zeta";    
Latex_Math_Array['D'] = "Delta";   
Latex_Math_Array['G'] = "Gamma";   
Latex_Math_Array['Q'] = "Theta";   
Latex_Math_Array['L'] = "Lambda";  
Latex_Math_Array['Y'] = "Psi";     
Latex_Math_Array['P'] = "Pi";      
Latex_Math_Array['S'] = "Sigma";   
Latex_Math_Array['U'] = "Upsilon"; 
Latex_Math_Array['V'] = "Phi";     
Latex_Math_Array['O'] = "Omega";   
Latex_Math_Array[6]   = "rightarrow";  % C-f
Latex_Math_Array[2]   = "leftarrow";   % C-b
Latex_Math_Array[16]  = "uparrow";     % C-p
Latex_Math_Array[14]  = "downarrow";   % C-n
Latex_Math_Array['<'] = "leq";     
Latex_Math_Array['>'] = "geq";     
Latex_Math_Array['~'] = "tilde";
Latex_Math_Array['I'] = "infty";   
Latex_Math_Array['A'] = "forall";  
Latex_Math_Array['E'] = "exists";  
Latex_Math_Array['!'] = "neg";     
Latex_Math_Array['i'] = "in";      
Latex_Math_Array['*'] = "times";   
Latex_Math_Array['.'] = "cdot";    
Latex_Math_Array['{'] = "subset";  
Latex_Math_Array['}'] = "supset";  
Latex_Math_Array['['] = "subseteq";
Latex_Math_Array[']'] = "supseteq";
Latex_Math_Array['/'] = "not";
Latex_Math_Array['\\'] = "setminus"; 
Latex_Math_Array['+'] = "cup";     
Latex_Math_Array['-'] = "cap";     
Latex_Math_Array['&'] = "wedge";   
Latex_Math_Array['|'] = "vee";     
Latex_Math_Array['('] = "langle";  
Latex_Math_Array[')'] = "rangle";  
Latex_Math_Array[5]   = "exp";         % C-e
Latex_Math_Array[19]  = "sin";         % C-s
Latex_Math_Array[3]   = "cos";         % C-c
Latex_Math_Array[30]  = "sup";         % C-^
Latex_Math_Array[31]  = "inf";         % C-_
Latex_Math_Array[4]   = "det";         % C-d
Latex_Math_Array[12]  = "lim";         % C-l
Latex_Math_Array[20]  = "tan";         % C-t
Latex_Math_Array['^'] = "hat";     
Latex_Math_Array['v'] = "vee";     
Latex_Math_Array['0'] = "emptyset";
 

define latex_insert_math ()
     {
  	variable arg = prefix_argument(-1);
  	variable ch = getkey ();
	
	if (ch > 127) return;
	
	if (arg != -1) insert_char ('$');

  	insert_char ('\\');
	insert (Latex_Math_Array[ch]);

	if (ch == 'c')   % position cursor correctly for \mathcal{}
	  {
	     if (arg != -1) { insert_char ('$'); go_left(2); }
	     else go_left_1 ();
	  }
	else { if (arg != -1) insert_char ('$'); }
     }

private define get_math_mode ()
{
   if (blocal_var_exists ("Latex_Math_Mode"))
     return get_blocal_var ("Latex_Math_Mode");
   return 0;
}

private define set_math_mode (x)
{
   create_blocal_var ("Latex_Math_Mode");
   set_blocal_var (x, "Latex_Math_Mode");
}

define latex_quoted_insert ()
{
   if (get_math_mode ())
     latex_insert_math ();
   else
     call ("quoted_insert");
}

private define latex_math_mode_internal ()
{
   set_math_mode (1);
   set_mode("LaTeX Math", 0x1 | 0x20);
   local_setkey ("latex_quoted_insert", "`");
   run_mode_hooks ("latex_math_mode_hook");
}

define latex_math_mode ()
{
   latex_mode ();
   latex_math_mode_internal ();
}

define latex_toggle_math_mode ()
{
   if (get_math_mode ())
     {
	set_math_mode (0);
	latex_mode ();
	return;
     }
   latex_math_mode_internal ();
}
