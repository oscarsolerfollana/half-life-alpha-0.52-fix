/*
 * cargador.c -- winmm.dll de la Half-Life Alpha 0.52 corregida.
 *
 * El motor (enginegl.exe) importa WINMM.dll y Windows la busca primero en
 * la carpeta del juego: esta DLL se carga antes de que arranque el motor,
 * reenvia a la winmm.dll del sistema todo lo que se le pide (stubs de
 * winmm_stubs.c) y aplica los arreglos EN MEMORIA, sin tocar ningun
 * fichero de Valve:
 *   - comprueba por hash que el .exe es el enginegl.exe original de la 0.52
 *     (si no lo es, no toca nada: solo hace de winmm);
 *   - aplica los parches del motor (parches.h, solo bytes nuevos);
 *   - se engancha a LoadLibraryA para parchear hl.dll en cuanto se carga
 *     (tambien comprobando su hash);
 *   - arranca el resto (hlalpha.c): guardar y cargar, persistencia de
 *     mapas, linea de ordenes por defecto, etc.
 */
#include <windows.h>
#include <string.h>
#include "parches.h"

#define A_IAT_LOADLIBRARYA 0xD3F52C    /* IAT de enginegl.exe */
#define HASH_ENGINEGL_CD   0x8E8BBD4989F8D1FFULL   /* enginegl.exe del CD de la alpha */

void reg(const char *fmt, ...);
void hlalpha_iniciar(void);

/* ---------------- reenvio a la winmm.dll del sistema ---------------- */

#define WINMM_N_EXTERNO 193
extern void *winmm_real[];
extern const char *const winmm_nombre[];
extern const unsigned short winmm_ordinal[];

static HMODULE winmm_sistema;

void *__cdecl winmm_resolver(int i)
{
    if (!winmm_sistema) {
        char ruta[MAX_PATH];
        UINT n = GetSystemDirectoryA(ruta, MAX_PATH - 12);
        lstrcpyA(ruta + n, "\\winmm.dll");
        winmm_sistema = LoadLibraryA(ruta);
        if (!winmm_sistema) {
            MessageBoxA(NULL, "No se pudo cargar la winmm.dll del sistema.",
                        "Half-Life Alpha", MB_ICONERROR);
            ExitProcess(1);
        }
    }
    if (!winmm_real[i])
        winmm_real[i] = (void *)GetProcAddress(winmm_sistema,
            winmm_nombre[i] ? winmm_nombre[i] : MAKEINTRESOURCEA(winmm_ordinal[i]));
    return winmm_real[i];
}

/* ---------------- identificar los binarios de Valve ---------------- */

static unsigned long long hash_fichero(const char *ruta)
{
    unsigned long long h = 0xCBF29CE484222325ULL;
    unsigned char buf[65536];
    DWORD leidos, i;
    HANDLE f = CreateFileA(ruta, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    while (ReadFile(f, buf, sizeof(buf), &leidos, NULL) && leidos)
        for (i = 0; i < leidos; i++) { h ^= buf[i]; h *= 0x100000001B3ULL; }
    CloseHandle(f);
    return h;
}

static unsigned long long hash_modulo(HMODULE m)
{
    char ruta[MAX_PATH];
    if (!GetModuleFileNameA(m, ruta, MAX_PATH)) return 0;
    return hash_fichero(ruta);
}

/* aplica una lista de parches en memoria; base_real - base_pref es cuanto
   se movio el modulo respecto a su base preferida */
static int aplicar(const parche_t *p, unsigned int base_real, unsigned int base_pref)
{
    int n = 0;
    DWORD viejo;
    for (; p->n; p++) {
        unsigned char *dst = (unsigned char *)(p->va - base_pref + base_real);
        if (!VirtualProtect(dst, p->n, PAGE_EXECUTE_READWRITE, &viejo)) continue;
        memcpy(dst, p->b, p->n);
        VirtualProtect(dst, p->n, viejo, &viejo);
        n++;
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    return n;
}

/* ---------------- hl.dll: se parchea al cargarse ---------------- */

typedef HMODULE (WINAPI *fn_loadlib_t)(LPCSTR);
static fn_loadlib_t loadlibrary_real;

static HMODULE WINAPI mi_LoadLibraryA(LPCSTR nombre)
{
    HMODULE m = loadlibrary_real(nombre);
    if (m && nombre) {
        int n = lstrlenA(nombre);
        if (n >= 6 && lstrcmpiA(nombre + n - 6, "hl.dll") == 0 &&
            (n == 6 || nombre[n - 7] == '\\' || nombre[n - 7] == '/')) {
            if (hash_modulo(m) == HASH_HL_DLL)
                reg("hl.dll original cargada en 0x%08x: %d parches aplicados en memoria",
                    (unsigned int)m, aplicar(parches_hl, (unsigned int)m, BASE_HL_DLL));
            else
                reg("AVISO: %s no es el hl.dll original de la alpha 0.52, no se parchea", nombre);
        }
    }
    return m;
}

/* ---------------- arranque ---------------- */

BOOL WINAPI DllMain(HINSTANCE inst, DWORD motivo, LPVOID reservado)
{
    (void)reservado;
    if (motivo != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(inst);

    reg("=== winmm.dll de hl-alpha-052-fixes cargada ===");
    /*
     * Dos enginegl.exe con el mismo codigo: el del CD de la alpha y una copia
     * que circula con el bit LARGE_ADDRESS_AWARE activado en la cabecera PE
     * (Characteristics 0x12a en vez de 0x10a, y la suma recalculada). Solo
     * cambian esos 4 bytes de cabecera, asi que los parches valen igual.
     */
    {
        unsigned long long h = hash_modulo(GetModuleHandleA(NULL));
        if (h != HASH_ENGINEGL && h != HASH_ENGINEGL_CD) {
            reg("el ejecutable no es el enginegl.exe original de la alpha 0.52: no se toca nada");
            return TRUE;
        }
        reg("enginegl.exe original (%s)", h == HASH_ENGINEGL_CD ? "el del CD" :
            "copia con LARGE_ADDRESS_AWARE");
    }
    reg("%d parches del motor aplicados en memoria",
        aplicar(parches_exe, (unsigned int)GetModuleHandleA(NULL), 0x400000));

    {   /* enganche de LoadLibraryA en la IAT del motor */
        unsigned int *iat = (unsigned int *)A_IAT_LOADLIBRARYA;
        FARPROC real = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
        DWORD viejo;
        if (*iat == (unsigned int)real && VirtualProtect(iat, 4, PAGE_READWRITE, &viejo)) {
            loadlibrary_real = (fn_loadlib_t)real;
            *iat = (unsigned int)mi_LoadLibraryA;
            VirtualProtect(iat, 4, viejo, &viejo);
        } else {
            reg("AVISO: la IAT de LoadLibraryA no es la esperada; hl.dll no se parcheara");
        }
    }

    hlalpha_iniciar();
    return TRUE;
}
