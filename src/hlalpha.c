/*
 * hlalpha.c -- los arreglos de la Half-Life Alpha 0.52 que se aplican en
 * tiempo de ejecucion (antes hlsave.dll). Va dentro de winmm.dll: ver
 * cargador.c, que comprueba los binarios y aplica los parches estaticos.
 */
/*
 * hlsave.dll - guardado y cargado completos para Half-Life Alpha 0.52
 *
 * Por que existe: el motor de la alpha solo guarda las entvars de cada
 * entidad, en texto. El estado privado C++ (el bloque que hl.dll reserva
 * por entidad) no se escribe nunca, porque el Save de la DLL es una traza
 * vacia y su Restore es un "ret" pelado. Y la carga ni siquiera existe:
 * quedo el parser de texto de Quake, que no entiende el formato nuevo.
 *
 * Aqui se hace lo que falta:
 *   - se arregla el desbordamiento del bufer de guardado (bug de 1997)
 *   - se anota el tamaño del bloque privado de cada entidad
 *   - "save" añade al fichero un bloque propio con TODO el estado
 *   - "load" se reescribe entero
 *
 * Se inyecta redirigiendo el punto de entrada de enginegl.exe.
 */

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "motor.h"

/* ================================================================== */
/*  registro a fichero                                                */
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
/*  parcheo de memoria                                                */
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
/*  base de los modulos                                               */
/* ================================================================== */
/*
 * hl.dll NO carga en 0x10000000. Trae tabla de reubicaciones y Windows
 * la mueve a donde le parece: en tres arranques seguidos salio en
 * 0x1f160000, 0x3daf0000 y 0x3d5e0000. Eso importa mucho aqui, porque el
 * offset 0 de cada bloque privado es el puntero a la vtable C++ de la
 * clase, o sea una direccion DENTRO de hl.dll. Guardada en crudo solo
 * vale mientras el modulo siga en el mismo sitio, y no sigue: al volver a
 * abrir el juego la partida cargada salta a punteros muertos. Por eso van
 * como offset dentro del modulo.
 *
 * El .exe si tiene base fija (0x400000, sin reubicaciones), asi que los
 * punteros a sus datos estaticos -- entre ellos las cadenas -- se pueden
 * dejar tal cual.
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
/*  tamaño de los bloques privados                                    */
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
/*  utilidades                                                        */
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

/* Deja constancia del estado del jugador, para comprobar que lo que se
   restaura es lo que se guardo. wvsprintf no sabe de decimales, van enteros. */
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
        /* suma de control del bloque privado: si cuadra antes y despues,
           el estado C++ ha viajado entero */
        unsigned char *p = (unsigned char *)privado_de(e);
        int tam = tam_privado[1], k;
        unsigned int suma = 0;
        for (k = 0; p && k < tam; k++) suma = suma * 31u + p[k];

        reg("%s: t=%dms vida=%d org=(%d %d %d) vang=(%d %d %d) priv=%d suma=0x%08x",
            cuando, (int)(SV_TIME * 1000.0), (int)vida,
            org  ? (int)org[0]  : 0, org  ? (int)org[1]  : 0, org  ? (int)org[2]  : 0,
            vang ? (int)vang[0] : 0, vang ? (int)vang[1] : 0, vang ? (int)vang[2] : 0,
            tam, suma);
    }
}

/*
 * Los angulos de vista se restauran en el servidor (van en las entvars),
 * pero el CLIENTE lleva su propia copia y no se entera. En el Quake del
 * que sale este motor, la forma de obligarle a mirar donde toca es poner
 * fixangle=1 en la entidad del jugador: entonces el servidor le manda un
 * svc_setangle con v.angles y el cliente salta a esa direccion.
 * Sin esto el jugador carga mirando siempre al mismo sitio.
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
        reg("vista: faltan campos (v_angle=%d angles=%d fixangle=%d)",
            dva != 0, dang != 0, dfix != 0);
        return;
    }

    va  = (float *)((char *)e + ED_VARS_OFS + dva->ofs * 4);
    ang = (float *)((char *)e + ED_VARS_OFS + dang->ofs * 4);

    /*
     * svc_setangle manda v.angles, no v_angle, asi que hay que igualarlos.
     *
     * Pero el BALANCEO (indice 2) va a cero a proposito. Es el efecto de
     * inclinacion al andar de lado, y el motor lo recalcula solo en cada
     * fotograma a partir de la velocidad: no es un angulo de vista de
     * verdad, no se debe guardar ni reponer. Si se le fuerza al cliente,
     * se le queda pegado en cl.viewangles y nadie lo vuelve a poner a
     * cero, asi que cargas torcido y te quedas torcido para siempre.
     * OJO: con esto no basta, SV_ClientThink lo recalcula de la velocidad
     * antes de mandarlo; lo que lo arregla es que el cliente descarte el
     * balanceo del svc_setangle (parchear.ps1, NOTAS apartado 21).
     */
    ang[0] = va[0];
    ang[1] = va[1];
    ang[2] = 0.0f;
    *(float *)((char *)e + ED_VARS_OFS + dfix->ofs * 4) = 1.0f;

    reg("vista: v_angle=(%d %d %d), balanceo %d descartado, fixangle puesto",
        (int)va[0], (int)va[1], (int)va[2], (int)va[2]);
}

static char *cadena_motor(unsigned int ofs)
{
    char *base = *(char **)A_STRINGS;
    return base ? base + ofs : NULL;
}

/* ================================================================== */
/*  formato del bloque propio                                         */
/* ================================================================== */

#define MARCA_EXTRA   0x58444C43u   /* CLDX */
#define VERSION_EXTRA 3

#define RELOC_EDICT   1   /* puntero a un edict                           */
#define RELOC_PRIVADO 2   /* puntero a otro bloque privado                */
#define RELOC_DLL     3   /* puntero dentro de hl.dll (vtables y metodos) */
#define RELOC_VIVO    4   /* puntero a algo del motor que no sabemos
                             traducir: al cargar se deja el que tenga el
                             proceso recien arrancado, no el guardado     */

typedef struct {
    unsigned int desplazamiento;
    unsigned int clase;
    unsigned int destino;
    unsigned int dentro;
} reloc_t;

#define MAX_RELOCS 8192

/* ---------------- escritura ---------------- */

/*
 * Todo el bloque se monta en memoria y se escribe de una vez al final. Con
 * un WriteFile por campo (miles por guardado) el bloque tardaba 1,3-1,5 s
 * en c1a3; el guardado del propio motor, 6 ms. El HANDLE se ignora.
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
 * Contexto del escaneo. Es el mismo para las entvars y para los bloques
 * privados: en los dos sitios hay punteros y en los dos hay que anotarlos.
 * Antes solo se escaneaban los bloques privados, y por eso el
 * pContainingEntity de las entvars se guardaba en crudo.
 */
static struct {
    unsigned char **privs;
    int            *tams;
    int             nedicts;
    unsigned int    minp, maxp;        /* horquilla de los bloques privados */
    unsigned int    edbase, edtop;     /* array de edicts                   */
    unsigned int    venbase, ventop;   /* ventana del hunk, ver abajo       */
} esc;

