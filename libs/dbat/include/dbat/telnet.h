/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)telnet.h	8.2 (Berkeley) 12/15/93
 */

#ifndef _ARPA_TELNET_H
#define _ARPA_TELNET_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Definitions for the TELNET protocol.
 */
constexpr int IAC   = 255; /* interpret as command: */
constexpr int DONT  = 254; /* you are not to use option */
constexpr int DO    = 253; /* please, you use option */
constexpr int WONT  = 252; /* I won't use option */
constexpr int WILL  = 251; /* I will use option */
constexpr int SB    = 250; /* interpret as subnegotiation */
constexpr int GA    = 249; /* you may reverse the line */
constexpr int EL    = 248; /* erase the current line */
constexpr int EC    = 247; /* erase the current character */
constexpr int AYT   = 246; /* are you there */
constexpr int AO    = 245; /* abort output--but let prog finish */
constexpr int IP    = 244; /* interrupt process--permanently */
constexpr int BREAK = 243; /* break */
constexpr int DM    = 242; /* data mark--for connect. cleaning */
constexpr int NOP   = 241; /* nop */
constexpr int SE    = 240; /* end sub negotiation */
constexpr int EOR   = 239; /* end of record (transparent mode) */
constexpr int ABORT = 238; /* Abort process */
constexpr int SUSP  = 237; /* Suspend process */
constexpr int xEOF  = 236; /* End of file: EOF is already used... */

constexpr int SYNCH = 242; /* for telfunc calls */

#ifdef TELCMDS
char *telcmds[] = {
    "EOF",
    "SUSP",
    "ABORT",
    "EOR",
    "SE",
    "NOP",
    "DMARK",
    "BRK",
    "IP",
    "AO",
    "AYT",
    "EC",
    "EL",
    "GA",
    "SB",
    "WILL",
    "WONT",
    "DO",
    "DONT",
    "IAC",
    0,
};
#else
extern char *telcmds[];
#endif

constexpr int TELCMD_FIRST = xEOF;
constexpr int TELCMD_LAST  = IAC;
#define TELCMD_OK(x) ((unsigned int)(x) <= TELCMD_LAST && \
                      (unsigned int)(x) >= TELCMD_FIRST)
#define TELCMD(x) telcmds[(x) - TELCMD_FIRST]

/* telnet options */
constexpr int TELOPT_BINARY         = 0;   /* 8-bit data path */
constexpr int TELOPT_ECHO           = 1;   /* echo */
constexpr int TELOPT_RCP            = 2;   /* prepare to reconnect */
constexpr int TELOPT_SGA            = 3;   /* suppress go ahead */
constexpr int TELOPT_NAMS           = 4;   /* approximate message size */
constexpr int TELOPT_STATUS         = 5;   /* give status */
constexpr int TELOPT_TM             = 6;   /* timing mark */
constexpr int TELOPT_RCTE           = 7;   /* remote controlled transmission and echo */
constexpr int TELOPT_NAOL           = 8;   /* negotiate about output line width */
constexpr int TELOPT_NAOP           = 9;   /* negotiate about output page size */
constexpr int TELOPT_NAOCRD         = 10;  /* negotiate about CR disposition */
constexpr int TELOPT_NAOHTS         = 11;  /* negotiate about horizontal tabstops */
constexpr int TELOPT_NAOHTD         = 12;  /* negotiate about horizontal tab disposition */
constexpr int TELOPT_NAOFFD         = 13;  /* negotiate about formfeed disposition */
constexpr int TELOPT_NAOVTS         = 14;  /* negotiate about vertical tab stops */
constexpr int TELOPT_NAOVTD         = 15;  /* negotiate about vertical tab disposition */
constexpr int TELOPT_NAOLFD         = 16;  /* negotiate about output LF disposition */
constexpr int TELOPT_XASCII         = 17;  /* extended ascic character set */
constexpr int TELOPT_LOGOUT         = 18;  /* force logout */
constexpr int TELOPT_BM             = 19;  /* int8_tmacro */
constexpr int TELOPT_DET            = 20;  /* data entry terminal */
constexpr int TELOPT_SUPDUP         = 21;  /* supdup protocol */
constexpr int TELOPT_SUPDUPOUTPUT   = 22;  /* supdup output */
constexpr int TELOPT_SNDLOC         = 23;  /* send location */
constexpr int TELOPT_TTYPE          = 24;  /* terminal type */
constexpr int TELOPT_EOR            = 25;  /* end or record */
constexpr int TELOPT_TUID           = 26;  /* TACACS user identification */
constexpr int TELOPT_OUTMRK         = 27;  /* output marking */
constexpr int TELOPT_TTYLOC         = 28;  /* terminal location number */
constexpr int TELOPT_3270REGIME     = 29;  /* 3270 regime */
constexpr int TELOPT_X3PAD          = 30;  /* X.3 PAD */
constexpr int TELOPT_NAWS           = 31;  /* window size */
constexpr int TELOPT_TSPEED         = 32;  /* terminal speed */
constexpr int TELOPT_LFLOW          = 33;  /* remote flow control */
constexpr int TELOPT_LINEMODE       = 34;  /* Linemode option */
constexpr int TELOPT_XDISPLOC       = 35;  /* X Display Location */
constexpr int TELOPT_OLD_ENVIRON    = 36;  /* Old - Environment variables */
constexpr int TELOPT_AUTHENTICATION = 37;  /* Authenticate */
constexpr int TELOPT_ENCRYPT        = 38;  /* Encryption option */
constexpr int TELOPT_NEW_ENVIRON    = 39;  /* New - Environment variables */
constexpr int TELOPT_EXOPL          = 255; /* extended-options-list */

