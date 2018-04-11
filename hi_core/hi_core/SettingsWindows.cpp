/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


String getUncamelcasedId(const Identifier& id)
{
	auto n = id.toString();

	String pretty;

	auto ptr = n.getCharPointer();

	bool lastWasUppercase = true;

	while (!ptr.isEmpty())
	{
		if (ptr.isUpperCase() && !lastWasUppercase)
		{
			pretty << " ";
		}

		lastWasUppercase = ptr.isUpperCase();

		pretty << ptr.getAddress()[0];

		ptr++;
	}

	return pretty;
}


#define P(p) if (prop == p) { s << "### " << getUncamelcasedId(p) << "\n";
#define D(x) s << x << "\n";
#define P_() return s; } 

struct SettingDescription
{
    
    
	static String getDescription(const Identifier& prop)
	{
		String s;

		P(HiseSettings::Project::Name);
		D("The name of the project. This will be also the name of the plugin binaries");
		P_();

		P(HiseSettings::Project::Version);
		D("The version number of the project. Try using semantic versioning (`1.0.0`) for this.  ");
		D("The version number will be used to handle the user preset backward compatibility.");
		D("> Be aware that some hosts (eg. Logic) are very picky when they detect different plugin binaries with the same version.");
		P_();

		P(HiseSettings::Project::BundleIdentifier);
		D("This is a unique identifier used by Apple OS to identify the app. It must be formatted as reverse domain like this:");
		D("> `com.your-company.product`");
		P_();

		P(HiseSettings::Project::PluginCode);
		D("The code used to identify the plugins. This has to be four characters with the first one being uppercase like this:");
		D("> `Abcd`");
		P_();

		P(HiseSettings::Project::EmbedAudioFiles);
		D("If this is **enabled**, it will embed all audio files (impulse responses & loops) **as well as images** into the plugin.");
		D("This will not affect samples - they will always be streamed.  ");
		D("If it's **disabled**, it will use the resource files found in the app data directory and you need to make sure that your installer");
		D("copies them to the right location:");
		D("> **Windows:** `%APPDATA%\\Company\\Product\\`");
		D("> **macOS:** `~/Library/Application Support/Company/Product/`");
		D("Normally you would try to embed them into the binary, however if you have a lot of images and audio files (> 50MB)");
		D("the compiler will crash with an **out of heap space** error, so in this case you're better off not embedding them.");
		P_();

		P(HiseSettings::Project::AdditionalDspLibraries);
		D("If you have written custom DSP objects that you want to embed statically, you have to supply the class names of each DspModule class here");
		P_();

		P(HiseSettings::Project::WindowsStaticLibFolder);
		D("If you need to link a static library on Windows, supply the absolute path to the folder here. Unfortunately, relative paths do not work well with the VS Linker");
		P_();

		P(HiseSettings::Project::OSXStaticLibs);
		D("If you need to link a static library on macOS, supply the path to the .a library file here.");
		P_();

		P(HiseSettings::Project::ExtraDefinitionsWindows);
		D("This field can be used to add preprocessor definitions. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```");
		P_();
			
		P(HiseSettings::Project::ExtraDefinitionsOSX);
		D("This field can be used to add preprocessor definitions. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```");
		P_();

		P(HiseSettings::Project::ExtraDefinitionsIOS);
		D("This field can be used to add preprocessor definitions. Use it to tailor the compile options for HISE for the project.");
		D("#### Examples");
		D("```javascript");
		D("ENABLE_ALL_PEAK_METERS=0");
		D("NUM_POLYPHONIC_VOICES=100");
		D("```");
		P_();
	
		P(HiseSettings::Project::AppGroupID);
		D("If you're compiling an iOS app, you need to add an App Group to your Apple ID for this project and supply the name here.");
		D("App Group IDs must have reverse-domain format and start with group, like:");
		D("> `group.company.product`");
		P_();

		P(HiseSettings::User::Company);
		D("Your company name. This will be used for the path to the app data directory so make sure you don't use weird characters here");
		P_();

		P(HiseSettings::User::CompanyCode);
		D("The unique code to identify your company. This must be 4 characters with the first one being uppercase like this:");
		D("> `Abcd`");
		P_();

		
		P(HiseSettings::User::TeamDevelopmentID);
		D("If you have a Apple Developer Account, enter the Developer ID here in order to code sign your app / plugin after compilation");
		P_();

		P(HiseSettings::Compiler::VisualStudioVersion);
		D("Set the VS version that you've installed. Make sure you always use the latest one, since I need to regularly deprecate the oldest version");
		P_();

		P(HiseSettings::Compiler::HisePath);
		D("This is the path to the source code of HISE. It must be the root folder of the repository (so that the folders `hi_core`, `hi_modules` etc. are immediate child folders.  ");
		D("This will be used for the compilation of the exported plugins and also contains all necessary SDKs (ASIO, VST, etc).");
		D("> Always make sure you are using the **exact** same source code that was used to build HISE or there will be unpredicatble issues.");
		P_();

		P(HiseSettings::Compiler::UseIPP);
		D("If enabled, HISE uses the FFT routines from the Intel Performance Primitive library (which can be downloaded for free) in order");
		D("to speed up the convolution reverb");
		D("> If you use the convolution reverb in your project, this is almost mandatory, but there are a few other places that benefit from having this library");
		P_();

		P(HiseSettings::Scripting::CodeFontSize);
		D("Changes the font size of the scripting editor. Beware that on newer versions of macOS, some font sizes will not be displayed (Please don't ask why...).  ");
		D("So if you're script is invisible, this might be the reason.");
		P_();

		P(HiseSettings::Scripting::EnableCallstack);
		D("This enables a stacktrace that shows the order of function calls that lead to the error (or breakpoint).");
		D("#### Example: ")
		D("```javascript");
		D("Interface: Breakpoint 1 was hit ");
		D(":  someFunction() - Line 5, column 18");
		D(":  onNoteOn() - Line 3, column 2");
		D("```");
		D("A breakpoint was set on the function `someFunction` You can see in the stacktrace that it was called in the `onNoteOn` callback.  ");
		D("Double clicking on the line in the console jumps to each location.");
		P_();

		P(HiseSettings::Scripting::CompileTimeout);
		D("Sets the timeout for the compilation of a script in **seconds**. Whenever the compilation takes longer, it will abort and show a error message.");
		D("This prevents hanging if you accidentally create endless loops like this:");
		D("```javascript");
		D("while(true)");
		D(" x++;");
		D("");
		D("```");
		P_();

		P(HiseSettings::Scripting::GlobalScriptPath);
		D("There is a folder that can be used to store global script files like additional API functions or generic UI widget definitions.");
		D("By default, this folder is stored in the application data folder, but you can choose to redirect it to another location, which may be useful if you want to put it under source control.");
		D("You can include scripts that are stored in this location by using the `{GLOBAL_SCRIPT_FOLDER}` wildcard:");
		D("```javascript");
		D("// Includes 'File.js'");
		D("include(\"{GLOBAL_SCRIPT_FOLDER}File.js\");");
		D("```");
		P_();

		P(HiseSettings::Scripting::EnableDebugMode);
		D("This enables the debug logger which creates a log file containing performance issues and system specifications.");
		D("It's the same functionality as found in the compiled plugins.");
		P_();

		P(HiseSettings::Other::EnableAutosave);
		D("The autosave function will store up to 5 archive files called `AutosaveXXX.hip` in the archive folder of the project.");
		D("In a rare and almost never occuring event of a crash, this might be your saviour...");
		P_();

		P(HiseSettings::Other::AutosaveInterval);
		D("The interval for the autosaver in minutes. This must be a number between `1` and `30`.");
		P_();

		return s;

	};
};

