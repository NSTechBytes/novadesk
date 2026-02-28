import { widgetWindow } from "novadesk";

var win = new widgetWindow({
    id: "path-module",
    width: 200,
    height: 200,
    backgroundColor: "rgb(255, 255, 255)",
    script: "ui/ui.js"
});

var path_Join = path.join(__dirname, "assets", "icon.png");
var path_Basename1 = path.basename("/widgets/ui.js");      
var path_Basename2 = path.basename("/widgets/ui.js", ".js"); 
var path_Dirname = path.dirname("/widgets/ui.js"); // "/widgets"
var path_Extname1 = path.extname("icon.png");   // ".png"
var path_Extname2 = path.extname("index.html"); // ".html"
var path_IsAbsolute1 = path.isAbsolute("C:\\widgets\\ui.js"); // true
var path_IsAbsolute2 = path.isAbsolute("ui.js");              // false
var path_Normalize = path.normalize("C:\\Users\\Documents\\..\\Desktop\\.\\widget.js");
var path_Relative = path.relative(__dirname, path.join(__dirname, "ui.js")); // "ui.js"
var path_Format = path.format({ dir: "/widgets", base: "index.js" }); // "/widgets/index.js"
var parsed = path.parse("/widgets/index.js");

console.log("==================== Main Script Output ====================")
console.log("path_Join: " + path_Join);
console.log("path_Basename1: " + path_Basename1);
console.log("path_Basename2: " + path_Basename2);
console.log("path_Dirname: " + path_Dirname);
console.log("path_Extname1: " + path_Extname1);
console.log("path_Extname2: " + path_Extname2);
console.log("path_IsAbsolute1: " + path_IsAbsolute1);
console.log("path_IsAbsolute2: " + path_IsAbsolute2);
console.log("path_Normalize: " + path_Normalize);
console.log("path_Relative: " + path_Relative);
console.log("path_Format: " + path_Format);
console.log("path_Parsed: " + JSON.stringify(parsed));
