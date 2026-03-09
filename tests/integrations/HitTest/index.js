import {widgetWindow} from "novadesk"

var widget = new widgetWindow({
    id: "HitTest",
    title: "Hit Test",
    width: 400,
    height: 400,
    script: "hitTest.ui.js",
    backgroundColor: "rgb(10,10,10)"
})
