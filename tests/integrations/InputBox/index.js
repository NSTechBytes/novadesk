import {widgetWindow,addon} from "novadesk";

const inputBoxWIndow = new widgetWindow({
    width: 400,
    height: 300,
    id: "inputBoxWindow",
    backgroundColor: "black"

});

const inputBox = addon.load(path.join(__addonsPath, "InputBox.dll"));

inputBox.show({
//   inputType: "Integer",
//   minValue: 0,
//   maxValue: 100,
  width:200,
  height:50,
  unfocusDismiss:false,
  placeholder: "558",
  defaultValue:"1000",
//   borderColor:"blue",
//   borderThickness:5,
  onEnter: () => console.log("Value:", inputBox.lastText()),
  onInvalid: () => console.log("Invalid:", inputBox.lastText())
});