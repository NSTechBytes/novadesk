!logToFile;

testWidget = new widgetWindow({});

testWidget.addText({
  id: "testText",
  text: "Hello World",
  fontsize: 20,
  x: 10,
  y: 10,
  fontcolor: "rgb(255,255,255)",
  onleftmouseup:
    "novadesk.log('Widget Properties:', JSON.stringify(WidgetProperties));",
});


WidgetProperties = testWidget.getProperties();

novadesk.log("Widget Properties:", JSON.stringify(WidgetProperties));

novadesk.onReady(function () {
  novadesk.log("On Ready FUnction Called");
});
