const var sine = Libraries.load("tcc").createModule("SineGenerator");

const var Knob = Content.addKnob("Knob", 28, 7);
function prepareToPlay(sampleRate, blockSize)
{
	sine.prepareToPlay(sampleRate, blockSize);
}
function processBlock(channels)
{
	sine >> channels;
}
function onControl(number, value)
{
	sine.setParameter(0, value);
	
}
