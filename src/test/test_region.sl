private variable Failed = 0;

private define prepare_region (prefix, str, suffix)
{
   erase_buffer ();
   insert(prefix);
   push_mark ();
   insert(str);
   push_spot();
   insert (suffix);
   pop_spot();
}


private define test_xform_region (in, out, op)
{
   variable prefix_suffixes
     = ["|", "|A", "A|", "A|B", "|a", "a|", "a|b", " | "];
   foreach (prefix_suffixes)
     {
	variable prefix_suffix = ();
	prefix_suffix = strchop (prefix_suffix, '|', 0);
	variable p = prefix_suffix[0], s = prefix_suffix[1];
	prepare_region (p, in, s);

	xform_region (op);
	mark_buffer ();
	variable out1 = bufsubstr ();
	variable out0 = p + out + s;
	variable in0 = p + in + s;
	if (out1 != out0)
	  {
	     () = fprintf (stderr, "xform_region ('%c') failed: %s->%s, expected %s\n",
		     op, in0, out1, out0);
	     Failed++;
	  }
     }
}

test_xform_region ("", "", 'u');
test_xform_region ("", "", 'd');
test_xform_region ("", "", 'c');
test_xform_region ("", "", 'x');

test_xform_region ("c", "C", 'u');
test_xform_region ("c", "c", 'd');
test_xform_region ("c", "C", 'c');
test_xform_region ("c", "C", 'x');

test_xform_region ("C", "C", 'u');
test_xform_region ("C", "c", 'd');
test_xform_region ("C", "C", 'c');
test_xform_region ("C", "c", 'x');

test_xform_region ("CD", "CD", 'u');
test_xform_region ("CD", "cd", 'd');
test_xform_region ("CD", "Cd", 'c');
test_xform_region ("CD", "cd", 'x');

test_xform_region ("cD", "CD", 'u');
test_xform_region ("cD", "cd", 'd');
test_xform_region ("cD", "Cd", 'c');
test_xform_region ("cD", "Cd", 'x');

test_xform_region ("Cd", "CD", 'u');
test_xform_region ("Cd", "cd", 'd');
test_xform_region ("Cd", "Cd", 'c');
test_xform_region ("Cd", "cD", 'x');

test_xform_region ("cd", "CD", 'u');
test_xform_region ("cd", "cd", 'd');
test_xform_region ("cd", "Cd", 'c');
test_xform_region ("cd", "CD", 'x');

test_xform_region ("CD ef", "CD EF", 'u');
test_xform_region ("CD ef", "cd ef", 'd');
test_xform_region ("CD ef", "Cd Ef", 'c');
test_xform_region ("CD ef", "cd EF", 'x');

test_xform_region ("cD Ef", "CD EF", 'u');
test_xform_region ("cD Ef", "cd ef", 'd');
test_xform_region ("cD Ef", "Cd Ef", 'c');
test_xform_region ("cD Ef", "Cd eF", 'x');

test_xform_region ("Cd eF", "CD EF", 'u');
test_xform_region ("Cd eF", "cd ef", 'd');
test_xform_region ("Cd eF", "Cd Ef", 'c');
test_xform_region ("Cd eF", "cD Ef", 'x');

test_xform_region ("cd EF", "CD EF", 'u');
test_xform_region ("cd EF", "cd ef", 'd');
test_xform_region ("cd EF", "Cd Ef", 'c');
test_xform_region ("cd EF", "CD ef", 'x');

test_xform_region ("CD efg", "CD EFG", 'u');
test_xform_region ("CD efg", "cd efg", 'd');
test_xform_region ("CD efg", "Cd Efg", 'c');
test_xform_region ("CD efg", "cd EFG", 'x');

test_xform_region ("cD EfG", "CD EFG", 'u');
test_xform_region ("cD EfG", "cd efg", 'd');
test_xform_region ("cD EfG", "Cd Efg", 'c');
test_xform_region ("cD EfG", "Cd eFg", 'x');

test_xform_region ("Cd eFg", "CD EFG", 'u');
test_xform_region ("Cd eFg", "cd efg", 'd');
test_xform_region ("Cd eFg", "Cd Efg", 'c');
test_xform_region ("Cd eFg", "cD EfG", 'x');

test_xform_region ("cd EFG", "CD EFG", 'u');
test_xform_region ("cd EFG", "cd efg", 'd');
test_xform_region ("cd EFG", "Cd Efg", 'c');
test_xform_region ("cd EFG", "CD efg", 'x');

