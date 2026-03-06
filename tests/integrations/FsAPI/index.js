import { app } from "novadesk";
import * as fs from "fs";

console.log("=== FsAPI Integration (Latest API) ===");

const baseDir = __dirname + "\\tmp_fs_api";
const nestedDir = baseDir + "\\nested\\child";
const fileA = baseDir + "\\a.txt";
const fileB = baseDir + "\\b.txt";
const fileC = baseDir + "\\c.txt";

console.log("mkdir recursive:", fs.mkdir(nestedDir, true));
console.log("writeFile a:", fs.writeFile(fileA, "Hello"));
console.log("append via writeFile(a, append=true):", fs.writeFile(fileA, " World", true));
console.log("readFile a:", fs.readFile(fileA));
console.log("copyFile a->b:", fs.copyFile(fileA, fileB));
console.log("rename b->c:", fs.rename(fileB, fileC));
console.log("readdir baseDir:", JSON.stringify(fs.readdir(baseDir)));
console.log("stat a:", JSON.stringify(fs.stat(fileA)));
console.log("exists c:", fs.exists(fileC));
console.log("unlink a:", fs.unlink(fileA));
console.log("unlink c:", fs.unlink(fileC));

app.exit();
