# hl-alpha-052-fixes

Arreglos no oficiales para la **Half-Life Alpha 0.52** (1997): guardar y cargar partida
completos, persistencia de los mapas entre transiciones, corrección de crashes del motor,
puntería, IA, controles y gráficos. La lista completa está en [`docs/ARREGLOS.md`](docs/ARREGLOS.md)
y el detalle técnico (direcciones, desensamblado, pruebas) en [`docs/NOTAS.txt`](docs/NOTAS.txt).

**Proyecto no oficial, sin relación con Valve. No incluye ningún fichero de Valve:** hace falta
una copia propia de la Half-Life Alpha 0.52. Nada de la alpha se modifica en disco; todos los
cambios se aplican en memoria al arrancar.

## Instalación

1. Copia la carpeta de la alpha 0.52 donde quieras (la que tiene `enginegl.exe` y `valve\`).
2. Copia `winmm.dll` en esa carpeta, junto a `enginegl.exe`, y `valve\autoexec.cfg` en su
   `valve\` (sustituye al del CD). Los dos vienen en el zip de
   [Releases](../../releases); el `autoexec.cfg` también está en este repo.
3. Aparta el `Opengl32.dll` que trae la alpha (renómbralo, p. ej. `Opengl32-3dfx.dll`). Es el
   MiniGL de 3Dfx y hace que el juego use la emulación de Glide, mucho más lenta; sin él se usa
   el OpenGL de la tarjeta.
4. Abre `enginegl.exe` con doble clic. Arranca en el menú, a la resolución del escritorio.

Para desinstalar, borra `winmm.dll`.

### Controles (`valve\autoexec.cfg`)

El `autoexec.cfg` del CD deja el ratón sin apuntar (arriba/abajo hace andar) y las teclas F con
órdenes de pruebas de Valve. El de este repo trae unos controles actuales:

| Tecla | Acción | Tecla | Acción |
|---|---|---|---|
| W A S D / flechas | moverse | ratón | apuntar |
| Botón izq. / der. | disparo / disparo secundario | / | arma siguiente |
| Espacio | saltar | C, Ctrl | agacharse |
| E, B | usar | Q | arma anterior |
| 1–4 | armas | Tab | puntuaciones |
| Shift | correr | ` o ~ | consola |
| F2 / F3 | menú guardar / cargar | F6 / F7 | guardado / carga rápida |
| F4 | opciones | F10 | salir |
| F12 | captura | - / + | tamaño del HUD |

Se ejecuta en cada arranque, así que manda sobre lo que se cambie desde el menú de opciones: para
cambiar una tecla de forma fija, edita el fichero.

La DLL solo actúa si reconoce los ficheros: comprueba por hash que `enginegl.exe` y
`valve\dlls\hl.dll` son los originales de la 0.52. Si no, no toca nada. Se aceptan dos
`enginegl.exe` con el mismo código: el del CD de la alpha y una copia que circula con el bit
`LARGE_ADDRESS_AWARE` activado en la cabecera (solo cambian 4 bytes de cabecera). Lo que hace queda
registrado en `hlalpha.log`, junto a `enginegl.exe`.

## Cómo funciona

`enginegl.exe` importa `WINMM.dll`, y Windows la busca primero en la carpeta del juego. La
`winmm.dll` de este proyecto:

- reenvía a la `winmm.dll` del sistema todas sus funciones (las 193, no solo las que usa el
  motor: cualquier DLL que se cargue después y use winmm se enganchará a esta);
- aplica en memoria los parches estáticos del motor y, en cuanto el motor la carga, los de
  `hl.dll` (`src/parches.h`: solo los bytes nuevos, ninguno de Valve);
- engancha funciones del motor para guardar y cargar, la persistencia de mapas, la línea de
  órdenes por defecto, etc. (`src/hlalpha.c`).

## Compilar

Con el mingw i686 del [w64devkit](https://github.com/skeeto/w64devkit):

    ./compilar.sh          # -> build/winmm.dll

Herramientas (Node):

- `tools/generar_winmm.js`: genera `src/winmm.def` y `src/winmm_stubs.c` a partir de la
  `winmm.dll` de 32 bits del sistema (`C:\Windows\SysWOW64\winmm.dll`).
- `tools/generar_parches.js`: genera `src/parches.h` comparando los binarios originales con
  versiones parcheadas. Se usó una vez para pasar a memoria los parches que antes se aplicaban
  a los ficheros; los nuevos se pueden añadir a mano en `parches.h`.

## Pruebas

`hlalpha.c` incluye un gancho para pruebas automáticas a través de cambios de mapa: en la
entrada número N a un mapa de la sesión, si existe `valve\zprueba_N.cfg` se ejecuta, y si
existe `valve\zprueba_pos.txt` (`x y z cabeceo giro`) coloca al jugador. Sin esos ficheros no
hace nada.
