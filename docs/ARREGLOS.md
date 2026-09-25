# Half-Life Alpha 0.52 — arreglos

Registro de todo lo arreglado en el motor (`enginegl.exe`) y en la DLL del juego (`hl.dll`) de la
alpha de 1997. Es la base para documentar el proyecto en GitHub. El detalle técnico completo
(direcciones, desensamblado, pruebas) está en [`NOTAS.txt`](NOTAS.txt); el apartado
correspondiente se cita como **[N]**.

Todo va en `winmm.dll`, que el motor carga al arrancar; nada de la alpha se modifica en disco.
La etiqueta al final de cada arreglo dice dónde está:

- **`parches.h`** (`src/parches.h`): bytes que se escriben en memoria sobre `enginegl.exe` o
  `hl.dll` (solo los nuevos; los originales se reconocen por su hash).
- **`hlalpha.c`** (`src/hlalpha.c`): funciones del motor enganchadas en memoria (guardar y
  cargar, transiciones, línea de órdenes por defecto...).
- **`valve/autoexec.cfg`**: la configuración de controles que se copia junto al juego.

---

## Guardar y cargar partida

### Guardar tumbaba el juego · [1]
- **Síntoma:** al guardar, el juego se cerraba.
- **Causa:** el guardado reservaba un búfer fijo de 64 KB y cualquier mapa real ocupa más (los
  `.sav` de fábrica pesan unos 66 KB). Se desbordaba siempre y corrompía la memoria.
- **Arreglo:** búfer ampliado a 1 MB. *(hlalpha.c)*

### Cargar partida no existía · [2]–[8]
- **Síntoma:** la alpha guarda en el formato nuevo de Half-Life, pero la carga era el lector de
  texto de Quake, que no lo entiende; además `hl.dll` no guarda el estado interno C++ de las
  entidades (su `Save` y `Restore` están vacíos).
- **Arreglo:** guardado completo propio: se añade al `.sav` un bloque (formato `CLDX`) con las
  variables y el estado privado de todas las entidades, y la carga se reescribe entera.
  *(hlalpha.c)*

### Cargar tras cerrar y volver a abrir el juego · [13]
- **Síntoma:** cargar funcionaba en la misma sesión, pero petaba al reabrir el juego.
- **Causa:** el `.sav` guardaba direcciones de memoria de ese proceso; `hl.dll` se carga en una
  dirección distinta en cada arranque.
- **Arreglo:** los punteros se guardan como desplazamientos y se recolocan al cargar (formato
  `CLDX` v3). *(hlalpha.c)*

### Cargar desde una partida multijugador tumbaba el juego · [26]
- **Síntoma:** con una partida multijugador en marcha, F7 acababa en un error del motor.
- **Causa:** con un servidor en marcha el motor no deja cambiar el número de jugadores, así que el
  mapa arrancaba para varios, con las entidades numeradas de otra forma, y el guardado de un
  jugador se volcaba encima cruzado.
- **Arreglo:** la carga cierra antes la partida en curso; y si aun así el mapa arranca para más
  de un jugador, no se restaura nada. *(hlalpha.c)*

### Enlaces entre entidades rotos al cargar · [8]
- **Causa:** poner a cero la cabecera de cada entidad rompía la lista enlazada de áreas del motor.
- **Arreglo:** la cabecera se conserva al reponer cada entidad. *(hlalpha.c)*

### Cargar en frío (nada más arrancar) · [9]
- **Arreglo parcial:** la restauración se hace cuando el cliente termina de conectarse (comando
  `begin`), no justo tras `map`. *(hlalpha.c)*

### Al cargar se miraba siempre en la misma dirección · [10]
- **Causa:** el servidor restauraba los ángulos, pero el cliente llevaba su propia copia.
- **Arreglo:** tras cargar se fuerza la vista del cliente (`fixangle`). *(hlalpha.c)*

### Al cargar te quedabas inclinado · [11], [21]
- **Síntoma:** si guardabas moviéndote de lado, o atascado en una puerta que te hace daño,
  al cargar empezabas con la cámara torcida y no se enderezaba nunca.
- **Causa:** al forzar la vista, el servidor le manda al cliente sus tres ángulos, y el tercero
  es el balanceo del *modelo* del jugador, que se recalcula a partir de la velocidad lateral
  (la puerta te empuja de lado). El cliente lo copiaba en su vista y nadie lo volvía a poner a
  cero. Poner el balanceo a cero al cargar ([11]) no bastaba: se recalculaba antes de enviarse.
- **Arreglo:** el cliente descarta el balanceo que le llega al forzarle la vista; el balanceo al
  andar se sigue viendo porque se aplica aparte, al dibujar. *(parches.h)*

### El guardado tardaba más de un segundo · [41]
- **Síntoma:** cada guardado (y cada cambio de nivel, que guarda el mapa que se deja) congelaba el
  juego alrededor de 1,3 s.
