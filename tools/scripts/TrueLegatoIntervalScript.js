
var lastNote = 0;

Sampler.enableRoundRobin(false);;

VelocityCC = Content.addComboBox("VelocityCC", 10, 10);

VelocityCC.addItem("Velocity");

for(i = 0; i < 127; i++)
{
	VelocityCC.addItem("CC " + (i+1));
}

VelocityCC.addItem("Aftertouch");
VelocityCC.addItem("Pitchbend");

var velocityValue = 0;

bypassButton = Content.addButton("bypassButton", 150, 10);
bypassButton.set("text", "Bypass");

function onNoteOn()
{
	if(!bypassButton.getValue())
	{
		Sampler.setActiveGroup(lastNote);
	
		if(VelocityCC.getValue() != 0)
		{
			Message.setVelocity(velocityValue);
		}
	
		lastNote = Message.getNoteNumber();
	}
}
function onNoteOff()
{
	
}
function onController()
{
	if(VelocityCC.getValue() == 129 && Message.getControllerNumber() == 129)
	{
		velocityValue = Message.getControllerValue() / 128.0;
	}
	else if(VelocityCC.getValue() == Message.getControllerNumber())
	{
		velocityValue = Message.getControllerValue();
	}
}
function onTimer()
{
	
}
function onControl(number, value)
{
	if(number == bypassButton)
	{
		Sampler.enableRoundRobin(value);
	}
}
