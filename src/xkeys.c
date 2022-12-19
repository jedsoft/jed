/* Copyright (c) 1992-2022 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
/* xkeys.c */
/* The first character is the length of the string */
#define MAX_KEYSYM_STRING_LEN 8

typedef char KeySym_Mapping_Type[MAX_KEYSYM_STRING_LEN + 1];

static KeySym_Mapping_Type KeySym_Map_FF[256] =
{
   "\004\033[3~",	/* 0xFF00                        Remove */
   "",			/* 0xFF01 */
   "",			/* 0xFF02 */
   "",			/* 0xFF03 */
   "",			/* 0xFF04 */
   "",			/* 0xFF05 */
   "",			/* 0xFF06 */
   "",			/* 0xFF07 */
   "\001\177",		/* 0xFF08 	 XK_BackSpace */
   "\001\t",		/* 0xFF09 	 XK_Tab */
   "\001\n",		/* 0xFF0A 	 XK_Linefeed */
   "",			/* 0xFF0B 	 XK_Clear */
   "",			/* 0xFF0C */
   "\001\r",		/* 0xFF0D 	 XK_Return */
   "",			/* 0xFF0E */
   "",			/* 0xFF0F */
   "",			/* 0xFF10 */
   "",			/* 0xFF11 */
   "",			/* 0xFF12 */
   "",			/* 0xFF13 	 XK_Pause */
   "",			/* 0xFF14 	 XK_Scroll_Lock */
   "",			/* 0xFF15 */
   "",			/* 0xFF16 */
   "",			/* 0xFF17 */
   "",			/* 0xFF18 */
   "",			/* 0xFF19 */
   "",			/* 0xFF1A */
   "\001\033",		/* 0xFF1B 	 XK_Escape */
   "",			/* 0xFF1C */
   "",			/* 0xFF1D */
   "",			/* 0xFF1E */
   "",			/* 0xFF1F */
   "",			/* 0xFF20 	 XK_Multi_key */
   "",			/* 0xFF21 	 XK_Kanji */
   "",			/* 0xFF22 	 XK_Muhenkan */
   "",			/* 0xFF23 	 XK_Henkan_Mode 	 XK_Henkan */
   "",			/* 0xFF24 	 XK_Romaji */
   "",			/* 0xFF25 	 XK_Hiragana */
   "",			/* 0xFF26 	 XK_Katakana */
   "",			/* 0xFF27 	 XK_Hiragana_Katakana */
   "",			/* 0xFF28 	 XK_Zenkaku */
   "",			/* 0xFF29 	 XK_Hankaku */
   "",			/* 0xFF2A 	 XK_Zenkaku_Hankaku */
   "",			/* 0xFF2B 	 XK_Touroku */
   "",			/* 0xFF2C 	 XK_Massyo */
   "",			/* 0xFF2D 	 XK_Kana_Lock */
   "",			/* 0xFF2E 	 XK_Kana_Shift */
   "",			/* 0xFF2F 	 XK_Eisu_Shift */
   "",			/* 0xFF30 	 XK_Eisu_toggle */
   "",			/* 0xFF31 */
   "",			/* 0xFF32 */
   "",			/* 0xFF33 */
   "",			/* 0xFF34 */
   "",			/* 0xFF35 */
   "",			/* 0xFF36 */
   "",			/* 0xFF37 */
   "",			/* 0xFF38 */
   "",			/* 0xFF39 */
   "",			/* 0xFF3A */
   "",			/* 0xFF3B */
   "",			/* 0xFF3C */
   "",			/* 0xFF3D */
   "",			/* 0xFF3E */
   "",			/* 0xFF3F */
   "",			/* 0xFF40 */
   "",			/* 0xFF41 */
   "",			/* 0xFF42 */
   "",			/* 0xFF43 */
   "",			/* 0xFF44 */
   "",			/* 0xFF45 */
   "",			/* 0xFF46 */
   "",			/* 0xFF47 */
   "",			/* 0xFF48 */
   "",			/* 0xFF49 */
   "",			/* 0xFF4A */
   "",			/* 0xFF4B */
   "",			/* 0xFF4C */
   "",			/* 0xFF4D */
   "",			/* 0xFF4E */
   "",			/* 0xFF4F */
   "\004\033[1~",	/* 0xFF50 	 XK_Home */
   "\003\033[D",	/* 0xFF51 	 XK_Left */
   "\003\033[A",	/* 0xFF52 	 XK_Up */
   "\003\033[C",	/* 0xFF53 	 XK_Right */
   "\003\033[B",	/* 0xFF54 	 XK_Down */
   "\004\033[5~",	/* 0xFF55 	 XK_Prior */
   "\004\033[6~",	/* 0xFF56 	 XK_Next */
   "\004\033[4~",	/* 0xFF57 	 XK_End */
   "\001\001",		/* 0xFF58 	 XK_Begin */
   "",			/* 0xFF59 */
   "",			/* 0xFF5A */
   "",			/* 0xFF5B */
   "",			/* 0xFF5C */
   "",			/* 0xFF5D */
   "",			/* 0xFF5E */
   "",			/* 0xFF5F */
   "\004\033[4~",	/* 0xFF60 	 XK_Select */
   "",			/* 0xFF61 	 XK_Print */
   "",			/* 0xFF62 	 XK_Execute */
   "\004\033[2~",	/* 0xFF63 	 XK_Insert */
   "",			/* 0xFF64 */
   "\001\037",		/* 0xFF65 	 XK_Undo 	 ^_ */
   "",			/* 0xFF66 	 XK_Redo */
   "\005\033[29~",	/* 0xFF67 	 XK_Menu         Do */
   "\004\033[1~",	/* 0xFF68 	 XK_Find */
   "\001\007",		/* 0xFF69 	 XK_Cancel */
   "\005\033[28~",	/* 0xFF6A 	 XK_Help */
   "",			/* 0xFF6B 	 XK_Break */
   "",			/* 0xFF6C */
   "",			/* 0xFF6D */
   "",			/* 0xFF6E */
   "",			/* 0xFF6F */
   "",			/* 0xFF70 */
   "",			/* 0xFF71 */
   "",			/* 0xFF72 */
   "",			/* 0xFF73 */
   "",			/* 0xFF74 */
   "",			/* 0xFF75 */
   "",			/* 0xFF76 */
   "",			/* 0xFF77 */
   "",			/* 0xFF78 */
   "",			/* 0xFF79 */
   "",			/* 0xFF7A */
   "",			/* 0xFF7B */
   "",			/* 0xFF7C */
   "",			/* 0xFF7D */
   "",			/* 0xFF7E */
   "",			/* 0xFF7F 	 XK_Num_Lock */
   "",			/* 0xFF80 	 XK_KP_Space */
   "",			/* 0xFF81 */
   "",			/* 0xFF82 */
   "",			/* 0xFF83 */
   "",			/* 0xFF84 */
   "",			/* 0xFF85 */
   "",			/* 0xFF86 */
   "",			/* 0xFF87 */
   "",			/* 0xFF88 */
   "",			/* 0xFF89 	 XK_KP_Tab */
   "",			/* 0xFF8A */
   "",			/* 0xFF8B */
   "",			/* 0xFF8C */
   "\003\033OM",	/* 0xFF8D 	 XK_KP_Enter */
   "",			/* 0xFF8E */
   "",			/* 0xFF8F */
   "",			/* 0xFF90 */
   "\003\033OP",	/* 0xFF91 	 XK_KP_F1        PF1 */
   "\003\033OQ",	/* 0xFF92 	 XK_KP_F2        PF2 */
   "\003\033OR",	/* 0xFF93 	 XK_KP_F3        PF3 */
   "\003\033OS",	/* 0xFF94 	 XK_KP_F4        PF4 */
   "\003\033Ow",	/* 0xFF95 	 XK_KP_Home */
   "\003\033Ot",	/* 0xFF96 	 XK_KP_Left */
   "\003\033Ox",	/* 0xFF97 	 XK_KP_Up */
   "\003\033Ov",	/* 0xFF98 	 XK_KP_Right */
   "\003\033Or",	/* 0xFF99 	 XK_KP_Down */
   "\003\033Oy",	/* 0xFF9A 	 XK_KP_Prior */
   "\003\033Os",	/* 0xFF9B 	 XK_KP_Next */
   "\003\033Oq",	/* 0xFF9C 	 XK_KP_End */
   "\003\033Ou",	/* 0xFF9D 	 XK_KP_Begin */
   "\003\033Op",	/* 0xFF9E 	 XK_KP_Insert */
   "\003\033On",	/* 0xFF9F 	 XK_KP_Delete */
   "",			/* 0xFFA0 */
   "",			/* 0xFFA1 */
   "",			/* 0xFFA2 */
   "",			/* 0xFFA3 */
   "",			/* 0xFFA4 */
   "",			/* 0xFFA5 */
   "",			/* 0xFFA6 */
   "",			/* 0xFFA7 */
   "",			/* 0xFFA8 */
   "",			/* 0xFFA9 */
   "\001*",			/* 0xFFAA 	 XK_KP_Multiply */
   "\001+",			/* 0xFFAB 	 XK_KP_Add */
   "\003\033Ol",	/* 0xFFAC 	 XK_KP_Separator KP , */
   "\003\033Om",	/* 0xFFAD 	 XK_KP_Subtract  KP - */
   "\003\033On",	/* 0xFFAE 	 XK_KP_Decimal   KP . */
   "\001/",			/* 0xFFAF 	 XK_KP_Divide */
   "\003\033Op",	/* 0xFFB0 	 XK_KP_0 */
   "\003\033Oq",	/* 0xFFB1 	 XK_KP_1 */
   "\003\033Or",	/* 0xFFB2 	 XK_KP_2 */
   "\003\033Os",	/* 0xFFB3 	 XK_KP_3 */
   "\003\033Ot",	/* 0xFFB4 	 XK_KP_4 */
   "\003\033Ou",	/* 0xFFB5 	 XK_KP_5 */
   "\003\033Ov",	/* 0xFFB6 	 XK_KP_6 */
   "\003\033Ow",	/* 0xFFB7 	 XK_KP_7 */
   "\003\033Ox",	/* 0xFFB8 	 XK_KP_8 */
   "\003\033Oy",	/* 0xFFB9 	 XK_KP_9 */
   "",			/* 0xFFBA */
   "",			/* 0xFFBB */
   "",			/* 0xFFBC */
   "\001=",			/* 0xFFBD 	 XK_KP_Equal */

   "\005\033[11~",	/* 0xFFBE 	 XK_F1 */  /* xterm convention */
   "\005\033[12~",	/* 0xFFBF 	 XK_F2 */  /* xterm convention */
   "\005\033[13~",	/* 0xFFC0 	 XK_F3 */  /* xterm convention */
   "\005\033[14~",	/* 0xFFC1 	 XK_F4 */  /* xterm convention */
   "\005\033[15~",	/* 0xFFC2 	 XK_F5 */  /* xterm convention */
   "\005\033[17~",	/* 0xFFC3 	 XK_F6 */
   "\005\033[18~",	/* 0xFFC4 	 XK_F7 */
   "\005\033[19~",	/* 0xFFC5 	 XK_F8 */
   "\005\033[20~",	/* 0xFFC6 	 XK_F9 */
   "\005\033[21~",	/* 0xFFC7 	 XK_F10 */
   "\005\033[23~",	/* 0xFFC8 	 XK_F11 	 XK_L1 */
   "\005\033[24~",	/* 0xFFC9 	 XK_F12 	 XK_L2 */
   "\005\033[25~",	/* 0xFFCA 	 XK_F13 	 XK_L3 */
   "\005\033[26~",	/* 0xFFCB 	 XK_F14 	 XK_L4 */
   "\005\033[28~",	/* 0xFFCC 	 XK_F15 	 XK_L5 */
   "\005\033[29~",	/* 0xFFCD 	 XK_F16 	 XK_L6 */
   "\005\033[31~",	/* 0xFFCE 	 XK_F17 	 XK_L7 */
   "\005\033[32~",	/* 0xFFCF 	 XK_F18 	 XK_L8 */
   "\005\033[33~",	/* 0xFFD0 	 XK_F19 	 XK_L9 */
   "\005\033[34~",	/* 0xFFD1 	 XK_F20 	 XK_L10 */
   "\005\033[35~",	/* 0xFFD2 	 XK_F21 	 XK_R1 */
   "\005\033[36~",	/* 0xFFD3 	 XK_F22 	 XK_R2 */
   "\005\033[37~",	/* 0xFFD4 	 XK_F23 	 XK_R3 */
   "\005\033[38~",	/* 0xFFD5 	 XK_F24 	 XK_R4 */
   "\005\033[39~",	/* 0xFFD6 	 XK_F25 	 XK_R5 */
   "\005\033[40~",	/* 0xFFD7 	 XK_F26 	 XK_R6 */
   "\005\033[41~",	/* 0xFFD8 	 XK_F27 	 XK_R7 */
   "\005\033[42~",	/* 0xFFD9 	 XK_F28 	 XK_R8 */
   "\005\033[43~",	/* 0xFFDA 	 XK_F29 	 XK_R9 */
   "\005\033[44~",	/* 0xFFDB 	 XK_F30 	 XK_R10 */
   "\005\033[45~",	/* 0xFFDC 	 XK_F31 	 XK_R11 */
   "\005\033[46~",	/* 0xFFDD 	 XK_F32 	 XK_R12 */
   "\005\033[47~",	/* 0xFFDE 	 XK_R13 	 XK_F33 */
   "\005\033[48~",	/* 0xFFDF 	 XK_F34 	 XK_R14 */
   "\005\033[49~",	/* 0xFFE0 	 XK_F35 	 XK_R15 */
   "",			/* 0xFFE1 	 XK_Shift_L 	 XK_Shift_L */
   "",			/* 0xFFE2 	 XK_Shift_R */
   "",			/* 0xFFE3 	 XK_Control_L */
   "",			/* 0xFFE4 	 XK_Control_R */
   "",			/* 0xFFE5 	 XK_Caps_Lock */
   "",			/* 0xFFE6 	 XK_Shift_Lock */
   "",			/* 0xFFE7 	 XK_Meta_L */
   "",			/* 0xFFE8 	 XK_Meta_R */
   "",			/* 0xFFE9 	 XK_Alt_L */
   "",			/* 0xFFEA 	 XK_Alt_R */
   "",			/* 0xFFEB 	 XK_Super_L */
   "",			/* 0xFFEC 	 XK_Super_R */
   "",			/* 0xFFED 	 XK_Hyper_L */
   "",			/* 0xFFEE 	 XK_Hyper_R */
   "",			/* 0xFFEF */
   "",			/* 0xFFF0 */
   "",			/* 0xFFF1 */
   "",			/* 0xFFF2 */
   "",			/* 0xFFF3 */
   "",			/* 0xFFF4 */
   "",			/* 0xFFF5 */
   "",			/* 0xFFF6 */
   "",			/* 0xFFF7 */
   "",			/* 0xFFF8 */
   "",			/* 0xFFF9 */
   "",			/* 0xFFFA */
   "",			/* 0xFFFB */
   "",			/* 0xFFFC */
   "",			/* 0xFFFD */
   "",			/* 0xFFFE */
   "\001\177"		/* 0xFFFF 	 XK_Delete */
};