/*
 * Ventana alrededor del array de edicts para reconocer punteros al hunk
 * del motor. Estrecha a proposito: reconocer punteros por el valor tiene
 * el riesgo de confundir un float con una direccion, y cuanto mas ancha
 * la horquilla, mas facil es el falso positivo. 1 MB basta -- lo que se
 * ha visto en la practica cae a menos de 2 KB del array -- y deja fuera
 * el rango donde caen los floats normales de un mapa.
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

/* ¿hay memoria de verdad en esa direccion? segundo filtro de la ventana */
/*
 * VirtualQuery tarda ~2,5 ms por llamada en este sistema (algo la
 * intercepta), y se consultaba una vez por entidad casi siempre por la
 * misma direccion (pSystemGlobals): 510 llamadas = 1,3 s por guardado.
 * Se recuerda la ultima region consultada; las direcciones que caen
 * dentro se contestan sin llamar al sistema.
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

        /* dentro del array de edicts */
        if (esc.edbase && v >= esc.edbase && v < esc.edtop) {
            unsigned int d = v - esc.edbase;
            salida[n].desplazamiento = o;
            salida[n].clase   = RELOC_EDICT;
            salida[n].destino = d / (unsigned int)EDICT_SIZE;
            salida[n].dentro  = d % (unsigned int)EDICT_SIZE;
            n++;
            continue;
        }

        /* dentro de otro bloque privado */
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

        /* dentro de hl.dll: vtables y punteros a metodos */
        if (tam_dll && v >= base_dll && v < base_dll + tam_dll) {
            salida[n].desplazamiento = o;
            salida[n].clase   = RELOC_DLL;
            salida[n].destino = 0;
            salida[n].dentro  = v - base_dll;
            n++;
            continue;
        }

        /* datos estaticos del .exe: base fija, se dejan tal cual */
        if (tam_exe && v >= base_exe && v < base_exe + tam_exe) continue;

        /* algo del hunk del motor que no sabemos nombrar */
        if (v >= esc.venbase && v < esc.ventop && direccion_viva(v)) {
            salida[n].desplazamiento = o;
            salida[n].clase   = RELOC_VIVO;
            salida[n].destino = 0;
            salida[n].dentro  = v;          /* solo para la traza */
            n++;
        }
    }
    return n;
}

/*
 * Las entvars se copian en crudo, pero los campos de tipo cadena guardan
 * un desplazamiento dentro de la tabla de cadenas del motor, que se
 * reconstruye distinta en cada arranque. Asi que esos van aparte, como
 * texto, y al cargar se rehacen con ED_ParseEpair.
 */
static void escribir_cadenas_de(HANDLE h, void *ed)
{
    int n = num_fielddefs(), i;
    unsigned int cuenta = 0;
    unsigned int pos_cuenta = sal.n;

    escribir_u32(h, 0);                       /* hueco para la cuenta */

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

    if (cuenta && !sal.error)                 /* rellenar la cuenta */
        memcpy(sal.d + pos_cuenta, &cuenta, 4);
}

/* cuenta las reubicaciones por clase, solo para la traza */
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
        reg("anexar: num_edicts fuera de rango (%d)", nedicts);
        return;
    }

    for (i = 0; i < 5; i++) por_clase[i] = 0;
    localizar_modulos();
    if (!tam_dll) {
        Con_Printf("hlsave: no encuentro hl.dll en memoria, no se guarda el estado completo\n");
        reg("anexar: hl.dll no localizada, abortado");
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

    h = NULL;                        /* se escribe en memoria (sal) */
    sal.n = 0;
    sal.error = 0;

    escribir_u32(h, MARCA_EXTRA);
    escribir_u32(h, VERSION_EXTRA);
    escribir_cadena(h, MAPNAME);
    escribir(h, (void *)A_SV_TIME, 8);
    escribir_u32(h, (unsigned int)nedicts);
    escribir_u32(h, (unsigned int)tam_entvars());
    escribir_u32(h, base_dll);       /* donde cargo hl.dll en esta sesion */

    for (i = 0; i < nedicts; i++) {
        void *e = edict_n(i);
        unsigned int nr = 0;

        if (edict_libre(e)) { escribir_u32(h, 1); continue; }
        escribir_u32(h, 0);

        /*
         * Las entvars tambien llevan punteros: el pContainingEntity de
         * cada entidad apunta a su propio edict, y hay algun puntero mas
         * a estructuras del motor. Antes se copiaban en crudo, que es lo
         * que hacia petar la carga en otra sesion.
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

    /* y ahora, de una vez, al final del fichero que acaba de escribir el motor */
    if (sal.error) {
        reg("anexar: sin memoria para montar el bloque, no se guarda el estado completo");
        goto fin;
    }
    h = CreateFileA(ruta, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        reg("anexar: no se pudo abrir %s (error %lu)", ruta, GetLastError());
        goto fin;
    }
    SetFilePointer(h, 0, NULL, FILE_END);
    {
        DWORD w = 0;
        if (!WriteFile(h, sal.d, sal.n, &w, NULL) || w != sal.n)
            reg("anexar: escritura incompleta en %s (%lu de %u bytes)", ruta, w, sal.n);
    }
    CloseHandle(h);
    Con_Printf("hlsave: %d entidades (%d con estado privado), %d punteros\n",
               ocupados, con_datos, total_relocs);
    registrar_jugador("AL GUARDAR");
    reg("guardado: edicts=%d ocupados=%u con_datos=%u relocs=%u mapa=%s",
        nedicts, ocupados, con_datos, total_relocs, MAPNAME);
    reg("guardado: punteros por clase: edict=%u privado=%u hl.dll=%u vivo=%u"
        "  (hl.dll en 0x%08x)",
        por_clase[RELOC_EDICT], por_clase[RELOC_PRIVADO],
        por_clase[RELOC_DLL], por_clase[RELOC_VIVO], base_dll);

fin:
    HeapFree(GetProcessHeap(), 0, privs);
    HeapFree(GetProcessHeap(), 0, tams);
}

/* ================================================================== */
/*  persistencia de los mapas entre transiciones                      */
/* ================================================================== */
/*
 * La alpha arranca cada mapa de cero en cada changelevel: al volver a un
 * mapa, botones, puertas y enemigos estaban como al principio. Ahora, al
 * salir por una transicion se guarda el estado del mapa en
 * valve\SAVE\<mapa>.niv (el mismo bloque CLDX de los guardados) y al
 * volver a entrar se repone, menos el jugador, que llega por el landmark
 * con su vida, armas y velocidad. "map" (partida nueva) los borra; "save"
 * los copia a valve\SAVE\<partida>\ y "load" los recupera de ahi, para
 * que los demas mapas esten como cuando se guardo.
 */
static int conservar_jugador = 0;   /* mi_restaurar: no tocar el edict 1 */
static int nivel_pendiente   = 0;   /* el proximo begin viene de un changelevel */

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
    if (n) reg("niveles: borrados %d estados de %s", n, dir);
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
    reg("niveles: copiados %d estados de %s a %s", n, de, a);
}

/* guarda el estado del mapa actual en SAVE\<mapa>.niv (antes de salir) */
static void guardar_nivel_actual(void)
{
    char dir[512], ruta[600];
    HANDLE h;

    if (SVS_MAXCLIENTS != 1 || !SV_EDICTS || !*MAPNAME) return;
    dir_niveles(dir, NULL);
    CreateDirectoryA(dir, NULL);
    wsprintfA(ruta, "%s/%s.niv", dir, MAPNAME);
    h = CreateFileA(ruta, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) { reg("niveles: no se pudo crear %s", ruta); return; }
    CloseHandle(h);
    anexar_bloques(ruta);
    reg("niveles: guardado el estado de %s al salir", MAPNAME);
}

static void __cdecl mi_save(void)
{
    char ruta[512], de[512], a[512];
    const char *nombre;
    FILETIME antes;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (Cmd_Argc() == 2 && lstrcmpiA(Cmd_Argv(1), "quick") == 0) {
        /* F6/F7 de fabrica: el guardado rapido va a la ranura 12 del menu (s11) */
        Cbuf_InsertText("save s11\n");
        return;
    }

    LARGE_INTEGER frec, c0, c1, c2, c3;
    QueryPerformanceFrequency(&frec);
    QueryPerformanceCounter(&c0);
    GetSystemTimeAsFileTime(&antes);
    ((fn_void_t)A_HOST_SAVEGAME)();      /* el guardado original, intacto */
    QueryPerformanceCounter(&c1);

    if (Cmd_Argc() != 2) return;
    nombre = Cmd_Argv(1);
    if (!nombre || !*nombre) return;

    wsprintfA(ruta, "%s/%s.sav", GAMEDIR, nombre);
    /*
     * Solo si el motor ACABA de escribir el fichero: si se nego a guardar
     * ("Can't savegame with a dead player"...) y ya habia un .sav con ese
     * nombre, se le anadiria un bloque a una partida vieja. Margen de 2 s
     * por la resolucion de la fecha de los ficheros.
     */
    if (!GetFileAttributesExA(ruta, GetFileExInfoStandard, &info)) return;
    {
        ULARGE_INTEGER t0, t1;
        t0.LowPart = antes.dwLowDateTime;  t0.HighPart = antes.dwHighDateTime;
        t1.LowPart = info.ftLastWriteTime.dwLowDateTime;
        t1.HighPart = info.ftLastWriteTime.dwHighDateTime;
        if (t1.QuadPart + 20000000ULL < t0.QuadPart) {
            reg("save: el motor no escribio %s, no se anade nada", ruta);
            return;
        }
    }
    anexar_bloques(ruta);
    QueryPerformanceCounter(&c2);

    /* los estados de los demas mapas visitados van con esta partida */
    dir_niveles(de, NULL);
    dir_niveles(a, nombre);
    CreateDirectoryA(de, NULL);
    CreateDirectoryA(a, NULL);
    borrar_niveles(a);
    copiar_niveles(de, a);
    QueryPerformanceCounter(&c3);
    reg("tiempos del guardado: motor %d ms, bloque propio %d ms, estados de mapas %d ms",
        (int)((c1.QuadPart - c0.QuadPart) * 1000 / frec.QuadPart),
        (int)((c2.QuadPart - c1.QuadPart) * 1000 / frec.QuadPart),
        (int)((c3.QuadPart - c2.QuadPart) * 1000 / frec.QuadPart));
}

/* ================================================================== */
/*  lectura                                                           */
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

/* busca la marca CLDX por el final del fichero */
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
/*  carga                                                             */
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
 * Situa el lector justo despues de la cabecera del bloque propio.
 * Devuelve 0 si el bloque no esta o no cuadra.
 */
static int abrir_bloque(lector_t *l, cabecera_t *c)
{
    l->d = fichero; l->n = fichero_n; l->p = 0; l->error = 0;
    if (!situar_bloque(l)) return 0;
    if (leer_u32(l) != MARCA_EXTRA) return 0;
    if (leer_u32(l) != VERSION_EXTRA) return 0;
    leer_cadena(l);                          /* mapa */
    leer_bytes(l, c->tiempo, 8);
    c->nedicts           = leer_u32(l);
    c->tamvars           = leer_u32(l);
    c->base_dll_guardada = leer_u32(l);
    return !l->error;
}

/*
 * Recorre el bloque entero sin tocar nada, solo para comprobar que cuadra.
 * Importante: sin esto, un fichero corrupto dejaria la partida a medio
 * destruir, porque la restauracion libera el estado antes de reponerlo.
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
        if (leer_u32(&l)) continue;             /* libre */
        leer_bytes(&l, NULL, c.tamvars);
        nr = leer_u32(&l);                      /* punteros de las entvars */
        if (nr > MAX_RELOCS) return 0;
        leer_bytes(&l, NULL, nr * (unsigned int)sizeof(reloc_t));
        ncad = leer_u32(&l);
        if (ncad > 4096) return 0;
        for (j = 0; j < ncad && !l.error; j++) { leer_u32(&l); leer_cadena(&l); }
        tampriv = leer_u32(&l);
        if (tampriv > 0x10000) return 0;
        if (tampriv) leer_bytes(&l, NULL, tampriv);
        nr = leer_u32(&l);                      /* punteros del bloque */
        if (nr > MAX_RELOCS) return 0;
        leer_bytes(&l, NULL, nr * (unsigned int)sizeof(reloc_t));
    }
    return !l.error;
}

/* ================================================================== */
/*  estado vivo del mapa recien arrancado                             */
/* ================================================================== */
/*
 * Hay punteros del motor que no sabemos traducir (clase VIVO). Para esos
 * el valor bueno no es el del fichero -- que es de otro proceso -- sino
 * el que tiene el mapa recien arrancado, que apunta a las estructuras de
 * ESTA sesion. Asi que antes de destruir nada se guarda una copia.
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
 * Un trigger_changelevel se apaga solo en cuanto lo pisas (solid a
 * SOLID_NOT y su funcion de toque a nulo), antes de pedir el cambio. Si
 * el cambio no llega a hacerse -- la salida a c1a1c cuando el mapa no
 * existia y se ignoraba --, el trigger se queda apagado, se guarda asi y
 * la salida ya no vuelve a funcionar en esa partida por mucho que se
 * cargue. Un cambio que si se hace carga otro mapa, asi que en un
 * guardado un trigger_changelevel apagado es siempre un cambio fallido:
 * se rearma con lo que tenia el mismo edict en el mapa recien arrancado.
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
        /* misma clase en los dos (la vtable ya esta recolocada) */
        if (*(unsigned int *)p != *(unsigned int *)viva) continue;
        toque = *(unsigned int *)(viva + PRIV_TOQUE);
        if (!toque) continue;

        *(unsigned int *)(p + PRIV_TOQUE) = toque;
        *solid = SOLID_TRIGGER;
        ((void (__cdecl *)(void *, int))A_SV_LINKEDICT)(e, 0);
        reg("restaurar: salida a %.32s (edict %d) estaba apagada, rearmada",
            (char *)(p + PRIV_MAPA_DESTINO), i);
    }
}

