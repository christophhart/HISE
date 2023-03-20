namespace SampleMapHandling
{
const var ComboBox2 = Content.getComponent("ComboBox2");

const var mapList = Sampler.getSampleMapList();

ComboBox2.set("items", mapList.join("\n"));

const var Sampler1 = Synth.getSampler("Sampler1");

inline function onComboBox2Control(component, value)
{
	if(value)
	{
		Sampler1.loadSampleMap(mapList[value-1]);
		
	}
};

Content.getComponent("ComboBox2").setControlCallback(onComboBox2Control);	
}