- **Causa:** una consulta al sistema sobre la memoria que en este equipo tarda ~2,5 ms se repetía
  una vez por entidad, casi siempre para la misma dirección.
- **Arreglo:** se recuerda la respuesta. El guardado completo baja a unos 10 ms. *(hlalpha.c)*

### Menú de partidas ilegible y sin guardado rápido · [17]
- **Síntoma:** el menú de cargar (F3) mostraba todas las ranuras como `VALV` y no incluía el
  guardado rápido.
- **Causa:** el menú lee los `.sav` como si fueran de Quake; y el rápido se guardaba como
  `quick.sav`, fuera de las 12 ranuras `s0`–`s11`.
- **Arreglo:** cada ranura muestra mapa, fecha y hora; las partidas de fábrica (1997, sin bloque
  `CLDX`) salen como no cargables y no se pueden elegir; la ranura 12 es el guardado rápido
  (F6/F7 guardan y cargan `s11`). *(hlalpha.c + valve/autoexec.cfg)*

---

## Cambios de nivel

### Al cambiar de mapa el jugador llegaba parado · [27]
- **Síntoma:** al pasar una transición se perdía la inercia.
- **Causa:** la alpha guarda la velocidad del jugador al cruzar la salida, junto con la posición
  y la vista, pero al aparecer en el mapa nuevo repone todo menos la velocidad.
- **Arreglo:** el jugador aparece con la velocidad que llevaba. *(parches.h, sobre hl.dll)*

### Al cambiar de mapa la vista salía invertida · [34]
- **Síntoma:** si mirabas hacia abajo al cruzar una transición, aparecías mirando hacia arriba (y al
  revés).
- **Causa:** al aparecer en el mapa nuevo, el juego le mandaba al cliente la inclinación del
  modelo del jugador (invertida y dividida entre 3) en vez de la de la vista.
- **Arreglo:** se manda la vista que tenías. *(parches.h, sobre hl.dll)*

### Los mapas conservan su estado al volver · [39]
- **Antes:** al volver a un mapa por una transición, botones, puertas y enemigos estaban como al
  principio.
- **Ahora:** al salir de un mapa se guarda su estado y al volver se repone (tú llegas con tu
  vida, armas y velocidad). Las partidas guardadas incluyen el estado de los mapas visitados; una
  partida nueva empieza con todos los mapas de cero. *(hlalpha.c)*
- **Corrección [40]:** tras varias idas y vueltas petaba: las entidades creadas durante la
  partida se restauraban con un puntero interno nulo. Ahora se reponen bien, y las entidades
  restauradas se vuelven a enlazar en su sitio del mapa.

---

## Crashes y cuelgues

### Guardar tras cambiar de arma tumbaba el juego · [29]
- **Síntoma:** "NUM_FOR_EDICT: Bad pointer" al guardar después de cambiar de arma.
- **Causa:** los datos del juego declaran el arma actual como si fuera una referencia a otra
  entidad; al guardar, el motor intenta convertirla y, tras un cambio de arma, el valor se sale
  de rango.
- **Arreglo:** al entrar en cada mapa ese campo se trata como número. *(hlalpha.c)*

### Crash con muchos agujeros de bala · [14]
- **Síntoma:** acceso inválido al dibujar (`enginegl.exe+0x3881d`).
- **Causa:** la lista de superficies con calcomanías tiene 500 entradas fijas; el motor escribía
  la entrada nueva **antes** de comprobar el límite, machacando la memoria de detrás.
- **Arreglo:** si la lista está llena, la superficie sobrante no se dibuja ese fotograma.
  *(parches.h)*

### Crash dibujando agujeros de bala en caras grandes · [31]
- **Síntoma:** cierre del juego (acceso inválido en `enginegl.exe+0x38882`).
- **Causa:** para dibujar una calcomanía, el motor copia el polígono de la pared a un búfer de
  20 vértices sin comprobar cuántos tiene; en caras de más de 16 vértices (116 en toda la alpha)
  el búfer se desbordaba sobre las variables del bucle.
- **Arreglo:** en esas caras no se dibujan calcomanías. *(parches.h)*

### "SZ_GetSpace: overflow" con muchos enemigos a la vez · [19], [35]
- **Síntoma:** disparando a varios enemigos a la vez (una granada entre 3-4 perros), primero un
  Engine Error y, tras el primer arreglo, "Illegible server message" y la partida se cortaba.
- **Causa:** los búferes de mensajes del servidor por fotograma son de 1 KB y los mensajes de
  `hl.dll` no comprueban el espacio. El primer arreglo ([19]) vaciaba el búfer al llenarse, pero
  eso pasaba a mitad de un mensaje y el cliente recibía un mensaje roto.
