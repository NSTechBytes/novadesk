import { app } from "novadesk";

const enableDebugging = app.enableDebugging(true);

if (enableDebugging) {
  console.log("Debugging is enabled");
} else {
  console.log("Debugging is disabled");
}

console.log("This is log message");
console.info("This is info log");
console.error("This is error message");
console.warn("This is warn message");
console.debug("This is debug message");
print("Hello")

setTimeout(() => {
  app.enableDebugging(false);
  console.debug("Debugging is disabled");
  app.exit();
}, 2000);
