var clockWindow = new widgetWindow({
  id: "clock_Widget",
  script: "clock/clock.js"
});

setInterval(function () {
  ipc.send('update_Elements');
  novadesk.log("Main:update_Elements");
}, 1000);

var systemWindow = new widgetWindow({
  id: "system_Widget",
  script: "system/system.js"
});