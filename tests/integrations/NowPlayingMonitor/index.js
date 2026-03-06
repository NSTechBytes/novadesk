import { app } from "novadesk";
import * as system from "system";

console.log("=== NowPlayingMonitor Integration (Latest API) ===");

const id = setInterval(function () {
  console.log("NowPlaying:", JSON.stringify(system.nowPlaying.stats()));
}, 1000);

setTimeout(function () {
  console.log("Set nowPlaying position to 50%");
  system.nowPlaying.setPosition(50, true);
}, 5000);

setTimeout(function () {
  clearInterval(id);
  app.exit();
}, 12000);