#undef P
#undef D
#undef P_


class SettingWindows::Content : public Component
{
public:

	Content()
	{
		addAndMakeVisible(&properties);
		properties.setLookAndFeel(&pplaf);

		pplaf.setFontForAll(GLOBAL_BOLD_FONT());
		pplaf.setLabelWidth(190);

	}

	void resized() override
	{
		properties.setBounds(getLocalBounds());
	}

	HiPropertyPanelLookAndFeel pplaf;

	PropertyPanel properties;

};

SettingWindows::SettingWindows(HiseSettings::Data& dataObject_) :
	dataObject(dataObject_),
	projectSettings("Project"),
	developmentSettings("Development"),
	audioSettings("Audio & Midi"),
	allSettings("All"),
	applyButton("Save"),
	cancelButton("Cancel"),
	undoButton("Undo"),
	refresher(this)
{
	addAndMakeVisible(&projectSettings);
	projectSettings.addListener(this);
	projectSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&developmentSettings);
	developmentSettings.addListener(this);
	developmentSettings.setLookAndFeel(&tblaf);

#if IS_STANDALONE_APP
	addAndMakeVisible(&audioSettings);
	audioSettings.addListener(this);
	audioSettings.setLookAndFeel(&tblaf);
#endif

	addAndMakeVisible(&allSettings);
	allSettings.addListener(this);
	allSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&applyButton);
	applyButton.addListener(this);
	applyButton.setLookAndFeel(&alaf);
	applyButton.addShortcut(KeyPress(KeyPress::returnKey));

	addAndMakeVisible(&cancelButton);
	cancelButton.addListener(this);
	cancelButton.setLookAndFeel(&alaf);
	cancelButton.addShortcut(KeyPress(KeyPress::escapeKey));

	addAndMakeVisible(&undoButton);
	undoButton.addListener(this);
	undoButton.setLookAndFeel(&alaf);
	undoButton.addShortcut(KeyPress('z', ModifierKeys::commandModifier, 'Z'));

	projectSettings.setRadioGroupId(1, dontSendNotification);
	allSettings.setRadioGroupId(1, dontSendNotification);
	developmentSettings.setRadioGroupId(1, dontSendNotification);
	audioSettings.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(currentContent = new Content());

	

	addAndMakeVisible(&fuzzySearchBox);
	fuzzySearchBox.addListener(this);
	fuzzySearchBox.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
	fuzzySearchBox.setFont(GLOBAL_FONT());
	fuzzySearchBox.setSelectAllWhenFocused(true);
	fuzzySearchBox.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

	

	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::ProjectSettings));
	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::UserSettings));
	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::CompilerSettings));
	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::ScriptingSettings));
	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::OtherSettings));
	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::AudioSettings));
	settings.add(createFileBasedValueTreeObject(HiseSettings::SettingFiles::MidiSettings));


	dataObject.data.addListener(this);

	setSize(800, 650);

	allSettings.setToggleState(true, sendNotificationSync);
}

SettingWindows::~SettingWindows()
{
	dataObject.data.removeListener(this);

	if (saveOnDestroy)
	{
		for (auto& s : settings)
		{
			s->save();
		}
	}
	else
	{
		while (undoManager.canUndo())
			undoManager.undo();
	}
}