constexpr int NTELOPTS = (1 + TELOPT_NEW_ENVIRON);
#ifdef TELOPTS
char *telopts[NTELOPTS + 1] = {
    "BINARY",
    "ECHO",
    "RCP",
    "SUPPRESS GO AHEAD",
    "NAME",
    "STATUS",
    "TIMING MARK",
    "RCTE",
    "NAOL",
    "NAOP",
    "NAOCRD",
    "NAOHTS",
    "NAOHTD",
    "NAOFFD",
    "NAOVTS",
    "NAOVTD",
    "NAOLFD",
    "EXTEND ASCII",
    "LOGOUT",
    "BYTE MACRO",
    "DATA ENTRY TERMINAL",
    "SUPDUP",
    "SUPDUP OUTPUT",
    "SEND LOCATION",
    "TERMINAL TYPE",
    "END OF RECORD",
    "TACACS UID",
    "OUTPUT MARKING",
    "TTYLOC",
    "3270 REGIME",
    "X.3 PAD",
    "NAWS",
    "TSPEED",
    "LFLOW",
    "LINEMODE",
    "XDISPLOC",
    "OLD-ENVIRON",
    "AUTHENTICATION",
    "ENCRYPT",
    "NEW-ENVIRON",
    0,
};
constexpr int TELOPT_FIRST = TELOPT_BINARY;
constexpr int TELOPT_LAST  = TELOPT_NEW_ENVIRON;
#define TELOPT_OK(x) ((unsigned int)(x) <= TELOPT_LAST)
#define TELOPT(x)    telopts[(x) - TELOPT_FIRST]
#endif

/* sub-option qualifiers */
constexpr int TELQUAL_IS    = 0; /* option is... */
constexpr int TELQUAL_SEND  = 1; /* send option */
constexpr int TELQUAL_INFO  = 2; /* ENVIRON: informational version of IS */
constexpr int TELQUAL_REPLY = 2; /* AUTHENTICATION: client version of IS */
constexpr int TELQUAL_NAME  = 3; /* AUTHENTICATION: client version of IS */

constexpr int LFLOW_OFF         = 0; /* Disable remote flow control */
constexpr int LFLOW_ON          = 1; /* Enable remote flow control */
constexpr int LFLOW_RESTART_ANY = 2; /* Restart output on any char */
constexpr int LFLOW_RESTART_XON = 3; /* Restart output only on XON */

/*
 * LINEMODE suboptions
 */

constexpr int LM_MODE        = 1;
constexpr int LM_FORWARDMASK = 2;
constexpr int LM_SLC         = 3;

constexpr int MODE_EDIT     = 0x01;
constexpr int MODE_TRAPSIG  = 0x02;
constexpr int MODE_ACK      = 0x04;
constexpr int MODE_SOFT_TAB = 0x08;
constexpr int MODE_LIT_ECHO = 0x10;

constexpr int MODE_MASK = 0x1f;

/* Not part of protocol, but needed to simplify things... */
constexpr int MODE_FLOW   = 0x0100;
constexpr int MODE_ECHO   = 0x0200;
constexpr int MODE_INBIN  = 0x0400;
constexpr int MODE_OUTBIN = 0x0800;
constexpr int MODE_FORCE  = 0x1000;

constexpr int SLC_SYNCH = 1;
constexpr int SLC_BRK   = 2;
constexpr int SLC_IP    = 3;
constexpr int SLC_AO    = 4;
constexpr int SLC_AYT   = 5;
constexpr int SLC_EOR   = 6;
constexpr int SLC_ABORT = 7;
constexpr int SLC_EOF   = 8;
constexpr int SLC_SUSP  = 9;
constexpr int SLC_EC    = 10;
constexpr int SLC_EL    = 11;
constexpr int SLC_EW    = 12;
constexpr int SLC_RP    = 13;
constexpr int SLC_LNEXT = 14;
constexpr int SLC_XON   = 15;
constexpr int SLC_XOFF  = 16;
constexpr int SLC_FORW1 = 17;
constexpr int SLC_FORW2 = 18;

constexpr int NSLC = 18;

/*
 * For backwards compatability, we define SLC_NAMES to be the
 * list of names if SLC_NAMES is not defined.
 */
