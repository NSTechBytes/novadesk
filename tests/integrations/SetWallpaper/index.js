import { app } from "novadesk";
import * as system from "system";

console.log("=== SetWallpaper Integration (Latest API) ===");
const imagePath = "C:\\Users\\nasirshahbaz\\OneDrive\\Desktop\\pics\\2.png";
console.log("set(fill):", system.wallpaper.set(imagePath, "fill"));
console.log("set(stretch):", system.wallpaper.set(imagePath, "stretch"));
console.log("currentPath:", system.wallpaper.getCurrentPath());
app.exit();
