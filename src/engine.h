/*
 * engine.h - addresses and structures of enginegl.exe (Half-Life Alpha 0.52)
 *
 * Fixed base 0x400000, no ASLR. All of this comes from disassembling the
 * binary; the full reasoning is in docs/NOTES.txt.
 */
#ifndef MOTOR_H
#define MOTOR_H

/* --- functions --------------------------------------------------- */
#define A_CON_PRINTF        0x41754E   /* Con_Printf(fmt, ...)            */
#define A_CMD_ARGC          0x4269D3   /* int Cmd_Argc(void)              */
#define A_CMD_ARGV          0x4269D9   /* char *Cmd_Argv(int)             */
#define A_HOST_SAVEGAME     0x42CFC4   /* the original save               */
#define A_HOST_LOADGAME     0x42D1BD   /* the broken load                 */
#define A_EDICT_NUM         0x409E31   /* edict_t *EDICT_NUM(int)         */
#define A_ED_FINDFIELD      0x408996   /* ddef_t *ED_FindField(char *)    */
#define A_ED_PARSEEPAIR     0x40947B   /* ED_ParseEpair(base, key, s)     */
#define A_ALLOC_PRIVATE     0x40A013   /* PvAllocEntPrivateData(ed, size) */
#define A_FREE_PRIVATE      0x40A045   /* FreeEntPrivateData(ed)          */
#define A_SPRINTF           0x442460
#define A_DEFAULT_EXT       0x42F38A   /* COM_DefaultExtension(buf, ext)  */
#define A_CBUF_INSERT       0x42643E   /* Cbuf_InsertText: queues AT THE FRONT */
#define A_CMD_ADDCOMMAND    0x426AB1   /* Cmd_AddCommand(name, fn)        */
#define A_HOST_MAP          0x42C93D   /* the "map" command               */
#define A_PUSH_MAP_HANDLER  0x42E76E   /* operand of the handler's push   */
#define A_HOST_BEGIN        0x42E1D7   /* "begin" command: end of the
                                          client handshake                */
#define A_PUSH_BEGIN_HANDLER 0x42E8A0

/* entvars field table (classic Quake ddef_t, 8 bytes)                  */
#define A_PROGS             0xD39280   /* dprograms_t *, numfielddefs +0x1c */
#define A_FIELDDEFS         0xD39284   /* ddef_t *                        */
#define A_STRINGS           0xD3928C   /* char * string base              */
#define A_SV_TIME           0xC60F20   /* double sv.time                  */

/* --- data ---------------------------------------------------------- */
#define A_NUM_EDICTS        0xC61CFC   /* int   sv.num_edicts             */
#define A_SV_EDICTS         0xC61D04   /* edict_t *sv.edicts              */
#define A_EDICT_SIZE        0xD39274   /* int   size of one edict         */
#define A_GAMEDIR           0xC64A30   /* char  com_gamedir[]             */
#define A_MAPNAME           0xC60F38   /* char  sv.name[] (current map)   */

/* the table of 64 pointers the engine lends to hl.dll */
#define A_TABLA_DLL         0x45C1A0
#define IDX_CHANGELEVEL     5    /* pfnChangeLevel(map, landmark), 0x40403a */
#define A_PFN_CHANGELEVEL   0x40403A
#define IDX_ALLOC_PRIVATE   51
#define IDX_GET_PRIVATE     52
#define IDX_FREE_PRIVATE    53

/* places where the commands are registered: operand of the "push imm32" */
#define A_PUSH_LOAD_HANDLER 0x42E8E8
#define A_PUSH_SAVE_HANDLER 0x42E8FA

/*
 * Original alpha bug: saving allocates a FIXED 64 KB buffer (push 0x10000)
 * and then writes the whole state into it. Any real map exceeds 64 KB --
 * the four stock .sav files weigh between 66,026 and 66,987 bytes -- so it
 * always overflows, corrupts the heap and the game crashes when freeing
 * the buffer, after having written the file. That is why the "done."
 * message never appeared.
 * Operand of the push, to be enlarged to 1 MB.
 */
#define A_TAM_BUFFER_SAVE   0x42D037
#define TAM_BUFFER_NUEVO    0x100000

/* --- level change ----------------------------------------------- */
#define A_HOST_CHANGELEVEL   0x42CBED  /* "changelevel" command           */
#define A_HOST_CHANGELEVEL2  0x42D57B  /* "changelevel2" command          */
#define A_PUSH_CHANGELEVEL_HANDLER  0x42E7A4
#define A_PUSH_CHANGELEVEL2_HANDLER 0x42E7B6
/* pfnChangeLevel (0x40403a) sets these to 1 when the change is requested
   and only the new map's startup clears them                          */
