Content.setHeight(200);

const var timerPanel = Content.addPanel("timerPanel", 181, 3);
// [JSON timerPanel]
Content.setPropertiesFromJSON("timerPanel", {
  "width": 203,
  "height": 196
});
// [/JSON timerPanel]

timerPanel.setTimerCallback(function()
{
	leftPeak.setValue(Engine.getDecibelsForGainFactor(peakMeter.getParameter(peakMeter.PeakLevelLeft)));
	rightPeak.setValue(Engine.getDecibelsForGainFactor(peakMeter.getParameter(peakMeter.PeakLevelRight)));
	leftRMS.setValue(Engine.getDecibelsForGainFactor(peakMeter.getParameter(peakMeter.RMSLevelLeft)));
	rightRMS.setValue(Engine.getDecibelsForGainFactor(peakMeter.getParameter( peakMeter.RMSLevelRight)));
});

timerPanel.startTimer(30);


const var peakMeter = Libraries.load("core").createModule("peak_meter");

const var leftPeak = Content.addKnob("leftPeak", 232, 14);
// [JSON leftPeak]
Content.setPropertiesFromJSON("leftPeak", {
  "width": 43,
  "height": 179,
  "min": -100,
  "max": 0,
  "bgColour": 1426063360,
  "itemColour": 1723974081,
  "itemColour2": 4211081216,
  "mode": "Decibel",
  "style": "Vertical",
  "stepSize": "0.1",
  "middlePosition": -24,
  "suffix": " dB"
});
// [/JSON leftPeak]
const var rightPeak = Content.addKnob("rightPeak", 288, 14);
// [JSON rightPeak]
Content.setPropertiesFromJSON("rightPeak", {
  "width": 43,
  "height": 179,
  "min": -100,
  "max": 0,
  "bgColour": 1426063360,
  "itemColour": 1723974081,
  "itemColour2": 4211081216,
  "mode": "Decibel",
  "style": "Vertical",
  "stepSize": "0.1",
  "middlePosition": -24,
  "suffix": " dB"
});
// [/JSON rightPeak]

const var enablePeak = Content.addButton("enablePeak", 15, 15);
const var enableRMS = Content.addButton("enableRMS", 15, 53);
const var RMSDecayFactor = Content.addKnob("RMSDecayFactor", 16, 93);
const var PeakDecayFactor = Content.addKnob("PeakDecayFactor", 16, 148);

const var rightRMS = Content.addKnob("rightRMS", 337, 14);
// [JSON rightRMS]
Content.setPropertiesFromJSON("rightRMS", {
  "width": 30,
  "height": 180,
  "min": -100,
  "max": 0,
  "bgColour": 1426063360,
  "itemColour": 1720026501,
  "itemColour2": 4211081216,
  "mode": "Decibel",
  "style": "Vertical",
  "stepSize": "1",
  "middlePosition": -24,
  "suffix": " dB"
});
// [/JSON rightRMS]
const var leftRMS = Content.addKnob("leftRMS", 196, 14);
// [JSON leftRMS]
Content.setPropertiesFromJSON("leftRMS", {
  "width": 30,
  "height": 179,
  "min": -100,
  "max": 0,
  "bgColour": 1426063360,
  "itemColour": 1720026501,
  "itemColour2": 4211081216,
  "mode": "Decibel",
  "style": "Vertical",
  "stepSize": "1",
  "middlePosition": -24,
  "suffix": " dB"
});
// [/JSON leftRMS]
function prepareToPlay(sampleRate, blockSize)
{
	peakMeter.prepareToPlay(sampleRate, blockSize);
	
}
function processBlock(channels)
{
	peakMeter.processBlock(channels);
}
function onControl(number, value)
{
	switch(number)
	{
		case leftPeak:
		{
			// Insert logic here...
			break;
		}
		case rightPeak:
		{
			// Insert logic here...
			break;
		}
		case enablePeak:
		{
			peakMeter.setParameter( peakMeter.EnablePeak, value);
			break;
		}
		case enableRMS:
		{
			peakMeter.setParameter( peakMeter.EnableRMS, value);
			break;
		}
		case RMSDecayFactor:
		{
			peakMeter.setParameter( peakMeter.RMSDecayFactor, value);
			break;
		}
		case PeakDecayFactor:
		{
			peakMeter.setParameter( peakMeter.PeakDecayFactor, value);
			break;
		}
	}
}