/*
 * El trigger de la salida a c1a1c (*15 de c1a1a) es una franja de 22
 * unidades (z -118..-96) en lo alto del hueco de la puerta doble, que va
 * de -166 a -38: agachado se pasa por debajo sin tocarlo. Se estira a
 * todo el hueco cada vez que se entra al mapa (y despues de cargar, que
 * repone los mins/maxs del guardado).
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
            reg("salida a %s (edict %d) estirada a z %d..%d", salidas_estiradas[k].destino,
                i, (int)salidas_estiradas[k].zmin, (int)salidas_estiradas[k].zmax);
        }
    }
}

/*
 * hl.dll marca con EF_BRIGHTFIELD (un campo de particulas) al monstruo que
 * arranca atascado en la pared: una marca de depuracion para el mapeador.
 * parchear.ps1 la quita de hl.dll, pero los guardados de antes la llevan
 * en las entvars y se repondria al cargar. Nada mas en la alpha usa ese
 * efecto, asi que se quita de todas las entidades.
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
        reg("restaurar: quitado el campo de particulas (monstruo atascado) al edict %d", i);
    }
}

/*
 * El progs.dat de la alpha declara como "entity" campos que hl.dll usa
 * como enteros (sequence, weapon, weapons, ammo_*, items, button...). Al
 * guardar, el motor convierte cada campo entity en numero de edict
 * (NUM_FOR_EDICT), y "weapon" lleva en los bytes altos el estado del
 * cambio de arma: tras cambiar de arma la conversion se sale de rango y
 * el guardado aborta con "NUM_FOR_EDICT: Bad pointer". Se cambia en
 * memoria el tipo de "weapon" a float: el texto del .sav lleva un numero
 * inocuo y la carga repone el valor exacto desde el bloque CLDX. Los
 * progs se cargan con cada mapa, asi que se hace en cada "begin".
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
        reg("progs: campo weapon de entity a float (el guardado petaba tras cambiar de arma)");
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
 * Suma de control de un bloque ignorando los punteros. Sirve para
 * comparar el bloque del fichero con el que queda en memoria: todo lo
 * que no sea un puntero tiene que salir igual, y los punteros no pueden
 * salir igual porque son de otro proceso. Sin cerarlos la comparacion no
 * diria nada.
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
 * Aplica las reubicaciones que se pueden reconstruir a base de calculo.
 * Las de clase VIVO no pasan por aqui: se resuelven en la primera pasada,
 * que es cuando todavia existe el estado del mapa recien arrancado.
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
            /* la vtable y los punteros a metodos, rehechos sobre la base
               que tiene hl.dll AHORA, no la de cuando se guardo */
            if (r[k].dentro >= tam_dll) { (*fuera)++; continue; }
            valor = base_dll + r[k].dentro;
            break;
        default:
            continue;                       /* VIVO: ya resuelto */
        }

        *(unsigned int *)(destino + r[k].desplazamiento) = valor;
        por_clase[r[k].clase]++;
    }
}

/* segunda fase: el mapa ya esta cargado, ahora se repone el estado */
/*
 * Un edict ocupado en el mapa recien arrancado que en el estado guardado
 * estaba libre (p. ej. un monstruo que se mato y el juego retiro): hay que
 * dejarlo como lo deja ED_Free, desenlazado del mundo y sin modelo ni
 * solido. Si solo se marcaba libre seguia enlazado en el arbol de areas
 * como una entidad fantasma. Su bloque privado ya se libero antes.
 */
#define A_SV_UNLINKEDICT 0x429FE9   /* SV_UnlinkEdict(edict); lo llama SV_LinkEdict al empezar */
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

/* lee y descarta una entrada ocupada del bloque (tras su marca de libre) */
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
 * Al reponer un mapa por una transicion el reloj del servidor pasa al que
 * tenia ese mapa, pero el jugador trae tiempos absolutos del mapa del que
 * viene (air_finished, pain_finished...). Se desplazan lo mismo que el
 * reloj; si no, bajo el agua empezaria a ahogarse en el acto, por ejemplo.
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

    if (!fichero) { reg("restaurar: no hay fichero en memoria"); return; }

    /*
     * Sin la base de hl.dll no se pueden rehacer las vtables, y restaurar
     * a medias es peor que no restaurar: el motor petaria en la primera
     * llamada virtual.
     */
    localizar_modulos();
    if (!tam_dll) {
        Con_Printf("hlsave: no encuentro hl.dll en memoria, no se toca la partida\n");
        reg("restaurar: hl.dll no localizada, abortado sin tocar nada");
        return;
    }

    /* comprobar ANTES de destruir nada */
    if (SVS_MAXCLIENTS != 1) {
        /* en multijugador los edicts 1..maxclients son de los jugadores y
           todo lo demas va desplazado respecto al guardado */
        Con_Printf("hlsave: el mapa ha arrancado para %d jugadores, no se carga\n",
                   SVS_MAXCLIENTS);
        reg("restaurar: maxclients=%d, abortado sin tocar nada", SVS_MAXCLIENTS);
        return;
    }
    if (!validar_bloque()) {
        Con_Printf("hlsave: el guardado no cuadra, no se toca la partida\n");
        reg("restaurar: validacion fallida, abortado sin tocar nada");
        return;
    }

    if (!abrir_bloque(&l, &cab)) return;
    nedicts = cab.nedicts;
    tamvars = cab.tamvars;
    if (tamvars > sizeof(vars)) {
        reg("restaurar: entvars de %u bytes, no caben en el bufer", tamvars);
        return;
    }
    {
        double antes = SV_TIME;
        memcpy((void *)A_SV_TIME, cab.tiempo, 8);
        if (conservar_jugador) desplazar_tiempos_jugador(SV_TIME - antes);
    }

    reg("restaurar: hl.dll al guardar=0x%08x, ahora=0x%08x%s",
        cab.base_dll_guardada, base_dll,
        cab.base_dll_guardada == base_dll ? "" : "  (MOVIDA)");

    privs = (unsigned char **)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        sizeof(void *) * nedicts);
    if (!privs) return;
    for (i = 0; i < 5; i++) por_clase[i] = 0;

    /* la foto del estado vivo, antes de tocarlo */
    tomar_copia_viva();

    /*
     * Los unicos punteros VIVO que aparecen son pev->pSystemGlobals
     * (entvars +0x20c) y su copia en el bloque privado (+0x8): el mismo
     * valor en todas las entidades. Si el edict del mapa recien arrancado
     * no tiene de donde copiarlo (estaba libre: al volver a un mapa hay
     * entidades creadas jugando, restos...), se queda nulo y hl.dll peta
     * en cuanto la entidad piensa (0x10018d7d, lee [globales+0x7c]). Se
     * toma entonces el del mundo (edict 0), que siempre esta.
     */
    globales = *(unsigned int *)(vars_de(edict_n(0)) + 0x20c);

    /* limpiar lo que dejo el arranque del mapa */
    for (i = 0; i < (unsigned int)NUM_EDICTS && i < MAX_EDICTS_PROPIO; i++) {
        if (conservar_jugador && i == 1) continue;       /* el jugador se queda */
        mi_free_privado(edict_n((int)i));
    }

    /* --- primera pasada: entvars, cadenas y bloques privados --- */
    for (i = 0; i < nedicts; i++) {
        void *e = edict_n((int)i);
        unsigned int libre, ncad, j, tampriv, nr, k;

        libre = leer_u32(&l);
        if (l.error) break;

        /* transicion: el jugador es el que acaba de llegar, no el guardado;
           a quien le apunte se le da el bloque privado del jugador vivo */
        if (conservar_jugador && i == 1) {
            if (!libre) saltar_entrada(&l, tamvars);
            privs[i] = (unsigned char *)privado_de(e);
            continue;
        }

        /*
         * OJO: no se toca la cabecera del edict (bytes 0x04..0x77). Ahi
         * viven los enlaces de area, que son una lista doblemente enlazada
         * entre edicts. Si se ponen a cero sin desenlazar antes, los
         * vecinos quedan apuntando a un nodo muerto y el motor revienta al
         * recorrer la lista. Solo se reponen las entvars.
         */
        if (libre) {
            if (!edict_libre(e)) { descuadre++; liberar_como_ed_free(e); }
            *(int *)((char *)e + ED_FREE_OFS) = 1;
            continue;
        }

        if (edict_libre(e)) descuadre++;
        *(int *)((char *)e + ED_FREE_OFS) = 0;

        /*
         * Las entvars se leen a un bufer aparte para poder rescatar los
         * punteros de clase VIVO del edict que todavia esta en pie, antes
         * de sobreescribirlo.
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
            reg("restaurar: tam privado absurdo (%u) en edict %u", tampriv, i);
            l.error = 1;
            break;
        }

        /* los punteros del bloque: ahora solo los de clase VIVO, que
           necesitan la copia. El resto, en la segunda pasada, cuando
           existan todos los bloques. */
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
                    /* el gpGlobals del bloque privado: el del mundo */
                    *(unsigned int *)(privs[i] + des) = globales;
                    vivo_de_mundo++;
                } else {
                    /* sin copia de la que tirar, mejor nulo que muerto:
                       la DLL comprueba los nulos, las direcciones de otro
                       proceso no las comprueba nadie */
                    *(unsigned int *)(privs[i] + des) = 0;
                    sin_copia++;
                }
            }
        }
    }

    if (l.error) {
        reg("restaurar: fichero corrupto en la primera pasada (edict %u)", i);
        soltar_copia_viva();
        HeapFree(GetProcessHeap(), 0, privs);
        return;
    }

    NUM_EDICTS = (int)nedicts;

    /* --- segunda pasada: los punteros que se reconstruyen --- */
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
        bloque = l.d + l.p;                 /* el bloque tal cual esta en el .sav */
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

    reg("restaurar: punteros rehechos: edict=%u privado=%u hl.dll=%u vivo=%u"
        "  (sin copia=%u descartados=%u, globales del mundo=%u)",
        por_clase[RELOC_EDICT], por_clase[RELOC_PRIVADO],
        por_clase[RELOC_DLL], por_clase[RELOC_VIVO], sin_copia, fuera, vivo_de_mundo);
    reg("restaurar: contenido de los bloques privados: %u comparados, %u distintos",
        comparados, distintos);

    /*
     * Enlazar de nuevo en el mundo todo lo restaurado, con su posicion
     * guardada (como el loadgame de Quake): el enlace de area que se
     * conserva es el del mapa recien arrancado, y una entidad que se habia
     * movido (o que estaba libre al arrancar) quedaba enlazada donde no
     * esta.
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
        Con_Printf("hlsave: estado del mapa repuesto (%d entidades)\n", nedicts);
        registrar_jugador("TRAS VOLVER AL MAPA");
    } else {
        Con_Printf("hlsave: partida restaurada (%d entidades)\n", nedicts);
        forzar_vista_jugador();
        registrar_jugador("TRAS CARGAR");
    }
    reg("restaurar: hecho, %u edicts, %u descuadres ocupado/libre", nedicts, descuadre);
}

static void __cdecl mi_load(void)
{
    char ruta[512], orden[256];
    const char *nombre;
    lector_t l;
    char *mapa;
    if (Cmd_Argc() == 2 && lstrcmpiA(Cmd_Argv(1), "quick") == 0) {
        /* F6/F7 de fabrica: el guardado rapido va a la ranura 12 del menu (s11) */
        Cbuf_InsertText("load s11\n");
        return;
    }

    if (Cmd_Argc() != 2) {
        Con_Printf("load <nombre> : cargar una partida\n");
        return;
    }
    nombre = Cmd_Argv(1);
    wsprintfA(ruta, "%s/%s.sav", GAMEDIR, nombre);

    if (!cargar_fichero(ruta)) {
        Con_Printf("hlsave: no se pudo leer %s\n", ruta);
        return;
    }

    l.d = fichero; l.n = fichero_n; l.p = 0; l.error = 0;
    if (!situar_bloque(&l)) {
        Con_Printf("hlsave: %s no tiene estado completo (guardado con el motor original)\n",
                   nombre);
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return;
    }
    leer_u32(&l);
    if (leer_u32(&l) != VERSION_EXTRA) {
        Con_Printf("hlsave: ese .sav es de un formato anterior (los punteros\n");
        Con_Printf("        iban en crudo y no se pueden recolocar); hay que rehacerlo\n");
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return;
    }
    mapa = leer_cadena(&l);
    if (l.error || !*mapa) {
        Con_Printf("hlsave: no se pudo leer el nombre del mapa\n");
        HeapFree(GetProcessHeap(), 0, fichero);
        fichero = NULL;
        return;
    }
    lstrcpynA(mapa_pendiente, mapa, sizeof(mapa_pendiente));

    {   /* los demas mapas, como estaban cuando se guardo esta partida */
        char de[512], a[512];
        dir_niveles(a, NULL);
        dir_niveles(de, nombre);
        CreateDirectoryA(a, NULL);
        borrar_niveles(a);
        copiar_niveles(de, a);
    }
    nivel_pendiente = 0;

    Con_Printf("hlsave: cargando %s (mapa %s)...\n", nombre, mapa_pendiente);
    reg("load: fichero=%s mapa=%s tam=%u", ruta, mapa_pendiente, fichero_n);

    /*
     * No se puede registrar un comando propio: Cmd_AddCommand aborta el
     * juego si se llama despues de la inicializacion ("Cmd_AddCommand
     * after host_initialized"). Asi que se engancha el propio "map": se
     * deja la carga marcada como pendiente y mi_map restaura en cuanto el
     * mapa haya terminado de arrancar.
     */
    carga_pendiente = 1;
    /*
     * "disconnect" primero: con una partida multijugador en marcha,
     * "maxplayers" se niega ("can not be changed while a server is
     * running"), el mapa arrancaba para varios jugadores con otra
     * numeracion de edicts y el guardado se volcaba cruzado (peto en el
     * calculo de choques). disconnect (0x43e35e) apaga el servidor.
     */
    wsprintfA(orden, "disconnect\nmaxplayers 1\nmap %s\n", mapa_pendiente);
    Cbuf_InsertText(orden);
}

/* El comando "map" enganchado: arranca el mapa y, si venimos de un
   "load", repone el estado en cuanto el mapa esta en pie. */
static int mapa_existe(const char *mapa);
static void ampliar_bufers_servidor(void);
static void entidades_sin_pvs(int activar);

static void __cdecl mi_map(void)
{
    /*
     * "New Game" del menu de la alpha manda "map start", pero la alpha no
     * trae start.bsp: desde el menu no se podia empezar partida. Se va al
     * primer mapa que si trae.
     */
    if (Cmd_Argc() >= 2 && lstrcmpiA(Cmd_Argv(1), "start") == 0 && !mapa_existe("start")) {
        reg("map start: la alpha no trae start.bsp, se empieza en c1a1");
        Cbuf_InsertText("map c1a1\n");
        return;
    }

    if (!carga_pendiente) {
        char dir[512];
        dir_niveles(dir, NULL);
        borrar_niveles(dir);                 /* partida nueva: mundo nuevo */
        nivel_pendiente = 0;
    }

    ((fn_void_t)A_HOST_MAP)();
    ampliar_bufers_servidor();
    entidades_sin_pvs(SVS_MAXCLIENTS == 1);

    if (carga_pendiente)
        registrar_jugador("MAPA RECIEN ARRANCADO");
}

/*
 * El comando "begin" cierra el apreton de manos del cliente: a partir de
 * aqui el jugador ya existe de verdad, con su bloque privado reservado.
 * Restaurar antes de esto funcionaba en caliente por pura suerte (el
 * cliente ya venia conectado), pero en frio el jugador todavia no estaba
 * y el motor petaba al crearlo encima de lo restaurado.
 */
static void __cdecl mi_begin(void)
{
    ((fn_void_t)A_HOST_BEGIN)();
    ampliar_bufers_servidor();
    entidades_sin_pvs(SVS_MAXCLIENTS == 1);

    arreglar_tipo_weapon();
    if (carga_pendiente) {
        carga_pendiente = 0;
        registrar_jugador("CLIENTE YA DENTRO");
        mi_restaurar();
    } else {
        /* entrada normal (nueva partida o cambio de nivel): deja constancia
           de la velocidad con la que llega el jugador (NOTAS apartado 27) */
        ddef_t *dvel = (ddef_t *)ED_FindField("velocity");
        ddef_t *dorg = (ddef_t *)ED_FindField("origin");
        if (SV_EDICTS && NUM_EDICTS > 1 && dvel && dorg && !edict_libre(edict_n(1))) {
            float *v = (float *)(vars_de(edict_n(1)) + dvel->ofs * 4);
            float *o = (float *)(vars_de(edict_n(1)) + dorg->ofs * 4);
            ddef_t *dva = (ddef_t *)ED_FindField("v_angle");
            ddef_t *dan = (ddef_t *)ED_FindField("angles");
            float *va = dva ? (float *)(vars_de(edict_n(1)) + dva->ofs * 4) : v;
            float *an = dan ? (float *)(vars_de(edict_n(1)) + dan->ofs * 4) : v;
            reg("entrada en %s: org=(%d %d %d) velocidad=(%d %d %d) v_angle=(%d %d) angles=(%d %d)",
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
                    reg("niveles: %s ya se habia visitado, se repone su estado", MAPNAME);
                    conservar_jugador = 1;
                    mi_restaurar();
                    conservar_jugador = 0;
                }
            } else {
                reg("niveles: %s no se habia visitado, empieza de cero", MAPNAME);
            }
        }
    }
    estirar_salidas();

    {   /* pruebas automaticas: valvezprueba_N.cfg en la entrada N a un mapa */
        static int entradas;
        char f[512], orden[64];
        entradas++;
        wsprintfA(f, "%s/zprueba_%d.cfg", GAMEDIR, entradas);
        if (GetFileAttributesA(f) != INVALID_FILE_ATTRIBUTES) {
            wsprintfA(orden, "exec zprueba_%d.cfg\n", entradas);
            Cbuf_InsertText(orden);
            reg("prueba: exec zprueba_%d.cfg en %s", entradas, MAPNAME);
        }
        /* y valvezprueba_pos.txt ("x y z cabeceo giro"): colocar al jugador */
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
                    reg("prueba: jugador colocado en (%d %d %d) mirando (%d %d)",
                        (int)x, (int)y, (int)z, (int)pit, (int)yaw);
                }
            }
        }
    }
}

/* ================================================================== */
/*  cambios de nivel a mapas que no existen                           */
/* ================================================================== */
/*
 * La alpha trae transiciones a mapas que nunca incluyo: c1a1a -> c1a1c y
 * c3a2 -> c3a3. Al tocarlas, pfnChangeLevel congela la pantalla para la
 * placa de carga y el motor no encuentra el .bsp: o sale un error fatal
 * con un MessageBox escondido detras del juego a pantalla completa, o la
 * imagen se queda parada 60 s hasta "load failed.". Parece colgado.
 * Si el mapa no esta, el cambio se ignora y se deshacen las dos marcas;
 * si no, las demas salidas del mapa dejarian de funcionar hasta cargar
 * otro nivel (changelevel_issued solo lo limpia el arranque de un mapa).
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

    /* el trigger vuelve a disparar en cada fotograma mientras lo pisas */
    if (lstrcmpiA(ultimo, mapa) != 0 || GetTickCount() - cuando > 5000) {
        Con_Printf("La alpha no trae el mapa %s: esta salida no lleva a ningun sitio\n",
                   mapa);
        reg("changelevel a %s ignorado: el mapa no existe", mapa);
        lstrcpynA(ultimo, mapa, sizeof(ultimo));
        cuando = GetTickCount();
    }
    return 1;
}

/*
 * Donde se pueda, en vez de ignorar la salida se salta al siguiente
 * capitulo que si trae la alpha. Tiene que hacerse aqui, en la funcion que
 * llama hl.dll al tocar el trigger (entrada 5 de la tabla del motor), y no
 * lanzando otro "changelevel": ese comando escrito a mano peta en hl.dll
 * (SetChangeParms, hl.dll+0xb92d) incluso con el juego original. Cambiando
 * solo el nombre del mapa, todo lo demas sigue el camino normal del
 * trigger y se conservan la vida y las armas.
 */
typedef void (__cdecl *fn_changelevel_t)(char *mapa, char *landmark);
static fn_changelevel_t changelevel_original;

static const struct { const char *falta, *siguiente; } capitulos[] = {
    { "c1a1c", "c1a2a" },   /* fin del capitulo 1: salta al 2 */
};

static void __cdecl mi_pfn_changelevel(char *mapa, char *landmark)
{
    int i;
    if (mapa && !mapa_existe(mapa) && !*(int *)A_CHANGELEVEL_ISSUED) {
        for (i = 0; i < (int)(sizeof(capitulos) / sizeof(capitulos[0])); i++) {
            if (lstrcmpiA(mapa, capitulos[i].falta) == 0 &&
                mapa_existe(capitulos[i].siguiente)) {
                Con_Printf("La alpha no trae el mapa %s: se pasa al siguiente capitulo (%s)\n",
                           mapa, capitulos[i].siguiente);
                reg("changelevel a %s redirigido a %s (landmark \"%s\")", mapa,
                    capitulos[i].siguiente, landmark ? landmark : "");
                mapa = (char *)capitulos[i].siguiente;
                break;
            }
        }
    }
    /* si no hay capitulo siguiente, el comando llegara con el mapa que
       falta y mi_changelevel lo ignorara */
    changelevel_original(mapa, landmark);
}

