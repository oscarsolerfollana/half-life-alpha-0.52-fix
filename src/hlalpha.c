/*
 * hlalpha.c -- the Half-Life Alpha 0.52 fixes that are applied at run
 * time (formerly hlsave.dll). It lives inside winmm.dll: see loader.c,
 * which checks the binaries and applies the static patches.
 */
/*
 * hlsave.dll - complete saving and loading for Half-Life Alpha 0.52
 *
 * Why it exists: the alpha engine only saves each entity's entvars, as
 * text. The private C++ state (the block hl.dll allocates per entity) is
 * never written, because the DLL's Save is an empty stub and its Restore
 * is a bare "ret". And loading does not even exist: what is left is
 * Quake's text parser, which does not understand the new format.
 *
 * What is missing is done here:
 *   - the save buffer overflow is fixed (a 1997 bug)
 *   - the size of each entity's private block is recorded
 *   - "save" appends our own block with ALL the state to the file
 *   - "load" is rewritten from scratch
 *
 * (Originally it was injected by redirecting the entry point of
 * enginegl.exe.)
 */

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "engine.h"

/* ================================================================== */
/*  logging to a file                                                 */
/* ================================================================== */

void reg(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    HANDLE h;
    DWORD w;

    va_start(ap, fmt);
    wvsprintfA(buf, fmt, ap);
    va_end(ap);

    h = CreateFileA("hlalpha.log", FILE_APPEND_DATA, FILE_SHARE_READ,
                    NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    WriteFile(h, buf, (DWORD)strlen(buf), &w, NULL);
    WriteFile(h, "\r\n", 2, &w, NULL);
    CloseHandle(h);
}

/* ================================================================== */
/*  memory patching                                                   */
/* ================================================================== */

static int escribir_dword(unsigned int direccion, unsigned int valor)
{
    DWORD viejo;
    if (!VirtualProtect((void *)direccion, 4, PAGE_EXECUTE_READWRITE, &viejo))
        return 0;
    *(unsigned int *)direccion = valor;
    VirtualProtect((void *)direccion, 4, viejo, &viejo);
    return 1;
}

/* ================================================================== */
/*  module bases                                                      */
/* ================================================================== */
/*
 * hl.dll does NOT load at 0x10000000. It has a relocation table and
 * Windows moves it wherever it likes: in three consecutive launches it
 * ended up at 0x1f160000, 0x3daf0000 and 0x3d5e0000. That matters a lot
 * here, because offset 0 of every private block is the pointer to the
 * class's C++ vtable, i.e. an address INSIDE hl.dll. Saved raw it is only
 * valid while the module stays in the same place, and it does not: when
 * the game is reopened, the loaded game jumps to dead pointers. That is
 * why they are stored as offsets inside the module.
 *
 * The .exe does have a fixed base (0x400000, no relocations), so pointers
 * to its static data -- strings among them -- can be left as they are.
 */
static unsigned int base_dll, tam_dll, base_exe, tam_exe;

static void tam_de_modulo(void *m, unsigned int *base, unsigned int *tam)
{
    unsigned char *b = (unsigned char *)m;
    unsigned int e;

    *base = 0; *tam = 0;
    if (!b || *(unsigned short *)b != 0x5A4D) return;        /* MZ */
    e = *(unsigned int *)(b + 0x3C);
    if (*(unsigned int *)(b + e) != 0x00004550) return;      /* PE00 */
    *base = (unsigned int)b;
    *tam  = *(unsigned int *)(b + e + 24 + 56);              /* SizeOfImage */
}

static void localizar_modulos(void)
{
    tam_de_modulo(GetModuleHandleA("hl.dll"), &base_dll, &tam_dll);
    tam_de_modulo(GetModuleHandleA(NULL),     &base_exe, &tam_exe);
}

/* ================================================================== */
/*  size of the private blocks                                        */
/* ================================================================== */

static int tam_privado[MAX_EDICTS_PROPIO];

static int indice_de_edict(void *ed)
{
    char *base = SV_EDICTS;
    int   paso = EDICT_SIZE;
    long  d;

    if (!base || paso <= 0 || !ed) return -1;
    d = (char *)ed - base;
    if (d < 0 || (d % paso) != 0) return -1;
    d /= paso;
    if (d < 0 || d >= MAX_EDICTS_PROPIO) return -1;
    return (int)d;
}

static fn_alloc_t alloc_original;
static fn_free_t  free_original;

static void * __cdecl mi_alloc_privado(void *ed, int tam)
{
    void *r = alloc_original(ed, tam);
    int i = indice_de_edict(ed);
    if (i >= 0) tam_privado[i] = (r ? tam : 0);
    return r;
}

static void __cdecl mi_free_privado(void *ed)
{
    int i = indice_de_edict(ed);
    if (i >= 0) tam_privado[i] = 0;
    free_original(ed);
}

/* ================================================================== */
/*  utilities                                                         */
/* ================================================================== */

static void *edict_n(int i)       { return SV_EDICTS + (size_t)i * EDICT_SIZE; }
static int   edict_libre(void *e) { return *(int *)((char *)e + ED_FREE_OFS) != 0; }
static void *privado_de(void *e)  { return *(void **)((char *)e + ED_PRIVATE_OFS); }
static char *vars_de(void *e)     { return (char *)e + ED_VARS_OFS; }
static int   tam_entvars(void)    { return EDICT_SIZE - ED_VARS_OFS; }

static int num_fielddefs(void)
{
    char *progs = *(char **)A_PROGS;
    return progs ? *(int *)(progs + 0x1c) : 0;
}

static ddef_t *fielddef_n(int i)
{
    ddef_t *base = *(ddef_t **)A_FIELDDEFS;
    return base ? base + i : NULL;
}

/* Logs the player's state, to check that what is restored is what was
   saved. wvsprintf knows nothing about decimals, so they go as integers. */
static void registrar_jugador(const char *cuando)
{
    ddef_t *dh, *dorg, *dva;
    void *e;
    float vida = 0, *org = NULL, *vang = NULL;

    if (!SV_EDICTS || NUM_EDICTS < 2) return;
    e = edict_n(1);
    dh   = (ddef_t *)ED_FindField("health");
    dorg = (ddef_t *)ED_FindField("origin");
    dva  = (ddef_t *)ED_FindField("v_angle");
    if (dh)   vida = *(float *)((char *)e + ED_VARS_OFS + dh->ofs * 4);
    if (dorg) org  = (float *)((char *)e + ED_VARS_OFS + dorg->ofs * 4);
    if (dva)  vang = (float *)((char *)e + ED_VARS_OFS + dva->ofs * 4);

    {
        /* checksum of the private block: if it matches before and after,
           the C++ state has travelled intact */
        unsigned char *p = (unsigned char *)privado_de(e);
        int tam = tam_privado[1], k;
        unsigned int suma = 0;
        for (k = 0; p && k < tam; k++) suma = suma * 31u + p[k];

        reg("%s: t=%dms health=%d org=(%d %d %d) vang=(%d %d %d) priv=%d sum=0x%08x",
            cuando, (int)(SV_TIME * 1000.0), (int)vida,
            org  ? (int)org[0]  : 0, org  ? (int)org[1]  : 0, org  ? (int)org[2]  : 0,
            vang ? (int)vang[0] : 0, vang ? (int)vang[1] : 0, vang ? (int)vang[2] : 0,
            tam, suma);
    }
}

/*
 * The view angles are restored on the server (they are in the entvars),
 * but the CLIENT keeps its own copy and never finds out. In the Quake this
 * engine comes from, the way to force it to look in the right direction
 * is to set fixangle=1 on the player entity: the server then sends it an
 * svc_setangle with v.angles and the client snaps to that direction.
 * Without this the player always loads looking in the same direction.
 */
static void forzar_vista_jugador(void)
{
    ddef_t *dva, *dang, *dfix;
    void *e;
    float *va, *ang;

    if (!SV_EDICTS || NUM_EDICTS < 2) return;
    e = edict_n(1);
    if (edict_libre(e)) return;

    dva  = (ddef_t *)ED_FindField("v_angle");
    dang = (ddef_t *)ED_FindField("angles");
    dfix = (ddef_t *)ED_FindField("fixangle");
    if (!dva || !dang || !dfix) {
        reg("view: missing fields (v_angle=%d angles=%d fixangle=%d)",
            dva != 0, dang != 0, dfix != 0);
        return;
    }

    va  = (float *)((char *)e + ED_VARS_OFS + dva->ofs * 4);
    ang = (float *)((char *)e + ED_VARS_OFS + dang->ofs * 4);

    /*
     * svc_setangle sends v.angles, not v_angle, so they have to be made
     * equal.
     *
     * But the ROLL (index 2) is set to zero on purpose. It is the tilt
     * effect when strafing, and the engine recomputes it by itself every
     * frame from the velocity: it is not a real view angle, it must not be
     * saved or restored. If it is forced on the client, it sticks in
     * cl.viewangles and nobody ever sets it back to zero, so you load
     * tilted and stay tilted forever.
     * NOTE: this is not enough, SV_ClientThink recomputes it from the
     * velocity before sending it; what fixes it is making the client
     * discard the roll of svc_setangle (parchear.ps1, NOTES section 21).
     */
    ang[0] = va[0];
    ang[1] = va[1];
    ang[2] = 0.0f;
    *(float *)((char *)e + ED_VARS_OFS + dfix->ofs * 4) = 1.0f;

    reg("view: v_angle=(%d %d %d), roll %d discarded, fixangle set",
        (int)va[0], (int)va[1], (int)va[2], (int)va[2]);
}

static char *cadena_motor(unsigned int ofs)
{
    char *base = *(char **)A_STRINGS;
    return base ? base + ofs : NULL;
}

/* ================================================================== */
/*  format of our own block                                           */
/* ================================================================== */

#define MARCA_EXTRA   0x58444C43u   /* CLDX */
#define VERSION_EXTRA 3

#define RELOC_EDICT   1   /* pointer to an edict                          */
#define RELOC_PRIVADO 2   /* pointer to another private block             */
#define RELOC_DLL     3   /* pointer inside hl.dll (vtables and methods)  */
#define RELOC_VIVO    4   /* pointer to something in the engine that we
                             cannot translate: on load the value from the
                             freshly started process is kept, not the
                             saved one                                    */

typedef struct {
    unsigned int desplazamiento;
    unsigned int clase;
    unsigned int destino;
    unsigned int dentro;
} reloc_t;

#define MAX_RELOCS 8192

/* ---------------- writing ---------------- */

/*
 * The whole block is assembled in memory and written in one go at the
 * end. With one WriteFile per field (thousands per save) the block took
 * 1.3-1.5 s in c1a3; the engine's own save, 6 ms. The HANDLE is ignored.
 */
static struct { unsigned char *d; unsigned int n, cap; int error; } sal;

static void escribir(HANDLE h, const void *p, unsigned int n)
{
    (void)h;
    if (!n || sal.error) return;
    if (sal.n + n > sal.cap) {
        unsigned int cap = sal.cap ? sal.cap : 0x100000;
        unsigned char *nuevo;
        while (cap < sal.n + n) cap *= 2;
        nuevo = sal.d ? (unsigned char *)HeapReAlloc(GetProcessHeap(), 0, sal.d, cap)
                      : (unsigned char *)HeapAlloc(GetProcessHeap(), 0, cap);
        if (!nuevo) { sal.error = 1; return; }
        sal.d = nuevo;
        sal.cap = cap;
    }
    memcpy(sal.d + sal.n, p, n);
    sal.n += n;
}

static void escribir_u32(HANDLE h, unsigned int v) { escribir(h, &v, 4); }

static void escribir_cadena(HANDLE h, const char *s)
{
    if (!s) s = "";
    escribir(h, s, (unsigned int)strlen(s) + 1);
}

/*
 * Scan context. It is the same for the entvars and for the private
 * blocks: both contain pointers and in both they have to be recorded.
 * Previously only the private blocks were scanned, and that is why the
 * entvars' pContainingEntity was saved raw.
 */
static struct {
    unsigned char **privs;
    int            *tams;
    int             nedicts;
    unsigned int    minp, maxp;        /* range of the private blocks       */
    unsigned int    edbase, edtop;     /* edict array                       */
    unsigned int    venbase, ventop;   /* hunk window, see below            */
} esc;

/*
 * Window around the edict array to recognise pointers into the engine's
 * hunk. Deliberately narrow: recognising pointers by value risks mistaking
 * a float for an address, and the wider the range, the easier a false
 * positive. 1 MB is enough -- what has been seen in practice falls less
 * than 2 KB from the array -- and it leaves out the range where a map's
 * normal floats fall.
 */
#define VENTANA_HUNK 0x100000u

static void preparar_escaneo(unsigned char **privs, int *tams, int nedicts,
                             unsigned int minp, unsigned int maxp)
{
    esc.privs   = privs;
    esc.tams    = tams;
    esc.nedicts = nedicts;
    esc.minp    = minp;
    esc.maxp    = maxp;
    esc.edbase  = (unsigned int)SV_EDICTS;
    esc.edtop   = esc.edbase + (unsigned int)EDICT_SIZE * (unsigned int)nedicts;
    esc.venbase = esc.edbase > VENTANA_HUNK ? esc.edbase - VENTANA_HUNK : 0;
    esc.ventop  = esc.edtop + VENTANA_HUNK;
}

/* is there real memory at that address? second filter after the window */
/*
 * VirtualQuery takes ~2.5 ms per call on this system (something hooks
 * it), and it was queried once per entity, almost always for the same
 * address (pSystemGlobals): 510 calls = 1.3 s per save. The last queried
 * region is remembered; addresses that fall inside it are answered
 * without calling the system.
 */
static int direccion_viva(unsigned int v)
{
    static unsigned int reg_ini, reg_fin;
    static int reg_viva;
    MEMORY_BASIC_INFORMATION mbi;

    if (reg_fin && v >= reg_ini && v < reg_fin) return reg_viva;
    if (!VirtualQuery((void *)v, &mbi, sizeof(mbi))) return 0;
    reg_ini  = (unsigned int)mbi.BaseAddress;
    reg_fin  = reg_ini + (unsigned int)mbi.RegionSize;
    reg_viva = (mbi.State == MEM_COMMIT);
    return reg_viva;
}

static unsigned int buscar_relocs(unsigned char *bloque, unsigned int tam,
                                  reloc_t *salida, unsigned int maxsalida)
{
    unsigned int n = 0, o;

    for (o = 0; o + 4 <= tam && n < maxsalida; o += 4) {
        unsigned int v = *(unsigned int *)(bloque + o);

        if (v < 0x10000) continue;

        /* inside the edict array */
        if (esc.edbase && v >= esc.edbase && v < esc.edtop) {
            unsigned int d = v - esc.edbase;
            salida[n].desplazamiento = o;
            salida[n].clase   = RELOC_EDICT;
            salida[n].destino = d / (unsigned int)EDICT_SIZE;
            salida[n].dentro  = d % (unsigned int)EDICT_SIZE;
            n++;
            continue;
        }

        /* inside another private block */
        if (v >= esc.minp && v < esc.maxp) {
            int j, hallado = 0;
            for (j = 0; j < esc.nedicts; j++) {
                unsigned int p = (unsigned int)esc.privs[j];
                if (!p || esc.tams[j] <= 0) continue;
                if (v >= p && v < p + (unsigned int)esc.tams[j]) {
                    salida[n].desplazamiento = o;
                    salida[n].clase   = RELOC_PRIVADO;
                    salida[n].destino = (unsigned int)j;
                    salida[n].dentro  = v - p;
                    n++;
                    hallado = 1;
                    break;
                }
            }
            if (hallado) continue;
        }

        /* inside hl.dll: vtables and method pointers */
        if (tam_dll && v >= base_dll && v < base_dll + tam_dll) {
            salida[n].desplazamiento = o;
            salida[n].clase   = RELOC_DLL;
            salida[n].destino = 0;
            salida[n].dentro  = v - base_dll;
            n++;
            continue;
        }

        /* static data of the .exe: fixed base, left as they are */
        if (tam_exe && v >= base_exe && v < base_exe + tam_exe) continue;

        /* something in the engine's hunk that we cannot name */
        if (v >= esc.venbase && v < esc.ventop && direccion_viva(v)) {
            salida[n].desplazamiento = o;
            salida[n].clase   = RELOC_VIVO;
            salida[n].destino = 0;
            salida[n].dentro  = v;          /* only for the log */
            n++;
        }
    }
    return n;
}

/*
 * The entvars are copied raw, but string-type fields hold an offset into
 * the engine's string table, which is rebuilt differently on every
 * launch. So those go separately, as text, and on load they are rebuilt
 * with ED_ParseEpair.
 */
static void escribir_cadenas_de(HANDLE h, void *ed)
{
    int n = num_fielddefs(), i;
    unsigned int cuenta = 0;
    unsigned int pos_cuenta = sal.n;

    escribir_u32(h, 0);                       /* placeholder for the count */

    for (i = 1; i < n; i++) {
        ddef_t *d = fielddef_n(i);
        unsigned int valor;
        if (!d || (d->tipo & 0x7fff) != TIPO_CADENA) continue;
        valor = *(unsigned int *)(vars_de(ed) + d->ofs * 4);
        if (!valor) continue;
        escribir_u32(h, (unsigned int)i);
        escribir_cadena(h, cadena_motor(valor));
        cuenta++;
    }

    if (cuenta && !sal.error)                 /* fill in the count */
        memcpy(sal.d + pos_cuenta, &cuenta, 4);
}

/* counts the relocations by class, only for the log */
static void contar_clases(reloc_t *r, unsigned int n, unsigned int *por_clase)
{
    unsigned int k;
    for (k = 0; k < n; k++)
        if (r[k].clase < 5) por_clase[r[k].clase]++;
}

static void anexar_bloques(const char *ruta)
{
    HANDLE h;
    int nedicts = NUM_EDICTS, i;
    unsigned char **privs;
    int *tams;
    unsigned int minp = 0xFFFFFFFFu, maxp = 0;
    unsigned int total_relocs = 0, con_datos = 0, ocupados = 0;
    unsigned int por_clase[5];
    static reloc_t relocs[MAX_RELOCS];

    if (nedicts <= 0 || nedicts > MAX_EDICTS_PROPIO) {
        reg("append: num_edicts out of range (%d)", nedicts);
        return;
    }

    for (i = 0; i < 5; i++) por_clase[i] = 0;
    localizar_modulos();
    if (!tam_dll) {
        Con_Printf("hlsave: hl.dll not found in memory, full state not saved\n");
        reg("append: hl.dll not located, aborted");
        return;
    }

    privs = (unsigned char **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof(void *) * nedicts);
    tams  = (int *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                             sizeof(int) * nedicts);
    if (!privs || !tams) return;

    for (i = 0; i < nedicts; i++) {
        void *e = edict_n(i);
        if (edict_libre(e)) continue;
        ocupados++;
        privs[i] = (unsigned char *)privado_de(e);
        tams[i]  = tam_privado[i];
        if (privs[i] && tams[i] > 0) {
            unsigned int p = (unsigned int)privs[i];
            if (p < minp) minp = p;
            if (p + (unsigned int)tams[i] > maxp) maxp = p + (unsigned int)tams[i];
            con_datos++;
        }
    }

    preparar_escaneo(privs, tams, nedicts, minp, maxp);

    h = NULL;                        /* written to memory (sal) */
    sal.n = 0;
    sal.error = 0;

    escribir_u32(h, MARCA_EXTRA);
    escribir_u32(h, VERSION_EXTRA);
    escribir_cadena(h, MAPNAME);
    escribir(h, (void *)A_SV_TIME, 8);
    escribir_u32(h, (unsigned int)nedicts);
    escribir_u32(h, (unsigned int)tam_entvars());
    escribir_u32(h, base_dll);       /* where hl.dll loaded in this session */

    for (i = 0; i < nedicts; i++) {
        void *e = edict_n(i);
        unsigned int nr = 0;

        if (edict_libre(e)) { escribir_u32(h, 1); continue; }
        escribir_u32(h, 0);

        /*
         * The entvars carry pointers too: each entity's pContainingEntity
         * points to its own edict, and there are a few more pointers to
         * engine structures. They used to be copied raw, which is what
         * made loading crash in another session.
         */
        escribir(h, vars_de(e), (unsigned int)tam_entvars());
        nr = buscar_relocs((unsigned char *)vars_de(e),
                           (unsigned int)tam_entvars(),
                           relocs, MAX_RELOCS);
        escribir_u32(h, nr);
        escribir(h, relocs, nr * (unsigned int)sizeof(reloc_t));
        contar_clases(relocs, nr, por_clase);
        total_relocs += nr;

        escribir_cadenas_de(h, e);

        if (privs[i] && tams[i] > 0) {
            escribir_u32(h, (unsigned int)tams[i]);
            escribir(h, privs[i], (unsigned int)tams[i]);
            nr = buscar_relocs(privs[i], (unsigned int)tams[i],
                               relocs, MAX_RELOCS);
            escribir_u32(h, nr);
            escribir(h, relocs, nr * (unsigned int)sizeof(reloc_t));
            contar_clases(relocs, nr, por_clase);
            total_relocs += nr;
        } else {
            escribir_u32(h, 0);
            escribir_u32(h, 0);
        }
    }

    /* and now, in one go, at the end of the file the engine has just written */
    if (sal.error) {
        reg("append: out of memory to assemble the block, full state not saved");
        goto fin;
    }
    h = CreateFileA(ruta, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        reg("append: could not open %s (error %lu)", ruta, GetLastError());
        goto fin;
    }
    SetFilePointer(h, 0, NULL, FILE_END);
    {
        DWORD w = 0;
        if (!WriteFile(h, sal.d, sal.n, &w, NULL) || w != sal.n)
            reg("append: incomplete write to %s (%lu of %u bytes)", ruta, w, sal.n);
    }
    CloseHandle(h);
    Con_Printf("hlsave: %d entities (%d with private state), %d pointers\n",
               ocupados, con_datos, total_relocs);
    registrar_jugador("ON SAVE");
    reg("saved: edicts=%d in_use=%u with_data=%u relocs=%u map=%s",
        nedicts, ocupados, con_datos, total_relocs, MAPNAME);
    reg("saved: pointers by class: edict=%u private=%u hl.dll=%u live=%u"
        "  (hl.dll at 0x%08x)",
        por_clase[RELOC_EDICT], por_clase[RELOC_PRIVADO],
        por_clase[RELOC_DLL], por_clase[RELOC_VIVO], base_dll);

fin:
    HeapFree(GetProcessHeap(), 0, privs);
    HeapFree(GetProcessHeap(), 0, tams);
}

/* ================================================================== */
/*  map persistence across transitions                                */
/* ================================================================== */
/*
 * The alpha starts every map from scratch on each changelevel: when you
 * went back to a map, buttons, doors and enemies were as at the start.
 * Now, when leaving through a transition the map's state is saved to
 * valve\SAVE\<map>.niv (the same CLDX block as the saves) and restored on
 * re-entry, except the player, who arrives through the landmark with
 * their health, weapons and velocity. "map" (new game) deletes them;
 * "save" copies them to valve\SAVE\<savegame>\ and "load" gets them back
 * from there, so the other maps are as they were when the game was saved.
 */
static int conservar_jugador = 0;   /* mi_restaurar: do not touch edict 1 */
static int nivel_pendiente   = 0;   /* the next begin comes from a changelevel */

static void dir_niveles(char *out, const char *partida)
{
    if (partida) wsprintfA(out, "%s/SAVE/%s", GAMEDIR, partida);
    else         wsprintfA(out, "%s/SAVE", GAMEDIR);
}

static void borrar_niveles(const char *dir)
{
    char patron[600], f[600];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    int n = 0;

    wsprintfA(patron, "%s/*.niv", dir);
    h = FindFirstFileA(patron, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        wsprintfA(f, "%s/%s", dir, fd.cFileName);
        if (DeleteFileA(f)) n++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    if (n) reg("levels: deleted %d states from %s", n, dir);
}

static void copiar_niveles(const char *de, const char *a)
{
    char patron[600], f1[600], f2[600];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    int n = 0;

    CreateDirectoryA(a, NULL);
    wsprintfA(patron, "%s/*.niv", de);
    h = FindFirstFileA(patron, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        wsprintfA(f1, "%s/%s", de, fd.cFileName);
        wsprintfA(f2, "%s/%s", a, fd.cFileName);
        if (CopyFileA(f1, f2, FALSE)) n++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    reg("levels: copied %d states from %s to %s", n, de, a);
}

/* saves the current map's state to SAVE\<map>.niv (before leaving) */
static void guardar_nivel_actual(void)
{
    char dir[512], ruta[600];
    HANDLE h;

    if (SVS_MAXCLIENTS != 1 || !SV_EDICTS || !*MAPNAME) return;
    dir_niveles(dir, NULL);
    CreateDirectoryA(dir, NULL);
    wsprintfA(ruta, "%s/%s.niv", dir, MAPNAME);
    h = CreateFileA(ruta, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { reg("levels: could not create %s", ruta); return; }
    CloseHandle(h);
    anexar_bloques(ruta);
    reg("levels: saved the state of %s on exit", MAPNAME);
}

static void __cdecl mi_save(void)
{
    char ruta[512], de[512], a[512];
    const char *nombre;
    FILETIME antes;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (Cmd_Argc() == 2 && lstrcmpiA(Cmd_Argv(1), "quick") == 0) {
        /* stock F6/F7: the quick save goes to menu slot 12 (s11) */
        Cbuf_InsertText("save s11\n");
        return;
    }

    LARGE_INTEGER frec, c0, c1, c2, c3;
    QueryPerformanceFrequency(&frec);
    QueryPerformanceCounter(&c0);
    GetSystemTimeAsFileTime(&antes);
    ((fn_void_t)A_HOST_SAVEGAME)();      /* the original save, untouched */
    QueryPerformanceCounter(&c1);

    if (Cmd_Argc() != 2) return;
    nombre = Cmd_Argv(1);
    if (!nombre || !*nombre) return;

    wsprintfA(ruta, "%s/%s.sav", GAMEDIR, nombre);
    /*
     * Only if the engine HAS JUST written the file: if it refused to save
     * ("Can't savegame with a dead player"...) and there was already a
     * .sav with that name, a block would be appended to an old game.
     * 2 s margin because of the resolution of file timestamps.
     */
    if (!GetFileAttributesExA(ruta, GetFileExInfoStandard, &info)) return;
    {
        ULARGE_INTEGER t0, t1;
        t0.LowPart = antes.dwLowDateTime;  t0.HighPart = antes.dwHighDateTime;
        t1.LowPart = info.ftLastWriteTime.dwLowDateTime;
        t1.HighPart = info.ftLastWriteTime.dwHighDateTime;
        if (t1.QuadPart + 20000000ULL < t0.QuadPart) {
            reg("save: the engine did not write %s, nothing appended", ruta);
            return;
        }
    }
    anexar_bloques(ruta);
    QueryPerformanceCounter(&c2);

    /* the states of the other visited maps go with this savegame */
    dir_niveles(de, NULL);
    dir_niveles(a, nombre);
    CreateDirectoryA(de, NULL);
    CreateDirectoryA(a, NULL);
    borrar_niveles(a);
    copiar_niveles(de, a);
    QueryPerformanceCounter(&c3);
    reg("save timings: engine %d ms, own block %d ms, map states %d ms",
        (int)((c1.QuadPart - c0.QuadPart) * 1000 / frec.QuadPart),
        (int)((c2.QuadPart - c1.QuadPart) * 1000 / frec.QuadPart),
        (int)((c3.QuadPart - c2.QuadPart) * 1000 / frec.QuadPart));
}

/* ================================================================== */
/*  reading                                                           */
/* ================================================================== */

typedef struct {
    unsigned char *d;
    unsigned int   n, p;
    int            error;
} lector_t;

static unsigned int leer_u32(lector_t *l)
{
    unsigned int v;
    if (l->error || l->p + 4 > l->n) { l->error = 1; return 0; }
    v = *(unsigned int *)(l->d + l->p);
    l->p += 4;
    return v;
}

static char *leer_cadena(lector_t *l)
{
    char *s = (char *)(l->d + l->p);
    unsigned int i = l->p;
    if (l->error) return "";
    while (i < l->n && l->d[i]) i++;
    if (i >= l->n) { l->error = 1; return ""; }
    l->p = i + 1;
    return s;
}

static void leer_bytes(lector_t *l, void *destino, unsigned int n)
{
    if (l->error || l->p + n > l->n) { l->error = 1; return; }
    if (destino) memcpy(destino, l->d + l->p, n);
    l->p += n;
}

/* looks for the CLDX marker from the end of the file */
static int situar_bloque(lector_t *l)
{
    unsigned int i;
    if (l->n < 12) return 0;
    for (i = l->n - 12; i > 0; i--) {
        if (*(unsigned int *)(l->d + i) == MARCA_EXTRA) { l->p = i; return 1; }
    }
    return (*(unsigned int *)l->d == MARCA_EXTRA) ? (l->p = 0, 1) : 0;
}

/* ================================================================== */
/*  loading                                                           */
/* ================================================================== */

static unsigned char *fichero = NULL;
static unsigned int   fichero_n = 0;
static char           mapa_pendiente[128];
static int            carga_pendiente = 0;

static int cargar_fichero(const char *ruta)
{
    HANDLE h;
    DWORD leidos, tam;

    if (fichero) { HeapFree(GetProcessHeap(), 0, fichero); fichero = NULL; }

    h = CreateFileA(ruta, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    tam = GetFileSize(h, NULL);
    if (tam == INVALID_FILE_SIZE || tam < 16) { CloseHandle(h); return 0; }

    fichero = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, tam);
    if (!fichero) { CloseHandle(h); return 0; }
    if (!ReadFile(h, fichero, tam, &leidos, NULL) || leidos != tam) {
        CloseHandle(h);
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return 0;
    }
    CloseHandle(h);
    fichero_n = tam;
    return 1;
}

typedef struct {
    unsigned int  nedicts, tamvars, base_dll_guardada;
    unsigned char tiempo[8];        /* sv.time */
} cabecera_t;

/*
 * Positions the reader right after the header of our own block.
 * Returns 0 if the block is missing or does not add up.
 */
static int abrir_bloque(lector_t *l, cabecera_t *c)
{
    l->d = fichero; l->n = fichero_n; l->p = 0; l->error = 0;
    if (!situar_bloque(l)) return 0;
    if (leer_u32(l) != MARCA_EXTRA) return 0;
    if (leer_u32(l) != VERSION_EXTRA) return 0;
    leer_cadena(l);                          /* map */
    leer_bytes(l, c->tiempo, 8);
    c->nedicts           = leer_u32(l);
    c->tamvars           = leer_u32(l);
    c->base_dll_guardada = leer_u32(l);
    return !l->error;
}

/*
 * Walks the whole block without touching anything, just to check that it
 * adds up. Important: without this, a corrupt file would leave the game
 * half destroyed, because restoring frees the state before replacing it.
 */
static int validar_bloque(void)
{
    lector_t l;
    cabecera_t c;
    unsigned int i;

    if (!fichero) return 0;
    if (!abrir_bloque(&l, &c)) return 0;
    if (c.nedicts == 0 || c.nedicts > MAX_EDICTS_PROPIO) return 0;
    if (c.tamvars != (unsigned int)tam_entvars()) return 0;

    for (i = 0; i < c.nedicts && !l.error; i++) {
        unsigned int ncad, j, tampriv, nr;
        if (leer_u32(&l)) continue;             /* free */
        leer_bytes(&l, NULL, c.tamvars);
        nr = leer_u32(&l);                      /* entvars pointers */
        if (nr > MAX_RELOCS) return 0;
        leer_bytes(&l, NULL, nr * (unsigned int)sizeof(reloc_t));
        ncad = leer_u32(&l);
        if (ncad > 4096) return 0;
        for (j = 0; j < ncad && !l.error; j++) { leer_u32(&l); leer_cadena(&l); }
        tampriv = leer_u32(&l);
        if (tampriv > 0x10000) return 0;
        if (tampriv) leer_bytes(&l, NULL, tampriv);
        nr = leer_u32(&l);                      /* block pointers */
        if (nr > MAX_RELOCS) return 0;
        leer_bytes(&l, NULL, nr * (unsigned int)sizeof(reloc_t));
    }
    return !l.error;
}

/* ================================================================== */
/*  live state of the freshly started map                             */
/* ================================================================== */
/*
 * There are engine pointers we cannot translate (class VIVO). For those
 * the right value is not the one in the file -- which belongs to another
 * process -- but the one the freshly started map has, which points to
 * THIS session's structures. So a copy is taken before destroying
 * anything.
 */
static unsigned char *copia_viva[MAX_EDICTS_PROPIO];
static int            tam_copia_viva[MAX_EDICTS_PROPIO];

static void tomar_copia_viva(void)
{
    int i, n = NUM_EDICTS;

    for (i = 0; i < MAX_EDICTS_PROPIO; i++) {
        copia_viva[i] = NULL;
        tam_copia_viva[i] = 0;
    }
    if (n > MAX_EDICTS_PROPIO) n = MAX_EDICTS_PROPIO;

    for (i = 0; i < n; i++) {
        unsigned char *p = (unsigned char *)privado_de(edict_n(i));
        int t = tam_privado[i];
        if (!p || t <= 0) continue;
        copia_viva[i] = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)t);
        if (!copia_viva[i]) continue;
        memcpy(copia_viva[i], p, (size_t)t);
        tam_copia_viva[i] = t;
    }
}

/*
 * A trigger_changelevel turns itself off as soon as you step on it (solid
 * to SOLID_NOT and its touch function to null), before requesting the
 * change. If the change never happens -- the exit to c1a1c when the map
 * did not exist and was ignored --, the trigger stays off, is saved that
 * way and the exit never works again in that game no matter how often it
 * is loaded. A change that does happen loads another map, so in a save a
 * disabled trigger_changelevel is always a failed change: it is re-armed
 * with what the same edict had in the freshly started map.
 */
static void rearmar_salidas(void)
{
    ddef_t *dcls = (ddef_t *)ED_FindField("classname");
    ddef_t *dsol = (ddef_t *)ED_FindField("solid");
    int i;

    if (!dcls || !dsol) return;
    for (i = 1; i < NUM_EDICTS && i < MAX_EDICTS_PROPIO; i++) {
        void *e = edict_n(i);
        unsigned char *p = (unsigned char *)privado_de(e);
        unsigned char *viva = copia_viva[i];
        float *solid = (float *)(vars_de(e) + dsol->ofs * 4);
        const char *cls;
        unsigned int toque;

        if (edict_libre(e) || !p || !viva) continue;
        cls = cadena_motor(*(unsigned int *)(vars_de(e) + dcls->ofs * 4));
        if (!cls || lstrcmpA(cls, "trigger_changelevel") != 0) continue;
        if (*solid != 0.0f || *(unsigned int *)(p + PRIV_TOQUE) != 0) continue;
        if (tam_privado[i] <= PRIV_MAPA_DESTINO ||
            tam_copia_viva[i] != tam_privado[i]) continue;
        /* same class in both (the vtable is already relocated) */
        if (*(unsigned int *)p != *(unsigned int *)viva) continue;
        toque = *(unsigned int *)(viva + PRIV_TOQUE);
        if (!toque) continue;

        *(unsigned int *)(p + PRIV_TOQUE) = toque;
        *solid = SOLID_TRIGGER;
        ((void (__cdecl *)(void *, int))A_SV_LINKEDICT)(e, 0);
        reg("restore: exit to %.32s (edict %d) was disabled, re-armed",
            (char *)(p + PRIV_MAPA_DESTINO), i);
    }
}

/*
 * The trigger for the exit to c1a1c (*15 in c1a1a) is a 22-unit strip
 * (z -118..-96) at the top of the double door opening, which spans -166
 * to -38: crouching you pass under it without touching it. It is
 * stretched to the whole opening every time the map is entered (and
 * after loading, which restores the saved mins/maxs).
 */
static const struct { const char *mapa, *destino; float zmin, zmax; } salidas_estiradas[] = {
    { "c1a1a", "c1a1c", -170.0f, -34.0f },
};

static void estirar_salidas(void)
{
    ddef_t *dcls = (ddef_t *)ED_FindField("classname");
    ddef_t *dorg = (ddef_t *)ED_FindField("origin");
    ddef_t *dmin = (ddef_t *)ED_FindField("mins");
    ddef_t *dmax = (ddef_t *)ED_FindField("maxs");
    ddef_t *dtam = (ddef_t *)ED_FindField("size");
    int i, k;

    if (!SV_EDICTS || !dcls || !dorg || !dmin || !dmax) return;
    for (k = 0; k < (int)(sizeof(salidas_estiradas) / sizeof(salidas_estiradas[0])); k++) {
        if (lstrcmpiA(MAPNAME, salidas_estiradas[k].mapa) != 0) continue;
        for (i = 1; i < NUM_EDICTS && i < MAX_EDICTS_PROPIO; i++) {
            void *e = edict_n(i);
            unsigned char *p = (unsigned char *)privado_de(e);
            float *org, *mins, *maxs;
            const char *cls;

            if (edict_libre(e) || !p || tam_privado[i] <= PRIV_MAPA_DESTINO) continue;
            cls = cadena_motor(*(unsigned int *)(vars_de(e) + dcls->ofs * 4));
            if (!cls || lstrcmpA(cls, "trigger_changelevel") != 0) continue;
            if (lstrcmpiA((char *)(p + PRIV_MAPA_DESTINO), salidas_estiradas[k].destino) != 0)
                continue;

            org  = (float *)(vars_de(e) + dorg->ofs * 4);
            mins = (float *)(vars_de(e) + dmin->ofs * 4);
            maxs = (float *)(vars_de(e) + dmax->ofs * 4);
            mins[2] = salidas_estiradas[k].zmin - org[2];
            maxs[2] = salidas_estiradas[k].zmax - org[2];
            if (dtam) ((float *)(vars_de(e) + dtam->ofs * 4))[2] = maxs[2] - mins[2];
            ((void (__cdecl *)(void *, int))A_SV_LINKEDICT)(e, 0);
            reg("exit to %s (edict %d) stretched to z %d..%d", salidas_estiradas[k].destino,
                i, (int)salidas_estiradas[k].zmin, (int)salidas_estiradas[k].zmax);
        }
    }
}

/*
 * hl.dll marks with EF_BRIGHTFIELD (a particle field) any monster that
 * starts stuck in a wall: a debugging mark for the mapper. parchear.ps1
 * removes it from hl.dll, but older saves carry it in the entvars and it
 * would come back on load. Nothing else in the alpha uses that effect, so
 * it is removed from every entity.
 */
static void quitar_marcas_atasco(void)
{
    ddef_t *def = (ddef_t *)ED_FindField("effects");
    int i;

    if (!def) return;
    for (i = 1; i < NUM_EDICTS; i++) {
        void *e = edict_n(i);
        float *ef = (float *)(vars_de(e) + def->ofs * 4);
        int v;

        if (edict_libre(e)) continue;
        v = (int)*ef;
        if (!(v & EF_BRIGHTFIELD)) continue;
        *ef = (float)(v & ~EF_BRIGHTFIELD);
        reg("restore: removed the particle field (stuck monster) from edict %d", i);
    }
}

/*
 * The alpha's progs.dat declares as "entity" fields that hl.dll uses as
 * integers (sequence, weapon, weapons, ammo_*, items, button...). When
 * saving, the engine converts each entity field into an edict number
 * (NUM_FOR_EDICT), and "weapon" carries the weapon-switch state in its
 * high bytes: after switching weapons the conversion goes out of range
 * and the save aborts with "NUM_FOR_EDICT: Bad pointer". The type of
 * "weapon" is changed to float in memory: the .sav text gets a harmless
 * number and loading restores the exact value from the CLDX block. The
 * progs are loaded with every map, so this is done on every "begin".
 */
#define TIPO_FLOAT   2
#define TIPO_ENTIDAD 4
static void arreglar_tipo_weapon(void)
{
    ddef_t *d = (ddef_t *)ED_FindField("weapon");
    static int avisado;

    if (!d || (d->tipo & 0x7fff) != TIPO_ENTIDAD) return;
    d->tipo = (unsigned short)((d->tipo & 0x8000) | TIPO_FLOAT);
    if (!avisado) {
        reg("progs: weapon field changed from entity to float (saving crashed after switching weapons)");
        avisado = 1;
    }
}

static void soltar_copia_viva(void)
{
    int i;
    for (i = 0; i < MAX_EDICTS_PROPIO; i++) {
        if (copia_viva[i]) HeapFree(GetProcessHeap(), 0, copia_viva[i]);
        copia_viva[i] = NULL;
        tam_copia_viva[i] = 0;
    }
}

/*
 * Checksum of a block ignoring the pointers. Used to compare the block in
 * the file with the one left in memory: everything that is not a pointer
 * must come out the same, and the pointers cannot come out the same
 * because they belong to another process. Without zeroing them the
 * comparison would say nothing.
 */
static unsigned int suma_neutra(const unsigned char *b, unsigned int tam,
                                reloc_t *r, unsigned int n)
{
    unsigned int suma = 0, o, k;

    for (o = 0; o < tam; o++) {
        unsigned char c = b[o];
        for (k = 0; k < n; k++)
            if (o >= r[k].desplazamiento && o < r[k].desplazamiento + 4) {
                c = 0;
                break;
            }
        suma = suma * 31u + c;
    }
    return suma;
}

/*
 * Applies the relocations that can be rebuilt by calculation. Those of
 * class VIVO do not go through here: they are resolved in the first pass,
 * which is when the state of the freshly started map still exists.
 */
static void aplicar_relocs(unsigned char *destino, unsigned int tam,
                           reloc_t *r, unsigned int n,
                           unsigned char **privs, unsigned int nedicts,
                           unsigned int *por_clase, unsigned int *fuera)
{
    unsigned int k;

    for (k = 0; k < n; k++) {
        unsigned int valor;

        if (r[k].desplazamiento + 4 > tam) { (*fuera)++; continue; }

        switch (r[k].clase) {
        case RELOC_EDICT:
            if (r[k].destino >= nedicts) { (*fuera)++; continue; }
            valor = (unsigned int)edict_n((int)r[k].destino) + r[k].dentro;
            break;
        case RELOC_PRIVADO:
            if (r[k].destino >= nedicts || !privs[r[k].destino]) { (*fuera)++; continue; }
            valor = (unsigned int)privs[r[k].destino] + r[k].dentro;
            break;
        case RELOC_DLL:
            /* the vtable and method pointers, rebuilt on the base hl.dll
               has NOW, not the one it had when the game was saved */
            if (r[k].dentro >= tam_dll) { (*fuera)++; continue; }
            valor = base_dll + r[k].dentro;
            break;
        default:
            continue;                       /* VIVO: already resolved */
        }

        *(unsigned int *)(destino + r[k].desplazamiento) = valor;
        por_clase[r[k].clase]++;
    }
}

/* second phase: the map is already loaded, now the state is restored */
/*
 * An edict in use in the freshly started map that was free in the saved
 * state (e.g. a monster that was killed and removed by the game): it has
 * to be left as ED_Free leaves it, unlinked from the world and with no
 * model or solid. If it was only marked free it stayed linked in the area
 * tree as a ghost entity. Its private block was already freed earlier.
 */
#define A_SV_UNLINKEDICT 0x429FE9   /* SV_UnlinkEdict(edict); SV_LinkEdict calls it first */
static void liberar_como_ed_free(void *e)
{
    static const char *campos[] = { "modelindex", "model", "solid", "takedamage",
                                    "nextthink", "effects", "movetype" };
    int k;
    ((void (__cdecl *)(void *))A_SV_UNLINKEDICT)(e);
    for (k = 0; k < (int)(sizeof(campos) / sizeof(campos[0])); k++) {
        ddef_t *d = (ddef_t *)ED_FindField(campos[k]);
        if (d) *(unsigned int *)(vars_de(e) + d->ofs * 4) = 0;
    }
}

/* reads and discards an in-use entry of the block (after its free flag) */
static void saltar_entrada(lector_t *l, unsigned int tamvars)
{
    unsigned int nr, ncad, j, tampriv;

    leer_bytes(l, NULL, tamvars);
    nr = leer_u32(l);
    if (nr > MAX_RELOCS) { l->error = 1; return; }
    leer_bytes(l, NULL, nr * (unsigned int)sizeof(reloc_t));
    ncad = leer_u32(l);
    for (j = 0; j < ncad && !l->error; j++) { leer_u32(l); leer_cadena(l); }
    tampriv = leer_u32(l);
    if (tampriv) leer_bytes(l, NULL, tampriv);
    nr = leer_u32(l);
    if (nr > MAX_RELOCS) { l->error = 1; return; }
    leer_bytes(l, NULL, nr * (unsigned int)sizeof(reloc_t));
}

/*
 * When a map is restored through a transition the server clock switches
 * to the one that map had, but the player brings absolute times from the
 * map they come from (air_finished, pain_finished...). They are shifted
 * by the same amount as the clock; otherwise, underwater, for example,
 * the player would start drowning immediately.
 */
static void desplazar_tiempos_jugador(double delta)
{
    static const char *campos[] = { "nextthink", "air_finished", "pain_finished",
                                    "radsuit_finished", "teleport_time", "dmgtime" };
    int k;
    if (!SV_EDICTS || NUM_EDICTS < 2 || edict_libre(edict_n(1))) return;
    for (k = 0; k < (int)(sizeof(campos) / sizeof(campos[0])); k++) {
        ddef_t *d = (ddef_t *)ED_FindField(campos[k]);
        float *v;
        if (!d) continue;
        v = (float *)(vars_de(edict_n(1)) + d->ofs * 4);
        if (*v > 0.0f) *v += (float)delta;
    }
}

static void __cdecl mi_restaurar(void)
{
    lector_t l;
    cabecera_t cab;
    unsigned int nedicts, tamvars, i, descuadre = 0;
    unsigned int por_clase[5], sin_copia = 0, fuera = 0;
    unsigned int comparados = 0, distintos = 0;
    unsigned int globales = 0, vivo_de_mundo = 0;
    unsigned char **privs;
    static reloc_t relocs[MAX_RELOCS];
    static unsigned char vars[4096];

    if (!fichero) { reg("restore: no file in memory"); return; }

    /*
     * Without hl.dll's base the vtables cannot be rebuilt, and restoring
     * halfway is worse than not restoring: the engine would crash on the
     * first virtual call.
     */
    localizar_modulos();
    if (!tam_dll) {
        Con_Printf("hlsave: hl.dll not found in memory, the game is left untouched\n");
        reg("restore: hl.dll not located, aborted without touching anything");
        return;
    }

    /* check BEFORE destroying anything */
    if (SVS_MAXCLIENTS != 1) {
        /* in multiplayer edicts 1..maxclients belong to the players and
           everything else is shifted relative to the save */
        Con_Printf("hlsave: the map was started for %d players, not loading\n",
                   SVS_MAXCLIENTS);
        reg("restore: maxclients=%d, aborted without touching anything", SVS_MAXCLIENTS);
        return;
    }
    if (!validar_bloque()) {
        Con_Printf("hlsave: the save does not add up, the game is left untouched\n");
        reg("restore: validation failed, aborted without touching anything");
        return;
    }

    if (!abrir_bloque(&l, &cab)) return;
    nedicts = cab.nedicts;
    tamvars = cab.tamvars;
    if (tamvars > sizeof(vars)) {
        reg("restore: entvars of %u bytes do not fit in the buffer", tamvars);
        return;
    }
    {
        double antes = SV_TIME;
        memcpy((void *)A_SV_TIME, cab.tiempo, 8);
        if (conservar_jugador) desplazar_tiempos_jugador(SV_TIME - antes);
    }

    reg("restore: hl.dll when saved=0x%08x, now=0x%08x%s",
        cab.base_dll_guardada, base_dll,
        cab.base_dll_guardada == base_dll ? "" : "  (MOVED)");

    privs = (unsigned char **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof(void *) * nedicts);
    if (!privs) return;
    for (i = 0; i < 5; i++) por_clase[i] = 0;

    /* the snapshot of the live state, before touching it */
    tomar_copia_viva();

    /*
     * The only VIVO pointers that show up are pev->pSystemGlobals
     * (entvars +0x20c) and its copy in the private block (+0x8): the same
     * value in every entity. If the edict of the freshly started map has
     * nowhere to copy it from (it was free: when returning to a map there
     * are entities created while playing, gibs...), it stays null and
     * hl.dll crashes as soon as the entity thinks (0x10018d7d, reads
     * [globals+0x7c]). The world's one (edict 0), which always exists, is
     * used instead.
     */
    globales = *(unsigned int *)(vars_de(edict_n(0)) + 0x20c);

    /* clean up what the map startup left */
    for (i = 0; i < (unsigned int)NUM_EDICTS && i < MAX_EDICTS_PROPIO; i++) {
        if (conservar_jugador && i == 1) continue;       /* the player stays */
        mi_free_privado(edict_n((int)i));
    }

    /* --- first pass: entvars, strings and private blocks --- */
    for (i = 0; i < nedicts; i++) {
        void *e = edict_n((int)i);
        unsigned int libre, ncad, j, tampriv, nr, k;

        libre = leer_u32(&l);
        if (l.error) break;

        /* transition: the player is the one who just arrived, not the
           saved one; whatever points to it gets the live player's private
           block */
        if (conservar_jugador && i == 1) {
            if (!libre) saltar_entrada(&l, tamvars);
            privs[i] = (unsigned char *)privado_de(e);
            continue;
        }

        /*
         * NOTE: the edict header (bytes 0x04..0x77) is not touched. That is
         * where the area links live, a doubly linked list between edicts.
         * If they are zeroed without unlinking first, the neighbours are
         * left pointing to a dead node and the engine crashes when walking
         * the list. Only the entvars are restored.
         */
        if (libre) {
            if (!edict_libre(e)) { descuadre++; liberar_como_ed_free(e); }
            *(int *)((char *)e + ED_FREE_OFS) = 1;
            continue;
        }

        if (edict_libre(e)) descuadre++;
        *(int *)((char *)e + ED_FREE_OFS) = 0;

        /*
         * The entvars are read into a separate buffer so that the VIVO
         * pointers can be rescued from the edict that is still standing,
         * before overwriting it.
         */
        leer_bytes(&l, vars, tamvars);
        nr = leer_u32(&l);
        if (nr > MAX_RELOCS) { l.error = 1; break; }
        leer_bytes(&l, relocs, nr * (unsigned int)sizeof(reloc_t));
        for (k = 0; k < nr; k++) {
            unsigned int des = relocs[k].desplazamiento;
            if (relocs[k].clase != RELOC_VIVO || des + 4 > tamvars) continue;
            *(unsigned int *)(vars + des) = *(unsigned int *)(vars_de(e) + des);
            if (!*(unsigned int *)(vars + des) && des == 0x20c && globales) {
                *(unsigned int *)(vars + des) = globales;
                vivo_de_mundo++;
            }
            por_clase[RELOC_VIVO]++;
        }
        memcpy(vars_de(e), vars, tamvars);

        ncad = leer_u32(&l);
        for (j = 0; j < ncad && !l.error; j++) {
            unsigned int idx = leer_u32(&l);
            char *texto = leer_cadena(&l);
            ddef_t *d = fielddef_n((int)idx);
            if (d) ED_ParseEpair(vars_de(e), d, texto);
        }

        tampriv = leer_u32(&l);
        if (tampriv > 0 && tampriv < 0x10000) {
            void *p = alloc_original(e, (int)tampriv);
            if (p) {
                leer_bytes(&l, p, tampriv);
                tam_privado[i] = (int)tampriv;
                privs[i] = (unsigned char *)p;
            } else {
                leer_bytes(&l, NULL, tampriv);
            }
        } else if (tampriv) {
            reg("restore: absurd private size (%u) in edict %u", tampriv, i);
            l.error = 1;
            break;
        }

        /* the block's pointers: now only the VIVO ones, which need the
           copy. The rest in the second pass, once all the blocks
           exist. */
        nr = leer_u32(&l);
        if (nr > MAX_RELOCS) { l.error = 1; break; }
        leer_bytes(&l, relocs, nr * (unsigned int)sizeof(reloc_t));
        if (privs[i]) {
            for (k = 0; k < nr; k++) {
                unsigned int des = relocs[k].desplazamiento;
                if (relocs[k].clase != RELOC_VIVO) continue;
                if (des + 4 > (unsigned int)tam_privado[i]) { fuera++; continue; }
                if (copia_viva[i] && des + 4 <= (unsigned int)tam_copia_viva[i] &&
                    *(unsigned int *)(copia_viva[i] + des)) {
                    *(unsigned int *)(privs[i] + des) =
                        *(unsigned int *)(copia_viva[i] + des);
                    por_clase[RELOC_VIVO]++;
                } else if (des == 8 && globales) {
                    /* the private block's gpGlobals: the world's one */
                    *(unsigned int *)(privs[i] + des) = globales;
                    vivo_de_mundo++;
                } else {
                    /* with no copy to fall back on, better null than dead:
                       the DLL checks for nulls, nobody checks addresses
                       from another process */
                    *(unsigned int *)(privs[i] + des) = 0;
                    sin_copia++;
                }
            }
        }
    }

    if (l.error) {
        reg("restore: corrupt file in the first pass (edict %u)", i);
        soltar_copia_viva();
        HeapFree(GetProcessHeap(), 0, privs);
        return;
    }

    NUM_EDICTS = (int)nedicts;

    /* --- second pass: the pointers that are rebuilt --- */
    {
        cabecera_t c2;
        abrir_bloque(&l, &c2);
    }

    for (i = 0; i < nedicts && !l.error; i++) {
        void *e = edict_n((int)i);
        unsigned int libre, ncad, j, tampriv, nr;
        const unsigned char *bloque;

        libre = leer_u32(&l);
        if (libre) continue;
        if (conservar_jugador && i == 1) { saltar_entrada(&l, tamvars); continue; }

        leer_bytes(&l, NULL, tamvars);
        nr = leer_u32(&l);
        if (nr > MAX_RELOCS) { l.error = 1; break; }
        leer_bytes(&l, relocs, nr * (unsigned int)sizeof(reloc_t));
        aplicar_relocs((unsigned char *)vars_de(e), tamvars, relocs, nr,
                       privs, nedicts, por_clase, &fuera);

        ncad = leer_u32(&l);
        for (j = 0; j < ncad && !l.error; j++) { leer_u32(&l); leer_cadena(&l); }

        tampriv = leer_u32(&l);
        bloque = l.d + l.p;                 /* the block exactly as it is in the .sav */
        if (tampriv) leer_bytes(&l, NULL, tampriv);
        nr = leer_u32(&l);
        if (nr > MAX_RELOCS) { l.error = 1; break; }
        leer_bytes(&l, relocs, nr * (unsigned int)sizeof(reloc_t));
        if (privs[i]) {
            aplicar_relocs(privs[i], (unsigned int)tam_privado[i], relocs, nr,
                           privs, nedicts, por_clase, &fuera);

            if (tampriv == (unsigned int)tam_privado[i]) {
                comparados++;
                if (suma_neutra(bloque, tampriv, relocs, nr) !=
                    suma_neutra(privs[i], tampriv, relocs, nr))
                    distintos++;
            }
        }
    }

    reg("restore: pointers rebuilt: edict=%u private=%u hl.dll=%u live=%u"
        "  (no copy=%u discarded=%u, world globals=%u)",
        por_clase[RELOC_EDICT], por_clase[RELOC_PRIVADO],
        por_clase[RELOC_DLL], por_clase[RELOC_VIVO], sin_copia, fuera, vivo_de_mundo);
    reg("restore: private block contents: %u compared, %u different",
        comparados, distintos);

    /*
     * Link everything restored back into the world, at its saved position
     * (like Quake's loadgame): the area link that is kept is the one from
     * the freshly started map, and an entity that had moved (or that was
     * free at startup) stayed linked where it is not.
     */
    for (i = 1; i < nedicts; i++) {
        void *e = edict_n((int)i);
        if (edict_libre(e)) continue;
        ((void (__cdecl *)(void *, int))A_SV_LINKEDICT)(e, 0);
    }

    rearmar_salidas();
    quitar_marcas_atasco();
    soltar_copia_viva();
    HeapFree(GetProcessHeap(), 0, privs);
    HeapFree(GetProcessHeap(), 0, fichero);
    fichero = NULL;

    if (conservar_jugador) {
        Con_Printf("hlsave: map state restored (%d entities)\n", nedicts);
        registrar_jugador("AFTER RETURNING TO THE MAP");
    } else {
        Con_Printf("hlsave: game restored (%d entities)\n", nedicts);
        forzar_vista_jugador();
        registrar_jugador("AFTER LOADING");
    }
    reg("restore: done, %u edicts, %u in-use/free mismatches", nedicts, descuadre);
}

static void __cdecl mi_load(void)
{
    char ruta[512], orden[256];
    const char *nombre;
    lector_t l;
    char *mapa;
    if (Cmd_Argc() == 2 && lstrcmpiA(Cmd_Argv(1), "quick") == 0) {
        /* stock F6/F7: the quick save goes to menu slot 12 (s11) */
        Cbuf_InsertText("load s11\n");
        return;
    }

    if (Cmd_Argc() != 2) {
        Con_Printf("load <name> : load a saved game\n");
        return;
    }
    nombre = Cmd_Argv(1);
    wsprintfA(ruta, "%s/%s.sav", GAMEDIR, nombre);

    if (!cargar_fichero(ruta)) {
        Con_Printf("hlsave: could not read %s\n", ruta);
        return;
    }

    l.d = fichero; l.n = fichero_n; l.p = 0; l.error = 0;
    if (!situar_bloque(&l)) {
        Con_Printf("hlsave: %s has no full state (saved with the original engine)\n",
                   nombre);
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return;
    }
    leer_u32(&l);
    if (leer_u32(&l) != VERSION_EXTRA) {
        Con_Printf("hlsave: that .sav is from an older format (the pointers\n");
        Con_Printf("        were stored raw and cannot be relocated); it must be redone\n");
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return;
    }
    mapa = leer_cadena(&l);
    if (l.error || !*mapa) {
        Con_Printf("hlsave: could not read the map name\n");
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return;
    }
    lstrcpynA(mapa_pendiente, mapa, sizeof(mapa_pendiente));

    {   /* the other maps, as they were when this game was saved */
        char de[512], a[512];
        dir_niveles(a, NULL);
        dir_niveles(de, nombre);
        CreateDirectoryA(a, NULL);
        borrar_niveles(a);
        copiar_niveles(de, a);
    }
    nivel_pendiente = 0;

    Con_Printf("hlsave: loading %s (map %s)...\n", nombre, mapa_pendiente);
    reg("load: file=%s map=%s size=%u", ruta, mapa_pendiente, fichero_n);

    /*
     * A command of our own cannot be registered: Cmd_AddCommand aborts the
     * game if called after initialisation ("Cmd_AddCommand after
     * host_initialized"). So "map" itself is hooked: the load is flagged
     * as pending and mi_map restores as soon as the map has finished
     * starting.
     */
    carga_pendiente = 1;
    /*
     * "disconnect" first: with a multiplayer game running, "maxplayers"
     * refuses ("can not be changed while a server is running"), the map
     * started for several players with a different edict numbering and
     * the save was dumped misaligned (it crashed in the collision code).
     * disconnect (0x43e35e) shuts the server down.
     */
    wsprintfA(orden, "disconnect\nmaxplayers 1\nmap %s\n", mapa_pendiente);
    Cbuf_InsertText(orden);
}

/* The hooked "map" command: starts the map and, if we come from a
   "load", restores the state as soon as the map is up. */
static int mapa_existe(const char *mapa);
static void ampliar_bufers_servidor(void);
static void entidades_sin_pvs(int activar);

static void __cdecl mi_map(void)
{
    /*
     * "New Game" in the alpha's menu sends "map start", but the alpha does
     * not ship start.bsp: a game could not be started from the menu. It
     * goes to the first map that it does ship.
     */
    if (Cmd_Argc() >= 2 && lstrcmpiA(Cmd_Argv(1), "start") == 0 && !mapa_existe("start")) {
        reg("map start: the alpha does not ship start.bsp, starting at c1a1");
        Cbuf_InsertText("map c1a1\n");
        return;
    }

    if (!carga_pendiente) {
        char dir[512];
        dir_niveles(dir, NULL);
        borrar_niveles(dir);                 /* new game: new world */
        nivel_pendiente = 0;
    }

    ((fn_void_t)A_HOST_MAP)();
    ampliar_bufers_servidor();
    entidades_sin_pvs(SVS_MAXCLIENTS == 1);

    if (carga_pendiente)
        registrar_jugador("MAP JUST STARTED");
}

/*
 * The "begin" command closes the client handshake: from here on the
 * player really exists, with its private block allocated. Restoring
 * before this worked on a warm start by pure luck (the client was already
 * connected), but on a cold start the player was not there yet and the
 * engine crashed when creating it on top of the restored state.
 */
static void __cdecl mi_begin(void)
{
    ((fn_void_t)A_HOST_BEGIN)();
    ampliar_bufers_servidor();
    entidades_sin_pvs(SVS_MAXCLIENTS == 1);

    arreglar_tipo_weapon();
    if (carga_pendiente) {
        carga_pendiente = 0;
        registrar_jugador("CLIENT ALREADY IN");
        mi_restaurar();
    } else {
        /* normal entry (new game or level change): log the velocity the
           player arrives with (NOTES section 27) */
        ddef_t *dvel = (ddef_t *)ED_FindField("velocity");
        ddef_t *dorg = (ddef_t *)ED_FindField("origin");
        if (SV_EDICTS && NUM_EDICTS > 1 && dvel && dorg && !edict_libre(edict_n(1))) {
            float *v = (float *)(vars_de(edict_n(1)) + dvel->ofs * 4);
            float *o = (float *)(vars_de(edict_n(1)) + dorg->ofs * 4);
            ddef_t *dva = (ddef_t *)ED_FindField("v_angle");
            ddef_t *dan = (ddef_t *)ED_FindField("angles");
            float *va = dva ? (float *)(vars_de(edict_n(1)) + dva->ofs * 4) : v;
            float *an = dan ? (float *)(vars_de(edict_n(1)) + dan->ofs * 4) : v;
            reg("entering %s: org=(%d %d %d) velocity=(%d %d %d) v_angle=(%d %d) angles=(%d %d)",
                MAPNAME, (int)o[0], (int)o[1], (int)o[2], (int)v[0], (int)v[1], (int)v[2],
                (int)va[0], (int)va[1], (int)an[0], (int)an[1]);
        }
        if (nivel_pendiente) {
            char dir[512], ruta[600];
            nivel_pendiente = 0;
            dir_niveles(dir, NULL);
            wsprintfA(ruta, "%s/%s.niv", dir, MAPNAME);
            if (SVS_MAXCLIENTS == 1 && GetFileAttributesA(ruta) != INVALID_FILE_ATTRIBUTES) {
                if (cargar_fichero(ruta)) {
                    reg("levels: %s had already been visited, restoring its state", MAPNAME);
                    conservar_jugador = 1;
                    mi_restaurar();
                    conservar_jugador = 0;
                }
            } else {
                reg("levels: %s had not been visited, starting from scratch", MAPNAME);
            }
        }
    }
    estirar_salidas();

    {   /* automated tests: valve\zprueba_N.cfg on the Nth entry into a map */
        static int entradas;
        char f[512], orden[64];
        entradas++;
        wsprintfA(f, "%s/zprueba_%d.cfg", GAMEDIR, entradas);
        if (GetFileAttributesA(f) != INVALID_FILE_ATTRIBUTES) {
            wsprintfA(orden, "exec zprueba_%d.cfg\n", entradas);
            Cbuf_InsertText(orden);
            reg("test: exec zprueba_%d.cfg in %s", entradas, MAPNAME);
        }
        /* and valve\zprueba_pos.txt ("x y z pitch yaw"): place the player */
        wsprintfA(f, "%s/zprueba_pos.txt", GAMEDIR);
        {
            HANDLE hp = CreateFileA(f, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (hp != INVALID_HANDLE_VALUE) {
                char txt[128]; DWORD le = 0; float x, y, z, pit, yaw;
                ReadFile(hp, txt, sizeof(txt) - 1, &le, NULL); txt[le] = 0; CloseHandle(hp);
                if (sscanf(txt, "%f %f %f %f %f", &x, &y, &z, &pit, &yaw) == 5 &&
                    SV_EDICTS && NUM_EDICTS > 1 && !edict_libre(edict_n(1))) {
                    void *pj = edict_n(1);
                    ddef_t *dor = ED_FindField("origin"), *dve = ED_FindField("velocity");
                    ddef_t *dva = ED_FindField("v_angle"), *dan = ED_FindField("angles");
                    ddef_t *dfi = ED_FindField("fixangle");
                    float *o = (float *)(vars_de(pj) + dor->ofs * 4), *ve = (float *)(vars_de(pj) + dve->ofs * 4);
                    float *va = (float *)(vars_de(pj) + dva->ofs * 4), *an = (float *)(vars_de(pj) + dan->ofs * 4);
                    o[0] = x; o[1] = y; o[2] = z; ve[0] = ve[1] = ve[2] = 0;
                    va[0] = an[0] = pit; va[1] = an[1] = yaw; va[2] = an[2] = 0;
                    *(float *)(vars_de(pj) + dfi->ofs * 4) = 1.0f;
                    ((void (__cdecl *)(void *, int))A_SV_LINKEDICT)(pj, 0);
                    reg("test: player placed at (%d %d %d) looking at (%d %d)",
                        (int)x, (int)y, (int)z, (int)pit, (int)yaw);
                }
            }
        }
    }
}

/* ================================================================== */
/*  level changes to maps that do not exist                           */
/* ================================================================== */
/*
 * The alpha has transitions to maps it never included: c1a1a -> c1a1c and
 * c3a2 -> c3a3. When you touch them, pfnChangeLevel freezes the screen
 * for the loading plaque and the engine cannot find the .bsp: either a
 * fatal error comes up in a MessageBox hidden behind the fullscreen game,
 * or the image stays frozen for 60 s until "load failed.". It looks hung.
 * If the map is missing, the change is ignored and both flags are undone;
 * otherwise the map's other exits would stop working until another level
 * is loaded (changelevel_issued is only cleared by a map startup).
 */
static int mapa_existe(const char *mapa)
{
    char ruta[512];
    wsprintfA(ruta, "%s/maps/%s.bsp", GAMEDIR, mapa);
    return GetFileAttributesA(ruta) != INVALID_FILE_ATTRIBUTES;
}

static int cambio_a_mapa_ausente(void)
{
    static char ultimo[64];
    static DWORD cuando;
    const char *mapa;

    if (Cmd_Argc() < 2) return 0;
    mapa = Cmd_Argv(1);
    if (!mapa || !*mapa || mapa_existe(mapa)) return 0;

    *(int *)A_CHANGELEVEL_ISSUED = 0;
    *(int *)A_SCR_DISABLED_LOAD  = 0;

    /* the trigger fires again every frame while you stand on it */
    if (lstrcmpiA(ultimo, mapa) != 0 || GetTickCount() - cuando > 5000) {
        Con_Printf("The alpha does not ship map %s: this exit leads nowhere\n",
                   mapa);
        reg("changelevel to %s ignored: the map does not exist", mapa);
        lstrcpynA(ultimo, mapa, sizeof(ultimo));
        cuando = GetTickCount();
    }
    return 1;
}

/*
 * Where possible, instead of ignoring the exit, it jumps to the next
 * chapter that the alpha does ship. It has to be done here, in the
 * function hl.dll calls when the trigger is touched (entry 5 of the
 * engine's table), and not by issuing another "changelevel": that command
 * typed by hand crashes in hl.dll (SetChangeParms, hl.dll+0xb92d) even
 * with the original game. By changing only the map name, everything else
 * follows the trigger's normal path and health and weapons are kept.
 */
typedef void (__cdecl *fn_changelevel_t)(char *mapa, char *landmark);
static fn_changelevel_t changelevel_original;

static const struct { const char *falta, *siguiente; } capitulos[] = {
    { "c1a1c", "c1a2a" },   /* end of chapter 1: jump to 2 */
};

static void __cdecl mi_pfn_changelevel(char *mapa, char *landmark)
{
    int i;
    if (mapa && !mapa_existe(mapa) && !*(int *)A_CHANGELEVEL_ISSUED) {
        for (i = 0; i < (int)(sizeof(capitulos) / sizeof(capitulos[0])); i++) {
            if (lstrcmpiA(mapa, capitulos[i].falta) == 0 &&
                mapa_existe(capitulos[i].siguiente)) {
                Con_Printf("The alpha does not ship map %s: moving on to the next chapter (%s)\n",
                           mapa, capitulos[i].siguiente);
                reg("changelevel to %s redirected to %s (landmark \"%s\")", mapa,
                    capitulos[i].siguiente, landmark ? landmark : "");
                mapa = (char *)capitulos[i].siguiente;
                break;
            }
        }
    }
    /* if there is no next chapter, the command will arrive with the
       missing map and mi_changelevel will ignore it */
    changelevel_original(mapa, landmark);
}

static void __cdecl mi_changelevel(void)
{
    if (cambio_a_mapa_ausente()) return;
    guardar_nivel_actual();              /* the map being left, as it is */
    nivel_pendiente = 1;
    ((fn_void_t)A_HOST_CHANGELEVEL)();
}

static void __cdecl mi_changelevel2(void)
{
    if (cambio_a_mapa_ausente()) return;
    guardar_nivel_actual();
    nivel_pendiente = 1;
    ((fn_void_t)A_HOST_CHANGELEVEL2)();
}

/* ================================================================== */
/*  message buffer full                                               */
/* ================================================================== */
/*
 * "SZ_GetSpace: overflow without allowoverflow set": with several
 * creatures at once (3-4 dogs and a headcrab, under fire) one of the
 * server's message buffers fills up and SZ_GetSpace calls the fatal
 * error. The buffers that DO allow overflowing (each client's) do
 * something else: they are emptied, flagged as overflowed and the game
 * goes on, losing that frame's data. The same is done for all of them:
 * the call to the error at 0x42f1fb goes to sz_desborde, which records
 * which buffer it was and returns; the original code carries on down the
 * allowoverflow path. The "single message larger than the whole buffer"
 * error is left as it was.
 */
/*
 * sv.datagram (MSG_BROADCAST) and sv.reliable_datagram (MSG_ALL) are
 * 1 KB. With a grenade among 3-4 dogs (blood, gibs, sounds) sv.datagram
 * fills up IN THE MIDDLE OF A MESSAGE: emptying it there (the code below)
 * left the rest of the message at the start and the client rejected it
 * ("CL_ParseServerMessage: Illegible server message"). They are given a
 * large buffer: when sending it (0x431e2c) the engine copies sv.datagram
 * into the client's message ONLY if it fits entirely and otherwise
 * discards it whole, which is clean (that frame's effects are lost). The
 * reliable one is copied into each client's reliable message, of 8000
 * bytes. SV_SpawnServer points them back to their fixed buffers on every
 * map (0x432662..).
 * sizebuf_t: +8 data, +0xc maxsize, +0x10 cursize.
 */
static unsigned char datagram_grande[16384];
static unsigned char reliable_grande[4096];

static void ampliar_un_bufer(unsigned int sb, unsigned char *nuevo, int tam, const char *nombre)
{
    unsigned char **data = (unsigned char **)(sb + 8);
    int *maxsize = (int *)(sb + 0xc), *cursize = (int *)(sb + 0x10);
    static int avisado;

    if (*data == nuevo) return;
    if (*cursize < 0 || *cursize > *maxsize || *cursize > tam) return;
    memcpy(nuevo, *data, (size_t)*cursize);
    *data = nuevo;
    *maxsize = tam;
    if (avisado < 2) { reg("%s enlarged to %d bytes", nombre, tam); avisado++; }
}

static void ampliar_bufers_servidor(void)
{
    ampliar_un_bufer(A_SV_DATAGRAM, datagram_grande, sizeof(datagram_grande), "sv.datagram");
    ampliar_un_bufer(A_SV_RELIABLE, reliable_grande, sizeof(reliable_grande), "sv.reliable_datagram");
}

void __cdecl registrar_desborde(unsigned int *buf)
{
    static unsigned int avisados[8];
    static int navisados;
    const char *nombre;
    int i;

    for (i = 0; i < navisados; i++)
        if (avisados[i] == (unsigned int)buf) return;   /* one per buffer */
    if (navisados < 8) avisados[navisados++] = (unsigned int)buf;

    if      ((unsigned int)buf == A_SV_DATAGRAM) nombre = "sv.datagram (MSG_BROADCAST)";
    else if ((unsigned int)buf == A_SV_RELIABLE) nombre = "sv.reliable_datagram (MSG_ALL)";
    else if ((unsigned int)buf == A_SV_SIGNON)   nombre = "sv.signon (MSG_INIT)";
    else                                         nombre = "other";
    reg("SZ_GetSpace: %s filled up at 0x%08x (maxsize=%d cursize=%d, map %s);"
        " emptying it and carrying on", nombre, (unsigned int)buf, buf[3], buf[4], MAPNAME);
}

/* ebx = the sizebuf_t (SZ_GetSpace keeps it there); the stack is the call's */
extern void sz_desborde(void);
__asm__(".globl _sz_desborde\n"
        "_sz_desborde:\n"
        "    pushl %ebx\n"
        "    call  _registrar_desborde\n"
        "    addl  $4, %esp\n"
        "    ret\n");

/* ================================================================== */
/*  menu savegame list                                                */
/* ================================================================== */
/*
 * M_ScanSaves reads s0..s11.sav as if they were Quake saves: a number and
 * a text comment. The alpha's ones start with the binary Half-Life header
 * ("VALV"...), so every slot showed up as "VALV" and there was no way to
 * tell them apart. After the original scan each label is rewritten with
 * the map and the date, and those without the current CLDX block (the
 * three stock ones, from 1997) are left as not loadable.
 * The last slot (s11) is the quick save one: F6/F7 save and load there
 * (see autoexec.cfg).
 */
#define NUM_RANURAS   12
#define RANURA_RAPIDA 11

static void etiqueta_ranura(int i)
{
    char ruta[512], texto[128];
    char *etq = (char *)(A_MENU_NOMBRES + i * 40);
    int *cargable = (int *)(A_MENU_CARGABLE + i * 4);
    const char *pre = (i == RANURA_RAPIDA) ? "QUICK " : "";
    HANDLE h;
    DWORD tam, leidos;
    FILETIME ft, local;
    SYSTEMTIME st;
    unsigned char *d;
    lector_t l;
    char *mapa = NULL;

    wsprintfA(ruta, "%s/s%d.sav", GAMEDIR, i);
    h = CreateFileA(ruta, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        if (i == RANURA_RAPIDA) lstrcpynA(etq, "--- QUICK: empty ---", 40);
        return;                      /* the original already left it as empty */
    }
    tam = GetFileSize(h, NULL);
    GetFileTime(h, NULL, NULL, &ft);
    d = (tam != INVALID_FILE_SIZE && tam >= 16)
        ? (unsigned char *)HeapAlloc(GetProcessHeap(), 0, tam) : NULL;
    if (d && (!ReadFile(h, d, tam, &leidos, NULL) || leidos != tam)) {
        HeapFree(GetProcessHeap(), 0, d);
        d = NULL;
    }
    CloseHandle(h);

    if (d) {
        l.d = d; l.n = tam; l.p = 0; l.error = 0;
        if (situar_bloque(&l) && leer_u32(&l) == MARCA_EXTRA &&
            leer_u32(&l) == VERSION_EXTRA) {
            mapa = leer_cadena(&l);
            if (l.error || !*mapa) mapa = NULL;
        }
    }

    if (mapa) {
        FileTimeToLocalFileTime(&ft, &local);
        FileTimeToSystemTime(&local, &st);
        wsprintfA(texto, "%s%s  %02d/%02d %02d:%02d", pre, mapa,
                  st.wDay, st.wMonth, st.wHour, st.wMinute);
        *cargable = 1;
    } else {
        wsprintfA(texto, "%s(old, cannot be loaded)", pre);
        *cargable = 0;
    }
    lstrcpynA(etq, texto, 40);
    if (d) HeapFree(GetProcessHeap(), 0, d);
}

static void __cdecl mi_scan(void)
{
    int i;
    ((fn_void_t)A_M_SCANSAVES)();
    for (i = 0; i < NUM_RANURAS; i++) etiqueta_ranura(i);
}

/* repoints an engine "jmp rel32", checking where it used to go */
static int repuntar_jmp(unsigned int sitio, unsigned int antes, void *ahora)
{
    unsigned char *p = (unsigned char *)sitio;
    if (p[0] != 0xE9 || *(unsigned int *)(p + 1) != antes - (sitio + 5))
        return 0;
    return escribir_dword(sitio + 1, (unsigned int)ahora - (sitio + 5));
}

/* ================================================================== */
/*  shadows                                                           */
/* ================================================================== */
/*
 * With r_shadows 1 the engine draws the flat shadow of every studio model
 * (R_StudioDrawModel 0x4163f3, at 0x4165e4). It looks fine on NPCs, but
 * the first-person weapon's shadow floats next to the weapon and the
 * grenade's flies along with it. At 0x4165fd, where the engine only skips
 * it for additive rendering (rendermode 5, entity +0x98), it is now also
 * skipped for cl.viewent (0x849cb8, the first-person weapon) and for the
 * grenade model (model_t name, entity +0xa8).
 */
#define A_SOMBRA_CMP    0x4165FD   /* cmp dword [esi+98h],5 / je 0x416706  */
#define A_SOMBRA_SI     0x41660A   /* draw the shadow                       */
#define A_SOMBRA_NO     0x416706   /* skip it                               */
#define A_CL_VIEWENT    0x849CB8

int __cdecl sombra_permitida(unsigned char *ent)
{
    const char *modelo;

    if (*(int *)(ent + 0x98) == 5) return 0;               /* as before */
    static int visto_arma, visto_granada;

    modelo = *(const char **)(ent + 0xa8);
    if ((unsigned int)ent == A_CL_VIEWENT) {               /* the weapon   */
        if (!visto_arma) { visto_arma = 1; reg("shadow: no shadow for the first-person weapon (%.40s)", modelo ? modelo : "?"); }
        return 0;
    }
    if (modelo && strstr(modelo, "grenade")) {             /* the grenade  */
        if (!visto_granada) { visto_granada = 1; reg("shadow: no shadow for %.40s", modelo); }
        return 0;
    }
    return 1;
}

extern void sombra_puente(void);
__asm__(".globl _sombra_puente\n"
        "_sombra_puente:\n"
        "    pushl %ecx\n"
        "    pushl %edx\n"
        "    pushl %esi\n"
        "    call _sombra_permitida\n"
        "    addl $4, %esp\n"
        "    popl %edx\n"
        "    popl %ecx\n"
        "    testl %eax, %eax\n"
        "    jz 1f\n"
        "    pushl $0x41660a\n"
        "    ret\n"
        "1:  pushl $0x416706\n"
        "    ret\n");

static void enganchar_sombras(void)
{
    static const unsigned char orig[13] =
        { 0x83,0xBE,0x98,0x00,0x00,0x00,0x05, 0x0F,0x84,0xFC,0x00,0x00,0x00 };
    unsigned char *p = (unsigned char *)A_SOMBRA_CMP;
    DWORD viejo;
    int i;

    if (memcmp(p, orig, sizeof(orig)) != 0) {
        reg("WARNING: 0x%08x is not the shadow check, leaving it alone", A_SOMBRA_CMP);
        return;
    }
    if (!VirtualProtect(p, sizeof(orig), PAGE_EXECUTE_READWRITE, &viejo)) return;
    p[0] = 0xE9;
    *(int *)(p + 1) = (int)((unsigned int)sombra_puente - (A_SOMBRA_CMP + 5));
    for (i = 5; i < (int)sizeof(orig); i++) p[i] = 0x90;
    VirtualProtect(p, sizeof(orig), viejo, &viejo);
}

/* ================================================================== */
/*  entities that do not arrive through the PVS                       */
/* ================================================================== */
/*
 * The precomputed visibility of the alpha's maps has holes (NOTES
 * section 24). r_novis fixes the drawing of the world, but the server
 * still does not send entities whose leaves are not in the PVS: a door
 * 300 u ahead does not appear until you get close. With ONE player they
 * are all sent: in SV_WriteEntitiesToClient (0x431394) the jump that
 * discards the entity if none of its leaves is visible (0x43148b) is
 * nulled, and SV_SendClientDatagram's per-frame message (0x431dbf), which
 * was a 1 KB stack buffer, becomes 4 KB (loopback allows 8 KB). In
 * multiplayer the original code is kept: over the network a datagram of
 * more than 1 KB is not valid. It is decided on every map, with the
 * server already started.
 */
static const struct { unsigned int va; unsigned char orig[7], nuevo[7]; int n; } pvs_parches[] = {
    { 0x43148B, {0x0F,0x84,0xFC,0x04,0x00,0x00},       {0x90,0x90,0x90,0x90,0x90,0x90},       6 },
    { 0x431DC2, {0x81,0xEC,0x18,0x04,0x00,0x00},       {0x81,0xEC,0x18,0x10,0x00,0x00},       6 },
    { 0x431DC9, {0x8D,0x85,0xE8,0xFB,0xFF,0xFF},       {0x8D,0x85,0xE8,0xEF,0xFF,0xFF},       6 },
    { 0x431DD8, {0xC7,0x45,0xF8,0x00,0x04,0x00,0x00},  {0xC7,0x45,0xF8,0x00,0x10,0x00,0x00},  7 },
};

static void entidades_sin_pvs(int activar)
{
    static int estado = -1;
    int i;
    DWORD viejo;

    if (estado == activar) return;
    for (i = 0; i < (int)(sizeof(pvs_parches) / sizeof(pvs_parches[0])); i++) {
        unsigned char *p = (unsigned char *)pvs_parches[i].va;
        if (memcmp(p, pvs_parches[i].orig, pvs_parches[i].n) != 0 &&
            memcmp(p, pvs_parches[i].nuevo, pvs_parches[i].n) != 0) {
            reg("WARNING: unexpected bytes at 0x%08x, leaving entities alone", pvs_parches[i].va);
            return;
        }
    }
    for (i = 0; i < (int)(sizeof(pvs_parches) / sizeof(pvs_parches[0])); i++) {
        unsigned char *p = (unsigned char *)pvs_parches[i].va;
        if (!VirtualProtect(p, pvs_parches[i].n, PAGE_EXECUTE_READWRITE, &viejo)) return;
        memcpy(p, activar ? pvs_parches[i].nuevo : pvs_parches[i].orig, pvs_parches[i].n);
        VirtualProtect(p, pvs_parches[i].n, viejo, &viejo);
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    estado = activar;
    reg("entities: %s", activar ? "all are sent (one player, 4 KB message)"
                                : "only those in the PVS (multiplayer, original code)");
}

/* ================================================================== */
/*  installation                                                      */
/* ================================================================== */

static void instalar(void)
{
    unsigned int *tabla = (unsigned int *)A_TABLA_DLL;
    DWORD viejo;

    if (!VirtualProtect(tabla, 64 * 4, PAGE_READWRITE, &viejo)) {
        reg("could not unprotect the DLL table");
        return;
    }
    alloc_original = (fn_alloc_t)tabla[IDX_ALLOC_PRIVATE];
    free_original  = (fn_free_t) tabla[IDX_FREE_PRIVATE];
    tabla[IDX_ALLOC_PRIVATE] = (unsigned int)mi_alloc_privado;
    tabla[IDX_FREE_PRIVATE]  = (unsigned int)mi_free_privado;
    changelevel_original = (fn_changelevel_t)tabla[IDX_CHANGELEVEL];
    if ((unsigned int)changelevel_original == A_PFN_CHANGELEVEL)
        tabla[IDX_CHANGELEVEL] = (unsigned int)mi_pfn_changelevel;
    else
        reg("WARNING: entry %d was not pfnChangeLevel (0x%08x)",
            IDX_CHANGELEVEL, (unsigned int)changelevel_original);
    VirtualProtect(tabla, 64 * 4, viejo, &viejo);

    if ((unsigned int)alloc_original != A_ALLOC_PRIVATE)
        reg("WARNING: entry 51 was not the expected one (0x%08x)",
            (unsigned int)alloc_original);

    /* fix for the save buffer overflow (see engine.h) */
    if (*(unsigned int *)A_TAM_BUFFER_SAVE != 0x10000)
        reg("WARNING: the save buffer size was not 0x10000");
    else if (escribir_dword(A_TAM_BUFFER_SAVE, TAM_BUFFER_NUEVO))
        reg("save buffer enlarged from 64 KB to %u KB",
            TAM_BUFFER_NUEVO / 1024);

    if (!escribir_dword(A_PUSH_SAVE_HANDLER, (unsigned int)mi_save))
        reg("could not repoint the save handler");
    if (!escribir_dword(A_PUSH_LOAD_HANDLER, (unsigned int)mi_load))
        reg("could not repoint the load handler");
    if (*(unsigned int *)A_PUSH_MAP_HANDLER != A_HOST_MAP)
        reg("WARNING: the map handler was not where expected");
    else if (!escribir_dword(A_PUSH_MAP_HANDLER, (unsigned int)mi_map))
        reg("could not repoint the map handler");

    if (*(unsigned int *)A_PUSH_BEGIN_HANDLER != A_HOST_BEGIN)
        reg("WARNING: the begin handler was not where expected");
    else if (!escribir_dword(A_PUSH_BEGIN_HANDLER, (unsigned int)mi_begin))
        reg("could not repoint the begin handler");

    {
        unsigned char *p = (unsigned char *)A_SZ_CALL_ERROR;
        if (p[0] != 0xE8 ||
            *(unsigned int *)(p + 1) != A_SYS_ERROR - (A_SZ_CALL_ERROR + 5) ||
            !escribir_dword(A_SZ_CALL_ERROR + 1,
                            (unsigned int)sz_desborde - (A_SZ_CALL_ERROR + 5)))
            reg("WARNING: could not hook the SZ_GetSpace overflow");
    }

    if (*(unsigned int *)A_PUSH_CHANGELEVEL_HANDLER != A_HOST_CHANGELEVEL ||
        !escribir_dword(A_PUSH_CHANGELEVEL_HANDLER, (unsigned int)mi_changelevel))
        reg("WARNING: could not hook changelevel");
    if (*(unsigned int *)A_PUSH_CHANGELEVEL2_HANDLER != A_HOST_CHANGELEVEL2 ||
        !escribir_dword(A_PUSH_CHANGELEVEL2_HANDLER, (unsigned int)mi_changelevel2))
        reg("WARNING: could not hook changelevel2");

    if (!repuntar_jmp(A_JMP_SCAN_LOAD, A_M_SCANSAVES, mi_scan) ||
        !repuntar_jmp(A_JMP_SCAN_SAVE, A_M_SCANSAVES, mi_scan))
        reg("WARNING: could not hook the menu savegame list");

    enganchar_sombras();
    reg("installed");
}

/*
 * Default command line, so the .exe works by double-clicking it without
 * any .bat. The engine reads it with GetCommandLineA (IAT entry at
 * 0xd3f4b0), and this DLL is loaded before the CRT starts, so changing
 * that entry is enough. Only what is not already there is added; the
 * menu only if nothing is already being asked to start (+map, +load,
 * +togglemenu).
 */
#define A_IAT_GETCMDLINE 0xD3F4B0
static char linea_ordenes[2048];

static LPSTR WINAPI mi_GetCommandLineA(void) { return linea_ordenes; }

static void anadir_si_falta(const char *clave, const char *texto)
{
    if (strstr(linea_ordenes, clave)) return;
    if (lstrlenA(linea_ordenes) + lstrlenA(texto) + 2 >= (int)sizeof(linea_ordenes)) return;
    lstrcatA(linea_ordenes, " ");
    lstrcatA(linea_ordenes, texto);
}

static void preparar_linea_ordenes(void)
{
    FARPROC real = GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCommandLineA");
    unsigned int *iat = (unsigned int *)A_IAT_GETCMDLINE;
    DWORD viejo;

    if (!real || *iat != (unsigned int)real) {
        reg("WARNING: the IAT entry at 0x%08x is not GetCommandLineA, no default command line",
            A_IAT_GETCMDLINE);
        return;
    }
    {
        /*
         * The defaults go BEFORE the user's arguments: the "+" commands
         * run in order, and "+maxplayers 1" after a "+map" came too late
         * (the map started in multiplayer, where level changes do not
         * work). +togglemenu and +sizedown go at the end.
         */
        const char *orig = GetCommandLineA(), *resto = orig;
        char args[1024];
        int n;

        if (*resto == '"') { resto++; while (*resto && *resto != '"') resto++; if (*resto) resto++; }
        else while (*resto && *resto != ' ') resto++;
        n = (int)(resto - orig);
        if (n >= (int)sizeof(linea_ordenes) - 1) n = (int)sizeof(linea_ordenes) - 2;
        lstrcpynA(args, resto, sizeof(args));
        lstrcpynA(linea_ordenes, orig, n + 1);                 /* the executable */

        #define DEFECTO(clave, texto) \
            if (!strstr(args, clave)) { lstrcatA(linea_ordenes, " "); lstrcatA(linea_ordenes, texto); }
        DEFECTO("-heapsize", "-heapsize 524288")
        DEFECTO("-condebug", "-condebug")
        if (!strstr(args, "-width") && !strstr(args, "-height")) {
            /* the desktop resolution (previously a fixed 800x600) */
            /* the real display mode: GetSystemMetrics gives the scaled
               resolution if Windows scales above 100 % */
            char res[64];
            DEVMODEA dm;
            ZeroMemory(&dm, sizeof(dm));
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm) && dm.dmPelsWidth >= 640)
                wsprintfA(res, " -width %lu -height %lu", dm.dmPelsWidth, dm.dmPelsHeight);
            else
                lstrcpyA(res, " -width 800 -height 600");
            lstrcatA(linea_ordenes, res);
        }
        DEFECTO("-bpp", "-bpp 32")
        DEFECTO("+maxplayers", "+maxplayers 1")
        DEFECTO("r_novis", "+r_novis 1")           /* NOTES section 24 */
        DEFECTO("r_shadows", "+r_shadows 0")       /* NOTES section 37 */
        DEFECTO("crosshair", "+crosshair 1")       /* the CD's config.cfg sets it to 0 */
        #undef DEFECTO
        if (lstrlenA(linea_ordenes) + lstrlenA(args) + 1 < (int)sizeof(linea_ordenes))
            lstrcatA(linea_ordenes, args);
    }
    if (!strstr(linea_ordenes, "+map") && !strstr(linea_ordenes, "+load") &&
        !strstr(linea_ordenes, "+togglemenu"))
        lstrcatA(linea_ordenes, " +togglemenu");
    anadir_si_falta("+sizedown", "+sizedown");

    if (!VirtualProtect(iat, 4, PAGE_READWRITE, &viejo)) return;
    *iat = (unsigned int)mi_GetCommandLineA;
    VirtualProtect(iat, 4, viejo, &viejo);
    reg("command line: %s", linea_ordenes);
}

/* called by DllMain (loader.c) once the engine has been verified and patched */
void hlalpha_iniciar(void)
{
    preparar_linea_ordenes();
    instalar();
}
