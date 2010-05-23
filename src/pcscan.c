/* This file is included by ibmpc.c, i386.c, and os2.c.  It provides a
 * mapping from the scan code/shift state to an escape sequence.
 */
/* Copyright (c) 1992-2010 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#define RSHIFT_MASK	0x1
#define LSHIFT_MASK	0x2
#define CTRL_KEY_MASK	0x4
#define ALT_KEY_MASK	0x8
#define SHIFT_MASK	(RSHIFT_MASK|LSHIFT_MASK)

static unsigned char F_Keys[4][12] =
{
     { 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 133, 134 },
     { 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 135, 136 },
     { 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 137, 138 },
     { 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 139, 140 }
};

static unsigned char Small_Keypad_Keys[4][13] =
{
     { 'G', 'H', 'I', 0, 'K', 0, 'M', 0, 'O', 'P', 'Q', 'R', 'S' },   /* normal */
     { '0', '1', '2', 0, '3', 0, '4', 0, '5', '6', '7', '8', '9' },   /* shift */
     { 'w', 141, 132, 0, 's', 0, 't', 0, 'u', 145, 'v', 146, 147 },   /* ctrl */
     { 151, 152, 153, 0, 155, 0, 157, 0, 159, 160, 161, 162, 163 }   /* alt */
};

static unsigned char Num_Keypad_Keys[4][13] =
{
     { 'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n' },
     { '0', '1', '2', 0, '3',  0 , '4', 0, '5', '6', '7', '8', '9' },
     { 'w', 141, 132, 0, 's', 143, 't', 0, 'u', 145, 'v', 146, 147 },
     { 'w', 'x', 'y', 0, 't', 'u', 'v', 0, 'q', 'r', 's', 'p', 'n' }
};

static char *Alt_Map = "!@#$%^&*()-=\t*\0177QWERTYUIOP[]\r*ASDFGHJKL;'`*\\ZXCVBNM<>/";

static unsigned int default_scan_to_key (unsigned int scan, unsigned int shift,
					 unsigned char *chp)
{
   unsigned char ch;

   ch = (scan & 0xFF);
   chp[0] = ch;

   if ((ch != 0) && (ch != 0xE0))
     return 1;

   ch = (scan >> 8);

   if ((PC_Alt_Char == 0) || (shift != ALT_KEY_MASK))
     {
	chp[1] = ch;
	return 2;
     }

   if ((ch >= 14) && (ch <= 53))
     ch = (unsigned char) Alt_Map[ch];
   else if ((ch >= 120) && (ch <= 131))
     ch = (unsigned char) Alt_Map[ch - 120];
   else if (ch == 165) /* tab */
     ch = (unsigned char) Alt_Map[12];

   chp[0] = PC_Alt_Char;
   chp[1] = ch;

   return 2;
}

static unsigned int jed_scan_to_key (unsigned int scan, unsigned int shift,
				     unsigned char *chbuf)
{
   unsigned int i;
   unsigned int state;
   int c1;

   switch (scan)
     {
      case 0x0E08:		       /* backspace */
	*chbuf = 127;
	return 1;

      case 0xE02F:		       /* KEYPAD SLASH */
	c1 = 'Q';
	break;

      case 0x372A:		       /* KEYPAD STAR */
	c1 = 'R';
	break;

      case 0x4A2D:		       /* KEYPAD MINUS */
	c1 = 'S';
	break;

      case 0x4E2B:		       /* KEYPAD PLUS */
	c1 = 'm';
	break;

      case 0xE00D:		       /* KEYPAD ENTER */
	c1 = 'M';
	break;

      case 0x4700:            /* KEYPAD HOME */
      case 0x4800:            /* KEYPAD UP */
      case 0x4900:            /* KEYPAD PGUP */
      case 0x4B00:            /* KEYPAD LEFT */
      case 0x4C00:            /* KEYPAD 5 */
      case 0x4D00:            /* KEYPAD RIGHT */
      case 0x4F00:            /* KEYPAD END */
      case 0x5000:            /* KEYPAD DOWN */
      case 0x5100:            /* KEYPAD PGDN */
      case 0x5300:            /* KEYPAD DEL */
      case 0x5200:            /* KEYPAD INSERT */
 	state = 0;
 	if (shift & SHIFT_MASK) state = 1;
 	if (shift & CTRL_KEY_MASK) state = 2;
 	if (shift & ALT_KEY_MASK) state = 3;

	i = (scan >> 8);
 	c1 = Num_Keypad_Keys[state][i  - 0x47];
 	if (shift & (CTRL_KEY_MASK | ALT_KEY_MASK))
 	  {
 	     chbuf[0] = 0;
	     chbuf[1] = c1;
 	     return 2;
 	  }
 	break;

      case 0x47E0: 		       /* home */
      case 0x48E0:		       /* UP */
      case 0x49E0:		       /* PGUP */
      case 0x4BE0:		       /* LEFT */
      case 0x4DE0:		       /* RIGHT */
      case 0x4FE0:		       /* END */
      case 0x50E0:		       /* DOWN */
      case 0x51E0:		       /* PGDN */
      case 0x53E0:		       /* DEL */
      case 0x52E0:		       /* INSERT */

	chbuf[0] = 0xE0;
	state = 0;
	i = (scan >> 8) - 0x47;
	if (shift & SHIFT_MASK) state = 1;
	if (shift & CTRL_KEY_MASK) state = 2;
	if (shift & ALT_KEY_MASK)
	  {
	     chbuf[0] = 0;
	     state = 3;
	  }

	chbuf[1] = Small_Keypad_Keys[state][i];
	return 2;

      case 0x8500:		       /* F11 */
      case 0x8600:		       /* F12 */
	scan = 0x4500 + (scan - 0x8500);
	/* Drop */

      case 0x3b00:		       /* F1 */
      case 0x3c00:
      case 0x3d00:
      case 0x3e00:
      case 0x3f00:
      case 0x4000:
      case 0x4100:
      case 0x4200:
      case 0x4300:
      case 0x4400:		       /* F10 */
	i = scan >> 8;
	i = i - 0x3b;

	state = 0;
	if (shift & SHIFT_MASK) state = 1;
	if (shift & CTRL_KEY_MASK) state = 2;
	if (shift & ALT_KEY_MASK) state = 3;

	chbuf[0] = 0;
	chbuf[1] = F_Keys[state][i];
	return 2;

      case 0x1C0A:		       /* Ctrl-RETURN */
	return default_scan_to_key (0x1C0D, shift, chbuf);

      case 0x3920: 		       /* space */
	if (shift & CTRL_KEY_MASK)
	  scan = 0x0300;
	/* drop */

      default:
	return default_scan_to_key (scan, shift, chbuf);
     }

   chbuf[0] = 27;
   chbuf[1] = 'O';
   chbuf[2] = c1;
   return 3;
}
