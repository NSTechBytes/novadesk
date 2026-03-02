import { app } from "novadesk";
import * as system from "system";

console.log("=== RegisterHotkey Integration (Latest API) ===");

const hotkeyId = system.hotkey.register("WIN+D", {
  onKeyDown: function () { console.log("WIN+D down"); },
  onKeyUp: function () { console.log("WIN+D up"); }
});

const spaceId = system.hotkey.register("SPACE", {
  onKeyDown: function () { console.log("SPACE down"); },
  onKeyUp: function () { console.log("SPACE up"); }
});

console.log("Registered ids:", hotkeyId, spaceId);

setTimeout(function () {
  system.hotkey.unregister(hotkeyId);
  system.hotkey.unregister(spaceId);
  console.log("Hotkeys unregistered");
  app.exit();
}, 5000);
