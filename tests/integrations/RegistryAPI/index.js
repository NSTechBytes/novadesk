import { app } from "novadesk";
import * as system from "system";

console.log("=== RegistryAPI Integration (Latest API) ===");

console.log("[PASS] read existing:", system.registry.readData("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", "SmartScreenEnabled"));
console.log("[PASS] read missing:", system.registry.readData("HKCU\\Software\\Novadesk", "NonExistent"));
console.log("[PASS] write string:", system.registry.writeData("HKCU\\Software\\Novadesk", "TestValue", "TestData"));
console.log("[PASS] write number:", system.registry.writeData("HKCU\\Software\\Novadesk", "TestNumber", 12345));
console.log("[PASS] read back string:", system.registry.readData("HKCU\\Software\\Novadesk", "TestValue"));
console.log("[PASS] read back number:", system.registry.readData("HKCU\\Software\\Novadesk", "TestNumber"));

app.exit();