static void __cdecl mi_changelevel(void)
{
    if (cambio_a_mapa_ausente()) return;
    guardar_nivel_actual();              /* el mapa que se deja, tal cual */
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
/*  bufer de mensajes lleno                                           */
/* ================================================================== */
/*
 * "SZ_GetSpace: overflow without allowoverflow set": con varios bichos a
 * la vez (3-4 perros y un cangrejo, a tiros) se llena uno de los bufers
 * de mensajes del servidor y SZ_GetSpace llama al error fatal. Los bufers
 * que SI admiten desbordarse (los de cada cliente) hacen otra cosa: se
 * vacian, se marcan como desbordados y el juego sigue, perdiendo lo de
 * ese fotograma. Se hace lo mismo para todos: la llamada al error en
 * 0x42f1fb va a sz_desborde, que apunta que bufer era y vuelve; el
 * codigo original sigue por el camino de allowoverflow. El error de "un
 * solo mensaje mas grande que el bufer entero" se queda como estaba.
 */
/*
 * sv.datagram (MSG_BROADCAST) y sv.reliable_datagram (MSG_ALL) son de
 * 1 KB. Con una granada entre 3-4 perros (sangre, restos, sonidos) se
 * llena sv.datagram A MITAD DE UN MENSAJE: vaciarlo ahi (lo de abajo)
 * dejaba el resto del mensaje al principio y el cliente lo rechazaba
 * ("CL_ParseServerMessage: Illegible server message"). Se les da un bufer
 * grande: al mandarlo (0x431e2c) el motor copia sv.datagram al mensaje
 * del cliente SOLO si cabe entero y si no lo descarta completo, que es
 * limpio (se pierden los efectos de ese fotograma). El reliable se copia
 * al mensaje fiable de cada cliente, de 8000 bytes. SV_SpawnServer los
 * vuelve a apuntar a sus bufers fijos en cada mapa (0x432662..).
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
    if (avisado < 2) { reg("%s ampliado a %d bytes", nombre, tam); avisado++; }
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
        if (avisados[i] == (unsigned int)buf) return;   /* uno por bufer */
    if (navisados < 8) avisados[navisados++] = (unsigned int)buf;

    if      ((unsigned int)buf == A_SV_DATAGRAM) nombre = "sv.datagram (MSG_BROADCAST)";
    else if ((unsigned int)buf == A_SV_RELIABLE) nombre = "sv.reliable_datagram (MSG_ALL)";
    else if ((unsigned int)buf == A_SV_SIGNON)   nombre = "sv.signon (MSG_INIT)";
    else                                         nombre = "otro";
    reg("SZ_GetSpace: se lleno %s en 0x%08x (maxsize=%d cursize=%d, mapa %s);"
        " se vacia y se sigue", nombre, (unsigned int)buf, buf[3], buf[4], MAPNAME);
}

/* ebx = el sizebuf_t (SZ_GetSpace lo guarda ahi); la pila es la del call */
extern void sz_desborde(void);
__asm__(".globl _sz_desborde\n"
        "_sz_desborde:\n"
        "    pushl %ebx\n"
        "    call  _registrar_desborde\n"
        "    addl  $4, %esp\n"
        "    ret\n");

/* ================================================================== */
/*  lista de partidas del menu                                        */
/* ================================================================== */
/*
 * M_ScanSaves lee s0..s11.sav como si fueran de Quake: un numero y un
 * comentario en texto. Los de la alpha empiezan por la cabecera binaria
 * de Half-Life ("VALV"...), asi que todas las ranuras salian como "VALV"
 * y no habia forma de distinguirlas. Tras el escaneo original se reescribe
 * cada etiqueta con el mapa y la fecha, y las que no llevan el bloque
 * CLDX actual (las tres de fabrica, de 1997) quedan como no cargables.
 * La ultima ranura (s11) es la del guardado rapido: F6/F7 guardan y
 * cargan ahi (ver autoexec.cfg).
 */
#define NUM_RANURAS   12
#define RANURA_RAPIDA 11

