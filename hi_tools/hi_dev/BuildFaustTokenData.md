
# Build the Faust Library Autocomplete Tokens

These instructions show how to create the tokens for the autocomplete popup in the faust code editor. They process the JSON files from the Sublime syntax files to a C++ struct that can be loaded by the Autocomplete manager. 

This procedure needs to be executed whenever the Faust library changes.

1. Save this file somewhere (eg. `Desktop/faust.json`):

https://github.com/grame-cncm/faust/blob/master-dev/syntax-highlighting/sublime-text/Faust/faust.sublime-completions

2. Run this HISE script:

```javascript
var file = FileSystem.getFolder(FileSystem.Desktop).getChildFile("faust.json");

var obj = file.loadAsObject();


Console.clear();

file.getParentDirectory().getChildFile("faust.h");

var content = "";

content += 
"struct FaustLibraryToken 
{
    juce::String name;
    juce::String codeToInsert;
    juce::String url;
};

juce::Array<FaustLibraryToken> createFaustLibraryTokens()
{
    juce::Array<FaustLibraryToken> tokens;
";

for(k in obj.completions)
{
	
	var c1 = k.details.indexOf("<code>") + 6;
	var c2 = k.details.indexOf("</code>");
	
	var name = k.contents;
	var codeToInsert = k.details.substring(c1, c2).trim();
	
	var functionName = name.split(".")[1];

	// We need to remove the `_ : xxx : _` things
	if(codeToInsert.indexOf(":") != -1)
	{
		var items = codeToInsert.split(":");

		for(t in items)
		{
			if(t.indexOf(functionName) != -1)
			{
				codeToInsert = t.trim().replace(functionName, name);
				break;
			}
		}
	}

	var l1 = k.details.indexOf("http");
	var l2 = k.details.indexOf("'>Docs");
	var link = k.details.substring(l1, l2);
	
	content += "    tokens.add({\"";
	content += name;
	content += "\", \"";
	content += codeToInsert.replace("\"", "\\\"");
	content += "\", \"";
	content += link;
	content += "\"});\n";
	
}

content += "
    return tokens;
};";

var target = file.getParentDirectory().getChildFile("FaustTokenData.h");

target.writeString(content);

var target = file.getParentDirectory().getChildFile("FaustTokenData.h");

target.writeString(content);
```

3. Copy the `faust.h` file to `hi_tools/tools/FaustTokenData.h" and rebuild HISE