void SettingWindows::buttonClicked(Button* b)
{
	if (b == &allSettings) setContent({ HiseSettings::SettingFiles::ProjectSettings, 
										HiseSettings::SettingFiles::UserSettings, 
										HiseSettings::SettingFiles::CompilerSettings, 
										HiseSettings::SettingFiles::ScriptingSettings, 
										HiseSettings::SettingFiles::OtherSettings,
										HiseSettings::SettingFiles::AudioSettings,
										HiseSettings::SettingFiles::MidiSettings});
	if (b == &projectSettings) setContent({ HiseSettings::SettingFiles::ProjectSettings, 
											HiseSettings::SettingFiles::UserSettings });
	if (b == &developmentSettings) setContent({	HiseSettings::SettingFiles::CompilerSettings, 
											HiseSettings::SettingFiles::ScriptingSettings, 
											HiseSettings::SettingFiles::OtherSettings});
	if (b == &audioSettings) setContent({ HiseSettings::SettingFiles::AudioSettings,
										  HiseSettings::SettingFiles::MidiSettings});
	if (b == &applyButton)
	{
		saveOnDestroy = true;
		destroy();
	}
	if (b == &cancelButton) destroy();

	if (b == &undoButton) undoManager.undo();
}

void SettingWindows::resized()
{
	auto area = getLocalBounds().reduced(1);

	area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	searchBar.removeFromLeft(32);
	searchBar.removeFromBottom(4);
	fuzzySearchBox.setBounds(searchBar);

	auto bottom = area.removeFromBottom(80);

	bottom = bottom.withSizeKeepingCentre(240, 40);

	applyButton.setBounds(bottom.removeFromLeft(80).reduced(5));
	cancelButton.setBounds(bottom.removeFromLeft(80).reduced(5));
	undoButton.setBounds(bottom.removeFromLeft(80).reduced(5));

	auto left = area.removeFromLeft(120);

	
	allSettings.setBounds(left.removeFromTop(40));
	projectSettings.setBounds(left.removeFromTop(40));
	developmentSettings.setBounds(left.removeFromTop(40));
	audioSettings.setBounds(left.removeFromTop(40));

	currentContent->setBounds(area.reduced(10));
	
}


