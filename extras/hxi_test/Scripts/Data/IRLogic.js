const var IRList = Engine.loadAudioFilesIntoPool();

const var ComboBox1 = Content.getComponent("ComboBox1");

ComboBox1.set("items", IRList.join("\n"));

const var ConvolutionReverb1 = Synth.getAudioSampleProcessor("Convolution Reverb1");

inline function onComboBox1Control(component, value)
{
	if(value)
	{
		local ir = IRList[value-1];
		Console.print(ir);
		ConvolutionReverb1.setFile(ir);
		
	}
};

Content.getComponent("ComboBox1").setControlCallback(onComboBox1Control);