static KeySym_Mapping_Type Shift_KeySym_Map_FF[256] =
{
   "\004\033[3$",	/* 0xFF00                        Remove */
   "",			/* 0xFF01 */
   "",			/* 0xFF02 */
   "",			/* 0xFF03 */
   "",			/* 0xFF04 */
   "",			/* 0xFF05 */
   "",			/* 0xFF06 */
   "",			/* 0xFF07 */
   "\001\177",		/* 0xFF08 	 XK_BackSpace */
   "\003\033[Z",	/* 0xFF09 	 XK_Tab */
   "\001\n",		/* 0xFF0A 	 XK_Linefeed */
   "",			/* 0xFF0B 	 XK_Clear */
   "",			/* 0xFF0C */
   "\001\r",		/* 0xFF0D 	 XK_Return */
   "",			/* 0xFF0E */
   "",			/* 0xFF0F */
   "",			/* 0xFF10 */
   "",			/* 0xFF11 */
   "",			/* 0xFF12 */
   "",			/* 0xFF13 	 XK_Pause */
   "",			/* 0xFF14 	 XK_Scroll_Lock */
   "",			/* 0xFF15 */
   "",			/* 0xFF16 */
   "",			/* 0xFF17 */
   "",			/* 0xFF18 */
   "",			/* 0xFF19 */
   "",			/* 0xFF1A */
   "\001\033",		/* 0xFF1B 	 XK_Escape */
   "",			/* 0xFF1C */
   "",			/* 0xFF1D */
   "",			/* 0xFF1E */
   "",			/* 0xFF1F */
   "",			/* 0xFF20 	 XK_Multi_key */
   "",			/* 0xFF21 	 XK_Kanji */
   "",			/* 0xFF22 	 XK_Muhenkan */
   "",			/* 0xFF23 	 XK_Henkan_Mode 	 XK_Henkan */
   "",			/* 0xFF24 	 XK_Romaji */
   "",			/* 0xFF25 	 XK_Hiragana */
   "",			/* 0xFF26 	 XK_Katakana */
   "",			/* 0xFF27 	 XK_Hiragana_Katakana */
   "",			/* 0xFF28 	 XK_Zenkaku */
   "",			/* 0xFF29 	 XK_Hankaku */
   "",			/* 0xFF2A 	 XK_Zenkaku_Hankaku */
   "",			/* 0xFF2B 	 XK_Touroku */
   "",			/* 0xFF2C 	 XK_Massyo */
   "",			/* 0xFF2D 	 XK_Kana_Lock */
   "",			/* 0xFF2E 	 XK_Kana_Shift */
   "",			/* 0xFF2F 	 XK_Eisu_Shift */
   "",			/* 0xFF30 	 XK_Eisu_toggle */
   "",			/* 0xFF31 */
   "",			/* 0xFF32 */
   "",			/* 0xFF33 */
   "",			/* 0xFF34 */
   "",			/* 0xFF35 */
   "",			/* 0xFF36 */
   "",			/* 0xFF37 */
   "",			/* 0xFF38 */
   "",			/* 0xFF39 */
   "",			/* 0xFF3A */
   "",			/* 0xFF3B */
   "",			/* 0xFF3C */
   "",			/* 0xFF3D */
   "",			/* 0xFF3E */
   "",			/* 0xFF3F */
   "",			/* 0xFF40 */
   "",			/* 0xFF41 */
   "",			/* 0xFF42 */
   "",			/* 0xFF43 */
   "",			/* 0xFF44 */
   "",			/* 0xFF45 */
   "",			/* 0xFF46 */
   "",			/* 0xFF47 */
   "",			/* 0xFF48 */
   "",			/* 0xFF49 */
   "",			/* 0xFF4A */
   "",			/* 0xFF4B */
   "",			/* 0xFF4C */
   "",			/* 0xFF4D */
   "",			/* 0xFF4E */
   "",			/* 0xFF4F */
   "\004\033[1$",	/* 0xFF50 	 XK_Home */
   "\003\033[d",	/* 0xFF51 	 XK_Left */
   "\003\033[a",	/* 0xFF52 	 XK_Up */
   "\003\033[c",	/* 0xFF53 	 XK_Right */
   "\003\033[b",	/* 0xFF54 	 XK_Down */
   "\004\033[5$",	/* 0xFF55 	 XK_Prior */
   "\004\033[6$",	/* 0xFF56 	 XK_Next */
   "\004\033[4$",	/* 0xFF57 	 XK_End */
   "\001\001",		/* 0xFF58 	 XK_Begin */
   "",			/* 0xFF59 */
   "",			/* 0xFF5A */
   "",			/* 0xFF5B */
   "",			/* 0xFF5C */
   "",			/* 0xFF5D */
   "",			/* 0xFF5E */
   "",			/* 0xFF5F */
   "\004\033[4$",	/* 0xFF60 	 XK_Select */
   "",			/* 0xFF61 	 XK_Print */
   "",			/* 0xFF62 	 XK_Execute */
   "\004\033[2$",	/* 0xFF63 	 XK_Insert */
   "",			/* 0xFF64 */
   "\001\037",		/* 0xFF65 	 XK_Undo 	 ^_ */
   "",			/* 0xFF66 	 XK_Redo */
   "\005\033[29$",	/* 0xFF67 	 XK_Menu         Do */
   "\004\033[1$",	/* 0xFF68 	 XK_Find */
   "\001\007",		/* 0xFF69 	 XK_Cancel */
   "\005\033[28$",	/* 0xFF6A 	 XK_Help */
   "",			/* 0xFF6B 	 XK_Break */
   "",			/* 0xFF6C */
   "",			/* 0xFF6D */
   "",			/* 0xFF6E */
   "",			/* 0xFF6F */
   "",			/* 0xFF70 */
   "",			/* 0xFF71 */
   "",			/* 0xFF72 */
   "",			/* 0xFF73 */
   "",			/* 0xFF74 */
   "",			/* 0xFF75 */
   "",			/* 0xFF76 */
   "",			/* 0xFF77 */
   "",			/* 0xFF78 */
   "",			/* 0xFF79 */
   "",			/* 0xFF7A */
   "",			/* 0xFF7B */
   "",			/* 0xFF7C */
   "",			/* 0xFF7D */
   "",			/* 0xFF7E */
   "",			/* 0xFF7F 	 XK_Num_Lock */
   "",			/* 0xFF80 	 XK_KP_Space */
   "",			/* 0xFF81 */
   "",			/* 0xFF82 */
   "",			/* 0xFF83 */
   "",			/* 0xFF84 */
   "",			/* 0xFF85 */
   "",			/* 0xFF86 */
   "",			/* 0xFF87 */
   "",			/* 0xFF88 */
   "",			/* 0xFF89 	 XK_KP_Tab */
   "",			/* 0xFF8A */
   "",			/* 0xFF8B */
   "",			/* 0xFF8C */
   "\003\033OM",	/* 0xFF8D 	 XK_KP_Enter */
   "",			/* 0xFF8E */
   "",			/* 0xFF8F */
   "",			/* 0xFF90 */
   "\003\033OP",	/* 0xFF91 	 XK_KP_F1        PF1 */
   "\003\033OQ",	/* 0xFF92 	 XK_KP_F2        PF2 */
   "\003\033OR",	/* 0xFF93 	 XK_KP_F3        PF3 */
   "\003\033OS",	/* 0xFF94 	 XK_KP_F4        PF4 */
   "\003\033Ow",	/* 0xFF95 	 XK_KP_Home */
   "\003\033Ot",	/* 0xFF96 	 XK_KP_Left */
   "\003\033Ox",	/* 0xFF97 	 XK_KP_Up */
   "\003\033Ov",	/* 0xFF98 	 XK_KP_Right */
   "\003\033Or",	/* 0xFF99 	 XK_KP_Down */
   "\003\033Oy",	/* 0xFF9A 	 XK_KP_Prior */
   "\003\033Os",	/* 0xFF9B 	 XK_KP_Next */
   "\003\033Oq",	/* 0xFF9C 	 XK_KP_End */
   "\003\033Ou",	/* 0xFF9D 	 XK_KP_Begin */
   "\003\033Op",	/* 0xFF9E 	 XK_KP_Insert */
   "\003\033On",	/* 0xFF9F 	 XK_KP_Delete */
   "",			/* 0xFFA0 */
   "",			/* 0xFFA1 */
   "",			/* 0xFFA2 */
   "",			/* 0xFFA3 */
   "",			/* 0xFFA4 */
   "",			/* 0xFFA5 */
   "",			/* 0xFFA6 */
   "",			/* 0xFFA7 */
   "",			/* 0xFFA8 */
   "",			/* 0xFFA9 */
   "",			/* 0xFFAA 	 XK_KP_Multiply */
   "",			/* 0xFFAB 	 XK_KP_Add */
   "\003\033Ol",	/* 0xFFAC 	 XK_KP_Separator KP , */
   "\003\033Om",	/* 0xFFAD 	 XK_KP_Subtract  KP - */
   "\003\033On",	/* 0xFFAE 	 XK_KP_Decimal   KP . */
   "",			/* 0xFFAF 	 XK_KP_Divide */
   "\003\033Op",	/* 0xFFB0 	 XK_KP_0 */
   "\003\033Oq",	/* 0xFFB1 	 XK_KP_1 */
   "\003\033Or",	/* 0xFFB2 	 XK_KP_2 */
   "\003\033Os",	/* 0xFFB3 	 XK_KP_3 */
   "\003\033Ot",	/* 0xFFB4 	 XK_KP_4 */
   "\003\033Ou",	/* 0xFFB5 	 XK_KP_5 */
   "\003\033Ov",	/* 0xFFB6 	 XK_KP_6 */
   "\003\033Ow",	/* 0xFFB7 	 XK_KP_7 */
   "\003\033Ox",	/* 0xFFB8 	 XK_KP_8 */
   "\003\033Oy",	/* 0xFFB9 	 XK_KP_9 */
   "",			/* 0xFFBA */
   "",			/* 0xFFBB */
   "",			/* 0xFFBC */
   "",			/* 0xFFBD 	 XK_KP_Equal */
   "\005\033[11$",	/* 0xFFBE 	 XK_F1 */
   "\005\033[12$",	/* 0xFFBF 	 XK_F2 */
   "\005\033[13$",	/* 0xFFC0 	 XK_F3 */
   "\005\033[14$",	/* 0xFFC1 	 XK_F4 */
   "\005\033[15$",	/* 0xFFC2 	 XK_F5 */
   "\005\033[17$",	/* 0xFFC3 	 XK_F6 */
   "\005\033[18$",	/* 0xFFC4 	 XK_F7 */
   "\005\033[19$",	/* 0xFFC5 	 XK_F8 */
   "\005\033[20$",	/* 0xFFC6 	 XK_F9 */
   "\005\033[21$",	/* 0xFFC7 	 XK_F10 */
   "\005\033[23$",	/* 0xFFC8 	 XK_F11 	 XK_L1 */
   "\005\033[24$",	/* 0xFFC9 	 XK_F12 	 XK_L2 */
   "\005\033[25$",	/* 0xFFCA 	 XK_F13 	 XK_L3 */
   "\005\033[26$",	/* 0xFFCB 	 XK_F14 	 XK_L4 */
   "\005\033[28$",	/* 0xFFCC 	 XK_F15 	 XK_L5 */
   "\005\033[29$",	/* 0xFFCD 	 XK_F16 	 XK_L6 */
   "\005\033[31$",	/* 0xFFCE 	 XK_F17 	 XK_L7 */
   "\005\033[32$",	/* 0xFFCF 	 XK_F18 	 XK_L8 */
   "\005\033[33$",	/* 0xFFD0 	 XK_F19 	 XK_L9 */
   "\005\033[34$",	/* 0xFFD1 	 XK_F20 	 XK_L10 */
   "\005\033[35$",	/* 0xFFD2 	 XK_F21 	 XK_R1 */
   "\005\033[36$",	/* 0xFFD3 	 XK_F22 	 XK_R2 */
   "\005\033[37$",	/* 0xFFD4 	 XK_F23 	 XK_R3 */
   "\005\033[38$",	/* 0xFFD5 	 XK_F24 	 XK_R4 */
   "\005\033[39$",	/* 0xFFD6 	 XK_F25 	 XK_R5 */
   "\005\033[40$",	/* 0xFFD7 	 XK_F26 	 XK_R6 */
   "\005\033[41$",	/* 0xFFD8 	 XK_F27 	 XK_R7 */
   "\005\033[42$",	/* 0xFFD9 	 XK_F28 	 XK_R8 */
   "\005\033[43$",	/* 0xFFDA 	 XK_F29 	 XK_R9 */
   "\005\033[44$",	/* 0xFFDB 	 XK_F30 	 XK_R10 */
   "\005\033[45$",	/* 0xFFDC 	 XK_F31 	 XK_R11 */
   "\005\033[46$",	/* 0xFFDD 	 XK_F32 	 XK_R12 */
   "\005\033[47$",	/* 0xFFDE 	 XK_R13 	 XK_F33 */
   "\005\033[48$",	/* 0xFFDF 	 XK_F34 	 XK_R14 */
   "\005\033[49$",	/* 0xFFE0 	 XK_F35 	 XK_R15 */
   "",			/* 0xFFE1 	 XK_Shift_L 	 XK_Shift_L */
   "",			/* 0xFFE2 	 XK_Shift_R */
   "",			/* 0xFFE3 	 XK_Control_L */
   "",			/* 0xFFE4 	 XK_Control_R */
   "",			/* 0xFFE5 	 XK_Caps_Lock */
   "",			/* 0xFFE6 	 XK_Shift_Lock */
   "",			/* 0xFFE7 	 XK_Meta_L */
   "",			/* 0xFFE8 	 XK_Meta_R */
   "",			/* 0xFFE9 	 XK_Alt_L */
   "",			/* 0xFFEA 	 XK_Alt_R */
   "",			/* 0xFFEB 	 XK_Super_L */
   "",			/* 0xFFEC 	 XK_Super_R */
   "",			/* 0xFFED 	 XK_Hyper_L */
   "",			/* 0xFFEE 	 XK_Hyper_R */
   "",			/* 0xFFEF */
   "",			/* 0xFFF0 */
   "",			/* 0xFFF1 */
   "",			/* 0xFFF2 */
   "",			/* 0xFFF3 */
   "",			/* 0xFFF4 */
   "",			/* 0xFFF5 */
   "",			/* 0xFFF6 */
   "",			/* 0xFFF7 */
   "",			/* 0xFFF8 */
   "",			/* 0xFFF9 */
   "",			/* 0xFFFA */
   "",			/* 0xFFFB */
   "",			/* 0xFFFC */
   "",			/* 0xFFFD */
   "",			/* 0xFFFE */
   "\001\177"		/* 0xFFFF 	 XK_Delete */
};

