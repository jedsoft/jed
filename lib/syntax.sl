% syntax.sl		-*- SLang -*-
% The functions here are used to manipulate the syntax keyword tables
%
%  Note: Eventually code here will need to be changed to use character semantics.
%  The user should not have to worry about multibyte encodings.
%
   
private define add_hash_to_syntax_table (tbl, hash, len, n)
{
   variable i;
   hash = assoc_get_keys (hash);
   i = array_sort (hash);
   hash = strjoin (hash[i], "");
   () = define_keywords_n (tbl, hash, len, n);
   hash;
}


%!%+
%\function{add_keywords}
%\synopsis{add_keywords}
%\usage{String add_keywords (String tbl, String kws, Int len, Int n);}
%\description
% 
% Adds a set of keywords `kws', each of length `len', to the already
% existing syntax table `tbl'.  For convenience of the user, the function
% does alphabetical sorting and removes duplicate entries.
% 
% The previous list of keywords is returned.
%\seealso{define_keywords_n, create_syntax_table, add_keyword_n}
%!%-
define add_keywords (tbl, kws, len, n)
{
   variable okws, a, i, j, num, idx;
   variable hash;

   % add old keywords
   kws += define_keywords_n (tbl, "", len, n);

   num = strbytelen (kws) / len;
   !if (num) return "";

   hash = Assoc_Type[Int_Type];

   _for (0, num-1, 1)
     {
	i = ();
	hash [substrbytes (kws, 1 + i * len, len)] = 1;
     }
   
   add_hash_to_syntax_table (tbl, hash, len, n);
}

%!%+
%\function{add_keyword_n}
%\synopsis{add_keyword_n}
%\usage{Void add_keyword_n (String tbl, String kw, Int n);}
%\description
% 
% Adds a single keyword `kw' to the already existing syntax table `tbl'.
%\seealso{define_keywords_n, create_syntax_table, add_keywords}
%!%-
define add_keyword_n (tbl, kw, n)
{
   variable len = strbytelen (kw);
   !if (len) return;
   () = add_keywords (tbl, kw, len, n);
}

%!%+
%\function{add_keyword}
%\synopsis{add_keyword}
%\usage{Void add_keyword (String_Type tbl, String_Type kw);}
%\description
% 
% Adds a single keyword `kw' to the already existing syntax table `tbl'.
%\seealso{define_keywords_n, create_syntax_table, add_keyword_n}
%!%-
define add_keyword ()
{
   add_keyword_n (0);
}

%!%+
%\function{remove_keywords}
%\synopsis{remove_keywords}
%\usage{String remove_keywords (String tbl, String kws, Int len, Int n);}
%\description
% Removes a set of keywords `kws', each of length `len', from the already
% existing syntax table `tbl'.
%
% The previous list of keywords is returned.
%\seealso{add_keywords, define_keywords_n, create_syntax_table, add_keyword_n}
%!%-
define remove_keywords (tbl, kws, len, n)
{
   variable okws, num, nrem, i, rm;
   variable hash;

   % the old keywords
   okws = define_keywords_n (tbl, "", len, n);
   num = strbytelen (okws) / len;

   nrem = strbytelen (kws) / len;
   !if (nrem)
     {
	() = define_keywords_n (tbl, okws, len, n);
	return okws;
     }

   hash = Assoc_Type[Int_Type];
   _for (0, num-1, 1)
     {
	i = ();
	hash [substrbytes (okws, 1 + i * len, len)] = 1;
     }

   % remove unwanted entries
   _for (0, nrem-1, 1)
     {
	i = ();
	assoc_delete_key (hash, substrbytes (kws, 1 + i * len, len));
     }

   () = add_hash_to_syntax_table (tbl, hash, len, n);
   okws;
}
