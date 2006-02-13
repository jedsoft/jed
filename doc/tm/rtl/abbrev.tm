\function{abbrev_table_p}
\synopsis{Test whether an abbreviation table "name" exists}
\usage{Integer abbrev_table_p (String name)}
\description
  Returns non-zero if an abbreviation table with called \var{name} exists. If
  the table does not exist, it returns zero.
\done

\function{create_abbrev_table}
\synopsis{Create an abbreviation table "name"}
\usage{Void create_abbrev_table (String name, String word)}
\description
  Create an abbreviation table with name \var{name}.  The second parameter
  \var{word} is the list of characters used to represent a word for the
  table. If the empty string is passed for \var{word}, the characters that
  currently constitute a word are used.
\done

\function{define_abbrev}
\synopsis{Define an abbreviation}
\usage{Void define_abbrev (String tbl, String abbrv, String expans)}
\description
  This function is used to define an abbreviation \var{abbrv} that will be
  expanded to \var{expans}.  The definition will be placed in the table with
  name \var{tbl}.
\done

\function{delete_abbrev_table}
\synopsis{Delete the abbrev table "name"}
\usage{Void delete_abbrev_table (String name)}
\description
  Delete the abbrev table specified by \var{name}.
\done

\function{dump_abbrev_table}
\synopsis{Insert the abbreviation table "name"}
\usage{Void dump_abbrev_table (String name)}
\description
  This function inserts the contents of the abbreviation table called
  \var{name} into the current buffer.
\done

\function{list_abbrev_tables}
\synopsis{Return the names of all defined abbreviation tables}
\usage{Integer list_abbrev_tables ()}
\description
  This function returns the names of all currently defined
  abbreviation tables.  The top item on the stack will be the number of
  tables followed by the names of the tables.
\done

\function{use_abbrev_table}
\synopsis{Use the abbreviation table "table" for the current buffer}
\usage{Void use_abbrev_table (String table)}
\description
  Use the abbreviation table named \var{table} as the abbreviation table for
  the current buffer.  By default, the "Global" table is used.
\done

\function{what_abbrev_table}
\synopsis{Return info about the current abbreviation table}
\usage{(String, String) what_abbrev_table ()}
\description
  This functions returns both the name of the abbreviation table and the
  definition of the word for the table currently associated with the
  current buffer.  If none is defined it returns two empty strings.
\done

