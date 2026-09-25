/* GENERADO por tools/generar_winmm.js: no editar a mano.
 * Un stub por exportacion de la winmm.dll del sistema: la primera vez
 * resuelve la funcion real (winmm_resolver, en cargador.c) y salta a ella. */

#define WINMM_N 193
void *winmm_real[WINMM_N];
const char *const winmm_nombre[WINMM_N] = {
    0, /* solo ordinal 2 */
    "mciExecute",
    "CloseDriver",
    "DefDriverProc",
    "DriverCallback",
    "DrvGetModuleHandle",
    "GetDriverModuleHandle",
    "NotifyCallbackData",
    "OpenDriver",
    "PlaySound",
    "PlaySoundA",
    "PlaySoundW",
    "SendDriverMessage",
    "WOW32DriverCallback",
    "WOW32ResolveMultiMediaHandle",
    "WOWAppExit",
    "aux32Message",
    "auxGetDevCapsA",
    "auxGetDevCapsW",
    "auxGetNumDevs",
    "auxGetVolume",
    "auxOutMessage",
    "auxSetVolume",
    "joy32Message",
    "joyConfigChanged",
    "joyGetDevCapsA",
    "joyGetDevCapsW",
    "joyGetNumDevs",
    "joyGetPos",
    "joyGetPosEx",
    "joyGetThreshold",
    "joyReleaseCapture",
    "joySetCapture",
    "joySetThreshold",
    "mci32Message",
    "mciDriverNotify",
    "mciDriverYield",
    "mciFreeCommandResource",
    "mciGetCreatorTask",
    "mciGetDeviceIDA",
    "mciGetDeviceIDFromElementIDA",
    "mciGetDeviceIDFromElementIDW",
    "mciGetDeviceIDW",
    "mciGetDriverData",
    "mciGetErrorStringA",
    "mciGetErrorStringW",
    "mciGetYieldProc",
    "mciLoadCommandResource",
    "mciSendCommandA",
    "mciSendCommandW",
    "mciSendStringA",
    "mciSendStringW",
    "mciSetDriverData",
    "mciSetYieldProc",
    "mid32Message",
    "midiConnect",
    "midiDisconnect",
    "midiInAddBuffer",
    "midiInClose",
    "midiInGetDevCapsA",
    "midiInGetDevCapsW",
    "midiInGetErrorTextA",
    "midiInGetErrorTextW",
    "midiInGetID",
    "midiInGetNumDevs",
    "midiInMessage",
    "midiInOpen",
    "midiInPrepareHeader",
    "midiInReset",
    "midiInStart",
    "midiInStop",
    "midiInUnprepareHeader",
    "midiOutCacheDrumPatches",
    "midiOutCachePatches",
    "midiOutClose",
    "midiOutGetDevCapsA",
    "midiOutGetDevCapsW",
    "midiOutGetErrorTextA",
    "midiOutGetErrorTextW",
    "midiOutGetID",
    "midiOutGetNumDevs",
    "midiOutGetVolume",
    "midiOutLongMsg",
    "midiOutMessage",
    "midiOutOpen",
    "midiOutPrepareHeader",
    "midiOutReset",
    "midiOutSetVolume",
    "midiOutShortMsg",
    "midiOutUnprepareHeader",
    "midiStreamClose",
    "midiStreamOpen",
    "midiStreamOut",
    "midiStreamPause",
    "midiStreamPosition",
    "midiStreamProperty",
    "midiStreamRestart",
    "midiStreamStop",
    "mixerClose",
    "mixerGetControlDetailsA",
    "mixerGetControlDetailsW",
    "mixerGetDevCapsA",
    "mixerGetDevCapsW",
    "mixerGetID",
    "mixerGetLineControlsA",
    "mixerGetLineControlsW",
    "mixerGetLineInfoA",
    "mixerGetLineInfoW",
    "mixerGetNumDevs",
    "mixerMessage",
    "mixerOpen",
    "mixerSetControlDetails",
    "mmDrvInstall",
    "mmGetCurrentTask",
    "mmTaskBlock",
    "mmTaskCreate",
    "mmTaskSignal",
    "mmTaskYield",
    "mmioAdvance",
    "mmioAscend",
    "mmioClose",
    "mmioCreateChunk",
    "mmioDescend",
    "mmioFlush",
    "mmioGetInfo",
    "mmioInstallIOProcA",
    "mmioInstallIOProcW",
    "mmioOpenA",
    "mmioOpenW",
    "mmioRead",
    "mmioRenameA",
    "mmioRenameW",
    "mmioSeek",
    "mmioSendMessage",
    "mmioSetBuffer",
    "mmioSetInfo",
    "mmioStringToFOURCCA",
    "mmioStringToFOURCCW",
    "mmioWrite",
    "mmsystemGetVersion",
    "mod32Message",
    "mxd32Message",
    "sndPlaySoundA",
    "sndPlaySoundW",
    "tid32Message",
    "timeBeginPeriod",
    "timeEndPeriod",
    "timeGetDevCaps",
    "timeGetSystemTime",
    "timeGetTime",
    "timeKillEvent",
    "timeSetEvent",
    "waveInAddBuffer",
    "waveInClose",
    "waveInGetDevCapsA",
    "waveInGetDevCapsW",
    "waveInGetErrorTextA",
    "waveInGetErrorTextW",
    "waveInGetID",
    "waveInGetNumDevs",
    "waveInGetPosition",
    "waveInMessage",
    "waveInOpen",
    "waveInPrepareHeader",
    "waveInReset",
    "waveInStart",
    "waveInStop",
    "waveInUnprepareHeader",
    "waveOutBreakLoop",
    "waveOutClose",
    "waveOutGetDevCapsA",
    "waveOutGetDevCapsW",
    "waveOutGetErrorTextA",
    "waveOutGetErrorTextW",
    "waveOutGetID",
    "waveOutGetNumDevs",
    "waveOutGetPitch",
    "waveOutGetPlaybackRate",
    "waveOutGetPosition",
    "waveOutGetVolume",
    "waveOutMessage",
    "waveOutOpen",
    "waveOutPause",
    "waveOutPrepareHeader",
    "waveOutReset",
    "waveOutRestart",
    "waveOutSetPitch",
    "waveOutSetPlaybackRate",
    "waveOutSetVolume",
    "waveOutUnprepareHeader",
    "waveOutWrite",
    "wid32Message",
    "wod32Message",
};
const unsigned short winmm_ordinal[WINMM_N] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194};

