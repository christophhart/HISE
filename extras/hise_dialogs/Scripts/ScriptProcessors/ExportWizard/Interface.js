Content.makeFrontInterface(800, 700);

const var mp = Content.getComponent("MultipageDialog1");

mp.set("FontSize", 18);

//mp.set("Font", "Lato");

// FIRST PAGE =======================================================================================================================================================

const var firstPage = mp.addPage();
mp.add(firstPage, mp.types.MarkdownText, { Text: "This wizard will help you setup your system to be able to export projects with HISE.  \n> If you have already setup HISE for exporting, just tick the box below to skip the wizard."});

mp.add(firstPage, mp.types.Button, 
{ 
  ID: "skipEverything", 
  Text: "Skip wizard",
  Help: "Tick this button and click Next to skip to the end of the wizard and enable the export functions in the menu bar."
  
});

function prevDownload(id, state)
{
	var url = "https://github.com/christophhart/HISE/archive/refs/tags/" + Engine.getProjectInfo().HISEBuild + ".zip";
	state.sourceURL = url;
}

function skipIfDesired(id, state)
{
	if(state.skipEverything)
		mp.navigate(4, false);
};

mp.add(firstPage, mp.types.PersistentSettings, { 
  ID: "CompilerSettings", 
  Filename: "compilerSettings", 
  ParseJSON: false, 
  UseProject: false, 
  Items: ["HisePath:", "UseIPP:0"],
  UseChildState: true
});

mp.add(firstPage, mp.types.JavascriptFunction, { Code: mp.bindCallback("prevDownload", prevDownload, SyncNotification)});

mp.add(firstPage, mp.types.JavascriptFunction, { EventTrigger: "OnSubmit", Code: mp.bindCallback("skipIfDesired", skipIfDesired, SyncNotification)});

const var laf = Content.createLocalLookAndFeel();

laf.setInlineStyleSheet("
label
{
	width: 170px;
}

.checkbutton button:disabled
{
	opacity: 1.0;
}

.checkbutton button::before
{
	border-color: #e88;
}

.checkbutton button::before:checked
{
	border-color: #8e8;
}

.checkbutton button::after:checked
{
	background-image: \"84.t01hA4AQzO0rCwFBhv.QPRwiCwF..d.Q7iElCwV5o.BQzxZxCwFPdABQjPTxCwFlRBBQzxZxCwF..VDQHK5eCw1PA6CQfZJYCw1hA4AQzO0rCMVY\";
	margin: 6px;
	left: 3px;
	top: -2px;
	background-color: #8e8;
	box-shadow: 0px 0px 5px black;
}

#content
{
	padding: 20px 90px;
}
");

mp.setLocalLookAndFeel(laf);

// SECOND PAGE ======================================================================================================================================================

const var secondPage = mp.addPage();

// Load constants for OS and compiler settings from the App data folder
mp.add(secondPage, mp.types.OperatingSystem, {});

mp.add(secondPage, mp.types.PersistentSettings, { 
  ID: "CompilerSettings", 
  Filename: "compilerSettings", 
  ParseJSON: false, 
  UseProject: false, 
  Items: ["HisePath:", "UseIPP:0"],
  UseChildState: true
});


inline function checkIDE(id, state)
{
	if(state.WINDOWS)
	{
		local MSBuildPath = "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe";
		state.msBuildExists =  FileSystem.fromAbsolutePath(MSBuildPath).isFile();
		
		if(state.UseIPP)
		{
			local IppPath = "C:/Program Files (x86)/Intel/oneAPI/ipp/latest/include/ipp.h";
			state.ippExists = FileSystem.fromAbsolutePath(IppPath).isFile();
		}
		else
		{
			state.ippExists = true;
		}
	}
	else
	{
		Console.print("MACOS!");
	}
}

mp.add(secondPage, mp.types.JavascriptFunction, { ID: "checkIDE", Code: mp.bindCallback("checkIDE", checkIDE, SyncNotification)});


mp.add(secondPage, mp.types.MarkdownText, { Text: "First we'll check whether the IDE for your system is installed properly.  \n---"});



const var ideBranch = mp.add(secondPage, mp.types.Branch, { ID: "OS"});

const var linuxIDE = mp.add(ideBranch, mp.types.List, {});
const var windowsIDE = mp.add(ideBranch, mp.types.List, {});
const var macOSIDE = mp.add(ideBranch, mp.types.List, {});

const var xcodeExists = mp.add(macOSIDE, mp.types.Button, {
	ID: "xcodeExists",
	Enabled: false, 
	Text: "Checking Xcode", 
	Class: ".checkbutton",
	Help: "If this is not checked, please download and install the latest Xcode version from either [here](https://xcodereleases.com) or through the App Store.", 
	Required: true
});

const var xcPrettyExists = mp.add(macOSIDE, mp.types.Button, {
	ID: "xcPrettyExists",
	Enabled: false, 
	Text: "Checking xcpretty", 
	Class: ".checkbutton",
	Help: "If this is not checked, please download and install [xcpretty](https://github.com/xcpretty/xcpretty).  \n> The installation is as easy as opening a terminal window and type in `gem install xcpretty`.", 
	Required: true
});

const var msBuildExists = mp.add(windowsIDE, mp.types.Button, 
{ 
	ID: "msBuildExists", 
	Enabled: false, 
	Text: "MS Build installed:", 
	Class: ".checkbutton",
	Help: "If this is not checked, please download Visual Studio 2022 Community Edition from [here](https://visualstudio.microsoft.com/de/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&passive=false&cid=2030), then run the installer and make sure to include the C++ SDK in the install options.", 
	Required: true
});

const var ippExists = mp.add(windowsIDE, mp.types.Button,
{
	ID: "ippExists", 
	Enabled: false, 
	Text: "IPP installed:",
	Class: ".checkbutton",
	Help: "If this is not checked, please download the IPP Community Edition from [here](https://www.intel.com/content/www/us/en/developer/articles/tool/oneapi-standalone-components.html#ipp)  \n> Alternatively you can just go into the Hise Settings and untick the `UseIPP` setting, but it's highly recommended to install IPP for optimal performance", 
	Required: true
})

mp.add(secondPage, mp.types.MarkdownText, { Text: "\n---\nIf any of these tickboxes are not checked, click on the Help button for instructions, then repeat this wizard once you've installed the required tools."});

// Third page ============

const var thirdPage = mp.addPage();

mp.add(thirdPage, mp.types.MarkdownText, { Text: "Now we'll check whether the HISE source code has been downloaded and set as `HisePath`.  \n---"});

function checkHisePath(id, state)
{
	Console.print(trace(state));

	var exists = state.HisePath.length != 0;
	state.hisePathExists = exists;
	state.hisePathExtract = !exists;
	state.hisePathDownload = !exists;
	state.hiseVersionMatches = true;
}

mp.add(thirdPage, mp.types.PersistentSettings, { 
  ID: "CompilerSettings", 
  Filename: "compilerSettings", 
  ParseJSON: false, 
  UseProject: false, 
  Items: ["HisePath:", "UseIPP:0"],
  UseChildState: true
});

mp.add(thirdPage, mp.types.JavascriptFunction, { Code: mp.bindCallback("checkHisePath", checkHisePath, SyncNotification)});

const var hisePathExists = mp.add(thirdPage, mp.types.Button, 
{ 
	ID: "hisePathExists", 
	Class: ".checkbutton",
	Text: "HISE Path set:",
	Enabled: false,
	Help: "If this is not checked, then the wizard will download and extract the HISE path to the location you specify"
});

const var hiseVersionMatches = mp.add(thirdPage, mp.types.Button, 
{ 
	ID: "hiseVersionMatches", 
	Class: ".checkbutton",
	Text: "Version matches:",
	Enabled: false,
	Required: true,
	Help: "If this is not checked, then the source code in the HISE path does not match the HISE version of this build. This will lead to unpredicted behaviour so make sure that you're using the exact source code that was used for compiling HISE"
});

const var downloadList = mp.add(thirdPage, mp.types.List, { Foldable: true, Folded: true, Text: "Download Source code", Style: "margin-top: 20px;" });

const var hisePathBrowser = mp.add(downloadList, mp.types.FileSelector, { ID: "HisePath", Text: "Choose location", Help: "Choose a location for downloading and extracting the HISE path", Directory: true, Required: true});

const var hpColumn = mp.add(downloadList, mp.types.Column, {});
const var hisePathDownload = mp.add(hpColumn, mp.types.DownloadTask, { ID: "hisePathDownload", Text: "Download", Source: "$sourceURL", Target: "$HisePath/master.zip", EventTrigger: "OnSubmit" });
const var hisePathExtract = mp.add(hpColumn, mp.types.UnzipTask, { ID: "hisePathExtract", Source: "$HisePath/master.zip", Target: "$HisePath", SkipFirstFolder: true, "Cleanup": true, Text: "Extract", EventTrigger: "OnSubmit" });

mp.add(thirdPage, mp.types.MarkdownText, { Text: "\n---\nIf the tickbox is not checked, Click on the Download source tab to setup the folder and press next to download and extract the source code from GitHub."});

// Page 4: SDKs ======================================================================================================

function checkSDK(id, state)
{
	Console.print("CHECK");
	
	var toolsDir = FileSystem.fromAbsolutePath(state.HisePath).getChildFile("tools"); 
	var vst3sdk = toolsDir.getChildFile("SDK/VST3 SDK");
	
	var projucer = toolsDir.getChildFile("projucer/Projucer.exe");
	
	state.projucerWorks = projucer.startAsProcess("--help");
	
	mp.setElementValue(projucerWorks, state.projucerWorks);
	
	state.sdkExists = vst3sdk.isDirectory();
	state.sdkExtract = !state.sdkExists;
	
	Console.print(trace(state));
	
}

const var page4 = mp.addPage();

mp.add(page4, mp.types.PersistentSettings, { ID: "CompilerSettings", Filename: "compilerSettings", ParseJSON: false, UseProject: false, UseChildState: true});

mp.add(page4, mp.types.MarkdownText, { Text: "Now we'll check whether the SDKs are extracted within the HISE source folder.  \n---"});

mp.add(page4, mp.types.JavascriptFunction, { Code: mp.bindCallback("checkSDK", checkSDK, SyncNotification)});

const var projucerWorks = mp.add(page4, mp.types.Button, {
	ID: "projucerWorks",
	Class: ".checkbutton",
	Text: "Projucer works:",
	Enabled: false,
	Required: true,
	Help: "If this is not checked, then your system cannot launch the Projucer from the HISE source code folder"
});

mp.add(page4, mp.types.Button, {
	ID: "sdkExists",
	Class: ".checkbutton",
	Text: "SDKs extracted:",
	Enabled: false,
	Help: "The SDKs are necessary to compile VST3 and standalone apps with the ASIO driver"
});



const var sdkExtract = mp.add(page4, mp.types.UnzipTask, { ID: "sdkExtract", Text: "Extract", Source: "$HisePath/tools/SDK/sdk.zip", Target: "$HisePath/tools/SDK", EventTrigger: "OnSubmit" });

mp.add(page4, mp.types.MarkdownText, { Text: "\n---\nIf the tickbox is not checked, Click on the Next in order to extract the SDKs."});

const var page5 = mp.addPage();

mp.add(page5, mp.types.MarkdownText, { Text: "This concludes the export setup. Press finish to close this dialog and start exporting your projects!\n> If you have downloaded the source code you need to restart HISE before exporting."});

mp.add(page5, mp.types.JavascriptFunction, {
  Code: mp.bindCallback("onPost", function(){}, SyncNotification),
  EventTrigger: "OnSubmit"
});

mp.add(page5, mp.types.PersistentSettings, { 
  ID: "CompilerSettings", 
  Filename: "compilerSettings", 
  ParseJSON: false, 
  UseProject: false, 
  Items: ["HisePath:", "UseIPP:0", "ExportSetup:1"],
  UseChildState: true
});



Console.clear();

mp.show(true);

const var to = Engine.createTimerObject();

reg x = {};

to.setTimerCallback(function()
{
	var b64 = mp.exportAsMonolith("");
	
	Console.print(b64);

	//mp.navigate(4, false);
	this.stopTimer();
});

to.startTimer(100);

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
 