/*
 * loader.c -- winmm.dll for the fixed Half-Life Alpha 0.52.
 *
 * The engine (enginegl.exe) imports WINMM.dll and Windows looks for it
 * first in the game folder: this DLL is loaded before the engine starts,
 * forwards everything it is asked for to the system winmm.dll (stubs in
 * winmm_stubs.c) and applies the fixes IN MEMORY, without touching any
 * Valve file:
 *   - checks by hash that the .exe is the original 0.52 enginegl.exe
 *     (if it is not, it touches nothing: it just acts as winmm);
 *   - applies the engine patches (patches.h, new bytes only);
 *   - hooks LoadLibraryA to patch hl.dll as soon as it is loaded
 *     (also checking its hash);
 *   - starts the rest (hlalpha.c): save and load, map persistence,
 *     default command line, etc.
 */
#include <windows.h>
#include <string.h>
#include "patches.h"

#define A_IAT_LOADLIBRARYA 0xD3F52C    /* enginegl.exe IAT */
#define HASH_ENGINEGL_CD   0x8E8BBD4989F8D1FFULL   /* enginegl.exe from the alpha CD */

void reg(const char *fmt, ...);
void hlalpha_iniciar(void);

/* ---------------- forwarding to the system winmm.dll ---------------- */

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
            MessageBoxA(NULL, "Could not load the system winmm.dll.",
                        "Half-Life Alpha", MB_ICONERROR);
            ExitProcess(1);
        }
    }
    if (!winmm_real[i])
        winmm_real[i] = (void *)GetProcAddress(winmm_sistema,
            winmm_nombre[i] ? winmm_nombre[i] : MAKEINTRESOURCEA(winmm_ordinal[i]));
    return winmm_real[i];
}

/* ---------------- identifying Valve's binaries ---------------- */

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

/* applies a list of patches in memory; base_real - base_pref is how far
   the module moved from its preferred base */
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

/* ---------------- hl.dll: patched when it loads ---------------- */

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
                reg("original hl.dll loaded at 0x%08x: %d patches applied in memory",
                    (unsigned int)m, aplicar(parches_hl, (unsigned int)m, BASE_HL_DLL));
            else
                reg("WARNING: %s is not the original alpha 0.52 hl.dll, not patching", nombre);
        }
    }
    return m;
}

/* ---------------- startup ---------------- */

BOOL WINAPI DllMain(HINSTANCE inst, DWORD motivo, LPVOID reservado)
{
    (void)reservado;
    if (motivo != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(inst);

    reg("=== hl-alpha-052-fixes winmm.dll loaded ===");
    /*
     * Two enginegl.exe with the same code: the one from the alpha CD and a
     * copy that circulates with the LARGE_ADDRESS_AWARE bit set in the PE
     * header (Characteristics 0x12a instead of 0x10a, and the checksum
     * recomputed). Only those 4 header bytes change, so the patches apply
     * just the same.
     */
    {
        unsigned long long h = hash_modulo(GetModuleHandleA(NULL));
        if (h != HASH_ENGINEGL && h != HASH_ENGINEGL_CD) {
            reg("the executable is not the original alpha 0.52 enginegl.exe: touching nothing");
            return TRUE;
        }
        reg("original enginegl.exe (%s)", h == HASH_ENGINEGL_CD ? "the CD one" :
            "copy with LARGE_ADDRESS_AWARE");
    }
    reg("%d engine patches applied in memory",
        aplicar(parches_exe, (unsigned int)GetModuleHandleA(NULL), 0x400000));

    {   /* hook of LoadLibraryA in the engine's IAT */
        unsigned int *iat = (unsigned int *)A_IAT_LOADLIBRARYA;
        FARPROC real = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
        DWORD viejo;
        if (*iat == (unsigned int)real && VirtualProtect(iat, 4, PAGE_READWRITE, &viejo)) {
            loadlibrary_real = (fn_loadlib_t)real;
            *iat = (unsigned int)mi_LoadLibraryA;
            VirtualProtect(iat, 4, viejo, &viejo);
        } else {
            reg("WARNING: the LoadLibraryA IAT entry is not the expected one; hl.dll will not be patched");
        }
    }

    hlalpha_iniciar();
    return TRUE;
}