static KeySym_Mapping_Type Control_KeySym_Map_FF[256] =
{
   "\004\033[3^",	/* 0xFF00                        Remove */
   "",			/* 0xFF01 */
   "",			/* 0xFF02 */
   "",			/* 0xFF03 */
   "",			/* 0xFF04 */
   "",			/* 0xFF05 */
   "",			/* 0xFF06 */
   "",			/* 0xFF07 */
   "\001\177",		/* 0xFF08 	 XK_BackSpace */
   "\001\t",		/* 0xFF09 	 XK_Tab */
   "\001\n",		/* 0xFF0A 	 XK_Linefeed */
   "",			/* 0xFF0B 	 XK_Clear */
   "",			/* 0xFF0C */
   "\001\r",		/* 0xFF0D 	 XK_Return */
   "",			/* 0xFF0E */
   "",			/* 0xFF0F */
   "",			/* 0xFF10 */
   "",			/* 0xFF11 */
   "",			/* 0xFF12 */
   "",			/* 0xFF13 	 XK_Pause */
   "",			/* 0xFF14 	 XK_Scroll_Lock */
   "",			/* 0xFF15 */
   "",			/* 0xFF16 */
   "",			/* 0xFF17 */
   "",			/* 0xFF18 */
   "",			/* 0xFF19 */
   "",			/* 0xFF1A */
   "\001\033",		/* 0xFF1B 	 XK_Escape */
   "",			/* 0xFF1C */
   "",			/* 0xFF1D */
   "",			/* 0xFF1E */
   "",			/* 0xFF1F */
   "",			/* 0xFF20 	 XK_Multi_key */
   "",			/* 0xFF21 	 XK_Kanji */
   "",			/* 0xFF22 	 XK_Muhenkan */
   "",			/* 0xFF23 	 XK_Henkan_Mode 	 XK_Henkan */
   "",			/* 0xFF24 	 XK_Romaji */
   "",			/* 0xFF25 	 XK_Hiragana */
   "",			/* 0xFF26 	 XK_Katakana */
   "",			/* 0xFF27 	 XK_Hiragana_Katakana */
   "",			/* 0xFF28 	 XK_Zenkaku */
   "",			/* 0xFF29 	 XK_Hankaku */
   "",			/* 0xFF2A 	 XK_Zenkaku_Hankaku */
   "",			/* 0xFF2B 	 XK_Touroku */
   "",			/* 0xFF2C 	 XK_Massyo */
   "",			/* 0xFF2D 	 XK_Kana_Lock */
   "",			/* 0xFF2E 	 XK_Kana_Shift */
   "",			/* 0xFF2F 	 XK_Eisu_Shift */
   "",			/* 0xFF30 	 XK_Eisu_toggle */
   "",			/* 0xFF31 */
   "",			/* 0xFF32 */
   "",			/* 0xFF33 */
   "",			/* 0xFF34 */
   "",			/* 0xFF35 */
   "",			/* 0xFF36 */
   "",			/* 0xFF37 */
   "",			/* 0xFF38 */
   "",			/* 0xFF39 */
   "",			/* 0xFF3A */
   "",			/* 0xFF3B */
   "",			/* 0xFF3C */
   "",			/* 0xFF3D */
   "",			/* 0xFF3E */
   "",			/* 0xFF3F */
   "",			/* 0xFF40 */
   "",			/* 0xFF41 */
   "",			/* 0xFF42 */
   "",			/* 0xFF43 */
   "",			/* 0xFF44 */
   "",			/* 0xFF45 */
   "",			/* 0xFF46 */
   "",			/* 0xFF47 */
   "",			/* 0xFF48 */
   "",			/* 0xFF49 */
   "",			/* 0xFF4A */
   "",			/* 0xFF4B */
   "",			/* 0xFF4C */
   "",			/* 0xFF4D */
   "",			/* 0xFF4E */
   "",			/* 0xFF4F */
   "\004\033[1^",	/* 0xFF50 	 XK_Home */
   "\003\033[\004",	/* 0xFF51 	 XK_Left */
   "\003\033[\001",	/* 0xFF52 	 XK_Up */
   "\003\033[\003",	/* 0xFF53 	 XK_Right */
   "\003\033[\002",	/* 0xFF54 	 XK_Down */
   "\004\033[5^",	/* 0xFF55 	 XK_Prior */
   "\004\033[6^",	/* 0xFF56 	 XK_Next */
   "\004\033[4^",	/* 0xFF57 	 XK_End */
   "\001\001",		/* 0xFF58 	 XK_Begin */
   "",			/* 0xFF59 */
   "",			/* 0xFF5A */
   "",			/* 0xFF5B */
   "",			/* 0xFF5C */
   "",			/* 0xFF5D */
   "",			/* 0xFF5E */
   "",			/* 0xFF5F */
   "\004\033[4^",	/* 0xFF60 	 XK_Select */
   "",			/* 0xFF61 	 XK_Print */
   "",			/* 0xFF62 	 XK_Execute */
   "\004\033[2^",	/* 0xFF63 	 XK_Insert */
   "",			/* 0xFF64 */
   "\001\037",		/* 0xFF65 	 XK_Undo 	 ^_ */
   "",			/* 0xFF66 	 XK_Redo */
   "\005\033[29^",	/* 0xFF67 	 XK_Menu         Do */
   "\004\033[1^",	/* 0xFF68 	 XK_Find */
   "\001\007",		/* 0xFF69 	 XK_Cancel */
   "\005\033[28^",	/* 0xFF6A 	 XK_Help */
   "",			/* 0xFF6B 	 XK_Break */
   "",			/* 0xFF6C */
   "",			/* 0xFF6D */
   "",			/* 0xFF6E */
   "",			/* 0xFF6F */
   "",			/* 0xFF70 */
   "",			/* 0xFF71 */
   "",			/* 0xFF72 */
   "",			/* 0xFF73 */
   "",			/* 0xFF74 */
   "",			/* 0xFF75 */
   "",			/* 0xFF76 */
   "",			/* 0xFF77 */
   "",			/* 0xFF78 */
   "",			/* 0xFF79 */
   "",			/* 0xFF7A */
   "",			/* 0xFF7B */
   "",			/* 0xFF7C */
   "",			/* 0xFF7D */
   "",			/* 0xFF7E */
   "",			/* 0xFF7F 	 XK_Num_Lock */
   "",			/* 0xFF80 	 XK_KP_Space */
   "",			/* 0xFF81 */
   "",			/* 0xFF82 */
   "",			/* 0xFF83 */
   "",			/* 0xFF84 */
   "",			/* 0xFF85 */
   "",			/* 0xFF86 */
   "",			/* 0xFF87 */
   "",			/* 0xFF88 */
   "",			/* 0xFF89 	 XK_KP_Tab */
   "",			/* 0xFF8A */
   "",			/* 0xFF8B */
   "",			/* 0xFF8C */
   "\003\033OM",	/* 0xFF8D 	 XK_KP_Enter */
   "",			/* 0xFF8E */
   "",			/* 0xFF8F */
   "",			/* 0xFF90 */
   "\003\033OP",	/* 0xFF91 	 XK_KP_F1        PF1 */
   "\003\033OQ",	/* 0xFF92 	 XK_KP_F2        PF2 */
   "\003\033OR",	/* 0xFF93 	 XK_KP_F3        PF3 */
   "\003\033OS",	/* 0xFF94 	 XK_KP_F4        PF4 */
   "\003\033Ow",	/* 0xFF95 	 XK_KP_Home */
   "\003\033Ot",	/* 0xFF96 	 XK_KP_Left */
   "\003\033Ox",	/* 0xFF97 	 XK_KP_Up */
   "\003\033Ov",	/* 0xFF98 	 XK_KP_Right */
   "\003\033Or",	/* 0xFF99 	 XK_KP_Down */
   "\003\033Oy",	/* 0xFF9A 	 XK_KP_Prior */
   "\003\033Os",	/* 0xFF9B 	 XK_KP_Next */
   "\003\033Oq",	/* 0xFF9C 	 XK_KP_End */
   "\003\033Ou",	/* 0xFF9D 	 XK_KP_Begin */
   "\003\033Op",	/* 0xFF9E 	 XK_KP_Insert */
   "\003\033On",	/* 0xFF9F 	 XK_KP_Delete */
   "",			/* 0xFFA0 */
   "",			/* 0xFFA1 */
   "",			/* 0xFFA2 */
   "",			/* 0xFFA3 */
   "",			/* 0xFFA4 */
   "",			/* 0xFFA5 */
   "",			/* 0xFFA6 */
   "",			/* 0xFFA7 */
   "",			/* 0xFFA8 */
   "",			/* 0xFFA9 */
   "",			/* 0xFFAA 	 XK_KP_Multiply */
   "",			/* 0xFFAB 	 XK_KP_Add */
   "\003\033Ol",	/* 0xFFAC 	 XK_KP_Separator KP , */
   "\003\033Om",	/* 0xFFAD 	 XK_KP_Subtract  KP - */
   "\003\033On",	/* 0xFFAE 	 XK_KP_Decimal   KP . */
   "",			/* 0xFFAF 	 XK_KP_Divide */
   "\003\033Op",	/* 0xFFB0 	 XK_KP_0 */
   "\003\033Oq",	/* 0xFFB1 	 XK_KP_1 */
   "\003\033Or",	/* 0xFFB2 	 XK_KP_2 */
   "\003\033Os",	/* 0xFFB3 	 XK_KP_3 */
   "\003\033Ot",	/* 0xFFB4 	 XK_KP_4 */
   "\003\033Ou",	/* 0xFFB5 	 XK_KP_5 */
   "\003\033Ov",	/* 0xFFB6 	 XK_KP_6 */
   "\003\033Ow",	/* 0xFFB7 	 XK_KP_7 */
   "\003\033Ox",	/* 0xFFB8 	 XK_KP_8 */
   "\003\033Oy",	/* 0xFFB9 	 XK_KP_9 */
   "",			/* 0xFFBA */
   "",			/* 0xFFBB */
   "",			/* 0xFFBC */
   "",			/* 0xFFBD 	 XK_KP_Equal */
   "\005\033[11^",	/* 0xFFBE 	 XK_F1 */
   "\005\033[12^",	/* 0xFFBF 	 XK_F2 */
   "\005\033[13^",	/* 0xFFC0 	 XK_F3 */
   "\005\033[14^",	/* 0xFFC1 	 XK_F4 */
   "\005\033[15^",	/* 0xFFC2 	 XK_F5 */
   "\005\033[17^",	/* 0xFFC3 	 XK_F6 */
   "\005\033[18^",	/* 0xFFC4 	 XK_F7 */
   "\005\033[19^",	/* 0xFFC5 	 XK_F8 */
   "\005\033[20^",	/* 0xFFC6 	 XK_F9 */
   "\005\033[21^",	/* 0xFFC7 	 XK_F10 */
   "\005\033[23^",	/* 0xFFC8 	 XK_F11 	 XK_L1 */
   "\005\033[24^",	/* 0xFFC9 	 XK_F12 	 XK_L2 */
   "\005\033[25^",	/* 0xFFCA 	 XK_F13 	 XK_L3 */
   "\005\033[26^",	/* 0xFFCB 	 XK_F14 	 XK_L4 */
   "\005\033[28^",	/* 0xFFCC 	 XK_F15 	 XK_L5 */
   "\005\033[29^",	/* 0xFFCD 	 XK_F16 	 XK_L6 */
   "\005\033[31^",	/* 0xFFCE 	 XK_F17 	 XK_L7 */
   "\005\033[32^",	/* 0xFFCF 	 XK_F18 	 XK_L8 */
   "\005\033[33^",	/* 0xFFD0 	 XK_F19 	 XK_L9 */
   "\005\033[34^",	/* 0xFFD1 	 XK_F20 	 XK_L10 */
   "\005\033[35^",	/* 0xFFD2 	 XK_F21 	 XK_R1 */
   "\005\033[36^",	/* 0xFFD3 	 XK_F22 	 XK_R2 */
   "\005\033[37^",	/* 0xFFD4 	 XK_F23 	 XK_R3 */
   "\005\033[38^",	/* 0xFFD5 	 XK_F24 	 XK_R4 */
   "\005\033[39^",	/* 0xFFD6 	 XK_F25 	 XK_R5 */
   "\005\033[40^",	/* 0xFFD7 	 XK_F26 	 XK_R6 */
   "\005\033[41^",	/* 0xFFD8 	 XK_F27 	 XK_R7 */
   "\005\033[42^",	/* 0xFFD9 	 XK_F28 	 XK_R8 */
   "\005\033[43^",	/* 0xFFDA 	 XK_F29 	 XK_R9 */
   "\005\033[44^",	/* 0xFFDB 	 XK_F30 	 XK_R10 */
   "\005\033[45^",	/* 0xFFDC 	 XK_F31 	 XK_R11 */
   "\005\033[46^",	/* 0xFFDD 	 XK_F32 	 XK_R12 */
   "\005\033[47^",	/* 0xFFDE 	 XK_R13 	 XK_F33 */
   "\005\033[48^",	/* 0xFFDF 	 XK_F34 	 XK_R14 */
   "\005\033[49^",	/* 0xFFE0 	 XK_F35 	 XK_R15 */
   "",			/* 0xFFE1 	 XK_Shift_L 	 XK_Shift_L */
   "",			/* 0xFFE2 	 XK_Shift_R */
   "",			/* 0xFFE3 	 XK_Control_L */
   "",			/* 0xFFE4 	 XK_Control_R */
   "",			/* 0xFFE5 	 XK_Caps_Lock */
   "",			/* 0xFFE6 	 XK_Shift_Lock */
   "",			/* 0xFFE7 	 XK_Meta_L */
   "",			/* 0xFFE8 	 XK_Meta_R */
   "",			/* 0xFFE9 	 XK_Alt_L */
   "",			/* 0xFFEA 	 XK_Alt_R */
   "",			/* 0xFFEB 	 XK_Super_L */
   "",			/* 0xFFEC 	 XK_Super_R */
   "",			/* 0xFFED 	 XK_Hyper_L */
   "",			/* 0xFFEE 	 XK_Hyper_R */
   "",			/* 0xFFEF */
   "",			/* 0xFFF0 */
   "",			/* 0xFFF1 */
   "",			/* 0xFFF2 */
   "",			/* 0xFFF3 */
   "",			/* 0xFFF4 */
   "",			/* 0xFFF5 */
   "",			/* 0xFFF6 */
   "",			/* 0xFFF7 */
   "",			/* 0xFFF8 */
   "",			/* 0xFFF9 */
   "",			/* 0xFFFA */
   "",			/* 0xFFFB */
   "",			/* 0xFFFC */
   "",			/* 0xFFFD */
   "",			/* 0xFFFE */
   "\001\177"		/* 0xFFFF 	 XK_Delete */
};

