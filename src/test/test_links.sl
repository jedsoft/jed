static variable Failed = 0;

static variable Tmp_Root = make_tmp_file ("/tmp/jedtest");
% Creates the following directory layout under Tmp_Root:
%   /tmp/jedtest/ :
%      A/
%        dev/
%        dev/foo
%        dev/hoo  (hard link to foo)
%        file.c -> dev/foo
%      B/
%        dev2 -> /tmp/jedtest/A/dev
static variable Tmp_A = path_concat (Tmp_Root, "A");
static variable Tmp_A_dev = path_concat (Tmp_A, "dev");
static variable Tmp_A_dev_foo = path_concat (Tmp_A_dev, "foo");
static variable Tmp_A_dev_hoo = path_concat (Tmp_A_dev, "hoo");
static variable Tmp_A_filec = path_concat (Tmp_A, "file.c");
static variable Tmp_B = path_concat (Tmp_Root, "B");
static variable Tmp_B_dev2 = path_concat (Tmp_B, "dev2");

static define make_layout ()
{
   () = mkdir (Tmp_Root);
   () = mkdir (Tmp_A);
   () = mkdir (Tmp_A_dev);
   () = append_string_to_file (Tmp_A_dev_foo, Tmp_A_dev_foo);
   () = symlink (Tmp_A_dev_foo, Tmp_A_filec);
   () = hardlink (Tmp_A_dev_foo, Tmp_A_dev_hoo);
   () = mkdir (Tmp_B);
   () = symlink (Tmp_A_dev, Tmp_B_dev2);
}

% Tests read/write of /tmp/jedtest/A/file.c which is a
% symlink to /tmp/jedtest/A/dev/foo.
% Then it tries to read 
%   /tmp/jedtest/B/dev2/../file.c
% where /tmp/jedtest/B/dev2 is a symlink to /tmp/jedtest/A/dev
static define test_link_read_write_1 ()
{
   () = read_file (Tmp_A_filec);
   bob ();
   !if (looking_at (Tmp_A_dev_foo))
     {
	message ("Failed to read $Tmp_A_dev_foo via $Tmp_A_filec"$);
	Failed++;
	return;
     }

   if (buffer_filename () != Tmp_A_filec)
     {
	message ("Failed to get proper filename for %s, found %s", 
		 Tmp_A_filec, buffer_filename ());
	Failed++;
     }

   variable name;
   (,,name,) = getbuf_info ();
   if (name != path_basename (Tmp_A_filec))
     {
	Failed++;
	vmessage ("Failed to get proper buffer name for link, found %s", name);
     }
   
   bob ();
   insert (Tmp_A_dev_foo);
   save_buffer ();
   delbuf (whatbuf());

   () = read_file (Tmp_A_filec);
   bob ();
   !if (looking_at (Tmp_A_dev_foo+Tmp_A_dev_foo))
     {
	Failed++;
	vmessage ("Failed to write symlink file");
     }

   if (1 != file_status (Tmp_A_dev_foo + "~"))
     {
	Failed++;
	vmessage ("Unable to find backup file %s", Tmp_A_dev_foo);
     }

   delbuf (whatbuf ());

   % Second part of test
   variable dev2_file = path_concat (Tmp_B_dev2, "../file.c");
   () = read_file (dev2_file);
   bob();
   !if (looking_at (Tmp_A_dev_foo))
     {
	Failed++;
	vmessage ("failed to read %s via %s", Tmp_A_dev_foo, dev2_file);
     }
}

static define test_read_hard_link ()
{
   () = read_file (Tmp_A_dev_foo);
   variable a = whatbuf ();
   variable afile = buffer_filename ();
   () = read_file (Tmp_A_dev_hoo);
   variable b = whatbuf ();
   variable bfile = buffer_filename ();
   if ((a != b) or (afile != bfile))
     {
	Failed++;
	message ("failed to read hardlink $Tmp_A_dev_hoo as $Tmp_A_dev_foo"$);
	message ("  Got buffers $a and $b"$);
	delbuf (a);
	delbuf (b);
	return;
     }
   delbuf (a);
}
   
make_layout ();

test_link_read_write_1 ();
test_read_hard_link ();

exit (Failed);