void SettingWindows::paint(Graphics& g)
{
	g.fillAll(Colour((uint32)bgColour));

	auto area = getLocalBounds().reduced(1);

	
	

	auto top = area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	auto s_ = searchBar.removeFromBottom(4);
	auto shadow = FLOAT_RECTANGLE(s_);


	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));

	g.fillRect(searchBar);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));

	g.fillRect(shadow);

	g.setColour(Colour((uint32)tabBgColour));

	g.fillRect(top);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));

	g.setColour(Colours::white);
	g.drawText("Settings", FLOAT_RECTANGLE(top), Justification::centred);

	auto bottom = area.removeFromBottom(80);



	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour((uint32)tabBgColour)));


	g.fillRect(bottom);

	auto s_2 = bottom.removeFromTop(4);

	auto shadow2 = FLOAT_RECTANGLE(s_2);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow2.getY(), Colours::transparentBlack, 0.0f, shadow2.getBottom(), false));

	g.fillRect(shadow2);


	auto left = area.removeFromLeft(120);

	g.setGradientFill(ColourGradient(Colour((uint32)tabBgColour), 0.0f, 0.0f, Colour(0xFF222222), 0.0f, (float)getHeight(), false));


	g.fillRect(left);

	

	g.setColour(Colours::white.withAlpha(0.6f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	path.scaleToFit((float)searchBar.getX()+4.0f, (float)searchBar.getY() + 4.0f, 20.0f, 20.0f, true);

	g.fillPath(path);

	g.setColour(Colour(0xFF666666));
	g.drawRect(getLocalBounds(), 1);

}

void SettingWindows::textEditorTextChanged(TextEditor&)
{
	setContent(currentList);
}

void SettingWindows::setContent(SettingList list)
{
	currentContent->properties.clear();

	currentList = list;

	for (auto vt : settings)
	{
		if (list.contains(vt->s))
		{
			vt->fillPropertyPanel(currentContent->properties, fuzzySearchBox.getText().toLowerCase());
		}
	}
	
	resized();
}

void SettingWindows::TabButtonLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown)
{
	auto bounds = b.getLocalBounds();
	auto s_ = bounds.removeFromBottom(3);
	auto shadow = FLOAT_RECTANGLE(s_);

	if (b.getToggleState())
	{
		g.setColour(Colour((uint32)ColourValues::bgColour));
		g.fillRect(bounds);
		g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));
		g.fillRect(shadow);
	}
	
	if (isButtonDown)
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRect(bounds);
	}

	g.setColour(Colours::white.withAlpha(isMouseOverButton ? 1.0f : 0.8f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(b.getButtonText(), bounds.reduced(5), Justification::centredRight);

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(b.getBottom()-3, (float)b.getX(), (float)b.getRight());
}

void SettingWindows::FileBasedValueTree::fillPropertyPanel(PropertyPanel& panel, const String& searchText)
{
	Array<PropertyComponent*> props;

	for (auto c : getValueTree())
	{
		auto searchString = c.getProperty("description").toString() + " " + c.getType().toString();
		searchString = searchString.toLowerCase();

#if USE_BACKEND
		if (searchText.isEmpty() || FuzzySearcher::fitsSearch(searchText, searchString, 0.2))
		{
			addProperty(c, props);
		}
#else
		addProperty(c, props);
#endif
	}

	if (props.size() > 0)
	{
		panel.addSection(getId(), props);

		for (auto pr : props)
		{
			auto n = pr->getName().removeCharacters(" ");

			auto help = SettingDescription::getDescription(n);

			if (help.isNotEmpty())
			{
				MarkdownHelpButton* helpButton = new MarkdownHelpButton();
				helpButton->setFontSize(15.0f);
				helpButton->setHelpText(help);
				helpButton->attachTo(pr, MarkdownHelpButton::OverlayLeft);
			}
		}
	}
}

class ToggleButtonListPropertyComponent : public PropertyComponent,
										  public ToggleButtonList::Listener
{
public:

	ToggleButtonListPropertyComponent(const String& name, Value v_, const StringArray& names_) :
		PropertyComponent(name),
		v(v_),
		names(names_),
		l(names_, this)
	{
		values = BigInteger((int64)v_.getValue());

		addAndMakeVisible(&l);

		

		setPreferredHeight(l.getHeight());
	};

	void refresh() override
	{
		auto v_ = (int64)v.getValue();

		values = BigInteger(v_);

		for (int i = 0; i < names.size(); i++)
		{
			l.setValue(i, values[i], dontSendNotification);
		}

	}

	void periodicCheckCallback(ToggleButtonList* /*list*/) override
	{
	}

	void toggleButtonWasClicked(ToggleButtonList* /*list*/, int index, bool value) override
	{
		values.setBit(index, value);

		v = values.toInt64();
	}

	BigInteger values;

	ToggleButtonList l;
	Value v;
	StringArray names;
};

class FileNameValuePropertyComponent : public PropertyComponent,
									   public FilenameComponentListener
{
public:

	FileNameValuePropertyComponent(const String& name, File initialFile, Value v_) :
		PropertyComponent(name),
		v(v_),
		fc(name, initialFile, true, true, false, ".*", "", "Select directory")
	{
		addAndMakeVisible(fc);

		fc.addListener(this);	
	}

	void refresh() override
	{
		fc.setCurrentFile(v.getValue().toString(), true, dontSendNotification);
	}

	void filenameComponentChanged(FilenameComponent* /*fileComponentThatHasChanged*/) override
	{
		v.setValue(fc.getCurrentFile().getFullPathName());
	}

	FilenameComponent fc;
	Value v;
};

void SettingWindows::FileBasedValueTree::addProperty(ValueTree& c, Array<PropertyComponent*>& props)
{
	auto value = c.getPropertyAsValue("value", &p->undoManager);
	auto type = c.getProperty("type").toString();
	auto name = getUncamelcasedId(c.getType());
	auto id = c.getType();

	auto items = p->dataObject.getOptionsFor(id);

	if (HiseSettings::Data::isFileId(id))
	{
		auto fpc = new FileNameValuePropertyComponent(name, File(value.toString()), value);

		props.add(fpc);
	}
	else if (id == HiseSettings::Midi::MidiInput)
	{
		auto tblpc = new ToggleButtonListPropertyComponent(name, value, items);

		props.add(tblpc);
	}
	else if (items.size() > 0)
	{
		if (items[0] == "Yes")
		{
			auto bpc = new BooleanPropertyComponent(value, name, "Enabled");

			dynamic_cast<ToggleButton*>(bpc->getChildComponent(0))->setLookAndFeel(&blaf);

			bpc->setColour(BooleanPropertyComponent::ColourIds::backgroundColourId, Colours::transparentBlack);
			bpc->setColour(BooleanPropertyComponent::ColourIds::outlineColourId, Colours::transparentBlack);

			props.add(bpc);
		}
		else
		{
			Array<var> choiceValues;

			for (auto cv : items)
				choiceValues.add(cv);

			props.add(new ChoicePropertyComponent(value, name, items, choiceValues));
		}
	}
	
	else
	{
		props.add(new TextPropertyComponent(value, name, 1024, name.contains("Extra")));
	}
}

juce::String SettingWindows::FileBasedValueTree::getId() const
{
	return getUncamelcasedId(getValueTree().getType());
}

juce::Result SettingWindows::checkInput(const Identifier& id, const var& newValue)
{
	if (id == HiseSettings::Other::AutosaveInterval && !TestFunctions::isValidNumberBetween(newValue, { 1.0f, 30.0f }))
		return Result::fail("The autosave interval must be between 1 and 30 minutes");


	if (id == HiseSettings::Project::Version)
	{
		const String version = newValue.toString();
		SemanticVersionChecker versionChecker(version, version);

		if (!versionChecker.newVersionNumberIsValid())
		{
			return Result::fail("The version number is not a valid semantic version number. Use something like 1.0.0.\n " \
				"This is required for the user presets to detect whether it should ask for updating the presets after a version bump.");
		};
	}

	if (id == HiseSettings::Project::AppGroupID || id == HiseSettings::Project::BundleIdentifier)
	{
		const String wildcard = (id == HiseSettings::Project::BundleIdentifier) ?
										R"(com\.[\w\d-_]+\.[\w\d-_]+$)" :
										R"(group\.[\w\d-_]+\.[\w\d-_]+$)";

		if (!RegexFunctions::matchesWildcard(wildcard, newValue.toString()))
		{
			return Result::fail(id.toString() + " doesn't match the required format.");
		}
	}

	if (id == HiseSettings::Project::PluginCode || id == HiseSettings::User::CompanyCode)
	{
		const String pluginCode = newValue.toString();
		const String codeWildcard = "[A-Z][a-z][a-z][a-z]";

		if (pluginCode.length() != 4 || !RegexFunctions::matchesWildcard(codeWildcard, pluginCode))
		{
			return Result::fail("The code doesn't match the required formula. Use something like 'Abcd'\n" \
				"This is required for exported AU plugins to pass the AU validation.");
		};
	}
	
	if (id == HiseSettings::Project::Name || id == HiseSettings::User::Company)
	{
		const String name = newValue.toString();

		if (!name.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
		{
			return Result::fail("Illegal Project name\n" \
				"The Project name must not contain exotic characters");
		}

		if (name.isEmpty())
			return Result::fail("The project name / company name must not be empty");
	}
	
	if (id == HiseSettings::Compiler::HisePath)
	{
		File f = File(newValue.toString());

		if (!f.isDirectory())
			return Result::fail("The HISE path is not a valid directory");

		if (!f.getChildFile("hi_core").isDirectory())
			return Result::fail("The HISE path does not contain the HISE source code");
	}

	if (id == HiseSettings::Scripting::GlobalScriptPath && !File(newValue.toString()).isDirectory())
		return Result::fail("The global script folder is not a valid directory");
		

	return Result::ok();
}

void SettingWindows::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& p)
{
	const Identifier va("value");
	auto id = treeWhosePropertyHasChanged.getType();
	auto value = treeWhosePropertyHasChanged.getProperty("value");

	jassert(p == va);

	auto result = checkInput(id, value);

	if (result.wasOk())
	{
		settingWasChanged(id, value);
	}
	else
	{
		if(PresetHandler::showYesNoWindow("Wrong input", result.getErrorMessage() + "\nPress OK to load the default value."))
		{
			treeWhosePropertyHasChanged.setProperty(va, dataObject.getDefaultSetting(id), nullptr);
		}
	}
}

void SettingWindows::settingWasChanged(const Identifier& id, const var& newValue)
{
	if (id == HiseSettings::Scripting::EnableCallstack)
		dataObject.getMainController()->updateCallstackSettingForExistingScriptProcessors();

	else if (id == HiseSettings::Scripting::CodeFontSize)
		dataObject.getMainController()->getFontSizeChangeBroadcaster().sendChangeMessage();

	else if (id == HiseSettings::Other::EnableAutosave ||
			 id == HiseSettings::Other::AutosaveInterval)
		dataObject.getMainController()->getAutoSaver().updateAutosaving();

	else if (id == HiseSettings::Scripting::EnableDebugMode)
	{
		newValue ? dataObject.getMainController()->getDebugLogger().startLogging() :
				   dataObject.getMainController()->getDebugLogger().stopLogging();
	}

	else if (id == HiseSettings::Audio::Samplerate)
	{
		auto driver = dynamic_cast<AudioProcessorDriver*>(dataObject.getMainController());
		driver->setCurrentSampleRate(newValue.toString().getDoubleValue());
	}
	else if (id == HiseSettings::Audio::BufferSize)
	{
		auto driver = dynamic_cast<AudioProcessorDriver*>(dataObject.getMainController());
		driver->setCurrentBlockSize(newValue.toString().getIntValue());
	}
	else if (id == HiseSettings::Audio::Driver)
	{
		auto driver = dynamic_cast<AudioProcessorDriver*>(dataObject.getMainController());
		driver->deviceManager->setCurrentAudioDeviceType(newValue.toString(), true);

		auto device = driver->deviceManager->getCurrentAudioDevice();

		if (device == nullptr)
		{
			PresetHandler::showMessageWindow("Error initialising driver", "The audio driver could not be opened. The default settings will be loaded.", PresetHandler::IconType::Error);
			driver->resetToDefault();
			
		}

		dataObject.initialiseAudioDriverData(true);

		refresher.triggerAsyncUpdate();
		
	}
	else if (id == HiseSettings::Audio::Device)
	{
		auto driver = dynamic_cast<AudioProcessorDriver*>(dataObject.getMainController());
		driver->setAudioDevice(newValue.toString());


		auto device = driver->deviceManager->getCurrentAudioDevice();

		if (device == nullptr)
		{
			PresetHandler::showMessageWindow("Error initialising driver", "The audio driver could not be opened. The default settings will be loaded.", PresetHandler::IconType::Error);
			driver->resetToDefault();
		}

		dataObject.initialiseAudioDriverData(true);
		refresher.triggerAsyncUpdate();
		
	}
	else if (id == HiseSettings::Midi::MidiInput)
	{
		auto state = BigInteger((int64)newValue);
		auto driver = dynamic_cast<AudioProcessorDriver*>(dataObject.getMainController());

		auto midiNames = MidiInput::getDevices();

		for (int i = 0; i < midiNames.size(); i++)
		{
			driver->toggleMidiInput(midiNames[i], state[i]);
		}
	}
	else if (id == HiseSettings::Midi::MidiChannels)
	{
		auto sa = HiseSettings::ConversionHelpers::getChannelList();

		auto index = sa.indexOf(newValue.toString());

		BigInteger s = 0;
		s.setBit(index, true);

		auto intValue = s.toInteger();

		auto data = dataObject.getMainController()->getMainSynthChain()->getActiveChannelData();

		data->restoreFromData(intValue);

	}

}

void SettingWindows::valueTreeChildAdded(ValueTree&, ValueTree&)
{
	refresher.triggerAsyncUpdate();
}

hise::SettingWindows::FileBasedValueTree* SettingWindows::createFileBasedValueTreeObject(Identifier s)
{
	return new FileBasedValueTree(s, dataObject.getFileForSetting(s), this);
}

void SettingWindows::FileBasedValueTree::save()
{
	// This will be saved by the audio device manager
	if (s == HiseSettings::SettingFiles::MidiSettings || s == HiseSettings::SettingFiles::AudioSettings)
		return;

	for (auto c : getValueTree())
	{
		if (c.getProperty("options").toString() == "Yes&#10;No")
		{
			c.setProperty("value", c.getProperty("value")? "Yes" : "No", nullptr);
		}
	}

	ScopedPointer<XmlElement> xml = getValueTree().createXml();


	xml->writeToFile(f, "");
}

HiseSettings::Data::Data(MainController* mc_) :
	mc(mc_),
	data("SettingRoot")
{

	data.addChild(ValueTree(SettingFiles::ProjectSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::CompilerSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::GeneralSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::AudioSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::MidiSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::ScriptingSettings), -1, nullptr);

	loadDataFromFiles();
}

juce::File HiseSettings::Data::getFileForSetting(const Identifier& id) const
{
	auto handler = &GET_PROJECT_HANDLER(mc->getMainSynthChain());
	auto appDataFolder = File(PresetHandler::getDataFolder());

	if (id == SettingFiles::ProjectSettings)
		return handler->getWorkDirectory().getChildFile("project_info.xml");

	else if (id == SettingFiles::UserSettings)
		return handler->getWorkDirectory().getChildFile("user_info.xml");

	else if (id == SettingFiles::CompilerSettings)
		return appDataFolder.getChildFile("compilerSettings.xml");

	else if (id == SettingFiles::AudioSettings)
		return appDataFolder.getChildFile("DeviceSettings.xml");

	else if (id == SettingFiles::MidiSettings)
		return appDataFolder.getChildFile("DeviceSettings.xml");

	else if (id == SettingFiles::GeneralSettings)
		return appDataFolder.getChildFile("GeneralSettings.xml");

	else if (id == SettingFiles::ScriptingSettings)
		return appDataFolder.getChildFile("ScriptSettings.xml");

	else if (id == SettingFiles::OtherSettings)
		return appDataFolder.getChildFile("OtherSettings.xml");

	jassertfalse;

	return File();
}

void HiseSettings::Data::loadDataFromFiles()
{
	refreshProjectData();

	loadSettingsFromFile(SettingFiles::CompilerSettings);
	loadSettingsFromFile(SettingFiles::ScriptingSettings);
	loadSettingsFromFile(SettingFiles::OtherSettings);
	loadSettingsFromFile(SettingFiles::AudioSettings);
	loadSettingsFromFile(SettingFiles::MidiSettings);
	
}

void HiseSettings::Data::refreshProjectData()
{
	loadSettingsFromFile(SettingFiles::ProjectSettings);
	loadSettingsFromFile(SettingFiles::UserSettings);
}

void HiseSettings::Data::loadSettingsFromFile(const Identifier& id)
{
	auto f = getFileForSetting(id);

	ValueTree v = ConversionHelpers::loadValueTreeFromFile(f, id);

	if (!v.isValid())
		v = ValueTree(id);

	data.removeChild(data.getChildWithName(id), nullptr);
	data.addChild(v, -1, nullptr);

	addMissingSettings(v, id);
}

void HiseSettings::Data::addSetting(ValueTree& v, const Identifier& id)
{
	if (v.getChildWithName(id).isValid())
		return;

	ValueTree child(id);
	child.setProperty("value", getDefaultSetting(id), nullptr);
	v.addChild(child, -1, nullptr);
}

juce::StringArray HiseSettings::Data::getOptionsFor(const Identifier& id)
{
	if (id == Project::EmbedAudioFiles || 
		id == Compiler::UseIPP ||
		id == Scripting::EnableCallstack ||
		id == Other::EnableAutosave || 
		id == Scripting::EnableDebugMode)
		return { "Yes", "No" };

	if (id == Compiler::VisualStudioVersion)
		return { "Visual Studio 2015", "Visual Studio 2017" };

#if IS_STANDALONE_APP
	else if (id == Audio::Driver || id == Audio::Device || id == Audio::Samplerate || id == Audio::BufferSize)
	{
		auto manager = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager;
		StringArray sa;

		if (id == Audio::Driver)
		{
			const auto& list = manager->getAvailableDeviceTypes();

			for (auto l : list)
				sa.add(l->getTypeName());

		}
		else if (id == Audio::Device)
		{
			const auto currentDevice = manager->getCurrentDeviceTypeObject();

			return currentDevice->getDeviceNames();
		}
		else if (id == Audio::BufferSize)
		{
			const auto currentDevice = manager->getCurrentAudioDevice();
			
			const auto& bs = ConversionHelpers::getBufferSizesForDevice(currentDevice);

			for (auto l : bs)
				sa.add(String(l));
		}
		else if (id == Audio::Samplerate)
		{
			const auto currentDevice = manager->getCurrentAudioDevice();

			const auto& bs = ConversionHelpers::getSampleRates(currentDevice);

			for (auto l : bs)
				sa.add(String(roundDoubleToInt(l)));
		}
		return sa;
	}
	else if (id == Midi::MidiInput)
	{
		return MidiInput::getDevices();
	}
#endif
	else if (id == Midi::MidiChannels)
	{
		return ConversionHelpers::getChannelList();
	}


	return {};
}

bool HiseSettings::Data::isFileId(const Identifier& id)
{
	if (id == Compiler::HisePath)
		return true;

	if (id == Scripting::GlobalScriptPath)
		return true;

	return false;
}


void HiseSettings::Data::addMissingSettings(ValueTree& v, const Identifier &id)
{
	if (id == SettingFiles::ProjectSettings)
	{
		addSetting(v, Project::Name);
		addSetting(v, Project::Version);
		addSetting(v, Project::Description);
		addSetting(v, Project::BundleIdentifier);
		addSetting(v, Project::PluginCode);
		addSetting(v, Project::EmbedAudioFiles);
		addSetting(v, Project::AdditionalDspLibraries);
		addSetting(v, Project::WindowsStaticLibFolder);
		addSetting(v, Project::OSXStaticLibs);
		addSetting(v, Project::ExtraDefinitionsWindows);
		addSetting(v, Project::ExtraDefinitionsOSX);
		addSetting(v, Project::ExtraDefinitionsIOS);
		addSetting(v, Project::AppGroupID);
	}
	else if (id == SettingFiles::UserSettings)
	{
		addSetting(v, User::Company);
		addSetting(v, User::CompanyCode);
		addSetting(v, User::CompanyCopyright);
		addSetting(v, User::CompanyURL);
		addSetting(v, User::TeamDevelopmentID);
	}
	else if (id == SettingFiles::CompilerSettings)
	{
		addSetting(v, Compiler::HisePath);
		addSetting(v, Compiler::VisualStudioVersion);
		addSetting(v, Compiler::UseIPP);
	}
	else if (id == SettingFiles::ScriptingSettings)
	{
		addSetting(v, Scripting::CodeFontSize);
		addSetting(v, Scripting::EnableCallstack);
		addSetting(v, Scripting::CompileTimeout);
		addSetting(v, Scripting::EnableDebugMode);
		addSetting(v, Scripting::GlobalScriptPath);
	}
	else if (id == SettingFiles::OtherSettings)
	{
		addSetting(v, Other::EnableAutosave);
		addSetting(v, Other::AutosaveInterval);
	}
}

void HiseSettings::Data::initialiseAudioDriverData(bool forceReload/*=false*/)
{
#if IS_STANDALONE_APP
	auto v = data.getChildWithName(SettingFiles::AudioSettings);

	static const Identifier va("value");

	if (forceReload)
	{
		v.getChildWithName(Audio::Driver).setProperty(va, getDefaultSetting(Audio::Driver), nullptr);
		v.getChildWithName(Audio::Device).setProperty(va, getDefaultSetting(Audio::Device), nullptr);
		v.getChildWithName(Audio::Samplerate).setProperty(va, getDefaultSetting(Audio::Samplerate), nullptr);
		v.getChildWithName(Audio::BufferSize).setProperty(va, getDefaultSetting(Audio::BufferSize), nullptr);
	}
	else
	{
		addSetting(v, Audio::Driver);
		addSetting(v, Audio::Device);
		addSetting(v, Audio::Samplerate);
		addSetting(v, Audio::BufferSize);
	}
	
	auto v2 = data.getChildWithName(SettingFiles::MidiSettings);

	if (forceReload)
	{
		v2.getChildWithName(Midi::MidiInput).setProperty(va, getDefaultSetting(Midi::MidiInput), nullptr);
		v2.getChildWithName(Midi::MidiChannels).setProperty(va, getDefaultSetting(Midi::MidiChannels), nullptr);
	}
	else
	{
		addSetting(v2, Midi::MidiInput);
		addSetting(v2, Midi::MidiChannels);
	}
#endif

	
}



var HiseSettings::Data::getDefaultSetting(const Identifier& id)
{
	if (id == Project::Name)
	{
		auto handler = &GET_PROJECT_HANDLER(mc->getMainSynthChain());
		return handler->getWorkDirectory().getFileName();
	}
	else if (id == Project::Version) return "1.0.0";
	else if (id == Project::BundleIdentifier) return "com.myCompany.product";
	else if (id == Project::PluginCode) return "Abcd";
	else if (id == Project::EmbedAudioFiles) return "No";
	else if (id == Other::EnableAutosave) return "Yes";
	else if (id == Other::AutosaveInterval) return 5;
	else if (id == Scripting::CodeFontSize) return 17.0;
	else if (id == Scripting::EnableCallstack) return "No";
	else if (id == Scripting::CompileTimeout) return 5.0;
	else if (id == Compiler::VisualStudioVersion) return "Visual Studio 2017";
	else if (id == Compiler::UseIPP)				return "Yes";
	else if (id == User::CompanyURL) return "http://yourcompany.com";
	else if (id == User::CompanyCopyright) return "(c)2017, Company";
	else if (id == User::CompanyCode) return "Abcd";
	else if (id == User::Company) return "My Company";
	else if (id == Scripting::GlobalScriptPath)
	{
		File scriptFolder = File(PresetHandler::getDataFolder()).getChildFile("scripts");
		if (!scriptFolder.isDirectory())
			scriptFolder.createDirectory();

		return scriptFolder.getFullPathName();
	}
	else if (id == Scripting::EnableDebugMode)
		return mc->getDebugLogger().isLogging();
	else if (id == Audio::Driver)
		return dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager->getCurrentAudioDeviceType();
	else if (id == Audio::Device)
	{
		auto device = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager->getCurrentAudioDevice();
		return device != nullptr ? device->getName() : "No Device";
	}
	else if (id == Audio::Samplerate)
		return dynamic_cast<AudioProcessorDriver*>(mc)->getCurrentSampleRate();
	else if (id == Audio::BufferSize)
		return dynamic_cast<AudioProcessorDriver*>(mc)->getCurrentBlockSize();

	else if (id == Midi::MidiInput)
		return dynamic_cast<AudioProcessorDriver*>(mc)->getMidiInputState().toInt64();
	else if (id == Midi::MidiChannels)
	{
		auto state = BigInteger(dynamic_cast<AudioProcessorDriver*>(mc)->getChannelData());

		auto firstSetBit = state.getHighestBit();

		return ConversionHelpers::getChannelList()[firstSetBit];
	}
		

	return var();
}

bool SettingWindows::TestFunctions::isValidNumberBetween(var value, Range<float> range)
{
	auto number = value.toString().getFloatValue();

	if (std::isnan(number))
		return false;

	if (std::isinf(number))
		return false;

	number = FloatSanitizers::sanitizeFloatNumber(number);

	return range.contains(number);

}

void addChildWithValue(ValueTree& v, const Identifier& id, const var& newValue)
{
	static const Identifier va("value");
	ValueTree c(id);
	c.setProperty(va, newValue, nullptr);
	v.addChild(c, -1, nullptr);
}

juce::ValueTree HiseSettings::ConversionHelpers::loadValueTreeFromFile(const File& f, const Identifier& settingId)
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		return loadValueTreeFromXml(xml, settingId);
	}

	return ValueTree();
}

