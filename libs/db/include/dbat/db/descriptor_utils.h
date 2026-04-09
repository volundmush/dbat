#pragma once

#define STATE(d)	((d)->connected)

#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_CEDIT || STATE(d) == CON_PLAYING ||     \
                        STATE(d) == CON_TRIGEDIT || STATE(d) == CON_AEDIT ||    \
                        STATE(d) == CON_GEDIT || STATE(d) == CON_IEDIT ||       \
                        STATE(d) == CON_HEDIT || STATE(d) == CON_NEWSEDIT ||    \
                        STATE(d) == CON_POBJ)
#define IS_INMENU(d)    (STATE(d) == CON_MENU || STATE(d) == CON_EXDESC || STATE(d) == CON_UMENU || STATE(d) == CON_GET_USER || STATE(d) == CON_GET_EMAIL || STATE(d) == CON_CHPWD_GETOLD || STATE(d) == CON_CHPWD_GETNEW || STATE(d) == CON_CHPWD_VRFY || STATE(d) == CON_DELCNF1 || STATE(d) == CON_DELCNF2 || STATE(d) == CON_QRACE || STATE(d) == CON_QCLASS || STATE(d) == CON_CLASS_HELP || STATE(d) == CON_RACE_HELP || STATE(d) == CON_BONUS || STATE(d) == CON_NEGATIVE || STATE(d) == CON_DISTFEA || STATE(d) == CON_HW || STATE(d) == CON_AURA)