static KeySym_Mapping_Type Control_Shift_KeySym_Map_FF[256] =
{
   "\004\033[3^",	/* 0xFF00                        Remove */
   "",			/* 0xFF01 */
   "",			/* 0xFF02 */
   "",			/* 0xFF03 */
   "",			/* 0xFF04 */
   "",			/* 0xFF05 */
   "",			/* 0xFF06 */
   "",			/* 0xFF07 */
   "\001\177",		/* 0xFF08 	 XK_BackSpace */
   "\001\t",		/* 0xFF09 	 XK_Tab */
   "\001\n",		/* 0xFF0A 	 XK_Linefeed */
   "",			/* 0xFF0B 	 XK_Clear */
   "",			/* 0xFF0C */
   "\001\r",		/* 0xFF0D 	 XK_Return */
   "",			/* 0xFF0E */
   "",			/* 0xFF0F */
   "",			/* 0xFF10 */
   "",			/* 0xFF11 */
   "",			/* 0xFF12 */
   "",			/* 0xFF13 	 XK_Pause */
   "",			/* 0xFF14 	 XK_Scroll_Lock */
   "",			/* 0xFF15 */
   "",			/* 0xFF16 */
   "",			/* 0xFF17 */
   "",			/* 0xFF18 */
   "",			/* 0xFF19 */
   "",			/* 0xFF1A */
   "\001\033",		/* 0xFF1B 	 XK_Escape */
   "",			/* 0xFF1C */
   "",			/* 0xFF1D */
   "",			/* 0xFF1E */
   "",			/* 0xFF1F */
   "",			/* 0xFF20 	 XK_Multi_key */
   "",			/* 0xFF21 	 XK_Kanji */
   "",			/* 0xFF22 	 XK_Muhenkan */
   "",			/* 0xFF23 	 XK_Henkan_Mode 	 XK_Henkan */
   "",			/* 0xFF24 	 XK_Romaji */
   "",			/* 0xFF25 	 XK_Hiragana */
   "",			/* 0xFF26 	 XK_Katakana */
   "",			/* 0xFF27 	 XK_Hiragana_Katakana */
   "",			/* 0xFF28 	 XK_Zenkaku */
   "",			/* 0xFF29 	 XK_Hankaku */
   "",			/* 0xFF2A 	 XK_Zenkaku_Hankaku */
   "",			/* 0xFF2B 	 XK_Touroku */
   "",			/* 0xFF2C 	 XK_Massyo */
   "",			/* 0xFF2D 	 XK_Kana_Lock */
   "",			/* 0xFF2E 	 XK_Kana_Shift */
   "",			/* 0xFF2F 	 XK_Eisu_Shift */
   "",			/* 0xFF30 	 XK_Eisu_toggle */
   "",			/* 0xFF31 */
   "",			/* 0xFF32 */
   "",			/* 0xFF33 */
   "",			/* 0xFF34 */
   "",			/* 0xFF35 */
   "",			/* 0xFF36 */
   "",			/* 0xFF37 */
   "",			/* 0xFF38 */
   "",			/* 0xFF39 */
   "",			/* 0xFF3A */
   "",			/* 0xFF3B */
   "",			/* 0xFF3C */
   "",			/* 0xFF3D */
   "",			/* 0xFF3E */
   "",			/* 0xFF3F */
   "",			/* 0xFF40 */
   "",			/* 0xFF41 */
   "",			/* 0xFF42 */
   "",			/* 0xFF43 */
   "",			/* 0xFF44 */
   "",			/* 0xFF45 */
   "",			/* 0xFF46 */
   "",			/* 0xFF47 */
   "",			/* 0xFF48 */
   "",			/* 0xFF49 */
   "",			/* 0xFF4A */
   "",			/* 0xFF4B */
   "",			/* 0xFF4C */
   "",			/* 0xFF4D */
   "",			/* 0xFF4E */
   "",			/* 0xFF4F */
   "\004\033[1^",	/* 0xFF50 	 XK_Home */
   "\003\033[\004",	/* 0xFF51 	 XK_Left */
   "\003\033[\001",	/* 0xFF52 	 XK_Up */
   "\003\033[\003",	/* 0xFF53 	 XK_Right */
   "\003\033[\002",	/* 0xFF54 	 XK_Down */
   "\004\033[5^",	/* 0xFF55 	 XK_Prior */
   "\004\033[6^",	/* 0xFF56 	 XK_Next */
   "\004\033[4^",	/* 0xFF57 	 XK_End */
   "\001\001",		/* 0xFF58 	 XK_Begin */
   "",			/* 0xFF59 */
   "",			/* 0xFF5A */
   "",			/* 0xFF5B */
   "",			/* 0xFF5C */
   "",			/* 0xFF5D */
   "",			/* 0xFF5E */
   "",			/* 0xFF5F */
   "\004\033[4^",	/* 0xFF60 	 XK_Select */
   "",			/* 0xFF61 	 XK_Print */
   "",			/* 0xFF62 	 XK_Execute */
   "\004\033[2^",	/* 0xFF63 	 XK_Insert */
   "",			/* 0xFF64 */
   "\001\037",		/* 0xFF65 	 XK_Undo 	 ^_ */
   "",			/* 0xFF66 	 XK_Redo */
   "\005\033[29^",	/* 0xFF67 	 XK_Menu         Do */
   "\004\033[1^",	/* 0xFF68 	 XK_Find */
   "\001\007",		/* 0xFF69 	 XK_Cancel */
   "\005\033[28^",	/* 0xFF6A 	 XK_Help */
   "",			/* 0xFF6B 	 XK_Break */
   "",			/* 0xFF6C */
   "",			/* 0xFF6D */
   "",			/* 0xFF6E */
   "",			/* 0xFF6F */
   "",			/* 0xFF70 */
   "",			/* 0xFF71 */
   "",			/* 0xFF72 */
   "",			/* 0xFF73 */
   "",			/* 0xFF74 */
   "",			/* 0xFF75 */
   "",			/* 0xFF76 */
   "",			/* 0xFF77 */
   "",			/* 0xFF78 */
   "",			/* 0xFF79 */
   "",			/* 0xFF7A */
   "",			/* 0xFF7B */
   "",			/* 0xFF7C */
   "",			/* 0xFF7D */
   "",			/* 0xFF7E */
   "",			/* 0xFF7F 	 XK_Num_Lock */
   "",			/* 0xFF80 	 XK_KP_Space */
   "",			/* 0xFF81 */
   "",			/* 0xFF82 */
   "",			/* 0xFF83 */
   "",			/* 0xFF84 */
   "",			/* 0xFF85 */
   "",			/* 0xFF86 */
   "",			/* 0xFF87 */
   "",			/* 0xFF88 */
   "",			/* 0xFF89 	 XK_KP_Tab */
   "",			/* 0xFF8A */
   "",			/* 0xFF8B */
   "",			/* 0xFF8C */
   "\003\033OM",	/* 0xFF8D 	 XK_KP_Enter */
   "",			/* 0xFF8E */
   "",			/* 0xFF8F */
   "",			/* 0xFF90 */
   "\003\033OP",	/* 0xFF91 	 XK_KP_F1        PF1 */
   "\003\033OQ",	/* 0xFF92 	 XK_KP_F2        PF2 */
   "\003\033OR",	/* 0xFF93 	 XK_KP_F3        PF3 */
   "\003\033OS",	/* 0xFF94 	 XK_KP_F4        PF4 */
   "",			/* 0xFF95 */
   "",			/* 0xFF96 */
   "",			/* 0xFF97 */
   "",			/* 0xFF98 */
   "",			/* 0xFF99 */
   "",			/* 0xFF9A */
   "",			/* 0xFF9B */
   "",			/* 0xFF9C */
   "",			/* 0xFF9D */
   "",			/* 0xFF9E */
   "",			/* 0xFF9F */
   "",			/* 0xFFA0 */
   "",			/* 0xFFA1 */
   "",			/* 0xFFA2 */
   "",			/* 0xFFA3 */
   "",			/* 0xFFA4 */
   "",			/* 0xFFA5 */
   "",			/* 0xFFA6 */
   "",			/* 0xFFA7 */
   "",			/* 0xFFA8 */
   "",			/* 0xFFA9 */
   "",			/* 0xFFAA 	 XK_KP_Multiply */
   "",			/* 0xFFAB 	 XK_KP_Add */
   "\003\033Ol",	/* 0xFFAC 	 XK_KP_Separator KP , */
   "\003\033Om",	/* 0xFFAD 	 XK_KP_Subtract  KP - */
   "\003\033On",	/* 0xFFAE 	 XK_KP_Decimal   KP . */
   "",			/* 0xFFAF 	 XK_KP_Divide */
   "\003\033Op",	/* 0xFFB0 	 XK_KP_0 */
   "\003\033Oq",	/* 0xFFB1 	 XK_KP_1 */
   "\003\033Or",	/* 0xFFB2 	 XK_KP_2 */
   "\003\033Os",	/* 0xFFB3 	 XK_KP_3 */
   "\003\033Ot",	/* 0xFFB4 	 XK_KP_4 */
   "\003\033Ou",	/* 0xFFB5 	 XK_KP_5 */
   "\003\033Ov",	/* 0xFFB6 	 XK_KP_6 */
   "\003\033Ow",	/* 0xFFB7 	 XK_KP_7 */
   "\003\033Ox",	/* 0xFFB8 	 XK_KP_8 */
   "\003\033Oy",	/* 0xFFB9 	 XK_KP_9 */
   "",			/* 0xFFBA */
   "",			/* 0xFFBB */
   "",			/* 0xFFBC */
   "",			/* 0xFFBD 	 XK_KP_Equal */
   "\005\033[11^",	/* 0xFFBE 	 XK_F1 */
   "\005\033[12^",	/* 0xFFBF 	 XK_F2 */
   "\005\033[13^",	/* 0xFFC0 	 XK_F3 */
   "\005\033[14^",	/* 0xFFC1 	 XK_F4 */
   "\005\033[15^",	/* 0xFFC2 	 XK_F5 */
   "\005\033[17^",	/* 0xFFC3 	 XK_F6 */
   "\005\033[18^",	/* 0xFFC4 	 XK_F7 */
   "\005\033[19^",	/* 0xFFC5 	 XK_F8 */
   "\005\033[20^",	/* 0xFFC6 	 XK_F9 */
   "\005\033[21^",	/* 0xFFC7 	 XK_F10 */
   "\005\033[23^",	/* 0xFFC8 	 XK_F11 	 XK_L1 */
   "\005\033[24^",	/* 0xFFC9 	 XK_F12 	 XK_L2 */
   "\005\033[25^",	/* 0xFFCA 	 XK_F13 	 XK_L3 */
   "\005\033[26^",	/* 0xFFCB 	 XK_F14 	 XK_L4 */
   "\005\033[28^",	/* 0xFFCC 	 XK_F15 	 XK_L5 */
   "\005\033[29^",	/* 0xFFCD 	 XK_F16 	 XK_L6 */
   "\005\033[31^",	/* 0xFFCE 	 XK_F17 	 XK_L7 */
   "\005\033[32^",	/* 0xFFCF 	 XK_F18 	 XK_L8 */
   "\005\033[33^",	/* 0xFFD0 	 XK_F19 	 XK_L9 */
   "\005\033[34^",	/* 0xFFD1 	 XK_F20 	 XK_L10 */
   "\005\033[35^",	/* 0xFFD2 	 XK_F21 	 XK_R1 */
   "\005\033[36^",	/* 0xFFD3 	 XK_F22 	 XK_R2 */
   "\005\033[37^",	/* 0xFFD4 	 XK_F23 	 XK_R3 */
   "\005\033[38^",	/* 0xFFD5 	 XK_F24 	 XK_R4 */
   "\005\033[39^",	/* 0xFFD6 	 XK_F25 	 XK_R5 */
   "\005\033[40^",	/* 0xFFD7 	 XK_F26 	 XK_R6 */
   "\005\033[41^",	/* 0xFFD8 	 XK_F27 	 XK_R7 */
   "\005\033[42^",	/* 0xFFD9 	 XK_F28 	 XK_R8 */
   "\005\033[43^",	/* 0xFFDA 	 XK_F29 	 XK_R9 */
   "\005\033[44^",	/* 0xFFDB 	 XK_F30 	 XK_R10 */
   "\005\033[45^",	/* 0xFFDC 	 XK_F31 	 XK_R11 */
   "\005\033[46^",	/* 0xFFDD 	 XK_F32 	 XK_R12 */
   "\005\033[47^",	/* 0xFFDE 	 XK_R13 	 XK_F33 */
   "\005\033[48^",	/* 0xFFDF 	 XK_F34 	 XK_R14 */
   "\005\033[49^",	/* 0xFFE0 	 XK_F35 	 XK_R15 */
   "",			/* 0xFFE1 	 XK_Shift_L 	 XK_Shift_L */
   "",			/* 0xFFE2 	 XK_Shift_R */
   "",			/* 0xFFE3 	 XK_Control_L */
   "",			/* 0xFFE4 	 XK_Control_R */
   "",			/* 0xFFE5 	 XK_Caps_Lock */
   "",			/* 0xFFE6 	 XK_Shift_Lock */
   "",			/* 0xFFE7 	 XK_Meta_L */
   "",			/* 0xFFE8 	 XK_Meta_R */
   "",			/* 0xFFE9 	 XK_Alt_L */
   "",			/* 0xFFEA 	 XK_Alt_R */
   "",			/* 0xFFEB 	 XK_Super_L */
   "",			/* 0xFFEC 	 XK_Super_R */
   "",			/* 0xFFED 	 XK_Hyper_L */
   "",			/* 0xFFEE 	 XK_Hyper_R */
   "",			/* 0xFFEF */
   "",			/* 0xFFF0 */
   "",			/* 0xFFF1 */
   "",			/* 0xFFF2 */
   "",			/* 0xFFF3 */
   "",			/* 0xFFF4 */
   "",			/* 0xFFF5 */
   "",			/* 0xFFF6 */
   "",			/* 0xFFF7 */
   "",			/* 0xFFF8 */
   "",			/* 0xFFF9 */
   "",			/* 0xFFFA */
   "",			/* 0xFFFB */
   "",			/* 0xFFFC */
   "",			/* 0xFFFD */
   "",			/* 0xFFFE */
   "\001\177"		/* 0xFFFF 	 XK_Delete */
};

