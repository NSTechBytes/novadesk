!logToFile;

novadesk.log("--- Starting Log Fix Verification ---");

// Get the PATH environment variable (usually very long)
var path = system.getEnv("PATH");
novadesk.log("Current PATH length: " + (path ? path.length : 0));
novadesk.log("Current PATH: " + path);

// Generate an even longer string to be sure
var longStr = "A".repeat(5000);
novadesk.log("Long string length: " + longStr.length);
novadesk.log("Long string: " + longStr);

novadesk.log("--- Verification Finished ---");
