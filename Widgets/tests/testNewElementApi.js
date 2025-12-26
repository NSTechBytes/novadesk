// Test script for the new generalized Widget API
// Verifies setElementProperties, removeElements, and getElementProperties

var widget = new widgetWindow({
    id: "testApiWidget",
    x: 100,
    y: 100,
    width: 400,
    height: 300,
    backgroundcolor: "rgba(30, 30, 30, 0.9)"
});

// 1. Add elements
widget.addText({
    id: "text1",
    text: "Original Text",
    x: 10,
    y: 10,
    fontsize: 20,
    fontcolor: "rgba(9, 255, 0, 1)",
    onleftmouseup: "testElementProperty();",
    onmouseover: "testGetElementProperty();"
});

function testGetElementProperty() {
    var prop = widget.getElementProperties("text1")
    novadesk.log("Properties", JSON.stringify(prop));
}

function testElementProperty() {
    widget.setElementProperties("text1", { text: "nasir" })
}


// widget.addImage({
//     id: "img1",
//     path: "assets/logo.png",
//     x: 10,
//     y: 50,
//     width: 50,
//     height: 50
// });

// novadesk.log("Elements added.");

// // 2. Test getElementProperties
// var props = widget.getElementProperties("text1");
// if (props) {
//     novadesk.log("getElementProperties(text1) success: " + props.text + " at " + props.x + "," + props.y);
// } else {
//     novadesk.error("getElementProperties(text1) FAILED");
// }

// // 3. Test setElementProperties (Partial update)
// novadesk.log("Updating text1 properties...");
// widget.setElementProperties("text1", {
//     text: "Updated via General API",
//     fontcolor: "rgba(0, 255, 0, 1.0)",
//     x: 50
// });

// // Verify update
// var updatedProps = widget.getElementProperties("text1");
// if (updatedProps && updatedProps.text === "Updated via General API" && updatedProps.x === 50) {
//     novadesk.log("setElementProperties(text1) success.");
// } else {
//     novadesk.error("setElementProperties(text1) FAILED or verification failed. Props: " + JSON.stringify(updatedProps));
// }

// // 4. Test removeElements (single)
// novadesk.log("Removing img1...");
// widget.removeElements("img1");
// if (widget.getElementProperties("img1") === null) {
//     novadesk.log("removeElements(img1) success.");
// } else {
//     novadesk.error("removeElements(img1) FAILED.");
// }

// // 5. Add more for bulk test
// widget.addImage({ id: "bulk1", path: "assets/logo.png", x: 100, y: 100 });
// widget.addImage({ id: "bulk2", path: "assets/logo.png", x: 150, y: 100 });

// // 6. Test removeElements (array)
// novadesk.log("Removing bulk1 and bulk2...");
// widget.removeElements(["bulk1", "bulk2"]);
// if (widget.getElementProperties("bulk1") === null && widget.getElementProperties("bulk2") === null) {
//     novadesk.log("removeElements([array]) success.");
// } else {
//     novadesk.error("removeElements([array]) FAILED.");
// }

// // 7. Test removeElements (clear all)
// novadesk.log("Clearing all content...");
// widget.removeElements(); // or widget.removeElements(null)
// // Should result in empty elements list (we can't easily check count from JS yet, but we can check if text1 is gone)
// if (widget.getElementProperties("text1") === null) {
//     novadesk.log("removeElements() clear all success.");
// } else {
//     novadesk.error("removeElements() clear all FAILED.");
// }

// novadesk.log("All tests completed.");