#define SLC_NAMELIST "0", "SYNCH", "BRK", "IP", "AO", "AYT", "EOR",  \
                     "ABORT", "EOF", "SUSP", "EC", "EL", "EW", "RP", \
                     "LNEXT", "XON", "XOFF", "FORW1", "FORW2", 0,
#ifdef SLC_NAMES
char *slc_names[] = {
    SLC_NAMELIST};
#else
extern char *slc_names[];
#define SLC_NAMES SLC_NAMELIST
#endif

#define SLC_NAME_OK(x) ((unsigned int)(x) <= NSLC)
#define SLC_NAME(x)    slc_names[x]

constexpr int SLC_NOSUPPORT  = 0;
constexpr int SLC_CANTCHANGE = 1;
constexpr int SLC_VARIABLE   = 2;
constexpr int SLC_DEFAULT    = 3;
constexpr int SLC_LEVELBITS  = 0x03;

constexpr int SLC_FUNC  = 0;
constexpr int SLC_FLAGS = 1;
constexpr int SLC_VALUE = 2;

constexpr int SLC_ACK      = 0x80;
constexpr int SLC_FLUSHIN  = 0x40;
constexpr int SLC_FLUSHOUT = 0x20;

constexpr int OLD_ENV_VAR   = 1;
constexpr int OLD_ENV_VALUE = 0;
constexpr int NEW_ENV_VAR   = 0;
constexpr int NEW_ENV_VALUE = 1;
constexpr int ENV_ESC       = 2;
constexpr int ENV_USERVAR   = 3;

/*
 * AUTHENTICATION suboptions
 */

/*
 * Who is authenticating who ...
 */
constexpr int AUTH_WHO_CLIENT = 0; /* Client authenticating server */
constexpr int AUTH_WHO_SERVER = 1; /* Server authenticating client */
constexpr int AUTH_WHO_MASK   = 1;

/*
 * amount of authentication done
 */
constexpr int AUTH_HOW_ONE_WAY = 0;
constexpr int AUTH_HOW_MUTUAL  = 2;
constexpr int AUTH_HOW_MASK    = 2;

constexpr int AUTHTYPE_NULL        = 0;
constexpr int AUTHTYPE_KERBEROS_V4 = 1;
constexpr int AUTHTYPE_KERBEROS_V5 = 2;
constexpr int AUTHTYPE_SPX         = 3;
constexpr int AUTHTYPE_MINK        = 4;
constexpr int AUTHTYPE_CNT         = 5;

constexpr int AUTHTYPE_TEST = 99;

#ifdef AUTH_NAMES
char *authtype_names[] = {
    "NULL",
    "KERBEROS_V4",
    "KERBEROS_V5",
    "SPX",
    "MINK",
    0,
};
#else
extern char *authtype_names[];
#endif

#define AUTHTYPE_NAME_OK(x) ((unsigned int)(x) < AUTHTYPE_CNT)
#define AUTHTYPE_NAME(x)    authtype_names[x]

/*
 * ENCRYPTion suboptions
 */
constexpr int ENCRYPT_IS        = 0; /* I pick encryption type ... */
constexpr int ENCRYPT_SUPPORT   = 1; /* I support encryption types ... */
constexpr int ENCRYPT_REPLY     = 2; /* Initial setup response */
constexpr int ENCRYPT_START     = 3; /* Am starting to send encrypted */
constexpr int ENCRYPT_END       = 4; /* Am ending encrypted */
constexpr int ENCRYPT_REQSTART  = 5; /* Request you start encrypting */
constexpr int ENCRYPT_REQEND    = 6; /* Request you send encrypting */
constexpr int ENCRYPT_ENC_KEYID = 7;
constexpr int ENCRYPT_DEC_KEYID = 8;
constexpr int ENCRYPT_CNT       = 9;

constexpr int ENCTYPE_ANY       = 0;
constexpr int ENCTYPE_DES_CFB64 = 1;
constexpr int ENCTYPE_DES_OFB64 = 2;
constexpr int ENCTYPE_CNT       = 3;

#ifdef ENCRYPT_NAMES
char *encrypt_names[] = {
    "IS",
    "SUPPORT",
    "REPLY",
    "START",
    "END",
    "REQUEST-START",
    "REQUEST-END",
    "ENC-KEYID",
    "DEC-KEYID",
    0,
};
char *enctype_names[] = {
    "ANY",
    "DES_CFB64",
    "DES_OFB64",
    0,
};
#else
extern char *encrypt_names[];
extern char *enctype_names[];
#endif

#define ENCRYPT_NAME_OK(x) ((unsigned int)(x) < ENCRYPT_CNT)
#define ENCRYPT_NAME(x)    encrypt_names[x]

#define ENCTYPE_NAME_OK(x) ((unsigned int)(x) < ENCTYPE_CNT)
#define ENCTYPE_NAME(x)    enctype_names[x]
#ifdef __cplusplus
}
#endif
#endif /* _ARPA_TELNET_H */