juce::ValueTree HiseSettings::ConversionHelpers::loadValueTreeFromXml(XmlElement* xml, const Identifier& settingId)
{
	ValueTree v = ValueTree::fromXml(*xml);


	static const Identifier audioDeviceId("DEVICESETUP");

	if (v.getType() == audioDeviceId)
	{
		ValueTree v2(settingId);


		if (settingId == SettingFiles::AudioSettings)
		{
#if IS_STANDALONE_APP
			addChildWithValue(v2, HiseSettings::Audio::Driver, xml->getStringAttribute("deviceType"));
			addChildWithValue(v2, HiseSettings::Audio::Device, xml->getStringAttribute("audioOutputDeviceName"));
			addChildWithValue(v2, HiseSettings::Audio::Samplerate, xml->getStringAttribute("audioDeviceRate"));
			addChildWithValue(v2, HiseSettings::Audio::BufferSize, xml->getStringAttribute("bufferSize", "512"));
#endif

			return v2;
		}
		else if (settingId == SettingFiles::MidiSettings)
		{
			StringArray active;

			for (int i = 0; i < xml->getNumChildElements(); i++)
			{
				if (xml->getChildElement(i)->hasTagName("MIDIINPUT"))
				{
					active.add(xml->getChildElement(i)->getStringAttribute("name"));
				}
			}

			StringArray allInputs = MidiInput::getDevices();

			BigInteger values;

			for (auto input : active)
			{
				int index = allInputs.indexOf(input);

				if (index != -1)
					values.setBit(index, true);
			}

#if IS_STANDALONE_APP
			addChildWithValue(v2, Midi::MidiInput, values.toInt64());
#endif

			return v2;
		}
	}

	return v;
}



