/* Copyright (c) 1992-2023 John E. Davis
 * This file is part of JED editor library source.
 *
 * You may distribute this file under the terms the GNU General Public
 * License.  See the file COPYING for more information.
 */
typedef struct Mail_Type
{
   unsigned short buflen;
   unsigned short code;
   long addr;
   long ret, junk;
} Mail_Type;

/* Created by SDL V3.1-7     */
/* Source:  8-AUG-1988 10:36:21  */
#ifndef $MAILDEF_H
#define $MAILDEF_H

/*** MODULE $MAILDEF ***/
/*                                                                            */
/* Definitions needed for callable mail.                                      */
/*                                                                            */
/*                                                                            */
/* NOTE: New item codes must be appended to the end of                        */
/*       each category so users will not have to relink.                      */
/*       We allow 1024 entries in eatch category.                             */
/*                                                                            */
/*                                                                            */
/* Send input codes                                                           */
/*                                                                            */
#define MAIL$_SEND_SPARE_0 1
#define MAIL$_SEND_FOREIGN 2    /* Send foreign format message                */
#define MAIL$_SEND_CC_LINE 3    /* CC text specification                      */
#define MAIL$_SEND_DEFAULT_NAME 4       /* Default file name for send         */
#define MAIL$_SEND_DEFAULT_TRANSPORT 5  /* Default transport used for addressees      */
#define MAIL$_SEND_ERROR_ENTRY 6        /* Entry point for send error routine */
#define MAIL$_SEND_FILENAME 7   /* Specification of file name to send         */
#define MAIL$_SEND_FROM_LINE 8  /* From text specification                    */
#define MAIL$_SEND_NO_DEFAULT_TRANSPORT 9       /* Don't use any default transport on send    */
#define MAIL$_SEND_PERS_NAME 10 /* Personal name text for message             */
#define MAIL$_SEND_RECORD 11    /* Record to be sent                          */
#define MAIL$_SEND_RESULTSPEC 12        /* Resultant filespec bodypart        */
#define MAIL$_SEND_SERVER 13    /* Operate in server mode (signal success)    */
#define MAIL$_SEND_SUBJECT 14   /* Subject text specification                 */
#define MAIL$_SEND_SUCCESS_ENTRY 15     /* Entry point for send success routine       */
#define MAIL$_SEND_TO_LINE 16   /* To line text specification                 */
#define MAIL$_SEND_UFLAGS 17    /* User flags (for header) to send            */
#define MAIL$_SEND_USER_DATA 18 /* User specified context for action routines */
#define MAIL$_SEND_USERNAME 19  /* Username to add to the "To" list           */
#define MAIL$_SEND_USERNAME_TYPE 20     /* Username type - TO or CC           */
#define MAIL$_SEND_FID 21       /* FID of file to send                        */
#define MAIL$_SEND_NO_PERS_NAME 22      /* Send message without personal name */
#define MAIL$_SEND_IN_SPARE3 23
#define MAIL$_SEND_IN_SPARE4 24
#define MAIL$_SEND_IN_SPARE5 25
/*                                                                            */
/* Send output codes                                                          */
/*                                                                            */
#define MAIL$_SEND_COPY_REPLY 26        /* Copy self reply set in profile     */
#define MAIL$_SEND_COPY_SEND 27 /* Copy self send  set in profile             */
#define MAIL$_SEND_USER 28      /* Username of caller                         */
#define MAIL$_SEND_COPY_FORWARD 29      /* Copy self forward set in profile   */
#define MAIL$_SEND_OUT_SPARE2 30
#define MAIL$_SEND_OUT_SPARE3 31
#define MAIL$_SEND_OUT_SPARE4 32
#define MAIL$_SEND_OUT_SPARE5 33
#define MAIL$K_SEND_MIN_ITEM 1
#define MAIL$K_SEND_MAX_ITEM 33
#define MAIL$K_SEND_ITEMS 33
/*                                                                            */
/* file input codes                                                           */
/*                                                                            */
#define MAIL$_MAILFILE_SPARE_0 1025
#define MAIL$_MAILFILE_DEFAULT_NAME 1026        /* Default filespec to open   */
#define MAIL$_MAILFILE_FOLDER_ROUTINE 1027      /* Entry point of routine to process foldernames      */
#define MAIL$_MAILFILE_FULL_CLOSE 1028  /* Do a PURGE, CONVERT/RECLAIM if necessary on close  */
#define MAIL$_MAILFILE_NAME 1029        /* File spec to open                  */
#define MAIL$_MAILFILE_RECLAIM 1030     /* Do RECLAIM on PURGE command        */
#define MAIL$_MAILFILE_USER_DATA 1031   /* User specified context for action routines */
#define MAIL$_MAILFILE_WASTEBASKET_NAME 1032    /* New wastebasket name for file      */
#define MAIL$_MAILFILE_IN_SPARE1 1033
#define MAIL$_MAILFILE_IN_SPARE2 1034
#define MAIL$_MAILFILE_IN_SPARE3 1035
#define MAIL$_MAILFILE_IN_SPARE4 1036
#define MAIL$_MAILFILE_IN_SPARE5 1037
#define MAIL$_MAILFILE_IN_SPARE6 1038
#define MAIL$_MAILFILE_IN_SPARE7 1039
#define MAIL$_MAILFILE_IN_SPARE8 1040
#define MAIL$_MAILFILE_IN_SPARE9 1041
#define MAIL$_MAILFILE_IN_SPARE10 1042
#define MAIL$_MAILFILE_IN_SPARE11 1043
#define MAIL$_MAILFILE_IN_SPARE12 1044
#define MAIL$_MAILFILE_IN_SPARE13 1045
#define MAIL$_MAILFILE_IN_SPARE14 1046
#define MAIL$_MAILFILE_IN_SPARE15 1047
#define MAIL$_MAILFILE_IN_SPARE16 1048
#define MAIL$_MAILFILE_IN_SPARE17 1049
#define MAIL$_MAILFILE_IN_SPARE18 1050
#define MAIL$_MAILFILE_IN_SPARE19 1051
#define MAIL$_MAILFILE_IN_SPARE20 1052
/*                                                                            */
/* file output codes                                                          */
/*                                                                            */
#define MAIL$_MAILFILE_DATA_RECLAIM 1053        /* Number of data buckets reclaimed   */
#define MAIL$_MAILFILE_DATA_SCAN 1054   /* Number of data buckets scanned     */
#define MAIL$_MAILFILE_DELETED_BYTES 1055       /* Number of free bytes in mail file  */
#define MAIL$_MAILFILE_INDEX_RECLAIM 1056       /* Number of index buckets reclaimed  */
#define MAIL$_MAILFILE_MAIL_DIRECTORY 1057      /* Mail sub-directory specification   */
#define MAIL$_MAILFILE_MESSAGES_DELETED 1058    /* Number of messages deleted */
#define MAIL$_MAILFILE_RESULTSPEC 1059  /* Resultant file spec                */
#define MAIL$_MAILFILE_TOTAL_RECLAIM 1060       /* Total buckets reclaimed    */
#define MAIL$_MAILFILE_WASTEBASKET 1061 /* Wastebasket name                   */
#define MAIL$_MAILFILE_INDEXED 1062     /* ISAM file                          */
#define MAIL$_MAILFILE_OUT_SPARE2 1063
#define MAIL$_MAILFILE_OUT_SPARE3 1064
#define MAIL$_MAILFILE_OUT_SPARE4 1065
#define MAIL$_MAILFILE_OUT_SPARE5 1066
#define MAIL$_MAILFILE_OUT_SPARE6 1067
#define MAIL$_MAILFILE_OUT_SPARE7 1068
#define MAIL$_MAILFILE_OUT_SPARE8 1069
#define MAIL$_MAILFILE_OUT_SPARE9 1070
#define MAIL$_MAILFILE_OUT_SPARE10 1071
#define MAIL$_MAILFILE_OUT_SPARE11 1072
#define MAIL$_MAILFILE_OUT_SPARE12 1073
#define MAIL$_MAILFILE_OUT_SPARE13 1074
#define MAIL$_MAILFILE_OUT_SPARE14 1075
#define MAIL$_MAILFILE_OUT_SPARE15 1076
#define MAIL$_MAILFILE_OUT_SPARE16 1077
#define MAIL$_MAILFILE_OUT_SPARE17 1078
#define MAIL$_MAILFILE_OUT_SPARE18 1079
#define MAIL$_MAILFILE_OUT_SPARE19 1080
#define MAIL$_MAILFILE_OUT_SPARE20 1081
#define MAIL$K_MAILFILE_MIN_ITEM 1025
#define MAIL$K_MAILFILE_MAX_ITEM 1081
#define MAIL$K_MAILFILE_ITEMS 57
/*                                                                            */
/* message input codes                                                        */
/*                                                                            */
#define MAIL$_MESSAGE_SPARE_0 2048
#define MAIL$_MESSAGE_BACK 2049 /* Get previous message                       */
#define MAIL$_MESSAGE_BEFORE 2050       /* Select messages BEFORE date        */
#define MAIL$_MESSAGE_CC_SUBSTRING 2051 /* Select messages containing CC substring    */
#define MAIL$_MESSAGE_CONTINUE 2052     /* Read next record                   */
#define MAIL$_MESSAGE_FILE_ACTION 2053  /* File create action routine         */
#define MAIL$_MESSAGE_FOLDER_ACTION 2054        /* Folder create action routine       */
#define MAIL$_MESSAGE_DEFAULT_NAME 2055 /* Default file name                  */
#define MAIL$_MESSAGE_DELETE 2056       /* Delete message                     */
#define MAIL$_MESSAGE_ERASE 2057        /* Erase message                      */
#define MAIL$_MESSAGE_FILE_CTX 2058     /* File level context                 */
#define MAIL$_MESSAGE_FILENAME 2059     /* File name specification            */
#define MAIL$_MESSAGE_FLAGS 2060        /* Header flags specification         */
#define MAIL$_MESSAGE_FOLDER 2061       /* Folder name specification          */
#define MAIL$_MESSAGE_FROM_SUBSTRING 2062       /* Select messages containing FROM substring  */
#define MAIL$_MESSAGE_ID 2063   /* ID of message                              */
#define MAIL$_MESSAGE_NEXT 2064 /* Retrive NEXT message                       */
#define MAIL$_MESSAGE_SINCE 2065        /* Select messages SINCE date         */
#define MAIL$_MESSAGE_SUBJ_SUBSTRING 2066       /* Select messages containing SUBJ substring  */
#define MAIL$_MESSAGE_TO_SUBSTRING 2067 /* Select messages containing TO substring    */
#define MAIL$_MESSAGE_UFLAGS 2068       /* User flags specification           */
#define MAIL$_MESSAGE_AUTO_NEWMAIL 2069 /* Move newmail to MAIL auto          */
#define MAIL$_MESSAGE_USER_DATA 2070    /* User context for action routines   */
#define MAIL$_MESSAGE_FLAGS_MBZ 2071    /* Select messages with these flags set at zero       */
#define MAIL$_MESSAGE_MIN_CLASS 2072    /* Min access class for message       */
#define MAIL$_MESSAGE_MAX_CLASS 2073    /* Max access class for message       */
#define MAIL$_MESSAGE_IN_SPARE1 2074
#define MAIL$_MESSAGE_IN_SPARE2 2075
#define MAIL$_MESSAGE_IN_SPARE3 2076
#define MAIL$_MESSAGE_IN_SPARE4 2077
#define MAIL$_MESSAGE_IN_SPARE5 2078
#define MAIL$_MESSAGE_IN_SPARE6 2079
#define MAIL$_MESSAGE_IN_SPARE7 2080
#define MAIL$_MESSAGE_IN_SPARE8 2081
#define MAIL$_MESSAGE_IN_SPARE9 2082
#define MAIL$_MESSAGE_IN_SPARE10 2083
#define MAIL$_MESSAGE_IN_SPARE11 2084
#define MAIL$_MESSAGE_IN_SPARE12 2085
#define MAIL$_MESSAGE_IN_SPARE13 2086
#define MAIL$_MESSAGE_IN_SPARE14 2087
#define MAIL$_MESSAGE_IN_SPARE15 2088
#define MAIL$_MESSAGE_IN_SPARE16 2089
#define MAIL$_MESSAGE_IN_SPARE17 2090
#define MAIL$_MESSAGE_IN_SPARE18 2091
#define MAIL$_MESSAGE_IN_SPARE19 2092
#define MAIL$_MESSAGE_IN_SPARE20 2093
/*                                                                            */
/* message output codes                                                       */
/*                                                                            */
#define MAIL$_MESSAGE_CC 2094   /* CC text of message                         */
#define MAIL$_MESSAGE_CURRENT_ID 2095   /* ID of current message              */
#define MAIL$_MESSAGE_DATE 2096 /* Date of current message                    */
#define MAIL$_MESSAGE_EXTID 2097        /* Filespec of external message       */
#define MAIL$_MESSAGE_FILE_CREATED 2098 /* Mailfile created...                */
#define MAIL$_MESSAGE_FOLDER_CREATED 2099       /* Folder created...          */
#define MAIL$_MESSAGE_FROM 2100 /* From text of message                       */
#define MAIL$_MESSAGE_RECORD 2101       /* Record from message                */
#define MAIL$_MESSAGE_RECORD_TYPE 2102  /* Type of record, header or text     */
#define MAIL$_MESSAGE_REPLY_PATH 2103   /* Reply path of sender               */
#define MAIL$_MESSAGE_RESULTSPEC 2104   /* Resultant file spec                */
#define MAIL$_MESSAGE_RETURN_FLAGS 2105 /* Message header system flags        */
#define MAIL$_MESSAGE_RETURN_UFLAGS 2106        /* Message header user flags  */
#define MAIL$_MESSAGE_SELECTED 2107     /* Number of messages selected        */
#define MAIL$_MESSAGE_SENDER 2108       /* Sender name                        */
#define MAIL$_MESSAGE_SIZE 2109 /* Size of the current message                */
#define MAIL$_MESSAGE_SUBJECT 2110      /* Subject text of the message        */
#define MAIL$_MESSAGE_TO 2111   /* To text of the message                     */
#define MAIL$_MESSAGE_BUFFER 2112       /* Buffer address                     */
#define MAIL$_MESSAGE_RETURN_CLASS 2113 /* Class of current message           */
#define MAIL$_MESSAGE_BINARY_DATE 2114  /* Binary date/time quadword          */
#define MAIL$_MESSAGE_SPARE4 2115
#define MAIL$_MESSAGE_SPARE5 2116
#define MAIL$_MESSAGE_SPARE6 2117
#define MAIL$_MESSAGE_SPARE7 2118
#define MAIL$_MESSAGE_SPARE8 2119
#define MAIL$_MESSAGE_SPARE9 2120
#define MAIL$_MESSAGE_SPARE10 2121
#define MAIL$_MESSAGE_SPARE11 2122
#define MAIL$_MESSAGE_SPARE12 2123
#define MAIL$_MESSAGE_SPARE13 2124
#define MAIL$_MESSAGE_SPARE14 2125
#define MAIL$_MESSAGE_SPARE15 2126
#define MAIL$_MESSAGE_SPARE16 2127
#define MAIL$_MESSAGE_SPARE17 2128
#define MAIL$_MESSAGE_SPARE18 2129
#define MAIL$_MESSAGE_SPARE19 2130
/*                                                                            */
/* Constants returned                                                         */
/*                                                                            */
#define MAIL$_MESSAGE_NULL 2131 /* Null bodypart                              */
#define MAIL$_MESSAGE_HEADER 2132       /* Header record returned             */
#define MAIL$_MESSAGE_TEXT 2133 /* Text record returned                       */
#define MAIL$_MESSAGE_SPARE20 2134
#define MAIL$K_MESSAGE_MIN_ITEM 2048
#define MAIL$K_MESSAGE_MAX_ITEM 2134
#define MAIL$K_MESSAGE_ITEMS 87
/*                                                                            */
/* user input codes                                                           */
/*                                                                            */
#define MAIL$_USER_SPARE_0 3072
#define MAIL$_USER_FIRST 3073   /* Retrive first user record                  */
#define MAIL$_USER_NEXT 3074    /* Retrive next user record                   */
#define MAIL$_USER_USERNAME 3075        /* Retrive record for username        */
#define MAIL$_USER_SET_AUTO_PURGE 3076  /* Set auto-purge                     */
#define MAIL$_USER_SET_NO_AUTO_PURGE 3077       /* Clear auto-purge           */
#define MAIL$_USER_SET_SUB_DIRECTORY 3078       /* Set sub-dir field          */
#define MAIL$_USER_SET_NO_SUB_DIRECTORY 3079    /* Clear sub-dir field        */
#define MAIL$_USER_SET_FORWARDING 3080  /* Set forwarding address             */
#define MAIL$_USER_SET_NO_FORWARDING 3081       /* Clear forwarding address   */
#define MAIL$_USER_SET_PERSONAL_NAME 3082       /* Set personal name          */
#define MAIL$_USER_SET_NO_PERSONAL_NAME 3083    /* Clear personal name        */
#define MAIL$_USER_SET_COPY_SEND 3084   /* Set copy-send                      */
#define MAIL$_USER_SET_NO_COPY_SEND 3085        /* Clear copy send            */
#define MAIL$_USER_SET_COPY_REPLY 3086  /* Set copy reply                     */
#define MAIL$_USER_SET_NO_COPY_REPLY 3087       /* Clear copy reply           */
#define MAIL$_USER_SET_NEW_MESSAGES 3088        /* Set new message count      */
#define MAIL$_USER_CREATE_IF 3089       /* Create record if does not exist    */
#define MAIL$_USER_SET_MAILPLUS 3090    /* Set M+                             */
#define MAIL$_USER_SET_NO_MAILPLUS 3091 /* Clear M+                           */
#define MAIL$_USER_SET_TRANSPORT 3092   /* Set transport field                */
#define MAIL$_USER_SET_NO_TRANSPORT 3093        /* Clear transport field      */
#define MAIL$_USER_SET_EDITOR 3094      /* Set editor field                   */
#define MAIL$_USER_SET_NO_EDITOR 3095   /* Clear editor field                 */
#define MAIL$_USER_SET_QUEUE 3096       /* Set queue field                    */
#define MAIL$_USER_SET_NO_QUEUE 3097    /* Clear queue field                  */
#define MAIL$_USER_SET_USER1 3098       /* Set user1 field                    */
#define MAIL$_USER_SET_NO_USER1 3099    /* Clear user1 field                  */
#define MAIL$_USER_SET_USER2 3100       /* Set user2 field                    */
#define MAIL$_USER_SET_NO_USER2 3101    /* Clear user2 field                  */
#define MAIL$_USER_SET_USER3 3102       /* Set user3 field                    */
#define MAIL$_USER_SET_NO_USER3 3103    /* Clear user3 field                  */
#define MAIL$_USER_SET_FORM 3104        /* Set form field                     */
#define MAIL$_USER_SET_NO_FORM 3105     /* Clear form field                   */
#define MAIL$_USER_SET_COPY_FORWARD 3106        /* Set copy self forward      */
#define MAIL$_USER_SET_NO_COPY_FORWARD 3107     /* Clear copy self forward    */
#define MAIL$_USER_SET_CC_PROMPT 3108   /* Set CC prompting                   */
#define MAIL$_USER_SET_NO_CC_PROMPT 3109        /* Clear CC prompting         */
#define MAIL$_USER_SET_SPARE3 3110
#define MAIL$_USER_SET_NO_SPARE3 3111
#define MAIL$_USER_IN_SPARE1 3112
#define MAIL$_USER_IN_SPARE2 3113
#define MAIL$_USER_IN_SPARE3 3114
#define MAIL$_USER_IN_SPARE4 3115
#define MAIL$_USER_IN_SPARE5 3116
#define MAIL$_USER_IN_SPARE6 3117
#define MAIL$_USER_IN_SPARE7 3118
#define MAIL$_USER_IN_SPARE8 3119
#define MAIL$_USER_IN_SPARE9 3120
#define MAIL$_USER_IN_SPARE10 3121
#define MAIL$_USER_IN_SPARE11 3122
#define MAIL$_USER_IN_SPARE12 3123
#define MAIL$_USER_IN_SPARE13 3124
#define MAIL$_USER_IN_SPARE14 3125
#define MAIL$_USER_IN_SPARE15 3126
#define MAIL$_USER_IN_SPARE16 3127
#define MAIL$_USER_IN_SPARE17 3128
#define MAIL$_USER_IN_SPARE18 3129
#define MAIL$_USER_IN_SPARE19 3130
#define MAIL$_USER_IN_SPARE20 3131
/*                                                                            */
/* Output item parameters                                                     */
/*                                                                            */
#define MAIL$_USER_MAILPLUS 3132        /* M+ field                           */
#define MAIL$_USER_TRANSPORT 3133       /* Transport field                    */
#define MAIL$_USER_EDITOR 3134  /* Editor field                               */
#define MAIL$_USER_QUEUE 3135   /* Queue field                                */
#define MAIL$_USER_USER1 3136   /* User1 field                                */
#define MAIL$_USER_USER2 3137   /* User2 field                                */
#define MAIL$_USER_USER3 3138   /* User3 field                                */
#define MAIL$_USER_FORM 3139    /* Form field                                 */
#define MAIL$_USER_COPY_FORWARD 3140    /* Copy forward flag                  */
#define MAIL$_USER_SPARE3 3141
#define MAIL$_USER_RETURN_USERNAME 3142 /* Username of current record         */
#define MAIL$_USER_AUTO_PURGE 3143      /* Auto purge flag                    */
#define MAIL$_USER_SUB_DIRECTORY 3144   /* Sub-dir spec                       */
#define MAIL$_USER_FULL_DIRECTORY 3145  /* Full directory spec                */
#define MAIL$_USER_NEW_MESSAGES 3146    /* New message count                  */
#define MAIL$_USER_FORWARDING 3147      /* Forwarding field                   */
#define MAIL$_USER_PERSONAL_NAME 3148   /* Personal name field                */
#define MAIL$_USER_COPY_SEND 3149       /* Copy send flag                     */
#define MAIL$_USER_COPY_REPLY 3150      /* Copy reply flag                    */
#define MAIL$_USER_CAPTIVE 3151 /* User is captive                            */
#define MAIL$_USER_CC_PROMPT 3152       /* CC prompting flag                  */
#define MAIL$_USER_OUT_SPARE2 3153
#define MAIL$_USER_OUT_SPARE3 3154
#define MAIL$_USER_OUT_SPARE4 3155
#define MAIL$_USER_OUT_SPARE5 3156
#define MAIL$_USER_OUT_SPARE6 3157
#define MAIL$_USER_OUT_SPARE7 3158
#define MAIL$_USER_OUT_SPARE8 3159
#define MAIL$_USER_OUT_SPARE9 3160
#define MAIL$_USER_OUT_SPARE10 3161
#define MAIL$_USER_OUT_SPARE11 3162
#define MAIL$_USER_OUT_SPARE12 3163
#define MAIL$_USER_OUT_SPARE13 3164
#define MAIL$_USER_OUT_SPARE14 3165
#define MAIL$_USER_OUT_SPARE15 3166
#define MAIL$_USER_OUT_SPARE16 3167
#define MAIL$_USER_OUT_SPARE17 3168
#define MAIL$_USER_OUT_SPARE18 3169
#define MAIL$_USER_OUT_SPARE19 3170
#define MAIL$_USER_OUT_SPARE20 3171
#define MAIL$K_USER_MIN_ITEM 3072
#define MAIL$K_USER_MAX_ITEM 3171
#define MAIL$K_USER_ITEMS 100
/*                                                                            */
/* Special items                                                              */
/*                                                                            */
#define MAIL$_NOOP 4097 /* Do nothing - used for testing                      */
#define MAIL$_NOSIGNAL 4098     /* Don't signal errors                        */
#define MAIL$_NOPROBE 4099      /* Don't probe the item list buffers          */
#define MAIL$_TLD_INPUT 4100    /* This item is a tld to be used as input     */
#define MAIL$_TLD_OUTPUT 4101   /* This item describes a buffer to fill       */
/*                                                                            */
/* Username types for mail$send_add_address                                   */
/*                                                                            */
#define MAIL$_TO 1      /* Address part of the TO line                        */
#define MAIL$_CC 2      /* Address part of the CC line                        */
#define MAIL$_SPARE1 3
#define MAIL$_SPARE2 4
/*                                                                            */
/* Define the flag values for mail system flags                               */
/*                                                                            */
#define MAIL$M_NEWMSG 0x00000001
#define MAIL$M_REPLIED 0x00000002
#define MAIL$M_DEL 0x00000004
#define MAIL$M_EXTMSG 0x00000008
#define MAIL$M_EXTFNF 0x00000010
#define MAIL$M_SYSMSG 0x00000020
#define MAIL$M_EXTNSTD 0x00000040
#define MAIL$M_MARKED 0x00000080
#define MAIL$M_RECMODE 0x00000100
struct flagsdef {
    union {
        unsigned short int mail$w_flags;        /*Flags word                  */
        struct {
            unsigned mail$v_newmsg : 1; /*This is a new message               */
            unsigned mail$v_replied : 1;        /*This message has been replied to    */
            unsigned mail$v_del : 1;    /*This message is deleted             */
            unsigned mail$v_extmsg : 1; /*Message text in external file       */
            unsigned mail$v_extfnf : 1; /*External message file not found     */
            unsigned mail$v_sysmsg : 1; /*Message text in system file         */
            unsigned mail$v_extnstd : 1;        /*External file is not var-seq file   */
            unsigned mail$v_marked : 1; /*This message has been marked        */
            unsigned mail$v_recmode : 1;        /*This message should be read in record mode  */
            unsigned mail$v_fill_2 : 7;
            } mail$r_fill_1;
        } mail$r_fill_0;
} ;
#endif

