bgImage = Content.addImage("bgImage", 143, 26);
// [JSON bgImage]
Content.setPropertiesFromJSON("bgImage", {
  "x": 0,
  "y": 0,
  "width": 700,
  "height": 450,
  "fileName": "{PROJECT_FOLDER}bg.png"
});
// [/JSON bgImage]

// Set the size of the interface
Content.setHeight(bgImage.get("height"));
Content.setWidth(bgImage.get("width"));

// Tell the engine to use this script as main interface
Synth.addToFront(true);
function onNoteOn()
{
	
}
function onNoteOff()
{
	
}
function onController()
{
	
}
function onTimer()
{
	
}
function onControl(number, value)
{
	
}
