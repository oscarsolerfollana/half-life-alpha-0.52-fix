// Genera src/winmm.def y src/winmm_stubs.c: una winmm.dll que exporta TODO
// lo que exporta la winmm.dll de 32 bits del sistema y lo reenvia a ella.
// Hace falta exportarlo todo: cualquier DLL del sistema que se cargue
// despues y use winmm se enganchara a esta, no a la de Windows.
// Uso: node generar_winmm.js C:/Windows/SysWOW64/winmm.dll <carpeta src>
const fs = require("fs"), path = require("path");
const [dll, dst] = process.argv.slice(2);
const BS = String.fromCharCode(92);
const NL = BS + "n";            // la secuencia \n dentro de las cadenas de C
const Q = '"';

const d = fs.readFileSync(dll);
const pe = d.readInt32LE(0x3c), opt = pe + 24;
const ns = d.readUInt16LE(pe + 6), so = opt + d.readUInt16LE(pe + 20), sec = [];
for (let i = 0; i < ns; i++) {
  const b = so + i * 40;
  sec.push([d.readUInt32LE(b + 12), d.readUInt32LE(b + 16), d.readUInt32LE(b + 20), d.readUInt32LE(b + 8)]);
}
const r2o = r => { for (const [va, rs, ro, vs] of sec) if (r >= va && r < va + Math.max(rs, vs)) return ro + r - va; return null; };
const e = r2o(d.readUInt32LE(opt + 96));
const base = d.readUInt32LE(e + 16), nf = d.readUInt32LE(e + 20), nn = d.readUInt32LE(e + 24);
const af = r2o(d.readUInt32LE(e + 28)), an = r2o(d.readUInt32LE(e + 32)), ao = r2o(d.readUInt32LE(e + 36));
const cs = o => d.slice(o, d.indexOf(0, o)).toString();

const ex = [], con = new Set();
for (let i = 0; i < nn; i++) {
  const idx = d.readUInt16LE(ao + i * 2);
  con.add(idx);
  ex.push({ nombre: cs(r2o(d.readUInt32LE(an + i * 4))), ord: idx + base });
}
for (let i = 0; i < nf; i++) if (!con.has(i) && d.readUInt32LE(af + i * 4)) ex.push({ nombre: null, ord: i + base });
ex.sort((a, b) => a.ord - b.ord);

const L = [];   // lineas del .c
L.push("/* GENERADO por tools/generar_winmm.js: no editar a mano.");
L.push(" * Un stub por exportacion de la winmm.dll del sistema: la primera vez");
L.push(" * resuelve la funcion real (winmm_resolver, en cargador.c) y salta a ella. */");
L.push("");
L.push("#define WINMM_N " + ex.length);
L.push("void *winmm_real[WINMM_N];");
L.push("const char *const winmm_nombre[WINMM_N] = {");
for (const x of ex) L.push(x.nombre ? "    " + Q + x.nombre + Q + "," : "    0, /* solo ordinal " + x.ord + " */");
L.push("};");
L.push("const unsigned short winmm_ordinal[WINMM_N] = {" + ex.map(x => x.ord).join(",") + "};");
L.push("");
L.push("__asm__(");
const D = ["LIBRARY winmm", "EXPORTS"];
ex.forEach((x, i) => {
  const s = "p_winmm_" + i;
  D.push(x.nombre ? "    " + x.nombre + "=" + s + " @" + x.ord : "    " + s + " @" + x.ord + " NONAME");
  L.push("    " + Q + ".globl _" + s + NL + "_" + s + ":" + NL + Q);
  L.push("    " + Q + "    movl _winmm_real+" + (i * 4) + ", %eax" + NL + "    testl %eax, %eax" + NL + "    jnz 1f" + NL + Q);
  L.push("    " + Q + "    pushl $" + i + NL + "    call _winmm_resolver" + NL + "    addl $4, %esp" + NL + Q);
  L.push("    " + Q + "1:  jmp *%eax" + NL + Q);
});
L.push(");");
fs.writeFileSync(path.join(dst, "winmm.def"), D.join("\n") + "\n");
fs.writeFileSync(path.join(dst, "winmm_stubs.c"), L.join("\n") + "\n");
console.log(ex.length + " exportaciones (" + ex.filter(x => !x.nombre).length + " solo por ordinal)");
