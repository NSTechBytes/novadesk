var metrics = system.getDisplayMetrics();

// =====================
// Clock Widget
// =====================

var clockWindow = new widgetWindow({
  id: "clock_Widget",
  script: "clock/clock.js"
});


var clockWindowProperties = clockWindow.getProperties();
clockWindow.setProperties({
  x: metrics.primary.screenArea.width - clockWindowProperties.width - 20,
  y: 20
});

// Update properties variable after modification to get new position
clockWindowProperties = clockWindow.getProperties();

setInterval(function () {
  ipc.send('update_Elements');
  // Re-fetch properties to log current state
  var currentClockProps = clockWindow.getProperties();
  novadesk.log("Clock Window Position: " + currentClockProps.x + ", " + currentClockProps.y);
}, 1000);


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
  y: clockWindowProperties.y + clockWindowProperties.height + 20
});

// Update properties variable after modification to get new position
systemWindowProperties = systemWindow.getProperties();

setInterval(function () {
  ipc.send('update_Elements');
  // Re-fetch properties to log current state
  var currentSystemProps = systemWindow.getProperties();
  var currentClockProps = clockWindow.getProperties();
  novadesk.log("System Window Position: " + currentSystemProps.x + ", " + currentSystemProps.y);
  novadesk.log("Clock Window Position: " + currentClockProps.x + ", " + currentClockProps.y);
}, 1000);

// =====================
// Network Widget
// =====================
var networkWindow = new widgetWindow({
  id: "network_Widget",
  script: "network/network.js"
});

var networkWindowProperties = networkWindow.getProperties();
networkWindow.setProperties({
  x: metrics.primary.screenArea.width - networkWindowProperties.width - 20,
  y: systemWindowProperties.y + systemWindowProperties.height + 20
});

var network = new system.network();


setInterval(function () {
  var overallUsage = overallCPU.usage();
  var memStats = memory.stats();
  var netStats = network.stats()

  // Convert bytes to KB/s for display
  var netInKB = (netStats.netIn / 1024).toFixed(2);
  var netOutKB = (netStats.netOut / 1024).toFixed(2);

  ipc.send('cpu-usage', overallUsage);
  ipc.send('memory-usage', memStats.percent);
  ipc.send('net-in', netInKB);
  ipc.send('net-out', netOutKB);
}, 1000);