#define A_CHANGELEVEL_ISSUED 0xC60A00  /* svs.changelevel_issued          */
#define A_SCR_DISABLED_LOAD  0xC6544C  /* scr_disabled_for_loading        */
#define A_SV_LINKEDICT       0x42A218  /* SV_LinkEdict(edict, touch)      */
#define A_SVS_MAXCLIENTS     0xC609F0  /* int svs.maxclients (written by
                                          Host_Maxplayers_f, 0x427d69)  */
#define SVS_MAXCLIENTS       (*(int *)A_SVS_MAXCLIENTS)

/* hl.dll's trigger_changelevel (private block): touching it turns it off
   (hl.dll 0x1001c172: solid = SOLID_NOT and touch function set to null) */
#define PRIV_TOQUE           0x10      /* m_pfnTouch                      */
#define PRIV_MAPA_DESTINO    0x80      /* char[] map it leads to          */
#define SOLID_TRIGGER        1.0f
#define EF_BRIGHTFIELD       1         /* entvars.effects bit             */

/* --- SZ_GetSpace (0x42f1da) ------------------------------------------ */
#define A_SYS_ERROR          0x40A559  /* fatal error ("Engine Error")    */
#define A_SZ_CALL_ERROR      0x42F1FB  /* call Sys_Error for "overflow
                                          without allowoverflow set"     */
/* Quake's sizebuf_t: allowoverflow, overflowed, data, maxsize, cursize.
   WriteDest destinations (0x403c0d):                                  */
#define A_SV_DATAGRAM        0xC61D0C  /* MSG_BROADCAST, 1 KB per frame  */
#define A_SV_RELIABLE        0xC62120  /* MSG_ALL                        */
#define A_SV_SIGNON          0xC62534  /* MSG_INIT, the whole map        */

/* --- save/load menu --------------------------------------------- */
/* M_ScanSaves: reads s0..s11.sav and fills the menu list. It is only
   called via a jmp at the end of M_Menu_Load_f and M_Menu_Save_f.      */
#define A_M_SCANSAVES       0x41C434
#define A_JMP_SCAN_LOAD     0x41C543   /* jmp rel32 in M_Menu_Load_f      */
#define A_JMP_SCAN_SAVE     0x41C581   /* jmp rel32 in M_Menu_Save_f      */
#define A_MENU_NOMBRES      0xC67E40   /* char m_filenames[12][40]        */
#define A_MENU_CARGABLE     0xC67DD0   /* int  loadable[12]               */

/* --- offsets inside edict_t ------------------------------------ */
#define ED_FREE_OFS         0x00   /* int  free                          */
#define ED_PRIVATE_OFS      0x74   /* void *pvPrivateData                */
#define ED_VARS_OFS         0x78   /* entvars_t v                        */

/* --- types ------------------------------------------------------- */
typedef void  (__cdecl *fn_printf_t)(const char *, ...);
typedef int   (__cdecl *fn_argc_t)(void);
typedef char *(__cdecl *fn_argv_t)(int);
typedef void  (__cdecl *fn_void_t)(void);
typedef void *(__cdecl *fn_edictnum_t)(int);
typedef void *(__cdecl *fn_findfield_t)(const char *);
typedef int   (__cdecl *fn_parseepair_t)(void *, void *, const char *);
typedef void *(__cdecl *fn_alloc_t)(void *, int);
typedef void  (__cdecl *fn_free_t)(void *);
typedef void  (__cdecl *fn_cbuf_t)(const char *);
typedef void  (__cdecl *fn_addcmd_t)(const char *, void *);

/* ddef_t: descriptor of an entvars field (the classic Quake one) */
typedef struct {
    unsigned short tipo;      /* 1=string 2=float 3=vector 4=entity... */
    unsigned short ofs;       /* offset in dwords inside entvars */
    unsigned int   s_nombre;  /* offset of the name in the string table */
} ddef_t;

#define TIPO_CADENA 1

#define Cbuf_InsertText (*(fn_cbuf_t)A_CBUF_INSERT)
#define Cmd_AddCommand (*(fn_addcmd_t)A_CMD_ADDCOMMAND)
#define SV_TIME        (*(double *)A_SV_TIME)

#define Con_Printf   (*(fn_printf_t)A_CON_PRINTF)
#define Cmd_Argc     (*(fn_argc_t)A_CMD_ARGC)
#define Cmd_Argv     (*(fn_argv_t)A_CMD_ARGV)
#define EDICT_NUM    (*(fn_edictnum_t)A_EDICT_NUM)
#define ED_FindField (*(fn_findfield_t)A_ED_FINDFIELD)
#define ED_ParseEpair (*(fn_parseepair_t)A_ED_PARSEEPAIR)

#define NUM_EDICTS   (*(int *)A_NUM_EDICTS)
#define SV_EDICTS    (*(char **)A_SV_EDICTS)
#define EDICT_SIZE   (*(int *)A_EDICT_SIZE)
#define GAMEDIR      ((char *)A_GAMEDIR)
#define MAPNAME      ((char *)A_MAPNAME)

#define MAX_EDICTS_PROPIO 2048

#endif
