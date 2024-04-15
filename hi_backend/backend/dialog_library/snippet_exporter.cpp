// Put in the implementation definitions of every dialog here...




namespace hise {
namespace multipage {
namespace library {
using namespace juce;
Dialog* SnippetExporter::createDialog(State& state)
{
	DynamicObject::Ptr fullData = new DynamicObject();
	fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "ModalPopup", "Style": "#footer {\n\tflex-direction: row;\n\t\n}\n\n\n#header\n{\n\tpadding: 10px 30px;\n\tdisplay:flex;\n}\n\n.category-button label\n{\n\tdisplay: none;\n}\n\n.category-button button\n{\n\ttransform: none;\n\tpadding: 0px;\n\tcolor: #bbb;\n\tborder-radius: 0px;\n}\n\n\n.category-button button:hover\n{\n\tcolor: #eee;\n}\n\n.category-button button:active\n{\n\tcolor: #fff;\n\ttransform: scale(98%);\n\t\n}\n\n.category-button button:checked\n{\n\tbackground: #999;\n\tcolor: #222;\n}\n\n.first-child button\n{\n\tborder-radius: 50% 0px 50% 0px;\n}\n\n.last-child button\n{\n\tborder-radius: 0% 50% 0% 50%;\n}\n\n.category-button button::before\n{\n\tdisplay:none;\n}\n\n.category-button button::after\n{\n\tdisplay:none;\n}\n\n.markdown-editor\n{\n\theight: 400px;\n}\n\n.markdown-editor label\n{\n\tdisplay: none;\n}\n\n.markdown-editor input\n{\n\tpadding-top: 10px;\n\tfont-family: monospace;\n\tfont-size: 14px;\n}\n\n.markdown-editor input\n{\n\theight: 100%;\n\tvertical-align: top;\n}\n\n#previewButton\n{\n\twidth: 250px;\n}\n\n#previewButton label\n{\n\tdisplay: none;\n}\n\n#previewButton button\n{\n\tbackground: #222;\n\tborder-radius: 50%;\n\tcolor: white;\n\t\n\tcursor: pointer;\n}\n\n#previewButton button::before,\n#previewButton button::after\n{\n\tdisplay: none;\n}\n\n\nlabel\n{\n\twidth: 100px;\n}\n\n.preview\n{\n\twidth: 100%; \n\theight: 190px;\n\tmax-height: 200px;\n}\n\n.metadata-editor\n{\n\theight: 200px;\n}\n\n.metadata-editor input\n{\n\theight: 100%;\n\tmargin: 2px 3px;\n\tfont-family: monospace;\n\tfont-size: 13px;\n\tvertical-align: top;\n}\n", "UseViewport": true, "DialogWidth": 800, "DialogHeight": 700})"));
	fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Snippet Exporter", "Subtitle": "", "Image": "", "ProjectName": "SnippetExporter", "Company": "HISE", "Version": "1.0.0", "BinaryName": "My Binary", "UseGlobalAppData": false, "Icon": ""})"));
	using namespace factory;
	auto mp_ = new Dialog(var(fullData.get()), state, false);
	auto& mp = *mp_;
	auto& List_0 = mp.addPage<List>({
	  { mpid::Style, "gap: 20px;" }
	});

	List_0.addChild<PersistentSettings>({
	  { mpid::ID, "snippetBrowser" }, 
	  { mpid::Filename, "snippetBrowser" }, 
	  { mpid::UseChildState, 1 }, 
	  { mpid::Items, "snippetDirectory:\"\"\nauthor:\"\"" }, 
	  { mpid::UseProject, 0 }, 
	  { mpid::ParseJSON, 0 }
	});

	List_0.addChild<MarkdownText>({
	  { mpid::Text, "You can use this dialog to create a snippet markdown file that can be uploaded to the official HISE snippet repository. Please fill in the metadata and then click OK in order to create the file." }
	});

	List_0.addChild<TextInput>({
	  { mpid::Text, "Snippet Name" }, 
	  { mpid::ID, "name" }, 
	  { mpid::Style, "margin-top: 10px;" }, 
	  { mpid::EmptyText, "Enter the snippet name" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, "The snippet name. This should be a descriptive title with less than 80 characters." }
	});

	List_0.addChild<TextInput>({
	  { mpid::Text, "Author" }, 
	  { mpid::ID, "author" }, 
	  { mpid::EmptyText, "Enter your name" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, "The author name (you) that will be shown in the snippet browser. At least try to be serious and pass on the opportunity to put in `HISEFucker2000`..." }
	});

	auto& Column_5 = List_0.addChild<Column>({
	  { mpid::Style, "gap: 2px; " }
	});

	Column_5.addChild<SimpleText>({
	  { mpid::Text, "Category" }, 
	  { mpid::Style, "min-width: 80px; text-align: left;" }
	});

	Column_5.addChild<Button>({
	  { mpid::Text, "All" }, 
	  { mpid::ID, "category" }, 
	  { mpid::InitValue, "true" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Class, ".category-button .first-child" }
	});

	Column_5.addChild<Button>({
	  { mpid::Text, "Modules" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Class, ".category-button" }
	});

	Column_5.addChild<Button>({
	  { mpid::Text, "MIDI" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Class, ".category-button" }
	});

	Column_5.addChild<Button>({
	  { mpid::Text, "Scripting" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Class, ".category-button" }
	});

	Column_5.addChild<Button>({
	  { mpid::Text, "Scriptnode" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Class, ".category-button" }
	});

	Column_5.addChild<Button>({
	  { mpid::Text, "UI" }, 
	  { mpid::ID, "category" }, 
	  { mpid::Class, ".category-button .last-child" }, 
	  { mpid::Help, "The category that will be used for the snippet. This is an exclusive property so please select the one that is most suitable for your snippet.  \n> The category will also define how the default appearance of the HISE snippet playground will look like (eg. if you're using the **UI** category it will show the front interface upon loading)." }
	});

	auto& Column_13 = List_0.addChild<Column>({
	});

	Column_13.addChild<SimpleText>({
	  { mpid::Text, "Tags" }, 
	  { mpid::Style, "width: 200px; text-align: left;" }
	});

	Column_13.addChild<TagList>({
	  { mpid::ID, "tags" }, 
	  { mpid::Items, "OpenGL\nBroadcaster\nScriptPanel\nFullProject\nLottie\nFramework" }
	});

	List_0.addChild<FileSelector>({
	  { mpid::ID, "snippetDirectory" }, 
	  { mpid::Directory, 1 }, 
	  { mpid::SaveFile, 0 }
	});

	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_17 = mp.addPage<List>({
	  { mpid::Style, "gap: 10px; margin: 0px; padding: 0px;" }
	});

	List_17.addChild<MarkdownText>({
	  { mpid::Text, "Please enter the description for the snippet. It supports markdown & links so you can reference the documentation or other things." }
	});

	List_17.addChild<TextInput>({
	  { mpid::Text, "Description" }, 
	  { mpid::ID, "description" }, 
	  { mpid::Class, ".markdown-editor" }, 
	  { mpid::EmptyText, "Please enter the markdown description" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Height, "220" }, 
	  { mpid::Multiline, 1 }, 
	  { mpid::Help, "The markdown description for the snippet. This information will be shown in the preset browser and can be used to add additional information / explanations about this snippet." }, 
	  { mpid::CallOnTyping, 1 }
	});

	// Custom callback for page List_17
	List_17.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_20 = mp.addPage<List>({
	  { mpid::Style, "gap:20px;" }
	});

	List_20.addChild<MarkdownText>({
	  { mpid::Text, "Click next in order to write the file or modify the metadata / output filename manually. " }
	});

	List_20.addChild<FileSelector>({
	  { mpid::Text, "OutputFile" }, 
	  { mpid::ID, "outputFile" }, 
	  { mpid::InitValue, "$snippetDirectory/$name.md" }, 
	  { mpid::UseInitValue, 1 }, 
	  { mpid::Directory, 0 }, 
	  { mpid::SaveFile, 0 }
	});

	List_20.addChild<TextInput>({
	  { mpid::Text, "Metadata" }, 
	  { mpid::ID, "metadata" }, 
	  { mpid::Class, ".metadata-editor" }, 
	  { mpid::Height, 80 }, 
	  { mpid::Multiline, 1 }, 
	  { mpid::CallOnTyping, 0 }
	});

	auto& Column_24 = List_20.addChild<Column>({
	});

	Column_24.addChild<SimpleText>({
	  { mpid::Text, "Description" }, 
	  { mpid::Style, "width: 210px; text-align: left;" }
	});

	auto& List_26 = Column_24.addChild<List>({
	  { mpid::Style, "background: #222;padding: 20px; border: 1px solid #444;box-shadow: 0px 2px 5px rgba(0, 0, 0, 0.5);margin-top: 20px; border-radius: 5px;margin-bottom: 10px; margin-right: 3px;" }
	});

	List_26.addChild<MarkdownText>({
	  { mpid::Text, "Boink this is great" }, 
	  { mpid::ID, "preview" }, 
	  { mpid::Class, ".preview" }
	});

	List_20.addChild<JavascriptFunction>({
	  { mpid::CallOnNext, 0 }, 
	  { mpid::Code, "// Set the description preview\nvar preview = document.getElementById(\"preview\");\npreview.innerText = state.description;\n\nvar exportSnippet = document.bindCallback(\"exportSnippet\", function()\n{\n	return \"This will be the snippet\";\n});\n\n// Write the metadata\nvar md = \"\";\n\nfunction appendLine(key, value)\n{\n	md += \"\" + key + \": \";\n	\n	if(typeof(value) == \"object\")\n	{\n		for(i = 0; i < value.length; i++)\n		{\n			md += value[i];\n			if(i != value.length -1)\n			   md += \", \";\n		}\n		\n		md += \"\n\";\n	}\n	else\n	{\n		md += value + \"\n\";\n	}\n}\n\nmd += \"---\n\";\n\nvar CATEGORIES = [\"ALL\", \"Modules\", \"MIDI\", \"Scripting\", \"Scriptnode\", \"UI\"];\n\nappendLine(\"author\", state.author);\nappendLine(\"category\", CATEGORIES[state.category]);\nappendLine(\"tags\", state.tags);\nappendLine(\"active\", \"true\");\nappendLine(\"HiseSnippet\", exportSnippet());\n\nmd += \"---\n\";\nstate.metadata = md;\n\ndocument.getElementById(\"metadata\").updateElement();\n" }
	});

	List_20.addChild<JavascriptFunction>({
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Code, "// Write the file\nvar fileContent = state.metadata;\nfileContent += \"\n\" + state.description;\n\ndocument.writeFile(state.outputFile, fileContent);" }
	});

	// Custom callback for page List_20
	List_20.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_30 = mp.addPage<List>({
	});

	List_30.addChild<MarkdownText>({
	  { mpid::Text, "The file was created and written to the location. Press Finish in order to close the dialog." }
	});

	List_30.addChild<Button>({
	  { mpid::Text, "Show File" }, 
	  { mpid::ID, "showFile" }, 
	  { mpid::Help, "Opens the file in the Explorer / Finder" }
	});

	List_30.addChild<Launch>({
	  { mpid::Text, "$targetFile" }, 
	  { mpid::ID, "showFile" }, 
	  { mpid::CallOnNext, 1 }
	});

	// Custom callback for page List_30
	List_30.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise