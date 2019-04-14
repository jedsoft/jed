/* Copyright (c) 1992-2019 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
#include "config.h"
#include "jed-feat.h"

#if !defined (_MSC_VER) && !defined (__EMX__) && !defined(__WATCOMC__) && !defined(__IBMC__)
# include <dir.h>
#endif

#ifdef	__IBMC__
# include <ctype.h>
# include <direct.h>
#endif

#define INCL_BASE
#define INCL_NOPM
#define INCL_VIO
#define INCL_KBD
#define INCL_DOS

#include <os2.h>

#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#ifndef __IBMC__
# include <dos.h>
#endif
#include <errno.h>

#include "sysdep.h"
#include "screen.h"
#include "buffer.h"
#include "misc.h"
#include "hooks.h"
#if JED_HAS_SUBPROCESSES
#include "jprocess.h"
#endif

#include "dos_os2.c"

/* extern char *get_cwd (void); */
void delay (int time);

void set_kbd (void);
/* void os2_kbdhandler_thread (void); */

#define lowercase strlwr

KBDINFO	initialKbdInfo;	/* keyboard info		*/

static int atEnd = 0;

#ifdef __os2_16__

typedef USHORT APIRET;
typedef HSEM OS2_SEM;

# define DosRequestMutexSem(hmtx,timeout) DosSemRequest(hmtx,timeout)
# define DosReleaseMutexSem(hmtx) DosSemClear(hmtx)
# define DosCloseMutexSem(hmtx) DosCloseSem(hmtx)

#else /* !defined(__os2_16__) */

typedef HMTX OS2_SEM;

#endif

/* Semaphore code for accessing keyboard buffers in threads*/

static OS2_SEM Hmtx_Kbd ;     /* Mutex Semaphore */

APIRET CreateSem(OS2_SEM *Hmtx)
{
   APIRET result;
#ifdef __os2_16__
   static SemNo = 0;
   char SemName[32];
   sprintf(SemName, "\\SEM\\jed\\%u", getpid() + SemNo++);
   result = DosCreateSem (0, Hmtx, SemName);
#else
   result = DosCreateMutexSem (NULL, Hmtx, 0, 0);
#endif
   DosReleaseMutexSem (*Hmtx);
   return result;
}

APIRET CloseSem (OS2_SEM Hmtx)
{
   return( DosCloseMutexSem (Hmtx) );
}

/* define these using macros to improve efficiency */
#define RequestSem(Hmtx) DosRequestMutexSem (Hmtx, -1);
#define ReleaseSem(Hmtx) DosReleaseMutexSem (Hmtx);

#undef USING_INPUT_BUFFER
#undef DONE_WITH_INPUT_BUFFER

#define USING_INPUT_BUFFER RequestSem (Hmtx_Kbd);
#define DONE_WITH_INPUT_BUFFER ReleaseSem (Hmtx_Kbd);

/* do this using a macro to improve efficiency */
#define os2_buffer_keystring(s, n) \
if (!(n + Input_Buffer_Len > MAX_INPUT_BUFFER_LEN - 3)) { \
   \
   USING_INPUT_BUFFER \
   memcpy ((char *) Input_Buffer + Input_Buffer_Len, s, n); \
   Input_Buffer_Len += n; \
   DONE_WITH_INPUT_BUFFER \
}

#define THREADSTACKSIZE      32768
static TID Os2_Kbdhandler_ThreadID = (TID) 0 ;

/* in a separate thread, read keyboard and put the keys into JED's keyboard
 * buffer */

static int enhancedKeyboard;

#define keyWaiting() (Input_Buffer_Len)
#include "pcscan.c"

static void os2_thread_process_key (unsigned int c0, unsigned int c1,
				    unsigned int shift)
{
   unsigned char keybuf[16];
   unsigned int scan;
   unsigned int num;

   shift = shift & 0xF;
   scan = ((c1 << 8) | c0) & 0xFFFF;

   num = jed_scan_to_key (scan, shift, keybuf);
   os2_buffer_keystring (keybuf, num);
}

static void os2_kbdhandler_thread (void)
{
   KBDKEYINFO keyInfo;

   while (!atEnd)
     {
	/* at end is a flag */
	KbdCharIn(&keyInfo, IO_NOWAIT, 0);       /* get a character	*/
	if (keyInfo.fbStatus & 0x040)
	  {
	     /* found a char process it */

	     if (keyInfo.chChar == Jed_Abort_Char)
	       {
		  if (Ignore_User_Abort == 0) SLang_Error = USER_BREAK;
		  SLKeyBoard_Quit = 1;
	       }

	 /* now process them */
	 /* read codes from buffer */
	     os2_thread_process_key (keyInfo.chChar, keyInfo.chScan,
				     keyInfo.fsState);

	  }
	else
	  {
	     /* no char available*/
	     set_kbd();      /* is this necessary?  */
	     DosSleep (50);
	  }
     }
}

static void os2_kbdthread (void *Args)
{
   (void) Args;
   os2_kbdhandler_thread ();
   atEnd = 0;          /* reset the flag */
   _endthread();
}

/* The code below is in the main thread */

void set_kbd()
{
   KBDINFO kbdInfo;

   kbdInfo = initialKbdInfo;
   kbdInfo.fsMask &= ~0x0001;		/* not echo on		*/
   kbdInfo.fsMask |= 0x0002;		/* echo off		*/
   kbdInfo.fsMask &= ~0x0008;		/* cooked mode off	*/
   kbdInfo.fsMask |= 0x0004;		/* raw mode		*/
   kbdInfo.fsMask &= ~0x0100;		/* shift report	off	*/
   KbdSetStatus(&kbdInfo, 0);
}

int init_tty ()
{
   VIOCURSORINFO cursorInfo, OldcursorInfo;

   if (Batch) return 0;
#ifdef HAS_MOUSE
   if (X_Open_Mouse_Hook != NULL) (*X_Open_Mouse_Hook) ();
#endif

  /*  set ^C off */
   signal (SIGINT, SIG_IGN);
   signal (SIGBREAK, SIG_IGN);

   initialKbdInfo.cb = sizeof(initialKbdInfo);
   KbdGetStatus(&initialKbdInfo, 0);
   set_kbd();
   enhancedKeyboard = 1;

   CreateSem (&Hmtx_Kbd);

  /* start a separate to read the keyboard */
#if defined(__BORLANDC__)
   Os2_Kbdhandler_ThreadID = _beginthread (os2_kbdthread, THREADSTACKSIZE, NULL);
#else
   Os2_Kbdhandler_ThreadID = _beginthread (os2_kbdthread, NULL,  THREADSTACKSIZE, NULL);
#endif

   if ((int)Os2_Kbdhandler_ThreadID == -1)
     {
	exit_error ("init_tty: Error starting keyboard thread.", 0);
     }

   VioGetCurType (&OldcursorInfo, 0);
   cursorInfo.yStart = 1;
   cursorInfo.cEnd = 15;
   cursorInfo.cx = 1;
   cursorInfo.attr = 1;
   if (VioSetCurType (&cursorInfo, 0))
     VioSetCurType (&OldcursorInfo, 0);   /* reset to previous value */

   return 0;
}

void reset_tty ()
{
   if (Batch) return;
   atEnd = 1;                      /* set flag and wait until thread ends */
#if __os2_16__
   while (atEnd)
     {
	DosSleep (0);
     }
#else
   DosWaitThread (&Os2_Kbdhandler_ThreadID, DCWW_WAIT);
#endif

  /* close the keyboard */
   KbdSetStatus(&initialKbdInfo, 0); /* restore original state	*/
#ifdef HAS_MOUSE
   if (X_Close_Mouse_Hook != NULL) (*X_Close_Mouse_Hook) ();
#endif
   CloseSem (Hmtx_Kbd);
}

/* sleep for *tsecs tenths of a sec waiting for input */
int sys_input_pending(int *tsecs, int proc)
{
   int count = *tsecs * 5;

#if JED_HAS_SUBPROCESSES
   if ((proc >= 0) && (Batch || Input_Buffer_Len)) return(Input_Buffer_Len);
#else
   if (Batch || Input_Buffer_Len) return(Input_Buffer_Len);
#endif

   if (count)
     {
	while(count > 0)
	  {
	     delay(20);		       /* 20 ms or 1/50 sec */
#if JED_HAS_SUBPROCESSES
	     if ((proc >= 0) && keyWaiting ()) break;
	     if (proc)
	       {
		  int i = 0;
		  while (i < Num_Subprocesses)
		    {
		       if (Subprocess_Read_fds[i][0])
			 read_process_input (Subprocess_Read_fds[i][1]);
		       i++;
		    }
	       }
#else
	     if (keyWaiting ()) break;
#endif
	     count--;
	  }
	return(count);
     }

#if JED_HAS_SUBPROCESSES
   if (proc)
     {
	int i = 0;
	while (i < Num_Subprocesses)
	  {
	     if (Subprocess_Read_fds[i][0])
	       read_process_input (Subprocess_Read_fds[i][1]);
	     i++;
	  }
     }
   if (proc >= 0) return keyWaiting ();
#else
   return keyWaiting ();
#endif
}

unsigned char sys_getkey ()
{
   int weird = 300;
#if JED_HAS_SUBPROCESSES
   int all = Num_Subprocesses;
#else
   int all = 0;
#endif

   while (!sys_input_pending(&weird, all))
     {
	if (Display_Time)
	  {
	     JWindow->trashed = 1;
	     update((Line *) NULL, 0, 1, 0);
	  }
     }

/*   if (JMouse_Hide_Mouse_Hook != NULL) (*JMouse_Hide_Mouse_Hook) (0); */
   return my_getkey ();
}

void sys_flush_input()
{
   KbdFlushBuffer(0);
}

#if 0
int get_term_dimensions(int *w, int *h)
{
   VIOMODEINFO vioModeInfo;

   vioModeInfo.cb = sizeof(vioModeInfo);
   VioGetMode (&vioModeInfo, 0);
   *w = vioModeInfo.col;
   *h = vioModeInfo.row;
   return 0;
}
#endif

#if defined(_MSC_VER) || defined (__EMX__) || defined(__WATCOMC__)
int sys_chmod(SLFUTURE_CONST char *file, int what, mode_t *mode, uid_t *uid, gid_t *gid)
{
   struct stat buf;
   int m;

# ifdef _MSC_VER
   /* MSC stat() is broken on directory names ending with a slash */
   char path[_MAX_PATH];
   safe_strcpy(path, file, sizeof (path));
   deslash(file = path);
# endif

   if (what)
     {
	chmod(file, *mode);
	return(0);
     }

   if (stat(file, &buf) < 0) switch (errno)
     {
      case EACCES: return(-1); /* es = "Access denied."; break; */
      case ENOENT: return(0);  /* ms = "File does not exist."; */
      case ENOTDIR: return(-2); /* es = "Invalid Path."; */
      default: return(-3); /* "stat: unknown error."; break;*/
     }

   m = buf.st_mode;

   *mode = m & 0777;

   if (m & S_IFDIR) return (2);
   return(1);
}

#else

int sys_chmod(char *file, int what, mode_t *mode, uid_t *dum1, gid_t *dum2)
{
   FILESTATUS3 fileInfo;
   USHORT Result;
   char path[_MAX_PATH];
   (void) dum1; (void) dum2;

 /* DosQueryPathInfo won't work with directory names ending with a slash */
   safe_strcpy(path, file, sizeof(path));
   deslash(file = path);
   Result = DosQueryPathInfo (path, FIL_STANDARD, &fileInfo, sizeof (fileInfo));
   if (Result == ERROR_FILE_NOT_FOUND) return 0;
   else if (Result == ERROR_ACCESS_DENIED) return -1;
   else if (Result == ERROR_INVALID_PATH) return -2;
   else if (Result != NO_ERROR) return -3;

   *mode = fileInfo.attrFile;

   if (what)
     {
	fileInfo.attrFile = *mode;
	DosSetPathInfo (file, FIL_STANDARD, &fileInfo, sizeof (fileInfo), 0);
     }

   if (*mode & 0x10) return(2);
   else return (1);
}

#endif

static char Found_Dir[JED_MAX_PATH_LEN];
static char Found_File[JED_MAX_PATH_LEN];
static int Found_FLen;

#ifdef __os2_16__
# define DosFindFirst(FileName, DirHandle, Attribute, ResultBuf, \
		     ResultBufLen, SearchCount, Reserved) \
	DosFindFirst(FileName, DirHandle, Attribute, ResultBuf, \
		     ResultBufLen, SearchCount, 0)
#endif

int sys_findfirst(char *theFile)
{
   char *f, the_path[CCHMAXPATH], *file, *f1;
   char *pat;

#if defined(__os2_16__)
   FILEFINDBUF FindBuffer;
   USHORT FindCount;
   USHORT File_Attr;
#else
   FILEFINDBUF3 FindBuffer;
   ULONG FindCount;
   ULONG File_Attr;
#endif

   HDIR FindHandle;
   File_Attr = FILE_READONLY | FILE_DIRECTORY;

   file = jed_standardize_filename_static(theFile);
   f1 = f = extract_file(file);
   strcpy (Found_Dir, file);

   strcpy (Found_File, file);
   Found_FLen = strlen(Found_File);

   Found_Dir[(int) (f - file)] = 0;

   strcpy(the_path, file);

   while (*f1 && (*f1 != '*')) f1++;
   if (! *f1)
     {
	while (*f && (*f != '.')) f++;
	if (*f) strcat(the_path, "*"); else strcat(the_path, "*.*");
     }
   pat = the_path;

   FindHandle = 1;
   FindCount = 1;
   if (DosFindFirst(pat, &FindHandle, File_Attr, &FindBuffer,
		    sizeof (FindBuffer), &FindCount, FIL_STANDARD) != NO_ERROR)
     return 0;
   strcpy (theFile, Found_Dir);
   strcat (theFile, FindBuffer.achName);

   /* OS/2 HPFS has the same problem as Windows: the filesystem is
    * case-insensitive so if one searches for makefi*, it will return
    * Makefile.in between the others, so check for wrong values and reject
    * them
    */
   if (Jed_Filename_Case_Sensitive
       && (0 != strncmp (file, Found_File, Found_FLen))
       && !sys_findnext (file))
     return 0;

   if (FindBuffer.attrFile & FILE_DIRECTORY) fixup_dir (theFile);
   return 1;
}

int sys_findnext(char *file)
{
#ifdef __os2_16__
   FILEFINDBUF FindBuffer;
   USHORT FileCount;
#else
   FILEFINDBUF3 FindBuffer;
   ULONG FileCount;
#endif

   while (1)
     {
	FileCount = 1;

	if (DosFindNext(1, &FindBuffer, sizeof (FindBuffer), &FileCount))
	  return 0;
	else
	  {
	     strcpy (file, Found_Dir);
	     strcat (file, FindBuffer.achName);

	    /* OS/2 HPFS has the same problem as Windows: the filesystem is
	     * case-insensitive so if one searches for makefi*, it will return
	     * Makefile.in between the others, so check for wrong values and
	     * reject them */
	     if (Jed_Filename_Case_Sensitive
		 && (0 != strncmp (file, Found_File, Found_FLen)))
	       continue;

	     if (FindBuffer.attrFile & FILE_DIRECTORY) fixup_dir (file);
	     return 1;
	  }
     }
}

#if 0
unsigned long sys_file_mod_time(char *file)
{
   FILESTATUS3 fileInfo;
   struct time t;
   struct date d;

   if (DosQueryPathInfo (file, FIL_STANDARD, &fileInfo, sizeof (fileInfo))
       != NO_ERROR)
     return 0;
   t.ti_min = fileInfo.ftimeLastWrite.minutes;
   t.ti_hour = fileInfo.ftimeLastWrite.hours;
   t.ti_hund = 0;
   t.ti_sec = fileInfo.ftimeLastWrite.twosecs;
   d.da_day = fileInfo.fdateLastWrite.day;
   d.da_mon = fileInfo.fdateLastWrite.month;
   d.da_year = fileInfo.fdateLastWrite.year;
   return dostounix(&d, &t);
}
#endif

void delay (int time)
{
   DosSleep (time);
}

