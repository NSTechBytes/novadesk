var metrics = system.getDisplayMetrics();

// =====================
// Clock Widget
// =====================

var clockWindow = new widgetWindow({
  id: "clock_Widget",
  script: "clock/clock.js"
});

setInterval(function () {
  ipc.send('update_Elements');
}, 1000);

var clockWindowProperties = clockWindow.getProperties();
clockWindow.setProperties({
  x: metrics.primary.screenArea.width - clockWindowProperties.width - 20,
  y: 20
});

// =====================
// System Widget
// =====================
var systemWindow = new widgetWindow({
  id: "system_Widget",
  script: "system/system.js"
});

var overallCPU = new system.cpu();
var memory = new system.memory();

var systemWindowProperties = systemWindow.getProperties();
systemWindow.setProperties({
  x: metrics.primary.screenArea.width - systemWindowProperties.width - 20,
  y: clockWindowProperties.y + clockWindowProperties.height + 10
});
setInterval(function () {
  var overallUsage = overallCPU.usage();
  var stats = memory.stats();
  ipc.send('cpu-usage', overallUsage);
  ipc.send('memory-usage', stats.percent);
}, 1000);