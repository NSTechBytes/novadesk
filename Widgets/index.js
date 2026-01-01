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

// =====================
// Network Widget
// =====================
var networkWindow = new widgetWindow({
  id: "network_Widget",
  script: "network/network.js"
});

var network = new system.network();


setInterval(function () {
  var overallUsage = overallCPU.usage();
  var stats = memory.stats();
  var stats = network.stats()

  // Convert bytes to KB/s for display
  var netInKB = (stats.netIn / 1024).toFixed(2);
  var netOutKB = (stats.netOut / 1024).toFixed(2);

  // Convert total bytes to MB for display
  var totalInMB = (stats.totalIn / (1024 * 1024)).toFixed(2);
  var totalOutMB = (stats.totalOut / (1024 * 1024)).toFixed(2);

  novadesk.log("Network - In: " + netInKB + " KB/s, Out: " + netOutKB + " KB/s");
  novadesk.log("Total - In: " + totalInMB + " MB, Out: " + totalOutMB + " MB");

  ipc.send('cpu-usage', overallUsage);
  ipc.send('memory-usage', stats.percent);
  ipc.send('net-in', netInKB);
  ipc.send('net-out', netOutKB);
}, 1000);