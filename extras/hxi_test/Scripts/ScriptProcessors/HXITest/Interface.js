Content.makeFrontInterface(600, 600);

const var Label1 = Content.getComponent("Label1");

include("Subfolder/LabelCode.js");

include("Data/SampleMapLogic.js");

include("Data/IRLogic.js");

include("{GLOBAL_SCRIPT_FOLDER}PositionRandomizer.js");




namespace MIDIHandling
{
const var MIDIPlayer1 = Synth.getMidiPlayer("MIDI Player1");

const var midiList = MIDIPlayer1.getMidiFileList();

const var ComboBox3 = Content.getComponent("ComboBox3");

ComboBox3.set("items", midiList.join("\n"));


inline function onComboBox3Control(component, value)
{
	if(value)
	{
		MIDIPlayer1.setFile(midiList[value-1], true, true);
	}
};

Content.getComponent("ComboBox3").setControlCallback(onComboBox3Control);


inline function onButton1Control(component, value)
{
	if(!value)
		MIDIPlayer1.play(0);
	else
		MIDIPlayer1.stop(0);
};

Content.getComponent("Button1").setControlCallback(onButton1Control);


	
}



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
 