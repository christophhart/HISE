/** TCC Demo
*
*	A simple 1pole Lowpass
*/


reg a = 0.9;
reg b = 1.0 - a;

reg l = 0;
reg r = 0;

reg currentValue = 0.0;
reg lastValue = 0.0;

const var Knob = Content.addKnob("Knob", 26, 6);
// [JSON Knob]
Content.setPropertiesFromJSON("Knob", {
  "max": 0.98999999999999999112,
  "stepSize": "0.0010000000000000000208",
  "middlePosition": 0.9000000000000000222
});
// [/JSON Knob]

extern "C"
{
	void simpleLP(var buffer, var cutoff);
	
	float currentValue = 0.0f;
	float lastValue = 0.0f;
	
	float lastB = 0.0f;
	
	void simpleLP(var buffer, var cutoff)
	{
		float* data = getVarBufferData(buffer);
		int size = getVarBufferSize(buffer);
		
		const float b = 0.7f * lastB + 0.3f * varToFloat(cutoff);
		
		lastB = b;
		
		const float a = 1.0f - b;
		
		for(int i = 0; i < size; i++)
		{
			currentValue = data[i] * a + lastValue * b;
			data[i] = currentValue;
			lastValue = currentValue;
		}
	}
}function prepareToPlay(sampleRate, blockSize)
{
	
}
function processBlock(channels)
{
	l = channels[0];
	r = channels[1];
	
	simpleLP(l, Knob.getValue());
	
	
	/*
	for(i = 0; i < l.length; i++)
	{
		currentValue = lastValue * a + l[i] * b;
		l[i] = currentValue;
		lastValue = currentValue;
	}
	*/
	
	
	l >> r;
	
	
}
function onControl(number, value)
{
	a = value;
	b = 1.0 - a;
	
}