- **Arreglo:** los búferes de difusión pasan a 16 KB y 4 KB. Si un fotograma no cabe en el
  mensaje al cliente, el motor lo descarta entero (se pierden los efectos de ese fotograma, sin
  corromper nada). *(hlalpha.c)* — **Pendiente:** confirmarlo repitiendo la granada.

### La salida del capítulo 1 dejaba de funcionar · [20]
- **Síntoma:** al final de `c1a1a`, pasada la puerta doble, no se cambiaba de mapa.
- **Causa:** en `hl.dll` el trigger de cambio de nivel se desactiva en cuanto lo pisas, antes
  de pedir el cambio. Si el cambio fallaba (cuando la salida a `c1a1c` todavía se ignoraba), el
  trigger se quedaba desactivado, se guardaba así y ya no funcionaba en esa partida.
- **Arreglo:** al cargar, los triggers de cambio de nivel desactivados se vuelven a activar tal
  y como están en el mapa recién cargado. Un cambio que sí se hace carga otro mapa, así que un
  trigger desactivado dentro de un guardado es siempre un cambio fallido. *(hlalpha.c)*

### La salida del capítulo 1 se podía cruzar agachado · [22]
- **Causa:** el trigger de la salida de `c1a1a` es una franja estrecha en lo alto de la puerta;
  agachado se pasaba por debajo.
- **Arreglo:** al entrar en el mapa (o cargar) el trigger se estira a todo el hueco de la puerta.
  *(hlalpha.c)*

### Cuelgue al pasar a un mapa que no existe · [18]
- **Síntoma:** el juego se queda "pensando" en una transición (`c1a1a` → `c1a1c`).
- **Causa:** la alpha no incluye `c1a1c` ni `c3a3`, pero hay salidas hacia ellos. El motor
  congela la imagen y luego no encuentra el mapa (error en una ventana escondida tras la pantalla
  completa, o imagen parada 60 s).
- **Arreglo:** la salida a `c1a1c` lleva al siguiente capítulo que trae la alpha, `c1a2a`,
  conservando vida y armas. Se hace en la función a la que llama el trigger del mapa, cambiando
  solo el nombre del mapa de destino. Para `c3a3` no hay nada después (`c3a2a` es el último
  mapa): esa salida se ignora con un aviso por consola y se sigue jugando. En ambos casos las
  demás salidas del mapa siguen funcionando. *(hlalpha.c)*

---

## Gráficos

### Los agujeros de bala parpadeaban · [32]
- **Síntoma:** las calcomanías (agujeros, sangre) parpadeaban entre negro y gris sin moverse.
- **Causa:** el motor alterna la profundidad en fotogramas alternos (`gl_ztrick`) y compensaba
  mal la separación de las calcomanías respecto a la pared: en uno de cada dos fotogramas
  quedaban detrás.
- **Arreglo:** la separación se aplica siempre hacia la cámara. *(parches.h)*

### Sombras del arma y de la granada · [37]
- **Síntoma:** la sombra del arma en primera persona salía flotando a su lado, y la granada
  arrastraba una sombra en vuelo.
- **Arreglo:** esas dos ya no tienen sombra *(hlalpha.c)*. Además, por preferencia, las sombras
  vienen desactivadas (`r_shadows 0` en `autoexec.cfg`); si se reactivan, el arma y la granada
  siguen sin ella.

---

## Puntería

### La cruceta no estaba en el punto de mira · [15]
- **Síntoma:** los disparos salían por encima de la cruceta.
- **Causa:** la cruceta se dibujaba con la esquina del carácter `+` en el centro; al estirar la
  pantalla virtual de 320×200 a la resolución real quedaba 20 px por debajo y 6 a la derecha.
- **Arreglo:** cruceta recolocada en el centro real. *(parches.h)*

### Los disparos se desviaban según hacia dónde mirabas · [16]
- **Síntoma:** mirando arriba el tiro iba por debajo, mirando abajo por encima, y siempre algo a
  la derecha.
- **Causa:** el cliente mandaba sus ángulos al servidor en un byte (pasos de 1,4°) y truncando.
- **Arreglo:** los ángulos del movimiento viajan en 16 bits (error de 0,002°), como en el
  Half-Life final; el resto de ángulos se redondea en vez de truncar. *(parches.h)*

### La bala no salía del ojo · [16]
- **Causa:** `hl.dll` disparaba desde 10 unidades por delante y 4 por debajo del ojo.
- **Arreglo:** la bala sale del ojo del jugador. *(parches.h, sobre hl.dll)*

### Monstruos con un campo de partículas alrededor · [23]
- **Síntoma:** un cangrejo de `c1a2b` (y luego su cadáver) llevaba alrededor un efecto raro.
- **Causa:** si un monstruo arranca atascado en la pared, `hl.dll` avisa por consola de un error
  del mapa y le pone un campo de partículas como marca de depuración para el mapeador.
