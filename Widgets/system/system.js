// =====================
// CONFIG
// =====================
var CONFIG = {
    background: {
        width: 210,
        height: 100,
        image: "../assets/background.png"
    },
    colors: {
        font: "rgb(255,255,255)",
        accent: "rgba(74, 81, 183, 1)"
    },
    font: {
        time: 18,
        text: 12
    }
};

var DAYS = [
    "Sunday", "Monday", "Tuesday",
    "Wednesday", "Thursday", "Friday", "Saturday"
];

// Capture window reference (important for callbacks)
var ui = win;

// =====================
// UI ELEMENTS
// =====================
ui.addImage({
    id: "background_Image",
    width: CONFIG.background.width,
    height: CONFIG.background.height,
    path: CONFIG.background.image
});