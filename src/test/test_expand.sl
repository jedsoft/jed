() = chdir ("/tmp");

private variable Failed = 0;
define test_expand_filename (a, b)
{
   variable b1 = expand_filename (a);
   if (b != b1)
     {
	Failed = 1;
	message ("expand_filename ($a) -> $b1, expected $b"$);
     }
}

test_expand_filename ("", "/tmp/");
test_expand_filename ("./", "/tmp/");
test_expand_filename (".", "/tmp/");
test_expand_filename ("..", "/");
test_expand_filename ("../", "/");
test_expand_filename (".././", "/");
test_expand_filename ("foo.././", "/tmp/foo../");
test_expand_filename ("f.././", "/tmp/f../");
test_expand_filename ("fo.././", "/tmp/fo../");
test_expand_filename ("fo.././bar", "/tmp/fo../bar");
test_expand_filename ("././bar", "/tmp/bar");
test_expand_filename ("/./bar", "/bar");
test_expand_filename ("/./bar//gamma", "/gamma");
test_expand_filename ("//./bar//gamma", "/bar/gamma");
test_expand_filename ("//./bar//gamma/.", "/bar/gamma/");
test_expand_filename ("//./bar//gamma/..", "/bar/");
test_expand_filename ("/..", "/");
test_expand_filename ("/.", "/");
test_expand_filename ("~/foo", "$HOME/foo"$);
test_expand_filename ("/foo/bar/~/xx/..", "$HOME/"$);
test_expand_filename ("/foo/bar/~/~/xx/..", "$HOME/"$);
test_expand_filename ("~/foo/bar/~/~/xx/..", "$HOME/"$);
test_expand_filename ("~/foo/bar/~/~xx", "$HOME/~xx"$);
test_expand_filename ("~/foo/bar/~/xx~", "$HOME/xx~"$);
test_expand_filename ("~/foo/bar/~", "$HOME/foo/bar/~"$);

#ifntrue
% Only for __QNX__
test_expand_filename ("/foo/bar///1/foo/bar", "//1/foo/bar");
test_expand_filename ("//23", "//23");
test_expand_filename ("//23/bar///1/foo/bar", "//1/foo/bar");
test_expand_filename ("//23/bar///12/foo/bar", "//12/foo/bar");
test_expand_filename ("//23/bar/~/baz", "$HOME/baz"$);
#endif
%test_expand_filename ("~dph/foo", "/home/dph");

  
exit (Failed);
