var widget = new widgetWindow({
  id: "clock_Widget",
  backgroundcolor: "rgba(30, 30, 40, 0.9)",
  script: "clock/clock.js"
});

setInterval(function () {
  ipc.send('update_Elements');
}, 1000);