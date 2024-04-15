// Put in the header definitions of every dialog here...


namespace hise {
namespace multipage {
namespace library {
using namespace juce;
Dialog* SnippetBrowser::createDialog(State& state)
{
	DynamicObject::Ptr fullData = new DynamicObject();
	fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "Dark", "Style": "body\n{\n\tfont-size: 15px;\t\n}\n\n\n\n#footer,\n#header\n{\n\tdisplay: none;\n}\n\n\n\n#content\n{\n\tpadding: 15px;\n\talign-items: flex-start;\n\tbackground: #333;\n\tbox-shadow: inset 0px 0px 5px black;\n\tpadding-top: 24px;\n\t\n}\n\n#content::before\n{\n\tcontent: '';\n\n\tbackground: #222;\n\theight: 20px;\n\tborder-bottom: 1px solid #555;\n}\n\n.header-label\n{\n\tfont-size: 1.1em;\n\tfont-weight: bold;\n\tbackground: linear-gradient(to bottom, #222, #202020);\n\tcolor: #ddd;\n\tborder-radius: 4px;\n\tpadding: 0px;\n}\n\n\n\n.toggle-button\n{\n\tbackground: transparent;\n}\n\n.toggle-button:hover\n{\n\tbackground: transparent;\n}\n\n.tag-button\n{\n\tfont-size: 13px;\n}\n\n.category-button\n{\n\ttransform: none;\n\tpadding: 0px;\n\tcolor: #bbb;\n\twidth: auto;\n\tfont-size: 14px !important;\n\tfont-weight: 500;\n\tborder-radius: 0px;\n\tbackground: #242424;\n\theight: 28px;\n}\n\n.category-button button:hover\n{\n\tcolor: #eee;\n\tbackground: #222;\n}\n\n.category-button:active\n{\n\tcolor: #fff;\n\t\n\ttransform: scale(98%);\n\t\n}\n\n.category-button:checked\n{\n\tbackground: #bbb;\n\tcolor: #222;\n}\n\n.first-child\n{\n\tborder-radius: 50% 0px 50% 0px;\n}\n\n.last-child\n{\n\tborder-radius: 0% 50% 0% 50%;\n}\n\n.category-button::before\n{\n\tdisplay:none;\n}\n\n.category-button::after\n{\n\tdisplay:none;\n}\n\n\n#searchBar label\n{\n\tdisplay: none;\n}\n\n#searchBar input\n{\n\tpadding-left: 100vh;\n\theight: 32px;\n}\n\n#searchBar input::before\n{\n\tcontent: '';\n\twidth: 100vh;\n\tbackground-image: \"332.t0lHDd.QGD+iCI159e.QO.jZCkBBSPjNL8yPg4HHDoCS+LjXUJlKDoCS+LzvblCQxSCaCMLm4PD3BG4PhMLm4PTiky4PWq7MDoCLmNzM2RCQDK3qCw1++UDQ4QQzCw1Mc+CQimE2CwFFF7BQ.tptCIVluqBQ7bGvCQX7kPToeO7Pg4HHDU52COjXrnqDDU52COz++c.QIsVqC8+eGPD3BG4Ph8+eGPTgRE4PoB3ADoo3PNDIBd.Q6KGjCwF4P9.Q6KGjCIlzN9.Q0INjCcaiOPzWRE4P213CDAtvQNjX213CDs1gjNDGrbAQ1P7rCElifPjMDO6PhYJ7oPjMDO6P26XLDs1gjNz8NFCQfKbjCI18NFCQrxeeCYJ7oPTYC90Pg4HHDU1feMjX8n2EDU1feMDEM.AQPl.eCo8jOPzAw+3PrIBgGPzAw+3PiUF\";\n\tmargin: 7px;\n\tbackground: #333;\n\t\n}\n\n#snippetList\n{\n\tflex-grow: 1;\n\theight: auto;\n}\n\n\n.description\n{\n\tbackground: #282828;\n\tpadding: 10px;\n}\n\nth\n{\n\tfont-size: 1em;\n}\n\n.setup-page label\n{\n\tmin-width: 0px;\n}\n\n.setup-page button\n{\n\tpadding: 10px 10px;\n}\n\n\n\n.top-button\n{\n\tflex-grow: 0;\n\theight: 32px;\n\twidth: 32px;\n\tbackground: transparent;\n}\n\n.top-button:hover\n{\n\tbackground: transparent;\n}\n\n.top-button::before\n{\n\tborder: 0px;\n\tbackground: #bbb;\n}\n\n#addButton\n{\n\tmargin: 2px;\n}\n\n#addButton::before\n{\n\t\n\tbackground-image: \"320.t010HCBQd5VpCw1kao.Qd5VpCI1yGi.Qd5VpCA.fGPD8da5P..3ADAvsiNDa..3ADALRXNjX..3ADoTHUNzyGi.QfFojCc4VJPDnQJ4PrcMxfPDnQJ4PrcMxfPjbtszPhcMxfPDUeTzPrBgHDgA..MzYjNBQX..PCw1gakBQX..PCIlPuqBQX..PCgwMrPDUeTzPXbCKDImaKMDaXbCKDAZjRNDavQpPDAZjRNjXrfCQDAZjRND..VDQJERkCA.fEQDvHg4PrA.fEQD.2N5PhA.fEQD8da5PrfCQD4oaoNDbjJDQd5VpCwFF2vBQd5VpCwFF2vBQSij0CIFF2vBQIAW1CIz6pPz7+u8PGtUJDM++aODamQ5HDM++aOjXrBgHDM++aOz0HCBQIAW1CcMxfPzzHY8PrcMxfPjmtk5PiUF\";\n\n}\n\n#settingButton::before\n{\n\tbackground-image: \"598.t0lBhtBQ.ADQCIFF7fBQPgePCE+vjPDT3GzPH3UHDADPDMDa3rAHDQytdMjXrRQGDw+7gMjE7nAQb71YC48sWPDui61Pr8XvQPDyQI1PhISLOPDiFt1PvcPCDQLX1Mz.as.QhXSfCwly99.Q5B2hCI1K84.Q1yRjC4SxMPDtUc4P3wZCD45kcNDa..3ADIy+hNjXsM6ADYA3oNDC4g.QHQJrCYLxIPjoCb6ProNgPPjTFZ6Phol9QPD1Pv6PXJ+DDYU.AOj6RYAQFUQwCwlYVRAQtsgzCIl6lcAQzUg0CswgZPDnWj8PoXcGDY8.aODaYgcHDQnKPOjX1tNIDItVQODWTfBQhqUzCg6IqPDgt.8PrAdJuPj0Cr8Ph0NdxPDnWj8PijYMDQWEVOznogCQtsgzCwFIsZCQFUQwCIVdMjCQVEPvCgZA6PD1Pv6P5rGODIkg1NDadcyPDY5.2NjXXbHQDgDovNjhLUDQV.dpCA.fEQjL+K5Pr46T+PjqW14PhAqM+PDtUc4P+JnODYOKQNzWA0CQ5B2hCwFHkFDQhXSfCIVn3+CQDClcC4ry8PDiFt1Pn4yNDwbThMDapfTMDw63tMjXyOrLDwwamMjYq+BQ7OeXCkM4rPDM650ProfnqPDP.QzPi0FA.ZBQBIjhCIlwSwBQBIjhCAtCwPjy3N4Pf6PLDACXeNjXf6PLDA4AqNjwSwBQb3GsCQ.flPDG9Q6PhwDqfPDG9Q6PoD+FDA4AqNTJwuAQv.1mCIVJwuAQNi6jCwDqfPjPBo3PD.nIDIjPJNzXkA\";\n}\n\n#applyButton::before\n{\n\tbackground-image: \"75.t0F..d.QJxcrCwF..d.Q1MhhCw1mfjBQ1MhhCw1mfjBQnGnRCwF..VDQLAfmCw1mfjBQL7q0Cw1mfjBQJxcrCwF..d.QJxcrCMVY\";\n}\n\n#closeButton\n{\n\tmargin: 3px;\n}\n\n#closeButton::before\n{\n\tbackground-image: \t\"228.t0FXGUBQjNbqCw1Oo3.Q7++1CwF..d.Qx0pyCwFGd5AQBEGnCw12ZwAQBEGnCw12ZwAQ953lCwFGd5AQ953lCwF..d.QJTpVCw1Oo3.QH..PCwFXGUBQ.wiiCwFXGUBQJWahCwFj3dBQJWahCwFj3dBQ.wiiCw1tV6CQH..PCwF..VDQJTpVCwF0g4BQ953lCwVDkBCQ953lCwVDkBCQBEGnCwF0g4BQBEGnCwF..VDQx0pyCw1tV6CQ7++1CwFj3dBQjNbqCwFj3dBQdnjrCwFXGUBQdnjrCwFXGUBQjNbqCMVY\";\n}\n\n.top-button::before:hover\n{\n\tborder: 0px;\n\tbackground: #eee;\n}\n\n.top-button::after\n{\n\tdisplay: none;\n}\n\n#snippetDirectory label\n{\n\tdisplay: none;\n}\n\n.settings-panel label\n{\n\twidth: 80px;\n}\n\n#newName label\n{\n\tmax-width: 40px;\n\t\n}", "UseViewport": false, "DialogWidth": 400, "DialogHeight": 900})"));
	fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Header", "Subtitle": "Subtitle", "Image": "", "ProjectName": "SnippetBrowser", "Company": "HISE", "Version": "1.0.0", "BinaryName": "My Binary", "UseGlobalAppData": false, "Icon": ""})"));
	using namespace factory;
	auto mp_ = new Dialog(var(fullData.get()), state, false);
	auto& mp = *mp_;
	auto& List_0 = mp.addPage<List>({
	  { mpid::Style, "gap: 10px; height: 100%;" }
	});

	auto& Column_1 = List_0.addChild<Column>({
	  { mpid::Class, ".header-label" }
	});

	Column_1.addChild<Button>({
	  { mpid::ID, "addButton" }, 
	  { mpid::Code, R"(showAddPage();
)" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::UseOnValue, 1 }
	});

	Column_1.addChild<SimpleText>({
	  { mpid::Text, "Snippet Browser" }, 
	  { mpid::Style, "flex-grow: 1" }
	});

	Column_1.addChild<Button>({
	  { mpid::ID, "settingButton" }, 
	  { mpid::Code, "document.navigate(1);" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	auto& List_5 = List_0.addChild<List>({
	  { mpid::Style, "display: none;" }
	});

	List_5.addChild<PersistentSettings>({
	  { mpid::ID, "snippetBrowser" }, 
	  { mpid::Filename, "snippetBrowser" }, 
	  { mpid::UseChildState, 1 }, 
	  { mpid::Items, R"(snippetDirectory:"")" }, 
	  { mpid::UseProject, 0 }, 
	  { mpid::ParseJSON, 0 }
	});

	List_5.addChild<DirectoryScanner>({
	  { mpid::Source, "$snippetDirectory" }, 
	  { mpid::ID, "snippetRoot" }, 
	  { mpid::RelativePath, 1 }, 
	  { mpid::Wildcard, "*.md" }, 
	  { mpid::Directory, 0 }
	});

	List_0.addChild<TextInput>({
	  { mpid::ID, "searchBar" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Height, 80 }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::CallOnTyping, 1 }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Autofocus, 0 }
	});

	auto& Column_9 = List_0.addChild<Column>({
	  { mpid::Style, "gap:1px;height: auto;" }
	});

	Column_9.addChild<Button>({
	  { mpid::Text, "All" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::InitValue, "0" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Class, ".category-button .first-child" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	Column_9.addChild<Button>({
	  { mpid::Text, "Modules" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	Column_9.addChild<Button>({
	  { mpid::Text, "MIDI" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	Column_9.addChild<Button>({
	  { mpid::Text, "Scripting" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	Column_9.addChild<Button>({
	  { mpid::Text, "Scriptnode" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	Column_9.addChild<Button>({
	  { mpid::Text, "UI" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::Class, ".category-button .last-child" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	List_0.addChild<TagList>({
	  { mpid::ID, "tagList" }, 
	  { mpid::Code, "rebuildTable();" }, 
	  { mpid::UseOnValue, 1 }
	});

	List_0.addChild<Table>({
	  { mpid::ValueMode, "Row" }, 
	  { mpid::ID, "snippetList" }, 
	  { mpid::Columns, R"(name:Name;max-width:-1;
name:Author;max-width: 140px;width: 110px;)" }, 
	  { mpid::FilterFunction, "showItem" }
	});

	List_0.addChild<MarkdownText>({
	  { mpid::Text, "Please double click a snippet to load..." }, 
	  { mpid::ID, "descriptionDisplay" }, 
	  { mpid::Class, ".description" }, 
	  { mpid::Style, "font-size: 18px; height: 180px;" }
	});

	List_0.addChild<JavascriptFunction>({
	  { mpid::ID, "JavascriptFunctionId" }, 
	  { mpid::CallOnNext, 0 }, 
	  { mpid::Code, R"(var CATEGORIES = ["All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI"];

parsedData = [];

var rebuildTable = function()
{
	table.updateElement();
};

var showAddPage = function()
{
	state.saveFileButton = false;
	state.newName = "";
	state.description = "";
	state.newCategory = 0;
	state.allTagList = [];
	document.navigate(2);
};

var showItem = function(index, data)
{
	var name = data[0];

	if(state.category != 0)
	{
		var cc = CATEGORIES[state.category];
		var tc = parsedData[index].category;
		
		if(cc != tc)
			return false;
	}
	
	var tagFound = state.tagList.length == 0;
	
	for(i = 0; i < state.tagList.length; i++)
	{
		var tt = parsedData[index].tags;
		
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
};

var hiddenItems = ["README.md", "LICENSE.m", "_Template.md", "SnippetDB.md"];

var tags = [];

for(i = 0; i < state.snippetRoot.length; i++)
{
	var item = state.snippetRoot[i];

	if(hiddenItems.indexOf(item) != -1)
		continue;

	var content = document.readFile(state.snippetDirectory + "/" + state.snippetRoot[i]);
	content = content.substring(3, 100000);
	var end = content.indexOf("---");
	var header = content.substring(0, end);
	var description = content.substring(end + 4, 100000);
	var ml = header.split("\n");
	var data = {};
	
	data["name"] = item;
	data["description"] = description;
	
	data.tags = [];
	
	for(j = 0; j < ml.length; j++)
	{
		var kv = ml[j].split(":");
		var key = kv[0];
			
		if(key == "tags")
		{
			var tagList = kv[1].split(",");
			
			for(t = 0; t < tagList.length; t++)
			{
				var tt = tagList[t].trim();
				
				if(tt.length > 0)
					data.tags.push(tt);
			}
		}
		else
		{
			if(key.length > 0)
				data[key] = kv[1].trim();
		}
	}
	
	for(j = 0; j < data.tags.length; j++)
	{
		if(tags.indexOf(data.tags[j]) == -1)
			tags.push(data.tags[j]);
	}
	
	parsedData.push(data);
}

var tagItems = "";

for(i = 0; i < tags.length; i++)
	tagItems += tags[i] + "\n";

var tl = document.getElementById("tagList");
tl.setAttribute("items", tagItems);

var table = document.getElementById("snippetList");

var newItems = "";

for(i = 0; i < parsedData.length; i++)
{
	var item = parsedData[i];

	if(hiddenItems.indexOf(item.name) != -1)
		continue;

	newItems += item.name + "| " + item.author + " |";
	
	if(i != state.snippetRoot.length - 1)
		newItems += "\n";	
}

table.setAttribute("items", newItems);

table.addEventListener("select", function()
{
	var data = parsedData[this.originalRow];
	description.innerText = data.description;
});

var loadSnippet = document.bindCallback("loadSnippet", function(fullPath, category)
{
	Console.print("LOAD SNIPPET: " + fullPath + " from category " + category);
});

var loadFunction = function()
{
	var data = parsedData[this.originalRow];
	loadSnippet(data.HiseSnippet, data.category);
};

table.addEventListener("dblclick", loadFunction);
table.addEventListener("keydown", loadFunction);

var description = document.getElementById("descriptionDisplay");
description.innerText = "Please double click a snippet to load...";

if(state.snippetDirectory.length == 0)
{
	document.navigate(1);
}

rebuildTable();
)" }
	});

	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_20 = mp.addPage<List>({
	  { mpid::Style, "gap: 10px; " }, 
	  { mpid::Class, ".setup-page" }
	});

	auto& Column_21 = List_20.addChild<Column>({
	  { mpid::Class, ".header-label" }
	});

	Column_21.addChild<SimpleText>({
	  { mpid::Text, "Snippet Browser" }, 
	  { mpid::Class, "#headerlabel" }, 
	  { mpid::Style, "flex-grow: 1;" }
	});

	Column_21.addChild<Button>({
	  { mpid::ID, "applyButton" }, 
	  { mpid::Code, "document.navigate(0);" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::UseOnValue, 1 }, 
	  { mpid::NoLabel, 1 }
	});

	List_20.addChild<MarkdownText>({
	  { mpid::Text, R"(Setup the snippet directory and download the demo assets & snippet data.

)" }
	});

	List_20.addChild<PersistentSettings>({
	  { mpid::ID, "snippetBrowser" }, 
	  { mpid::Filename, "snippetBrowser" }, 
	  { mpid::UseChildState, 1 }, 
	  { mpid::Items, R"(snippetDirectory:"")" }, 
	  { mpid::UseProject, 0 }, 
	  { mpid::ParseJSON, 0 }
	});

	auto& List_26 = List_20.addChild<List>({
	  { mpid::Style, "background: #282828; padding: 20px; border-radius: 3px; border: 1px solid #444;" }, 
	  { mpid::Class, ".settings-panel" }
	});

	List_26.addChild<SimpleText>({
	  { mpid::Text, "Snippet Directory" }, 
	  { mpid::Style, "text-align: left; width: 100%;" }
	});

	List_26.addChild<FileSelector>({
	  { mpid::Text, "Directory" }, 
	  { mpid::ID, "snippetDirectory" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The location where you want to store the snippets." }, 
	  { mpid::Directory, 1 }, 
	  { mpid::SaveFile, 0 }, 
	  { mpid::NoLabel, 0 }
	});

	List_26.addChild<SimpleText>({
	  { mpid::Text, "Username" }, 
	  { mpid::Style, "text-align: left; width: 100%;margin-top: 10px;" }
	});

	List_26.addChild<TextInput>({
	  { mpid::ID, "TextInputId" }, 
	  { mpid::Style, "width: 100%;" }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, "The name that is used when you create snippets. Make it your HISE user forum name for increased karma!" }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 0 }
	});

	auto& List_31 = List_20.addChild<List>({
	  { mpid::Style, "background: #282828; padding: 20px; border-radius: 3px; border: 1px solid #444;" }
	});

	List_31.addChild<Button>({
	  { mpid::Text, "Snippets" }, 
	  { mpid::ID, "downloadSnippets" }, 
	  { mpid::InitValue, "true" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Help, "Untick this if you don't want to download the snippets (most likely because you've setup the target folder as Git repository already)." }
	});

	List_31.addChild<DownloadTask>({
	  { mpid::Text, "Download" }, 
	  { mpid::ID, "downloadSnippets" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Source, "https://github.com/qdr/HiseSnippetDB/archive/refs/heads/main.zip" }, 
	  { mpid::Target, "$snippetDirectory/snippets.zip" }, 
	  { mpid::UsePost, 0 }
	});

	List_31.addChild<UnzipTask>({
	  { mpid::Text, "Extract" }, 
	  { mpid::ID, "extractSnippets" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Overwrite, 1 }, 
	  { mpid::Source, "$snippetDirectory/snippets.zip" }, 
	  { mpid::Style, "margin-bottom: 40px;" }, 
	  { mpid::Target, "$snippetDirectory" }, 
	  { mpid::Cleanup, 1 }, 
	  { mpid::SkipFirstFolder, 1 }, 
	  { mpid::SkipIfNoSource, 1 }
	});

	List_31.addChild<Button>({
	  { mpid::Text, "Assets" }, 
	  { mpid::ID, "downloadAssets" }, 
	  { mpid::InitValue, "true" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Help, "Untick this if you don't want to download the example assets (about 50MB)" }
	});

	List_31.addChild<DownloadTask>({
	  { mpid::Text, "Download" }, 
	  { mpid::ID, "downloadAssets" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Source, "https://github.com/qdr/HiseSnippetDB/releases/download/1.0.0/Assets.zip" }, 
	  { mpid::Target, "$snippetDirectory/assets.zip" }, 
	  { mpid::UsePost, 0 }
	});

	List_31.addChild<UnzipTask>({
	  { mpid::Text, "Extract" }, 
	  { mpid::ID, "extractAssets" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Overwrite, 1 }, 
	  { mpid::Source, "$snippetDirectory/assets.zip" }, 
	  { mpid::Target, "$snippetDirectory/Assets" }, 
	  { mpid::Cleanup, 1 }, 
	  { mpid::SkipIfNoSource, 1 }, 
	  { mpid::SkipFirstFolder, 0 }
	});

	// Custom callback for page List_20
	List_20.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_38 = mp.addPage<List>({
	  { mpid::Style, "height: 100%;" }
	});

	auto& Column_39 = List_38.addChild<Column>({
	  { mpid::Class, ".header-label" }
	});

	Column_39.addChild<SimpleText>({
	  { mpid::Text, "Add snippet" }, 
	  { mpid::Style, "flex-grow: 1;" }
	});

	Column_39.addChild<Button>({
	  { mpid::ID, "closeButton" }, 
	  { mpid::Code, R"(document.navigate(0, false);
)" }, 
	  { mpid::Class, ".top-button" }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::UseOnValue, 1 }
	});

	auto& List_42 = List_38.addChild<List>({
	  { mpid::Style, "border: 1px solid #222; padding: 15px; gap: 15px; flex-grow: 1;" }
	});

	List_42.addChild<TextInput>({
	  { mpid::Text, "Name" }, 
	  { mpid::ID, "newName" }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, R"(Select the filename for the snippet. It will save a markdown file with the snippet data and the supplied metadata.
)" }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 0 }
	});

	auto& Column_44 = List_42.addChild<Column>({
	  { mpid::Style, "gap:1px;height: auto;" }
	});

	Column_44.addChild<Button>({
	  { mpid::Text, "All" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::InitValue, "0" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Class, ".category-button .first-child" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_44.addChild<Button>({
	  { mpid::Text, "Modules" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_44.addChild<Button>({
	  { mpid::Text, "MIDI" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_44.addChild<Button>({
	  { mpid::Text, "Scripting" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_44.addChild<Button>({
	  { mpid::Text, "Scriptnode" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::NoLabel, 1 }
	});

	Column_44.addChild<Button>({
	  { mpid::Text, "UI" }, 
	  { mpid::ID, "addCategory" }, 
	  { mpid::Class, ".category-button .last-child" }, 
	  { mpid::ButtonType, "Text" }, 
	  { mpid::NoLabel, 1 }
	});

	List_42.addChild<TagList>({
	  { mpid::ID, "addTagList" }, 
	  { mpid::Items, R"(ScriptPanel
ScriptAPI
full-project
Broadcaster
Layout
Modulator
GLSL
Lottie
FullProject
Tool
Private)" }
	});

	List_42.addChild<SimpleText>({
	  { mpid::Text, "Description" }, 
	  { mpid::Style, "width: 100%; text-align: left;" }
	});

	List_42.addChild<TextInput>({
	  { mpid::ID, "description" }, 
	  { mpid::Style, "flex-grow: 2;  font-family: monospace;vertical-align: top; font-size: 12px; padding-top: 8px; " }, 
	  { mpid::NoLabel, 1 }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Multiline, 1 }, 
	  { mpid::Autofocus, 0 }, 
	  { mpid::CallOnTyping, 1 }
	});

	List_42.addChild<SimpleText>({
	  { mpid::Text, "Preview" }, 
	  { mpid::Style, "width: 100%; text-align: left;" }
	});

	List_42.addChild<MarkdownText>({
	  { mpid::Text, R"(This is a lottie full project

> Delete me later!!!)" }, 
	  { mpid::ID, "descriptionPreview" }, 
	  { mpid::Class, ".description" }, 
	  { mpid::Style, "font-size: 18px; height: 180px;" }
	});

	List_42.addChild<Button>({
	  { mpid::Text, "Save file" }, 
	  { mpid::ID, "saveFileButton" }, 
	  { mpid::Code, "document.navigate(0, true);" }, 
	  { mpid::InitValue, "false" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::NoLabel, 0 }, 
	  { mpid::Help, "Click in order to save the file and return to the snippet browser." }, 
	  { mpid::ButtonType, "Toggle" }, 
	  { mpid::UseOnValue, 1 }
	});

	List_42.addChild<JavascriptFunction>({
	  { mpid::CallOnNext, 0 }, 
	  { mpid::Code, R"(
document.getElementById("descriptionPreview").innerText = " ";
document.getElementById("description").addEventListener("change", function()
{
	Console.print("VALUE: " + this.value);
	

	document.getElementById("descriptionPreview").innerText = this.value;
});

)" }
	});

	List_38.addChild<JavascriptFunction>({
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Code, R"(function appendLine(key, value)
{
	md += "" + key + ": ";
	
	if(typeof(value) == "object")
	{
		for(i = 0; i < value.length; i++)
		{
			md += value[i];
			if(i != value.length -1)
			   md += ", ";
		}
		
		md += "\n";
	}
	else
	{
		md += value + "\n";
	}
}

var exportSnippet = document.bindCallback("exportSnippet", function()
{
	return "This will be the snippet";
});

if(state.newName.length > 0)
{
	Console.print("SAVE");
	
	// Write the metadata
	var md = "";
	
	md += "---\n";
	
	var CATEGORIES = ["All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI"];
		
	appendLine("author", state.author);
	appendLine("category", CATEGORIES[state.addCategory]);
	appendLine("tags", state.addTagList);
	appendLine("active", "true");
	appendLine("HiseSnippet", exportSnippet());
	
	md += "---\n";
	
	// Write the file
	var fileContent = md;
	fileContent += "\n" + state.description;
	var newFile = state.snippetDirectory + "/" + state.newName + ".md";
	document.writeFile(newFile, fileContent);
	
}


)" }
	});

	// Custom callback for page List_38
	List_38.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	
	return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise