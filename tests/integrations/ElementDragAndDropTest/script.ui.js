// Window titlebar / Drag Area
ui.addShape({
  id: "titleBar",
  type: "rectangle",
  x: 0,
  y: 0,
  width: 560,
  height: 40,
  fillColor: "rgba(30, 36, 56, 0.95)",
  dragArea: true
});

ui.addText({
  id: "titleText",
  x: 20,
  y: 10,
  width: 300,
  height: 20,
  text: "Drag & Drop Area Demo (Drag Bar to Move)",
  fontSize: 14,
  fontColor: "#ffffff"
});

// Drop Zone 1: Main files drop area
ui.addShape({
  id: "dropZone",
  type: "rectangle",
  x: 30,
  y: 60,
  width: 500,
  height: 140,
  radius: 12,
  fillColor: "rgba(35, 45, 75, 0.8)",
  strokeColor: "rgba(100, 180, 255, 0.4)",
  strokeWidth: 2,
  dropTarget: true,
  onDragEnter: function (e) {
    console.log("[onDragEnter] Drop zone entered! Files pending:", e.totalFiles);
    ui.setElementProperties("dropZone", {
      fillColor: "rgba(45, 65, 115, 0.95)",
      strokeColor: "rgba(120, 220, 255, 1.0)",
      strokeWidth: 3
    });
    ui.setElementProperties("dropStatus", {
      text: "Release to drop " + (e.totalFiles || "") + " file(s)..."
    });
  },
  onDragOver: function (e) {
    // Continuous hover updates
  },
  onDragLeave: function (e) {
    console.log("[onDragLeave] Drag left drop zone");
    ui.setElementProperties("dropZone", {
      fillColor: "rgba(35, 45, 75, 0.8)",
      strokeColor: "rgba(100, 180, 255, 0.4)",
      strokeWidth: 2
    });
    ui.setElementProperties("dropStatus", {
      text: "Drag & Drop files here from Windows Explorer"
    });
  },
  onDrop: function (e) {
    console.log("[onDrop] Successfully received files (" + e.totalFiles + "):", e.files);
    ui.setElementProperties("dropZone", {
      fillColor: "rgba(35, 75, 55, 0.9)",
      strokeColor: "rgba(100, 255, 160, 0.9)",
      strokeWidth: 2
    });

    const fileListStr = e.files.slice(0, 3).join("\n") + (e.files.length > 3 ? "\n... and " + (e.files.length - 3) + " more" : "");
    ui.setElementProperties("dropStatus", {
      text: "Dropped " + e.totalFiles + " file(s)!"
    });
    ui.setElementProperties("filesListText", {
      text: fileListStr
    });
  }
});

ui.addText({
  id: "dropStatus",
  x: 50,
  y: 90,
  width: 460,
  height: 24,
  text: "Drag & Drop files here from Windows Explorer",
  fontSize: 14,
  fontColor: "#70c0ff"
});

ui.addText({
  id: "filesListText",
  x: 50,
  y: 120,
  width: 460,
  height: 70,
  text: "No files dropped yet.",
  fontSize: 12,
  fontColor: "#a0b0d0"
});

// Drop Zone 2: Container with dropTarget
ui.addShape({
  id: "containerDropArea",
  type: "rectangle",
  x: 30,
  y: 220,
  width: 500,
  height: 120,
  radius: 8,
  fillColor: "rgba(25, 30, 48, 0.9)",
  strokeColor: "rgba(255, 255, 255, 0.15)",
  strokeWidth: 1,
  dropTarget: true,
  onDragEnter: function (e) {
    ui.setElementProperties("containerDropArea", {
      strokeColor: "rgba(255, 200, 80, 0.9)"
    });
  },
  onDragLeave: function (e) {
    ui.setElementProperties("containerDropArea", {
      strokeColor: "rgba(255, 255, 255, 0.15)"
    });
  },
  onDrop: function (e) {
    console.log("[Secondary Drop Area] Files:", e.files);
    ui.setElementProperties("containerDropText", {
      text: "Received " + e.totalFiles + " file(s): " + e.files[0]
    });
  }
});

ui.addText({
  id: "containerDropText",
  x: 50,
  y: 260,
  width: 460,
  height: 30,
  text: "Secondary Drop Area (Any shape/element can be a drop target)",
  fontSize: 13,
  fontColor: "#e0e0e0"
});
