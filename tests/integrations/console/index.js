import { app } from "novadesk";
// app.enableDebugging(true);
const enableDebugging = app.enableDebugging(true);

if (enableDebugging) {
  console.log("Debugging is enabled");
} else {
  console.log("Debugging is disabled");
}

console.log("This is log message");
console.error("This is error message");
console.warn("This is warn message");
console.debug("This is debug message");