int IsHPFSFileSystem (char *directory)
{
   ULONG		lMap;
   BYTE		bData[128];
   BYTE		bName[3];
   int			i;
   char		*FName;
#if defined (__os2_16__)
   USHORT		cbData;
   USHORT		nDrive;
# define DosQueryCurrentDisk DosQCurDisk
#else
   ULONG		cbData;
   ULONG		nDrive;
   PFSQBUFFER2		pFSQ = (PFSQBUFFER2)bData;
#endif

#ifndef __WATCOMC__
   if ( _osmode == DOS_MODE )
     return FALSE;
#endif
   if (isalpha (directory[0]) && (directory[1] == ':'))
     nDrive = toupper (directory[0]) - '@';
   else
     DosQueryCurrentDisk (&nDrive, &lMap);

/* Set up the drive name */

   bName[0] = (char) (nDrive + '@');
   bName[1] = ':';
   bName[2] = 0;

   cbData = sizeof (bData);

#if defined(__os2_16__)
   if (DosQFSAttach (bName, 0, FSAIL_QUERYNAME, bData, &cbData, 0L))
#else
     if (DosQueryFSAttach (bName, 0, FSAIL_QUERYNAME, pFSQ, &cbData))
#endif
       return FALSE;

#if defined(__os2_16__)
   FName = bData + (*((USHORT *) (bData + 2)) + 7);
#else
   FName = pFSQ->szName + pFSQ->cbName + 1;
#endif

   if (strcmp(FName, "HPFS"))
     return FALSE;
   return(TRUE);
}

/* Code for EAs */

#if defined(__os2_16__)
/* Variables and data structures used for handling EA's, these are
   defined in the 32 BIT API, so these will be included in os2.h */

typedef USHORT APIRET;
typedef struct _FEA2         /* fea2 */
{
# if 0			/* This field is in the 32 bit structure */
   ULONG oNextEntryOffset;		/* New field */
# endif
   BYTE fEA;
   BYTE cbName;
   USHORT cbValue;
   CHAR szName[1];				/* New field */
}
FEA2;
typedef FEA2 *PFEA2;

typedef struct _FEA2LIST		/* fea2l */
{
   ULONG cbList;
   FEA2 list[ 1 ];
}
FEA2LIST;

typedef FEA2LIST *PFEA2LIST;

typedef struct _GEA2			/* gea2 */
{
# if 0
		/* New field - in the 32 bit structure */
   ULONG oNextEntryOffset;
# endif /* OS2_32 */
   BYTE cbName;
   CHAR szName[ 1 ];		/* New field */
}
GEA2;

typedef GEA2 *PGEA2;

typedef struct _GEA2LIST		/* gea2l */
{
   ULONG cbList;
   GEA2 list[ 1 ];
}
GEA2LIST;

typedef GEA2LIST *PGEA2LIST;

typedef struct _EAOP2			/* eaop2 */
{
   PGEA2LIST fpGEA2List;	/* GEA set */
   PFEA2LIST fpFEA2List;	/* FEA set */
   ULONG oError;			/* Offset of FEA error */
}
EAOP2;

/* typedef EAOP2 *PEAOP2; */

# define DosSetPathInfo(PathName, PathInfoLevel, PathInfoBuf, \
		       PathInfoBufSize, PathInfoFlags) \
        DosSetPathInfo(PathName, PathInfoLevel, PathInfoBuf, \
		       PathInfoBufSize, PathInfoFlags, 0)
# define DosQueryPathInfo(PathName, PathInfoLevel, PathInfoBuf, \
			 PathInfoBufSize) \
        DosQPathInfo(PathName, PathInfoLevel, PathInfoBuf, PathInfoBufSize, 0)
# define DosEnumAttribute(RefType, FileRef, EntryNum, EnumBuf, \
			 EnumBufSize, EnumCnt, InfoLevel) \
        DosEnumAttribute(RefType, FileRef, EntryNum, EnumBuf, \
			 EnumBufSize, EnumCnt, InfoLevel, 0)
#endif

/* The HoldFEA is used to hold individual EAs.  The member names correspond
   directly to those of the FEA structure.  Note however, that both szName
   and aValue are pointers to the values.  An additional field, next, is
   used to link the HoldFEA's together to form a linked list. */

struct _HoldFEA
{
   BYTE fEA;					/* Flag byte */
   BYTE cbName;
   USHORT cbValue;
   CHAR *szName;
   BYTE *aValue;
   struct _HoldFEA *next;
};

typedef struct _HoldFEA HOLDFEA;

#define MAX_GEA				500L /* Max size for a GEA List */
#define REF_ASCIIZ			1	/* Reference type for DosEnumAttribute */

#define GET_INFO_LEVEL1		1	/* Get info from SFT */
#define GET_INFO_LEVEL2		2	/* Get size of FEAlist */
#define GET_INFO_LEVEL3		3	/* Get FEAlist given the GEAlist */
#define GET_INFO_LEVEL4		4	/* Get whole FEAlist */
#define GET_INFO_LEVEL5		5	/* Get FSDname */

