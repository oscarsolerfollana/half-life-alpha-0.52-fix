/*
 * motor.h - direcciones y estructuras de enginegl.exe (Half-Life Alpha 0.52)
 *
 * Base fija 0x400000, sin ASLR. Todo esto sale de desensamblar el binario;
 * el razonamiento completo esta en NOTAS SOBRE GUARDAR Y CARGAR.txt.
 */
#ifndef MOTOR_H
#define MOTOR_H

/* --- funciones --------------------------------------------------- */
#define A_CON_PRINTF        0x41754E   /* Con_Printf(fmt, ...)            */
#define A_CMD_ARGC          0x4269D3   /* int Cmd_Argc(void)              */
#define A_CMD_ARGV          0x4269D9   /* char *Cmd_Argv(int)             */
#define A_HOST_SAVEGAME     0x42CFC4   /* el guardado original            */
#define A_HOST_LOADGAME     0x42D1BD   /* la carga rota                   */
#define A_EDICT_NUM         0x409E31   /* edict_t *EDICT_NUM(int)         */
#define A_ED_FINDFIELD      0x408996   /* ddef_t *ED_FindField(char *)    */
#define A_ED_PARSEEPAIR     0x40947B   /* ED_ParseEpair(base, key, s)     */
#define A_ALLOC_PRIVATE     0x40A013   /* PvAllocEntPrivateData(ed, tam)  */
#define A_FREE_PRIVATE      0x40A045   /* FreeEntPrivateData(ed)          */
#define A_SPRINTF           0x442460
#define A_DEFAULT_EXT       0x42F38A   /* COM_DefaultExtension(buf, ext)  */
#define A_CBUF_INSERT       0x42643E   /* Cbuf_InsertText: encola AL PRINCIPIO */
#define A_CMD_ADDCOMMAND    0x426AB1   /* Cmd_AddCommand(nombre, fn)      */
#define A_HOST_MAP          0x42C93D   /* el comando "map"                */
#define A_PUSH_MAP_HANDLER  0x42E76E   /* operando del push del manejador */
#define A_HOST_BEGIN        0x42E1D7   /* comando "begin": fin del apreton
                                          de manos del cliente            */
#define A_PUSH_BEGIN_HANDLER 0x42E8A0

/* tabla de campos de entvars (ddef_t clasico de Quake, 8 bytes)         */
#define A_PROGS             0xD39280   /* dprograms_t *, numfielddefs +0x1c */
#define A_FIELDDEFS         0xD39284   /* ddef_t *                        */
#define A_STRINGS           0xD3928C   /* char * base de cadenas          */
#define A_SV_TIME           0xC60F20   /* double sv.time                  */

/* --- datos ------------------------------------------------------- */
#define A_NUM_EDICTS        0xC61CFC   /* int   sv.num_edicts             */
#define A_SV_EDICTS         0xC61D04   /* edict_t *sv.edicts              */
#define A_EDICT_SIZE        0xD39274   /* int   tamaño de un edict        */
#define A_GAMEDIR           0xC64A30   /* char  com_gamedir[]             */
#define A_MAPNAME           0xC60F38   /* char  sv.name[] (mapa actual)   */

/* la tabla de 64 punteros que el motor presta a hl.dll */
#define A_TABLA_DLL         0x45C1A0
#define IDX_CHANGELEVEL     5    /* pfnChangeLevel(mapa, landmark), 0x40403a */
#define A_PFN_CHANGELEVEL   0x40403A
#define IDX_ALLOC_PRIVATE   51
#define IDX_GET_PRIVATE     52
#define IDX_FREE_PRIVATE    53

/* sitios donde se registran los comandos: operando de los "push imm32" */
#define A_PUSH_LOAD_HANDLER 0x42E8E8
#define A_PUSH_SAVE_HANDLER 0x42E8FA

/*
 * Bug original de la alpha: el guardado reserva un bufer FIJO de 64 KB
 * (push 0x10000) y luego le mete el estado entero. Cualquier mapa real
 * pasa de 64 KB -- los cuatro .sav que vienen de fabrica pesan entre
 * 66.026 y 66.987 bytes -- asi que siempre se desborda, corrompe el heap
 * y el juego revienta al liberar el bufer, despues de haber escrito el
 * fichero. Por eso el mensaje "done." no aparecia nunca.
 * Operando del push, a ampliar a 1 MB.
 */
#define A_TAM_BUFFER_SAVE   0x42D037
#define TAM_BUFFER_NUEVO    0x100000

/* --- cambio de nivel ---------------------------------------------- */
#define A_HOST_CHANGELEVEL   0x42CBED  /* comando "changelevel"           */
#define A_HOST_CHANGELEVEL2  0x42D57B  /* comando "changelevel2"          */
#define A_PUSH_CHANGELEVEL_HANDLER  0x42E7A4
#define A_PUSH_CHANGELEVEL2_HANDLER 0x42E7B6
/* los pone a 1 pfnChangeLevel (0x40403a) al pedir el cambio y solo los
   limpia el arranque del mapa nuevo                                   */
#define A_CHANGELEVEL_ISSUED 0xC60A00  /* svs.changelevel_issued          */
#define A_SCR_DISABLED_LOAD  0xC6544C  /* scr_disabled_for_loading        */
#define A_SV_LINKEDICT       0x42A218  /* SV_LinkEdict(edict, touch)      */
#define A_SVS_MAXCLIENTS     0xC609F0  /* int svs.maxclients (lo escribe
                                          Host_Maxplayers_f, 0x427d69)  */
#define SVS_MAXCLIENTS       (*(int *)A_SVS_MAXCLIENTS)

/* trigger_changelevel de hl.dll (bloque privado): al tocarlo se apaga
   (hl.dll 0x1001c172: solid = SOLID_NOT y funcion de toque a nulo)   */
#define PRIV_TOQUE           0x10      /* m_pfnTouch                      */
#define PRIV_MAPA_DESTINO    0x80      /* char[] mapa al que lleva        */
#define SOLID_TRIGGER        1.0f
#define EF_BRIGHTFIELD       1         /* bit de entvars.effects          */

/* --- SZ_GetSpace (0x42f1da) ------------------------------------------ */
#define A_SYS_ERROR          0x40A559  /* error fatal ("Engine Error")    */
#define A_SZ_CALL_ERROR      0x42F1FB  /* call Sys_Error del "overflow
                                          without allowoverflow set"     */
/* sizebuf_t de Quake: allowoverflow, overflowed, data, maxsize, cursize.
   Destinos de WriteDest (0x403c0d):                                   */
#define A_SV_DATAGRAM        0xC61D0C  /* MSG_BROADCAST, 1 KB por frame  */
#define A_SV_RELIABLE        0xC62120  /* MSG_ALL                        */
#define A_SV_SIGNON          0xC62534  /* MSG_INIT, todo el mapa         */

/* --- menu de guardar/cargar -------------------------------------- */
/* M_ScanSaves: lee s0..s11.sav y rellena la lista del menu. Solo se
   llama con un jmp al final de M_Menu_Load_f y de M_Menu_Save_f.       */
#define A_M_SCANSAVES       0x41C434
#define A_JMP_SCAN_LOAD     0x41C543   /* jmp rel32 en M_Menu_Load_f      */
#define A_JMP_SCAN_SAVE     0x41C581   /* jmp rel32 en M_Menu_Save_f      */
#define A_MENU_NOMBRES      0xC67E40   /* char m_filenames[12][40]        */
#define A_MENU_CARGABLE     0xC67DD0   /* int  loadable[12]               */

/* --- desplazamientos dentro de edict_t --------------------------- */
#define ED_FREE_OFS         0x00   /* int  free                          */
#define ED_PRIVATE_OFS      0x74   /* void *pvPrivateData                */
#define ED_VARS_OFS         0x78   /* entvars_t v                        */

/* --- tipos ------------------------------------------------------- */
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

/* ddef_t: descriptor de un campo de entvars (el clasico de Quake) */
typedef struct {
    unsigned short tipo;      /* 1=cadena 2=float 3=vector 4=entidad... */
    unsigned short ofs;       /* desplazamiento en dwords dentro de entvars */
    unsigned int   s_nombre;  /* offset del nombre en la tabla de cadenas */
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
