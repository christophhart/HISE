/** Oscilloscope.js - a simple waveform display of the current audio buffer
*	Author: Christoph Hart
*	
*/

Content.setHeight(200);

const var Panel = Content.addPanel("Panel", 10, 10);
// [JSON Panel]
Content.setPropertiesFromJSON("Panel", {
  "width": 640,
  "height": 180
});
// [/JSON Panel]

reg b = undefined;
reg path = Content.createPath();
reg isRunning = true;

/** Rebuilds the path with the current buffer content. */
inline function plotBuffer()
{
	local max = 1.0 - Zoom.getValue() * 0.99;
	
	path.clear();
	path.startNewSubPath(0.0, 0.0);
	
	path.lineTo(0.0, max);
	path.lineTo(0.0, -max);
	path.lineTo(0.0, 0.0);
	
	local i = 0.0;
	
	for(s in b)
	{
		path.lineTo(i++, s);
	}
	
	path.lineTo(b.length-1, 0.0);
	path.closeSubPath();
}

Panel.setPaintRoutine(function(g)
{
	g.fillAll(0x22000000);
	g.setColour(0x55ffffff);
	g.fillPath(path, [0, 0, this.getWidth(), this.getHeight()]);
	g.setColour(0xFF000000);
	g.drawRect([0, 0, this.getWidth(), this.getHeight()], 0.2);
});

Panel.setTimerCallback(function()
{
	plotBuffer();
	this.repaint();
});

Panel.startTimer(100);

const var Zoom = Content.addKnob("Zoom", 670, 40);
// [JSON Zoom]
Content.setPropertiesFromJSON("Zoom", {
  "mode": "NormalizedPercentage",
  "suffix": "%"
});
// [/JSON Zoom]
const var Hold = Content.addButton("Hold", 670, 160);
// [JSON Hold]
Content.setPropertiesFromJSON("Hold", {
  "saveInPreset": 0
});
// [/JSON Hold]
const var Update = Content.addKnob("Update", 670, 100);
// [JSON Update]
Content.setPropertiesFromJSON("Update", {
  "min": 50,
  "max": 1000,
  "mode": "Time",
  "stepSize": 1,
  "middlePosition": 200,
  "suffix": " ms"
});
// [/JSON Update]
const var idLabel = Content.addLabel("idLabel", 670, 3);
// [JSON idLabel]
Content.setPropertiesFromJSON("idLabel", {
  "text": "oscilloscope",
  "width": 130,
  "height": 30,
  "fontName": "Default",
  "fontSize": 24,
  "fontStyle": "Bold",
  "editable": false,
  "multiline": false
});
// [/JSON idLabel]

function prepareToPlay(sampleRate, blockSize)
{
	b = Buffer.create(blockSize);
}
function processBlock(channels)
{
	if(!Hold.getValue())
		channels[0] >> b;
}
function onControl(number, value)
{
	switch(number)
	{
		case Update:
		{
			if(isRunning)
			{
				Panel.startTimer(value);
			}
			break;
		}
		case Panel:
		{			
			break;
		}
		case Zoom:
		{
			if(!isRunning)
			{
				plotBuffer();
				Panel.repaint();
			}
			break;
		}
		case Hold:
		{
			isRunning = value < 0.5;
			isRunning ? Panel.startTimer(Math.max(50, Update.getValue())) : Panel.stopTimer();
			break;
		}
	}
}