juce::XmlElement* HiseSettings::ConversionHelpers::getConvertedXml(const ValueTree& v)
{
	return v.createXml();
}

Array<int> HiseSettings::ConversionHelpers::getBufferSizesForDevice(AudioIODevice* currentDevice)
{
	if (currentDevice == nullptr)
		return {};

	auto bufferSizes = currentDevice->getAvailableBufferSizes();

	if (bufferSizes.size() > 7)
	{
		Array<int> powerOfTwoBufferSizes;
		powerOfTwoBufferSizes.ensureStorageAllocated(6);
		if (bufferSizes.contains(64)) powerOfTwoBufferSizes.add(64);
		if (bufferSizes.contains(128)) powerOfTwoBufferSizes.add(128);
		if (bufferSizes.contains(256)) powerOfTwoBufferSizes.add(256);
		if (bufferSizes.contains(512)) powerOfTwoBufferSizes.add(512);
		if (bufferSizes.contains(1024)) powerOfTwoBufferSizes.add(1024);

		if (powerOfTwoBufferSizes.size() > 2)
			bufferSizes.swapWith(powerOfTwoBufferSizes);
	}

	auto currentSize = currentDevice->getCurrentBufferSizeSamples();

	bufferSizes.addIfNotAlreadyThere(currentSize);

	int defaultBufferSize = currentDevice->getDefaultBufferSize();

	bufferSizes.addIfNotAlreadyThere(defaultBufferSize);

	bufferSizes.sort();

	return bufferSizes;
}

Array<double> HiseSettings::ConversionHelpers::getSampleRates(AudioIODevice* currentDevice)
{

#if HISE_IOS

	Array<double> samplerates;

	samplerates.add(44100.0);
	samplerates.add(48000.0);

#else

	if (currentDevice == nullptr)
		return {};

	Array<double> allSamplerates = currentDevice->getAvailableSampleRates();
	Array<double> samplerates;

	if (allSamplerates.contains(44100.0)) samplerates.add(44100.0);
	if (allSamplerates.contains(48000.0)) samplerates.add(48000.0);
	if (allSamplerates.contains(88200.0)) samplerates.add(88200.0);
	if (allSamplerates.contains(96000.0)) samplerates.add(96000.0);
	if (allSamplerates.contains(176400.0)) samplerates.add(176400.0);
	if (allSamplerates.contains(192000.0)) samplerates.add(192000.0);

#endif

	return samplerates;

}

juce::StringArray HiseSettings::ConversionHelpers::getChannelList()
{
	StringArray sa;

	sa.add("All channels");
	for (int i = 0; i < 16; i++)
		sa.add("Channel " + String(i + 1));

	return sa;
}

} // namespace hise
