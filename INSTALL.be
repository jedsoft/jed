BeOS is largely Unix/GNU compatible so for installing you should read the
instructions in INSTALL.unx. But there are a few issues:

- If you get the message 'error: cannot guess host type; you must specifiy
  one' you need a autoconf/config.guess and autoconf/config.sub. Those from
  autoconf-2.13 work.

- BeOS neither needs nor supports -lm. You should remove them (after
  configure) from the Makefile manually.

- BeOS uses a blinking white-on-black cursor. This is largely unreadable in
  the default Jed 0.99 color-scheme.