static void etiqueta_ranura(int i)
{
    char ruta[512], texto[128];
    char *etq = (char *)(A_MENU_NOMBRES + i * 40);
    int *cargable = (int *)(A_MENU_CARGABLE + i * 4);
    const char *pre = (i == RANURA_RAPIDA) ? "RAPIDA " : "";
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
        if (i == RANURA_RAPIDA) lstrcpynA(etq, "--- RAPIDA: vacia ---", 40);
        return;                      /* el original ya la dejo como vacia */
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
        wsprintfA(texto, "%s(antigua, no se puede cargar)", pre);
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

/* repunta un "jmp rel32" del motor, comprobando a donde iba */
static int repuntar_jmp(unsigned int sitio, unsigned int antes, void *ahora)
{
    unsigned char *p = (unsigned char *)sitio;
    if (p[0] != 0xE9 || *(unsigned int *)(p + 1) != antes - (sitio + 5))
        return 0;
    return escribir_dword(sitio + 1, (unsigned int)ahora - (sitio + 5));
}

/* ================================================================== */
/*  sombras                                                           */
/* ================================================================== */
/*
 * Con r_shadows 1 el motor dibuja la sombra plana de todos los modelos de
 * estudio (R_StudioDrawModel 0x4163f3, en 0x4165e4). Queda bien en los
 * NPC, pero la del arma en primera persona sale flotando junto al arma y
 * la de la granada vuela con ella. En 0x4165fd, donde el motor solo se la
 * salta para el render aditivo (rendermode 5, entidad +0x98), se salta
 * ahora tambien para cl.viewent (0x849cb8, el arma en primera persona) y
 * para el modelo de la granada (nombre del model_t, entidad +0xa8).
 */
#define A_SOMBRA_CMP    0x4165FD   /* cmp dword [esi+98h],5 / je 0x416706  */
#define A_SOMBRA_SI     0x41660A   /* dibujar la sombra                     */
#define A_SOMBRA_NO     0x416706   /* saltarsela                            */
#define A_CL_VIEWENT    0x849CB8

int __cdecl sombra_permitida(unsigned char *ent)
{
    const char *modelo;

    if (*(int *)(ent + 0x98) == 5) return 0;               /* lo de antes */
    static int visto_arma, visto_granada;

    modelo = *(const char **)(ent + 0xa8);
    if ((unsigned int)ent == A_CL_VIEWENT) {               /* el arma      */
        if (!visto_arma) { visto_arma = 1; reg("sombra: sin sombra el arma en primera persona (%.40s)", modelo ? modelo : "?"); }
        return 0;
    }
    if (modelo && strstr(modelo, "grenade")) {             /* la granada   */
        if (!visto_granada) { visto_granada = 1; reg("sombra: sin sombra %.40s", modelo); }
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
        reg("AVISO: 0x%08x no es la comprobacion de la sombra, no se toca", A_SOMBRA_CMP);
        return;
    }
    if (!VirtualProtect(p, sizeof(orig), PAGE_EXECUTE_READWRITE, &viejo)) return;
    p[0] = 0xE9;
    *(int *)(p + 1) = (int)((unsigned int)sombra_puente - (A_SOMBRA_CMP + 5));
    for (i = 5; i < (int)sizeof(orig); i++) p[i] = 0x90;
    VirtualProtect(p, sizeof(orig), viejo, &viejo);
}

/* ================================================================== */
/*  entidades que no llegan por el PVS                                */
/* ================================================================== */
/*
 * La visibilidad precalculada de los mapas de la alpha tiene huecos
 * (apartado 24). r_novis arregla el dibujado del mundo, pero el servidor
 * sigue sin mandar las entidades cuyas hojas no salen en el PVS: una
 * puerta a 300 u delante no aparece hasta acercarte. Con UN jugador se
 * mandan todas: en SV_WriteEntitiesToClient (0x431394) se anula el salto
 * que descarta la entidad si ninguna de sus hojas es visible (0x43148b),
 * y el mensaje por fotograma de SV_SendClientDatagram (0x431dbf), que
 * era un bufer de pila de 1 KB, pasa a 4 KB (el loopback admite 8 KB).
 * En multijugador se deja el codigo original: por red un datagrama de mas
 * de 1 KB no vale. Se decide en cada mapa, con el servidor ya arrancado.
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
            reg("AVISO: bytes inesperados en 0x%08x, no se tocan las entidades", pvs_parches[i].va);
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
    reg("entidades: %s", activar ? "se mandan todas (un jugador, mensaje de 4 KB)"
                                 : "solo las del PVS (multijugador, codigo original)");
}

/* ================================================================== */
/*  instalacion                                                       */
/* ================================================================== */

static void instalar(void)
{
    unsigned int *tabla = (unsigned int *)A_TABLA_DLL;
    DWORD viejo;

    if (!VirtualProtect(tabla, 64 * 4, PAGE_READWRITE, &viejo)) {
        reg("no se pudo desproteger la tabla de la DLL");
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
        reg("AVISO: la entrada %d no era pfnChangeLevel (0x%08x)",
            IDX_CHANGELEVEL, (unsigned int)changelevel_original);
    VirtualProtect(tabla, 64 * 4, viejo, &viejo);

    if ((unsigned int)alloc_original != A_ALLOC_PRIVATE)
        reg("AVISO: la entrada 51 no era la esperada (0x%08x)",
            (unsigned int)alloc_original);

    /* arreglo del desbordamiento del bufer de guardado (ver motor.h) */
    if (*(unsigned int *)A_TAM_BUFFER_SAVE != 0x10000)
        reg("AVISO: el tamaño del bufer de save no era 0x10000");
    else if (escribir_dword(A_TAM_BUFFER_SAVE, TAM_BUFFER_NUEVO))
        reg("bufer de guardado ampliado de 64 KB a %u KB",
            TAM_BUFFER_NUEVO / 1024);

    if (!escribir_dword(A_PUSH_SAVE_HANDLER, (unsigned int)mi_save))
        reg("no se pudo repuntar el manejador de save");
    if (!escribir_dword(A_PUSH_LOAD_HANDLER, (unsigned int)mi_load))
        reg("no se pudo repuntar el manejador de load");
    if (*(unsigned int *)A_PUSH_MAP_HANDLER != A_HOST_MAP)
        reg("AVISO: el manejador de map no estaba donde se esperaba");
    else if (!escribir_dword(A_PUSH_MAP_HANDLER, (unsigned int)mi_map))
        reg("no se pudo repuntar el manejador de map");

    if (*(unsigned int *)A_PUSH_BEGIN_HANDLER != A_HOST_BEGIN)
        reg("AVISO: el manejador de begin no estaba donde se esperaba");
    else if (!escribir_dword(A_PUSH_BEGIN_HANDLER, (unsigned int)mi_begin))
        reg("no se pudo repuntar el manejador de begin");

    {
        unsigned char *p = (unsigned char *)A_SZ_CALL_ERROR;
        if (p[0] != 0xE8 ||
            *(unsigned int *)(p + 1) != A_SYS_ERROR - (A_SZ_CALL_ERROR + 5) ||
            !escribir_dword(A_SZ_CALL_ERROR + 1,
                            (unsigned int)sz_desborde - (A_SZ_CALL_ERROR + 5)))
            reg("AVISO: no se pudo enganchar el desborde de SZ_GetSpace");
    }

    if (*(unsigned int *)A_PUSH_CHANGELEVEL_HANDLER != A_HOST_CHANGELEVEL ||
        !escribir_dword(A_PUSH_CHANGELEVEL_HANDLER, (unsigned int)mi_changelevel))
        reg("AVISO: no se pudo enganchar changelevel");
    if (*(unsigned int *)A_PUSH_CHANGELEVEL2_HANDLER != A_HOST_CHANGELEVEL2 ||
        !escribir_dword(A_PUSH_CHANGELEVEL2_HANDLER, (unsigned int)mi_changelevel2))
        reg("AVISO: no se pudo enganchar changelevel2");

    if (!repuntar_jmp(A_JMP_SCAN_LOAD, A_M_SCANSAVES, mi_scan) ||
        !repuntar_jmp(A_JMP_SCAN_SAVE, A_M_SCANSAVES, mi_scan))
        reg("AVISO: no se pudo enganchar la lista del menu de partidas");

    enganchar_sombras();
    reg("instalado");
}

/*
 * Linea de ordenes por defecto, para que el .exe funcione con doble clic
 * sin ningun .bat. El motor la lee con GetCommandLineA (entrada de la IAT
 * en 0xd3f4b0), y esta DLL se carga antes del arranque del CRT, asi que
 * basta con cambiar esa entrada. Solo se anade lo que no venga ya; el
 * menu solo si no se pide ya arrancar algo (+map, +load, +togglemenu).
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
        reg("AVISO: la IAT en 0x%08x no es GetCommandLineA, sin linea por defecto",
            A_IAT_GETCMDLINE);
        return;
    }
    {
        /*
         * Los valores por defecto van ANTES que los argumentos del usuario:
         * los "+" se ejecutan en orden, y "+maxplayers 1" detras de un
         * "+map" llegaba tarde (el mapa arrancaba en multijugador, donde
         * los cambios de nivel no funcionan). El +togglemenu y el
         * +sizedown, al final.
         */
        const char *orig = GetCommandLineA(), *resto = orig;
        char args[1024];
        int n;

        if (*resto == '"') { resto++; while (*resto && *resto != '"') resto++; if (*resto) resto++; }
        else while (*resto && *resto != ' ') resto++;
        n = (int)(resto - orig);
        if (n >= (int)sizeof(linea_ordenes) - 1) n = (int)sizeof(linea_ordenes) - 2;
        lstrcpynA(args, resto, sizeof(args));
        lstrcpynA(linea_ordenes, orig, n + 1);                 /* el ejecutable */

        #define DEFECTO(clave, texto) \
            if (!strstr(args, clave)) { lstrcatA(linea_ordenes, " "); lstrcatA(linea_ordenes, texto); }
        DEFECTO("-heapsize", "-heapsize 524288")
        DEFECTO("-condebug", "-condebug")
        if (!strstr(args, "-width") && !strstr(args, "-height")) {
            /* la resolucion del escritorio (antes 800x600 fijo) */
            /* el modo real de la pantalla: GetSystemMetrics da la
               resolucion escalada si Windows escala por encima del 100 % */
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
        DEFECTO("r_novis", "+r_novis 1")           /* apartado 24 */
        DEFECTO("r_shadows", "+r_shadows 0")       /* apartado 37 */
        DEFECTO("crosshair", "+crosshair 1")       /* el config.cfg del CD la trae a 0 */
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
    reg("linea de ordenes: %s", linea_ordenes);
}

/* lo llama DllMain (cargador.c) cuando el motor ya esta verificado y parcheado */
void hlalpha_iniciar(void)
{
    preparar_linea_ordenes();
    instalar();
}