#define SET_INFO_LEVEL1		1	/* Set info in SFT */
#define SET_INFO_LEVEL2		2	/* Set FEAlist */

/* #define BufferSize 1024
static char FileBuffer [BufferSize]; */
static HOLDFEA *pHoldFEA;			/* EA linked-list pointer */

/* Free_FEAList (pFEA)
   Frees the memory used by the linked list of HOLDFEA's pointed to by pFEA */

void Free_FEAList( HOLDFEA *pFEA )
{
   HOLDFEA *next;	/* Holds the next field since we free the structure
			 before reading the current next field */

	/* Step through the list freeing all the fields */
   while( pFEA )
     {
		/* Free the fields of the struct */
	next = pFEA->next;
	if( pFEA->szName != NULL )
			/* Free if non-NULL name */
	  free(pFEA->szName);
	if( pFEA->aValue != NULL )
			/* Free if non-NULL value */
	  free(pFEA->aValue);

		/* Free the pFEA struct itself and move on to the next field */
	free(pFEA);
	pFEA = next;
     }
}

#if 0
/* Read the EA type, this is stored at the start of the EA value */

ULONG getEAType( const CHAR *Value )
{
   USHORT Type = *( USHORT * ) Value;

   return( Type );
}

/* Return the EA length, stored in pFEA, this done so that it is calculated
   in only one place */

ULONG getEALength( const HOLDFEA *pFEA )
{
   return( sizeof( FEA2 )
	  - sizeof( CHAR )	/* Don't include the first element of aValue */
	  + pFEA->cbName + 1	/* Length of ASCIIZ name */
	  + pFEA->cbValue );	/* The value length */
}

/* Set the first two words of the EA value, this is usually the
   EA type and EA size in pFEA, from the values Type and Size */

void setEATypeSize( const HOLDFEA *pFEA, const USHORT Type, const USHORT Size )
{
   USHORT *valPtr = ( USHORT * ) pFEA->aValue;
   valPtr[ 0 ] = Type;
   valPtr[ 1 ] = Size;
}

/* Read the first two words of the EA value, this is usually the
   EA type and EA size in pFEA, into the Type and Size */

void getEATypeSize( const HOLDFEA *pFEA, USHORT *Type, USHORT *Size )
{
   USHORT *valPtr = ( USHORT * ) pFEA->aValue;
   *Type = valPtr[ 0 ];
   *Size = valPtr[ 1 ];
}

/* Get the address of the EA value in pFEA, ie skip over the Type and Size */

void* getEADataVal (const HOLDFEA *pFEA)
{
	/* Skip over the type and size */
   return pFEA->aValue + 2 * sizeof ( USHORT );
}
#endif

/* QueryEAs (szFileName)
      find all EAs that file szFileName has
      return these in the linked list of HOLDFEAs
      if no EAs exist or the linked list cannot be created then
      return NULL
  This function is modelled after code from IBM's Toolkit */

