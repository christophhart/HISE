Content.makeFrontInterface(700, 650);

const var mp = Content.addMultipageDialog("mp", 0, 0);

mp.set("text", "Clean DSP network files");
mp.set("width", 700);
mp.set("height", 650);
mp.set("Font", "Lato");
mp.set("FontSize", 19);

mp.set("DialogWidth", 700);
mp.set("DialogHeight", 650);

const var fp = mp.addPage();

mp.add(fp, mp.types.MarkdownText, { Text: "This tool will cleanup code files from Faust / SNEX or external C++ files. Please select the file types you want to clean:" });

mp.setElementProperty(fp, mp.ids.Style, "gap: 15px; margin: 0px 40px;");


mp.add(fp, mp.types.Button, { ID: "CleanSNEX", Text: "SNEX Files", NoLabel: true});
mp.add(fp, mp.types.Button, { ID: "CleanFaust", Text: "Faust Files", NoLabel: true});
mp.add(fp, mp.types.Button, { ID: "CleanCpp", Text: "C++ Files", NoLabel: true});
mp.add(fp, mp.types.Button, { ID: "CleanNetworks", Text: "Scriptnode networks", NoLabel: true});

mp.add(fp, mp.types.JavascriptFunction, { EventTrigger: "OnSubmit", Code: "state.SkipSNEX = !state.CleanSNEX;"});

mp.add(fp, mp.types.MarkdownText, { Text: "Click next to select the files you want to delete in the next step." });


var listIds = {};

function setItems(id, state)
{
	var list = [];

	var networkRoot = FileSystem.getFolder(FileSystem.AudioFiles).getParentDirectory().getChildFile("DspNetworks");
	
	
	
	var rootDir;
	var wildcard = "*";

	if(id == "setItemNetworks")
	{
		 rootDir = networkRoot.getChildFile("Networks");
	}
	if(id == "setItemSNEX")
	{
		rootDir = networkRoot.getChildFile("CodeLibrary/faust");
	}
	if(id == "setItemFaust")
	{
		rootDir = networkRoot.getChildFile("CodeLibrary/faust");
	}
	if(id == "setItemCpp")
	{
		rootDir = networkRoot.getChildFile("ThirdParty");
		wildcard = "*.h";
	}
	
	var fileList = FileSystem.findFiles(rootDir, wildcard, false);

	for(f in fileList)
	{
		list.push(f.toString(3));
	}

	mp.setElementProperty(listIds[id], "Items", list);
	
}

function clearFunction(id)
{
	var x = mp.getState()[id.replace("clear", "list")];
	
	Console.print("DELETE: " + trace(x));
}

inline function addPage(id)
{
	local p = mp.addPage();
	
	mp.add(p, mp.types.MarkdownText, { Text: "**Cleaning " + id + " files**\n---\nPlease select the files you want to delete, then press Next to perform the operation."});
	
	mp.add(p, mp.types.JavascriptFunction, { "ID": "setItem" + id, Code: mp.bindCallback("setItems", setItems, SyncNotification)});
	
	local tl = mp.add(p, mp.types.TagList, { ID: "list" + id, Text: "Files", Items: ""});
	
	mp.add(p, mp.types.Skip, { "ID": "Clean" + id, SkipIfTrue: true });
	
	listIds["setItem" + id] = tl;
	listIds["clear" + id] = tl;
	

	
	mp.add(p, mp.types.MarkdownText, { Text: "\n---\n> This is not undoable so proceed with caution!"});
	
	mp.add(p, mp.types.JavascriptFunction, { ID: "clear" + id, EventTrigger: "OnSubmit", Code: mp.bindCallback("clearFile", clearFunction, AsyncNotification)});
	
	return p;
}


addPage("SNEX");
addPage("Faust");
addPage("Cpp");
addPage("Networks");

const var outro = mp.addPage();




mp.add(outro, mp.types.MarkdownText, { Text: "Press Finish to close the tool" });

mp.show(true);


function onNoteOn()
{
	local s = mp.exportAsMonolith("");
	Console.print(s);
	
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
 