static KeySym_Mapping_Type *KeySym_Maps[256];
static KeySym_Mapping_Type *Shift_KeySym_Maps[256];
static KeySym_Mapping_Type *Control_KeySym_Maps[256];
static KeySym_Mapping_Type *Control_Shift_KeySym_Maps[256];

static int init_xkeys (void)
{
   KeySym_Maps[0xFF] = KeySym_Map_FF;
   Shift_KeySym_Maps[0xFF] = Shift_KeySym_Map_FF;
   Control_Shift_KeySym_Maps[0xFF] = Control_Shift_KeySym_Map_FF;
   Control_KeySym_Maps[0xFF] = Control_KeySym_Map_FF;

   return 0;
}

static void x_set_keysym (int *np, int *shift, char *str) /*{{{*/
{
   unsigned int keysym = (unsigned int) *np;
   KeySym_Mapping_Type **maps, *map;
   unsigned int len;
   unsigned int i;

   str = SLang_process_keystring (str);
   if (str == NULL)
     return;
   len = (unsigned int) (unsigned char)str[0];
   if (len > MAX_KEYSYM_STRING_LEN)
     return;

   switch (*shift)
     {
      case '$':
	maps = Shift_KeySym_Maps;
	break;

      case '^':
	maps = Control_KeySym_Maps;
	break;

      case '%':
	maps = Control_Shift_KeySym_Maps;
	break;

      default:
	maps = KeySym_Maps;
	break;
     }
   i = (keysym >> 8)&0xFF;
   if (NULL == (map = maps[i]))
     {
	map = (KeySym_Mapping_Type *)SLcalloc (256, sizeof (KeySym_Mapping_Type));
	if (map == NULL)
	  return;
	maps[i] = map;
     }
   map += keysym&0xFF;

   memcpy ((char *)map, str, len);
   /* was: SLMEMCPY (map[n], str, MAX_KEYSYM_STRING_LEN); */
   *(char *)map -= 1;
}

/*}}}*/

static unsigned char *map_keysym_to_keyseq (unsigned int keysym, unsigned long mask)
{
   KeySym_Mapping_Type **maps;
   KeySym_Mapping_Type *map;
   unsigned char *keyseq;

   switch (mask)
     {
      default:
	maps = KeySym_Maps;
	break;

      case ShiftMask:
	maps = Shift_KeySym_Maps;
	break;

      case ControlMask:
	maps = Control_KeySym_Maps;
	break;

      case ShiftMask|ControlMask:
	maps = Control_Shift_KeySym_Maps;
	break;
     }

   map = maps [(keysym >> 8) & 0xFF];
   if (map == NULL)
     return NULL;
   keyseq = (unsigned char *) map[keysym & 0xFF];
   if (*keyseq == 0)
     return NULL;
   return keyseq;
}

