/** Transpose Script
*
*	This script simply transposes every incoming note by 2 semitones.
*/


function onNoteOn()
{
	Message.setNoteNumber(Message.getNoteNumber() + 2);
}
function onNoteOff()
{
	Message.setNoteNumber(Message.getNoteNumber() + 2);
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
