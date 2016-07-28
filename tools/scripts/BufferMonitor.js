/** Buffer Monitor
*
*	A script (for the Script FX Processor) that displays the content of the audio buffer for analysis.
*
*/

reg b = 0;
reg tempBuffer = 0;

const var Panel = Content.addPanel("Panel", 10, 10);
// [JSON Panel]
Content.setPropertiesFromJSON("Panel", {
  
  "width": 512,
  "height": 168
});
// [/JSON Panel]

const var refreshRate = Content.addKnob("refreshRate", 558, 15);
refreshRate.setRange(30, 700, 1);

const var scaler = Content.addKnob("scaler", 558, 83);
scaler.setRange(1.0, 50.0, 0.1);

Content.setHeight(200);

const var p = Content.createPath();

/** Rebuilds the buffer content. */
inline function recreatePath(updateBuffer)
{
	if(updateBuffer)
	{
		// Lock the buffer for a short time
		readLock(b);
		b >> tempBuffer;
	}
	
	p.clear();
	p.startNewSubPath(0.0, 0.5);
	p.lineTo(0.0, 1.0);
	p.lineTo(0.0, 0.0);
	p.lineTo(0.0, 0.5);
	
	for(i = 0; i < tempBuffer.length; i += 1)
	{
		p.lineTo(i / tempBuffer.length, 0.5 + (-0.5 * tempBuffer[i] * scaler.getValue()));
	}
	
	p.lineTo(1.0, 0.5);
	p.closeSubPath();
}

Panel.setPaintRoutine(function(g)
{
	var r = [0, 0, this["width"], this["height"]];
	
	g.setColour(0x22000000);
	g.drawRect(r, 1.0);
	g.drawLine(0.0, this["width"], this["height"]/2, this["height"]/2, 1);
	
	
	g.setGradientFill([0xFF333333, 0.0, 0.0, 0xFF111111, 0.0, this["height"]]);
	g.fillPath(p, r);
});

Panel.setTimerCallback(function()
{
	recreatePath(true);
	this.repaint();
});

Panel.stopTimer();

const var freezeButton = Content.addButton("freezeButton", 563, 151);
function prepareToPlay(sampleRate, blockSize)
{
	b = Buffer.create(blockSize);
	tempBuffer = Buffer.create(blockSize);
}
function processBlock(channels)
{
	writeLock(b);
	
	channels[0] >> b;
	
}
function onControl(number, value)
{
	if(number == refreshRate)
	{
		Panel.startTimer(value);	
	}
	if(number == freezeButton)
	{
		if(value)Panel.stopTimer();
		else Panel.startTimer(refreshRate.getValue());
	}
	if(number == scaler)
	{
		recreatePath(false);
		Panel.repaint();
		
	}
}