- **Arreglo:** se mantiene el aviso pero no la marca; al cargar se quita también de los
  guardados que ya la llevaban. *(parches.h, sobre hl.dll + hlalpha.c)*

---

## Inteligencia artificial

### Los científicos corrían en el sitio al seguirte · [28]
- **Síntoma:** tras pulsar E, el científico se quedaba con la animación de correr sin moverse,
  hasta que le disparabas.
- **Causa:** mientras un monstruo amistoso ve al jugador, su rutina de IA terminaba antes de
  ejecutar lo que estaba haciendo (seguirte). Un disparo le daba un enemigo y tomaba otro camino.
- **Arreglo:** en ese caso la rutina sigue hasta ejecutar su tarea. *(parches.h, sobre hl.dll)*

---

## Controles

### Las teclas 1, 2 y 3 cambian de arma directamente · [30]
- **Antes:** el número solo preseleccionaba el arma y había que confirmar con el disparo.
- **Ahora:** 1 = palanca, 2 = pistola, 3 = MP5, en el acto; lo mismo con arma siguiente/anterior.
  *(parches.h, sobre hl.dll)*

---

## Configuración

### El HUD no se veía · [12]
- No era un fallo: el HUD arranca apagado y se enciende con `sizedown` (tecla `-`). `winmm.dll`
  lo manda al arrancar (`+sizedown`). *(hlalpha.c)*

### El ratón no apuntaba y las teclas eran las de pruebas
- **Síntoma:** mover el ratón arriba y abajo hacía andar en vez de mirar; las teclas F cambiaban
  la pista del CD y varias letras hacían cosas de pruebas (`r_fullbright`, `host_speeds`...).
- **Causa:** el `autoexec.cfg` y el `config.cfg` del CD son los de desarrollo de Valve.
- **Arreglo:** un `autoexec.cfg` propio con WASD, apuntar con el ratón (`+mlook`), guardado
  rápido en F6/F7 y los menús en F2–F5. *(valve/autoexec.cfg)*

### Paredes que no se dibujaban hasta acercarte · [24]
- **Síntoma:** en algunos pasillos (p. ej. `c1a2b`) una pared lejana no se dibujaba y se veía a
  través; al acercarte aparecía.
- **Causa:** la visibilidad precalculada del propio mapa (1997) marca esa zona como no visible
  desde donde estás. Comprobado leyendo el `.bsp`: el motor hace lo que dicen los datos.
- **Arreglo:** `r_novis 1` en `autoexec.cfg`: el mundo se dibuja sin usar esa tabla.
  *(autoexec.cfg)*

### Puertas y otras entidades invisibles hasta acercarte · [38]
- **Síntoma:** una puerta delante de ti no aparecía hasta acercarte.
- **Causa:** la misma visibilidad defectuosa del mapa: el servidor solo manda las entidades que
  esa tabla da como visibles.
- **Arreglo:** jugando solo, el servidor manda todas las entidades (y el mensaje por fotograma
  pasa de 1 a 4 KB para que quepan). En multijugador se deja como estaba. *(hlalpha.c)*

### El juego iba a tirones mirando hacia ciertas zonas · [25]
- **Síntoma:** en un PC moderno, bajadas fuertes de fps según hacia dónde miras.
- **Causa:** el juego usaba el OpenGL de 3Dfx de la carpeta (MiniGL) sobre nGlide, que emula una
  Voodoo de 1997 sobre Direct3D 9: unos 1.600 polígonos costaban 30–39 ms por fotograma.
- **Arreglo:** la instalación aparta el `Opengl32.dll` de la carpeta y así se usa el OpenGL nativo
  de la tarjeta (1–2 ms en la misma vista). *(README, instalación)*

### Un solo ejecutable y "New Game" desde el menú · [33]
- **Antes:** había que arrancar con un `.bat`, y "New Game" del menú no funcionaba (pedía un
  mapa `start` que la alpha no trae).
- **Ahora:** `enginegl.exe` con doble clic abre el menú, y "New Game" empieza en `c1a1`.
  *(hlalpha.c)*

### Resolución del escritorio · [36]
- **Antes:** 800×600 fijo.
- **Ahora:** el juego arranca a la resolución de tu pantalla (por ejemplo 1920×1080). El HUD y los
  menús siguen siendo de 320×200 estirados. *(hlalpha.c)*

---

## Fallos conocidos sin arreglar

- **`changelevel <mapa>` escrito a mano en la consola** tumba el juego (`hl.dll+0xb92d`), también
  con el motor y la DLL originales. Jugando no pasa. Para cambiar de mapa: `map <mapa>`. [18]
- **Falta el mapa `c3a3`**: esa salida de `c3a2` no lleva a ningún sitio, porque la alpha no
  trae nada después. [18]