__asm__(
    ".globl _p_winmm_0\n_p_winmm_0:\n"
    "    movl _winmm_real+0, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $0\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_1\n_p_winmm_1:\n"
    "    movl _winmm_real+4, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $1\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_2\n_p_winmm_2:\n"
    "    movl _winmm_real+8, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $2\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_3\n_p_winmm_3:\n"
    "    movl _winmm_real+12, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $3\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_4\n_p_winmm_4:\n"
    "    movl _winmm_real+16, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $4\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_5\n_p_winmm_5:\n"
    "    movl _winmm_real+20, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $5\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_6\n_p_winmm_6:\n"
    "    movl _winmm_real+24, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $6\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_7\n_p_winmm_7:\n"
    "    movl _winmm_real+28, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $7\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_8\n_p_winmm_8:\n"
    "    movl _winmm_real+32, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $8\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_9\n_p_winmm_9:\n"
    "    movl _winmm_real+36, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $9\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_10\n_p_winmm_10:\n"
    "    movl _winmm_real+40, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $10\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_11\n_p_winmm_11:\n"
    "    movl _winmm_real+44, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $11\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_12\n_p_winmm_12:\n"
    "    movl _winmm_real+48, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $12\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_13\n_p_winmm_13:\n"
    "    movl _winmm_real+52, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $13\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_14\n_p_winmm_14:\n"
    "    movl _winmm_real+56, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $14\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_15\n_p_winmm_15:\n"
    "    movl _winmm_real+60, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $15\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_16\n_p_winmm_16:\n"
    "    movl _winmm_real+64, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $16\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_17\n_p_winmm_17:\n"
    "    movl _winmm_real+68, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $17\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_18\n_p_winmm_18:\n"
    "    movl _winmm_real+72, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $18\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_19\n_p_winmm_19:\n"
    "    movl _winmm_real+76, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $19\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_20\n_p_winmm_20:\n"
    "    movl _winmm_real+80, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $20\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_21\n_p_winmm_21:\n"
    "    movl _winmm_real+84, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $21\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_22\n_p_winmm_22:\n"
    "    movl _winmm_real+88, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $22\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_23\n_p_winmm_23:\n"
    "    movl _winmm_real+92, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $23\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_24\n_p_winmm_24:\n"
    "    movl _winmm_real+96, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $24\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_25\n_p_winmm_25:\n"
    "    movl _winmm_real+100, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $25\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_26\n_p_winmm_26:\n"
    "    movl _winmm_real+104, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $26\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_27\n_p_winmm_27:\n"
    "    movl _winmm_real+108, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $27\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_28\n_p_winmm_28:\n"
    "    movl _winmm_real+112, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $28\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_29\n_p_winmm_29:\n"
    "    movl _winmm_real+116, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $29\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_30\n_p_winmm_30:\n"
    "    movl _winmm_real+120, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $30\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_31\n_p_winmm_31:\n"
    "    movl _winmm_real+124, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $31\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_32\n_p_winmm_32:\n"
    "    movl _winmm_real+128, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $32\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_33\n_p_winmm_33:\n"
    "    movl _winmm_real+132, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $33\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_34\n_p_winmm_34:\n"
    "    movl _winmm_real+136, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $34\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_35\n_p_winmm_35:\n"
    "    movl _winmm_real+140, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $35\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_36\n_p_winmm_36:\n"
    "    movl _winmm_real+144, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $36\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_37\n_p_winmm_37:\n"
    "    movl _winmm_real+148, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $37\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_38\n_p_winmm_38:\n"
    "    movl _winmm_real+152, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $38\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_39\n_p_winmm_39:\n"
    "    movl _winmm_real+156, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $39\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_40\n_p_winmm_40:\n"
    "    movl _winmm_real+160, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $40\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_41\n_p_winmm_41:\n"
    "    movl _winmm_real+164, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $41\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_42\n_p_winmm_42:\n"
    "    movl _winmm_real+168, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $42\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_43\n_p_winmm_43:\n"
    "    movl _winmm_real+172, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $43\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_44\n_p_winmm_44:\n"
    "    movl _winmm_real+176, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $44\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_45\n_p_winmm_45:\n"
    "    movl _winmm_real+180, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $45\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_46\n_p_winmm_46:\n"
    "    movl _winmm_real+184, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $46\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_47\n_p_winmm_47:\n"
    "    movl _winmm_real+188, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $47\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_48\n_p_winmm_48:\n"
    "    movl _winmm_real+192, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $48\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_49\n_p_winmm_49:\n"
    "    movl _winmm_real+196, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $49\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_50\n_p_winmm_50:\n"
    "    movl _winmm_real+200, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $50\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_51\n_p_winmm_51:\n"
    "    movl _winmm_real+204, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $51\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_52\n_p_winmm_52:\n"
    "    movl _winmm_real+208, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $52\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_53\n_p_winmm_53:\n"
    "    movl _winmm_real+212, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $53\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_54\n_p_winmm_54:\n"
    "    movl _winmm_real+216, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $54\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_55\n_p_winmm_55:\n"
    "    movl _winmm_real+220, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $55\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_56\n_p_winmm_56:\n"
    "    movl _winmm_real+224, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $56\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_57\n_p_winmm_57:\n"
    "    movl _winmm_real+228, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $57\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_58\n_p_winmm_58:\n"
    "    movl _winmm_real+232, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $58\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_59\n_p_winmm_59:\n"
    "    movl _winmm_real+236, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $59\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_60\n_p_winmm_60:\n"
    "    movl _winmm_real+240, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $60\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_61\n_p_winmm_61:\n"
    "    movl _winmm_real+244, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $61\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_62\n_p_winmm_62:\n"
    "    movl _winmm_real+248, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $62\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_63\n_p_winmm_63:\n"
    "    movl _winmm_real+252, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $63\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_64\n_p_winmm_64:\n"
    "    movl _winmm_real+256, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $64\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_65\n_p_winmm_65:\n"
    "    movl _winmm_real+260, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $65\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_66\n_p_winmm_66:\n"
    "    movl _winmm_real+264, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $66\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_67\n_p_winmm_67:\n"
    "    movl _winmm_real+268, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $67\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_68\n_p_winmm_68:\n"
    "    movl _winmm_real+272, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $68\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_69\n_p_winmm_69:\n"
    "    movl _winmm_real+276, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $69\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_70\n_p_winmm_70:\n"
    "    movl _winmm_real+280, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $70\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_71\n_p_winmm_71:\n"
    "    movl _winmm_real+284, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $71\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_72\n_p_winmm_72:\n"
    "    movl _winmm_real+288, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $72\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_73\n_p_winmm_73:\n"
    "    movl _winmm_real+292, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $73\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_74\n_p_winmm_74:\n"
    "    movl _winmm_real+296, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $74\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_75\n_p_winmm_75:\n"
    "    movl _winmm_real+300, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $75\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_76\n_p_winmm_76:\n"
    "    movl _winmm_real+304, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $76\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_77\n_p_winmm_77:\n"
    "    movl _winmm_real+308, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $77\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_78\n_p_winmm_78:\n"
    "    movl _winmm_real+312, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $78\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_79\n_p_winmm_79:\n"
    "    movl _winmm_real+316, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $79\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_80\n_p_winmm_80:\n"
    "    movl _winmm_real+320, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $80\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_81\n_p_winmm_81:\n"
    "    movl _winmm_real+324, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $81\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_82\n_p_winmm_82:\n"
    "    movl _winmm_real+328, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $82\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_83\n_p_winmm_83:\n"
    "    movl _winmm_real+332, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $83\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_84\n_p_winmm_84:\n"
    "    movl _winmm_real+336, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $84\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_85\n_p_winmm_85:\n"
    "    movl _winmm_real+340, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $85\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_86\n_p_winmm_86:\n"
    "    movl _winmm_real+344, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $86\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_87\n_p_winmm_87:\n"
    "    movl _winmm_real+348, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $87\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_88\n_p_winmm_88:\n"
    "    movl _winmm_real+352, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $88\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_89\n_p_winmm_89:\n"
    "    movl _winmm_real+356, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $89\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_90\n_p_winmm_90:\n"
    "    movl _winmm_real+360, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $90\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_91\n_p_winmm_91:\n"
    "    movl _winmm_real+364, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $91\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_92\n_p_winmm_92:\n"
    "    movl _winmm_real+368, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $92\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_93\n_p_winmm_93:\n"
    "    movl _winmm_real+372, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $93\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_94\n_p_winmm_94:\n"
    "    movl _winmm_real+376, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $94\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_95\n_p_winmm_95:\n"
    "    movl _winmm_real+380, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $95\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_96\n_p_winmm_96:\n"
    "    movl _winmm_real+384, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $96\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_97\n_p_winmm_97:\n"
    "    movl _winmm_real+388, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $97\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_98\n_p_winmm_98:\n"
    "    movl _winmm_real+392, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $98\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_99\n_p_winmm_99:\n"
    "    movl _winmm_real+396, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $99\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_100\n_p_winmm_100:\n"
    "    movl _winmm_real+400, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $100\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_101\n_p_winmm_101:\n"
    "    movl _winmm_real+404, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $101\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_102\n_p_winmm_102:\n"
    "    movl _winmm_real+408, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $102\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_103\n_p_winmm_103:\n"
    "    movl _winmm_real+412, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $103\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_104\n_p_winmm_104:\n"
    "    movl _winmm_real+416, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $104\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_105\n_p_winmm_105:\n"
    "    movl _winmm_real+420, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $105\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_106\n_p_winmm_106:\n"
    "    movl _winmm_real+424, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $106\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_107\n_p_winmm_107:\n"
    "    movl _winmm_real+428, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $107\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_108\n_p_winmm_108:\n"
    "    movl _winmm_real+432, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $108\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_109\n_p_winmm_109:\n"
    "    movl _winmm_real+436, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $109\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_110\n_p_winmm_110:\n"
    "    movl _winmm_real+440, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $110\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_111\n_p_winmm_111:\n"
    "    movl _winmm_real+444, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $111\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_112\n_p_winmm_112:\n"
    "    movl _winmm_real+448, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $112\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_113\n_p_winmm_113:\n"
    "    movl _winmm_real+452, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $113\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_114\n_p_winmm_114:\n"
    "    movl _winmm_real+456, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $114\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_115\n_p_winmm_115:\n"
    "    movl _winmm_real+460, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $115\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_116\n_p_winmm_116:\n"
    "    movl _winmm_real+464, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $116\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_117\n_p_winmm_117:\n"
    "    movl _winmm_real+468, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $117\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_118\n_p_winmm_118:\n"
    "    movl _winmm_real+472, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $118\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_119\n_p_winmm_119:\n"
    "    movl _winmm_real+476, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $119\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_120\n_p_winmm_120:\n"
    "    movl _winmm_real+480, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $120\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_121\n_p_winmm_121:\n"
    "    movl _winmm_real+484, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $121\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_122\n_p_winmm_122:\n"
    "    movl _winmm_real+488, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $122\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_123\n_p_winmm_123:\n"
    "    movl _winmm_real+492, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $123\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_124\n_p_winmm_124:\n"
    "    movl _winmm_real+496, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $124\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_125\n_p_winmm_125:\n"
    "    movl _winmm_real+500, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $125\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_126\n_p_winmm_126:\n"
    "    movl _winmm_real+504, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $126\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_127\n_p_winmm_127:\n"
    "    movl _winmm_real+508, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $127\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_128\n_p_winmm_128:\n"
    "    movl _winmm_real+512, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $128\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_129\n_p_winmm_129:\n"
    "    movl _winmm_real+516, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $129\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_130\n_p_winmm_130:\n"
    "    movl _winmm_real+520, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $130\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_131\n_p_winmm_131:\n"
    "    movl _winmm_real+524, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $131\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_132\n_p_winmm_132:\n"
    "    movl _winmm_real+528, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $132\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_133\n_p_winmm_133:\n"
    "    movl _winmm_real+532, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $133\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_134\n_p_winmm_134:\n"
    "    movl _winmm_real+536, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $134\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_135\n_p_winmm_135:\n"
    "    movl _winmm_real+540, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $135\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_136\n_p_winmm_136:\n"
    "    movl _winmm_real+544, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $136\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_137\n_p_winmm_137:\n"
    "    movl _winmm_real+548, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $137\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_138\n_p_winmm_138:\n"
    "    movl _winmm_real+552, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $138\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_139\n_p_winmm_139:\n"
    "    movl _winmm_real+556, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $139\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_140\n_p_winmm_140:\n"
    "    movl _winmm_real+560, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $140\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_141\n_p_winmm_141:\n"
    "    movl _winmm_real+564, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $141\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_142\n_p_winmm_142:\n"
    "    movl _winmm_real+568, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $142\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_143\n_p_winmm_143:\n"
    "    movl _winmm_real+572, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $143\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_144\n_p_winmm_144:\n"
    "    movl _winmm_real+576, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $144\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_145\n_p_winmm_145:\n"
    "    movl _winmm_real+580, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $145\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_146\n_p_winmm_146:\n"
    "    movl _winmm_real+584, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $146\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_147\n_p_winmm_147:\n"
    "    movl _winmm_real+588, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $147\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_148\n_p_winmm_148:\n"
    "    movl _winmm_real+592, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $148\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_149\n_p_winmm_149:\n"
    "    movl _winmm_real+596, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $149\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_150\n_p_winmm_150:\n"
    "    movl _winmm_real+600, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $150\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_151\n_p_winmm_151:\n"
    "    movl _winmm_real+604, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $151\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_152\n_p_winmm_152:\n"
    "    movl _winmm_real+608, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $152\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_153\n_p_winmm_153:\n"
    "    movl _winmm_real+612, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $153\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_154\n_p_winmm_154:\n"
    "    movl _winmm_real+616, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $154\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_155\n_p_winmm_155:\n"
    "    movl _winmm_real+620, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $155\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_156\n_p_winmm_156:\n"
    "    movl _winmm_real+624, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $156\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_157\n_p_winmm_157:\n"
    "    movl _winmm_real+628, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $157\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_158\n_p_winmm_158:\n"
    "    movl _winmm_real+632, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $158\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_159\n_p_winmm_159:\n"
    "    movl _winmm_real+636, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $159\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_160\n_p_winmm_160:\n"
    "    movl _winmm_real+640, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $160\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_161\n_p_winmm_161:\n"
    "    movl _winmm_real+644, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $161\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_162\n_p_winmm_162:\n"
    "    movl _winmm_real+648, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $162\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_163\n_p_winmm_163:\n"
    "    movl _winmm_real+652, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $163\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_164\n_p_winmm_164:\n"
    "    movl _winmm_real+656, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $164\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_165\n_p_winmm_165:\n"
    "    movl _winmm_real+660, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $165\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_166\n_p_winmm_166:\n"
    "    movl _winmm_real+664, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $166\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_167\n_p_winmm_167:\n"
    "    movl _winmm_real+668, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $167\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_168\n_p_winmm_168:\n"
    "    movl _winmm_real+672, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $168\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_169\n_p_winmm_169:\n"
    "    movl _winmm_real+676, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $169\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_170\n_p_winmm_170:\n"
    "    movl _winmm_real+680, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $170\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_171\n_p_winmm_171:\n"
    "    movl _winmm_real+684, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $171\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_172\n_p_winmm_172:\n"
    "    movl _winmm_real+688, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $172\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_173\n_p_winmm_173:\n"
    "    movl _winmm_real+692, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $173\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_174\n_p_winmm_174:\n"
    "    movl _winmm_real+696, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $174\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_175\n_p_winmm_175:\n"
    "    movl _winmm_real+700, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $175\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_176\n_p_winmm_176:\n"
    "    movl _winmm_real+704, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $176\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_177\n_p_winmm_177:\n"
    "    movl _winmm_real+708, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $177\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_178\n_p_winmm_178:\n"
    "    movl _winmm_real+712, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $178\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_179\n_p_winmm_179:\n"
    "    movl _winmm_real+716, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $179\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_180\n_p_winmm_180:\n"
    "    movl _winmm_real+720, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $180\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_181\n_p_winmm_181:\n"
    "    movl _winmm_real+724, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $181\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_182\n_p_winmm_182:\n"
    "    movl _winmm_real+728, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $182\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_183\n_p_winmm_183:\n"
    "    movl _winmm_real+732, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $183\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_184\n_p_winmm_184:\n"
    "    movl _winmm_real+736, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $184\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_185\n_p_winmm_185:\n"
    "    movl _winmm_real+740, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $185\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_186\n_p_winmm_186:\n"
    "    movl _winmm_real+744, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $186\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_187\n_p_winmm_187:\n"
    "    movl _winmm_real+748, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $187\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_188\n_p_winmm_188:\n"
    "    movl _winmm_real+752, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $188\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_189\n_p_winmm_189:\n"
    "    movl _winmm_real+756, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $189\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_190\n_p_winmm_190:\n"
    "    movl _winmm_real+760, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $190\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_191\n_p_winmm_191:\n"
    "    movl _winmm_real+764, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $191\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
    ".globl _p_winmm_192\n_p_winmm_192:\n"
    "    movl _winmm_real+768, %eax\n    testl %eax, %eax\n    jnz 1f\n"
    "    pushl $192\n    call _winmm_resolver\n    addl $4, %esp\n"
    "1:  jmp *%eax\n"
);
