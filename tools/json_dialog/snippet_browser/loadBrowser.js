return showItem()

var CATEGORIES = ["All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI"];

function showItem(itemData, originalIndex)
{
	var name = itemData[0];

	if(state.category != 0)
	{
		var cc = CATEGORIES[state.category];
		var tc = state.parsedData[originalIndex].category;
		
		if(cc != tc)
			return false;
	}
	
	var tagFound = state.tagList.length == 0;
	
	for(i = 0; i < state.tagList.length; i++)
	{
		var tt = state.parsedData[originalIndex].tags;
		
		if(tt.indexOf(state.tagList[i]) != -1)
			tagFound = true;
	}
	
	if(!tagFound)
		return false;
	
	if(state.searchBar.length > 0)
	{
		if(name.toLowerCase().indexOf(state.searchBar.toLowerCase()) == -1)
			return false;
	}
	
	return true;
}
