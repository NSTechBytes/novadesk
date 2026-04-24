import { app, widgetWindow } from "novadesk";


const widget = new widgetWindow({
    id: "toolbar-test-widget",
    width: 200,
    height: 100,
    showInToolbar: true,
    backgroundColor: "#000000",
    toolbarIcon: "../assets/icon.ico",
    toolbarTitle: "Toolbar Integration Test",
    script: "./script.ui.js"
});

widget.on("close", () => console.log("Widget closing"));
widget.on("closed", () => console.log("Widget closed"));

function testWidgetMinimization() {
    widget.minimize();
   
}

function testWidgetRestoration() {
    widget.unMinimize();
    
}
 widget.on("minimize", () => console.log("minimized"));
 widget.on("unMinimize", () => console.log("restored"));

 widget.on("click", () => console.log("clicked"));
 widget.on("right-click", () => console.log("right clicked"));
 widget.on("scroll-up", () => console.log("scroll up"));
 widget.on("scroll-down", () => console.log("scroll down"));

 widget.on("double-click", () => {
  console.log("double clicked");
});


// setTimeout(() => {
//     testWidgetMinimization();
//     // testWidgetRestoration();
// }, 5000);

// setTimeout(() => {
//     // testWidgetUnMinimization();
//     testWidgetRestoration();
// }, 8000);