HOLDFEA *QueryEAs( const CHAR *szFileName )
{
   HOLDFEA *pHoldFEA;		/* The start of the linked list */

   CHAR *pAllocc = NULL;	/* Temp buffer used with DosEnumAttribute */
   CHAR *pBigAlloc = NULL;	/* Temp buffer to hold each EA as it is read in */
   USHORT cbBigAlloc = 0;

   ULONG ulEntryNum = 1;	/* Count of current EA to read (1-relative) */
   ULONG ulEnumCnt;		/* Number of EAs for Enum to return, always 1 */

   HOLDFEA *pLastIn = 0;	/* Points to last EA added, so new EA can link    */
   HOLDFEA *pNewFEA = NULL; /* Struct to build the new EA in                  */

   FEA2 *pFEA;				/* Used to read from Enum's return buffer */
   GEA2LIST *pGEAList;		/* Ptr used to set up buffer for DosQueryPathInfo() */
   EAOP2 eaopGet;			/* Used to call DosQueryPathInfo() */

   pAllocc = malloc( MAX_GEA );	/* Allocate room for a GEA list */
   pFEA = ( FEA2 * ) pAllocc;
   pHoldFEA = NULL;		/* Initialize the linked list */

	/* Loop through all the EA's adding them to the list */
   while( TRUE )
     {
	ulEnumCnt = 1;		/* No of EAs to retrieve */
	if( DosEnumAttribute( REF_ASCIIZ,	/* Read into EA name into */
			     szFileName,	/* pAlloc Buffer */
			     ulEntryNum, pAllocc, MAX_GEA, &ulEnumCnt,
			     ( LONG ) GET_INFO_LEVEL1 ) )
	  break;	/* An error */

		/* Exit if all the EA's have been read */
	if( ulEnumCnt != 1 )
	  break;

		/* Move on to next EA */
	ulEntryNum++;

		/* Try and allocate the HoldFEA structure */
	if( ( pNewFEA = malloc( sizeof( HOLDFEA ) ) ) == NULL )
	  {
	     free( pAllocc );
	     Free_FEAList( pHoldFEA );
	     return( NULL );
	  }

		/* Fill in the HoldFEA structure */
	pNewFEA->cbName = pFEA->cbName;
	pNewFEA->cbValue= pFEA->cbValue;
	pNewFEA->fEA = pFEA->fEA;
	pNewFEA->next = '\0';
/*		pNewFEA->next = NULL; */

		/* Allocate the two arrays */
	if( ( pNewFEA->szName = malloc( pFEA->cbName + 1 ) ) == NULL || \
			( pNewFEA->aValue = malloc( pFEA->cbValue ) ) == NULL )
	  {
			/* Out of memory, clean up and exit */
	     if( pNewFEA->szName )
	       free( pNewFEA->szName );
	     if( pNewFEA->aValue )
	       free( pNewFEA->aValue );

	     free( pAllocc );
	     free( pNewFEA );

	     Free_FEAList( pHoldFEA );
	     return( NULL );
	  }

		/* Copy EA name across */
	strcpy( pNewFEA->szName, pFEA->szName );
	cbBigAlloc = sizeof( FEA2LIST ) + pNewFEA->cbName + 1 + pNewFEA->cbValue;
							/* +1 is for '\0' */
	if( ( pBigAlloc = malloc( cbBigAlloc ) ) == NULL )
	  {
	     free( pNewFEA->szName );
	     free( pNewFEA->aValue );
	     free( pAllocc );
	     free( pNewFEA );
	     Free_FEAList( pHoldFEA );
	     return( NULL );
	  }

		/* Set up GEAlist structure */
	pGEAList = ( GEA2LIST * ) pAllocc;
	pGEAList->cbList = sizeof( GEA2LIST ) + pNewFEA->cbName;	/* + 1 for NULL */
#ifndef __os2_16__
	pGEAList->list[ 0 ].oNextEntryOffset = 0L;
#endif
	pGEAList->list[ 0 ].cbName = pNewFEA->cbName;
	strcpy( pGEAList->list[ 0 ].szName, pNewFEA->szName );

	eaopGet.fpGEA2List = ( GEA2LIST FAR * ) pAllocc;
	eaopGet.fpFEA2List = ( FEA2LIST FAR * ) pBigAlloc;

	eaopGet.fpFEA2List->cbList = cbBigAlloc;

		/* Get the complete EA info and copy the EA value */
	DosQueryPathInfo( szFileName, FIL_QUERYEASFROMLIST, ( PVOID ) &eaopGet, \
						  sizeof( EAOP2 ) );
	memcpy( pNewFEA->aValue, \
				pBigAlloc + sizeof( FEA2LIST ) + pNewFEA->cbName, \
				pNewFEA->cbValue );

		/* Release the temp. Enum buffer */
	free( pBigAlloc );

		/* Add to the list */
	if( pHoldFEA == NULL )
	  pHoldFEA = pNewFEA;
	else
	  pLastIn->next = pNewFEA;
	pLastIn = pNewFEA;
     }

	/* Free up the GEA buffer for DosEnum() */
   free( pAllocc );

   return pHoldFEA;
}

/* WriteEAs(szFileName, pHoldFEA)

   Write the EAs contained in the linked list pointed to by pHoldFEA
   to the file szFileName.

   Returns TRUE if the write was successful, FALSE otherwise */

