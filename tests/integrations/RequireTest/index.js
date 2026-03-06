import { app } from "novadesk";
import { name as moduleAName, bName } from "./moduleA.js";
import sum from "./sum.js";

console.log("=== RequireTest Migration (ESM) ===");
console.log("moduleA.name =", moduleAName);
console.log("moduleA.bName =", bName);
console.log("sum(2,3)=", sum(2, 3));
app.exit();
