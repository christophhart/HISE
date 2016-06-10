
lowerLimit = Content.addComboBox("lowerLimit", 0, 0);
upperLimit = Content.addComboBox("upperLimit", 150, 0);

for(i = 0; i < 127; i++)
{
	lowerLimit.addItem(Engine.getMidiNoteName(i));
	upperLimit.addItem(Engine.getMidiNoteName(i));
};

var tempNumber = 0;function onNoteOn()
{
	tempNumber = Message.getNoteNumber();
	
	if(tempNumber < lowerLimit.getValue() && tempNumber > upperLimit)
	{
		Message.ignoreEvent(true);
	}
}
function onNoteOff()
{
	tempNumber = Message.getNoteNumber();
	
	if(tempNumber < lowerLimit.getValue() && tempNumber > upperLimit)
	{
		Message.ignoreEvent(true);
	}
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