int WriteEAs( const char *szFileName, HOLDFEA *pHoldFEA )
{
   HOLDFEA *pHFEA = pHoldFEA;
   EAOP2 eaopWrite;
   CHAR *aPtr = NULL;
   USHORT usMemNeeded;
   APIRET rc;

   eaopWrite.fpGEA2List = NULL;
   while( pHFEA )                                  /* Go through each HoldFEA */
     {
	usMemNeeded = sizeof( FEA2LIST ) + pHFEA->cbName + 1 + pHFEA->cbValue;

	if( ( aPtr = malloc( usMemNeeded ) ) == NULL )
	  return FALSE;

      /* Fill in eaop structure */
	eaopWrite.fpFEA2List = ( FEA2LIST FAR * ) aPtr;
	eaopWrite.fpFEA2List->cbList = usMemNeeded;
#ifdef __os2_16__  /* ugly hack for difference in 16-bit FEA struct */
	eaopWrite.fpFEA2List->cbList -= ( sizeof(FEA2LIST) - sizeof(FEALIST) );
#else
	eaopWrite.fpFEA2List->list[ 0 ].oNextEntryOffset = 0L;
#endif
	eaopWrite.fpFEA2List->list[ 0 ].fEA = pHFEA->fEA;
	eaopWrite.fpFEA2List->list[ 0 ].cbName = pHFEA->cbName;
	eaopWrite.fpFEA2List->list[ 0 ].cbValue = pHFEA->cbValue;
	strcpy( eaopWrite.fpFEA2List->list[ 0 ].szName, pHFEA->szName );
	memcpy( eaopWrite.fpFEA2List->list[ 0 ].szName + pHFEA->cbName + 1, \
	     pHFEA->aValue, pHFEA->cbValue );

      /* Write out the EA */
	rc = DosSetPathInfo( szFileName, FIL_QUERYEASIZE,
			    ( PVOID ) &eaopWrite, sizeof( EAOP2 ),
			    DSPI_WRTTHRU );

      /* Free up the FEALIST struct */
	free( aPtr );

      /* If the write failed, leave now */
	if( rc )
	  return FALSE;

	pHFEA = pHFEA->next;
     }

   return( TRUE );
}

void sys_pause (int ms)
{
   DosSleep(ms);
}

#ifdef	__IBMC__
/* Adapted from the routines posted by Jochen Friedrich */

/* Note: the pipe() function assumes that the C filedescriptors are
 * equal to the OS/2 API filehandles... This is true for IBM VisualAge .0,
 * but may be false for other compilers...
 */
int pipe(unsigned long fd[2])
{
   unsigned long	status;

   if (DosCreatePipe(&(fd[0]), &(fd[1]), 4096) < 0)
     return (-1);
   if (DosQueryFHState(fd[0], &status) < 0)
     return (-1);
   status |= OPEN_FLAGS_NOINHERIT;
   status &= 0x7F88;
   if (DosSetFHState(fd[0], status) < 0)
     return (-1);
   if (DosQueryFHState(fd[1], &status) < 0)
     return (-1);
   status |= OPEN_FLAGS_NOINHERIT;
   status &= 0x7F88;
   if (DosSetFHState(fd[1], status) < 0)
     return (-1);
   _setmode(fd[0], O_BINARY);
   _setmode(fd[1], O_BINARY);
   return (0);
}

FILE *popen(char *cmd, char *mode)
{
   FILE		*our_end;
   unsigned long	fd[2], tempfd, dupfd;
   char		*comspec;

   pipe(fd);

   if (mode[0] == 'r')
     {
	our_end = fdopen(fd[0], mode);
	dupfd = 1;	/* stdout */
	tempfd = dup(dupfd);
	dup2(fd[1], dupfd);
	close(fd[1]);
     }
   else
     {
	our_end = fdopen(fd[1], mode);
	dupfd = 0;	/* stdin */
	tempfd = dup(dupfd);
	dup2(fd[0], dupfd);
	close(fd[0]);
     }

   comspec = getenv("COMSPEC");
   if (comspec == NULL)
     {
	comspec = "C:\\OS2\\CMD.EXE";
     }
   spawnlp(P_NOWAIT, comspec, comspec, "/c", cmd, NULL);

   dup2(tempfd, dupfd);
   close(tempfd);

   return (our_end);
}

int pclose(FILE *pip)
{
   int i;

   fclose(pip);
   wait(&i);
   return (0);
}
#endif
