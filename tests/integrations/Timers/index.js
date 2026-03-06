import { app, widgetWindow } from "novadesk";

var win = new widgetWindow({
    id: "timers-main-script",
    width: 200,
    height: 200,
    backgroundColor: "rgb(255, 255, 255)",
    script: "./ui/script.ui.js"
});

const id = setTimeout(() => {
    console.log("Fired after 3 seconds");
}, 3000);

setTimeout((greeting, name) => {
    console.log(greeting, name);
}, 1000, "Hello", "Novadesk");

let tick = 0;
const intervalId = setInterval(() => {
    tick += 1;
    console.log("tick", tick);
    if (tick >= 5) {
        clearInterval(intervalId);
        console.log("Interval stopped");
    }
}, 1000);

const timeoutId = setTimeout(() => {
    console.log("This will not run");
}, 5000);

clearTimeout(timeoutId);

const intervalId2 = setInterval(() => {
    console.log("Repeating");
}, 1000);

setTimeout(() => {
    clearInterval(intervalId2);
    console.log("Interval stopped after 5 seconds");
}, 5000);

win.on("close", function () {
    clearTimeout(id);
    clearInterval(intervalId);
    clearInterval(intervalId2);
    app.exit();
});
