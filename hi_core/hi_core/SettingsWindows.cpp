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

#define P(p) if (prop == p) { s << "### " << p.toString() << "\n";
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
	
		P(HiseSettings::Project::AppGroupId);
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

		P(HiseSettings::Compiler::UseIPP);
		D("If enabled, HISE uses the FFT routines from the Intel Performance Primitive library (which can be downloaded for free) in order");
		D("to speed up the convolution reverb");
		D("> If you use the convolution reverb in your project, this is almost mandatory, but there are a few other places that benefit from having this library");
		P_();

		return s;

	};

	

	static void sanityCheck()
	{
#if 0 // Project Sanity check
		const String version = GET_VALUE_FROM_XML(ProjectSettingWindow::Attributes::Version);

		SemanticVersionChecker versionChecker(version, version);

		if (!versionChecker.newVersionNumberIsValid())
		{
			return "The version number is not a valid semantic version number. Use something like 1.0.0.\n " \
				"This is required for the user presets to detect whether it should ask for updating the presets after a version bump.";
		};

		const String pluginCode = GET_VALUE_FROM_XML(ProjectSettingWindow::Attributes::PluginCode);
		const String codeWildcard = "[A-Z][a-z][a-z][a-z]";

		if (!RegexFunctions::matchesWildcard(codeWildcard, pluginCode))
		{
			return "The plugin code doesn't match the required formula. Use something like 'Abcd'\n" \
				"This is required for exported AU plugins to pass the AU validation.";
		};

		const String projectName = GET_VALUE_FROM_XML(ProjectSettingWindow::Attributes::Name);

		if (!projectName.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890 _-"))
		{
			return "Illegal Project name\n" \
				"The Project name must not contain exotic characters";
		}

#endif
	}
};

#undef P
#undef D
#undef P_


String SettingWindows::getSettingValue(int attributeIndex, ProjectHandler *handler)
{
	jassertfalse;

	return "";
}

void SettingWindows::checkAllSettings(ProjectHandler *handler)
{
	jassertfalse;
}

#if 0
File SettingWindows::getFileForSettingsWindow(Settings s, ProjectHandler *handler)
{
	switch (s)
	{
	case SettingWindows::Settings::Project:
		if (handler != nullptr) return handler->getWorkDirectory().getChildFile("project_info.xml");

		
		jassertfalse;
		return File();
		break;
	case SettingWindows::Settings::User:
		if (handler != nullptr) return handler->getWorkDirectory().getChildFile("user_info.xml");
		break;
	case SettingWindows::Settings::Compiler: return File(PresetHandler::getDataFolder()).getChildFile("compilerSettings.xml");
		break;
	case SettingWindows::Settings::Audio:	 return File(PresetHandler::getDataFolder()).getChildFile("DeviceSettings.xml");
	case SettingWindows::Settings::Global:	 return File(PresetHandler::getDataFolder()).getChildFile("GeneralSettings.xml");
	case SettingWindows::Settings::numSettings:
		break;
	default:
		break;
	}

	return File();
}
#endif


class SettingWindows::NewSettingWindows::Content : public Component
{
public:

	Content(NewSettingWindows* parent)
	{
		addAndMakeVisible(&properties);
		properties.setLookAndFeel(&pplaf);

		pplaf.setFontForAll(GLOBAL_BOLD_FONT());
		pplaf.setLabelWidth(190);

		setSearchText("");
	}

	void setSearchText(const String& searchText)
	{
		properties.clear();
	}

	void resized() override
	{
		properties.setBounds(getLocalBounds());
	}

	HiPropertyPanelLookAndFeel pplaf;

	PropertyPanel properties;

};

SettingWindows::NewSettingWindows::NewSettingWindows(HiseSettings::Data& dataObject_) :
	dataObject(dataObject_),
	projectSettings("Project"),
	globalSettings("Global"),
	allSettings("All"),
	applyButton("Save"),
	cancelButton("Cancel")
{
	addAndMakeVisible(&projectSettings);
	projectSettings.addListener(this);
	projectSettings.setLookAndFeel(&tblaf);

	addAndMakeVisible(&globalSettings);
	globalSettings.addListener(this);
	globalSettings.setLookAndFeel(&tblaf);

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

	projectSettings.setRadioGroupId(1, dontSendNotification);
	allSettings.setRadioGroupId(1, dontSendNotification);
	globalSettings.setRadioGroupId(1, dontSendNotification);

	addAndMakeVisible(currentContent = new Content(this));

	

	addAndMakeVisible(&fuzzySearchBox);
	fuzzySearchBox.addListener(this);
	fuzzySearchBox.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
	fuzzySearchBox.setFont(GLOBAL_FONT());
	fuzzySearchBox.setSelectAllWhenFocused(true);
	fuzzySearchBox.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

	settings.add(getProperlyFormattedValueTree(HiseSettings::SettingFiles::ProjectSettings));
	settings.add(getProperlyFormattedValueTree(HiseSettings::SettingFiles::UserSettings));
	settings.add(getProperlyFormattedValueTree(HiseSettings::SettingFiles::CompilerSettings));

	setSize(800, 650);

	allSettings.setToggleState(true, sendNotificationSync);
}

SettingWindows::NewSettingWindows::~NewSettingWindows()
{
	if (saveOnDestroy)
	{
		for (auto& s : settings)
		{
			s->save();
		}
	}
}

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





void SettingWindows::NewSettingWindows::resized()
{
	auto area = getLocalBounds().reduced(1);

	area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	searchBar.removeFromLeft(32);
	searchBar.removeFromBottom(4);
	fuzzySearchBox.setBounds(searchBar);

	auto bottom = area.removeFromBottom(80);

	bottom = bottom.withSizeKeepingCentre(200, 40);

	applyButton.setBounds(bottom.removeFromLeft(100).reduced(5));
	cancelButton.setBounds(bottom.removeFromLeft(100).reduced(5));

	auto left = area.removeFromLeft(120);

	
	allSettings.setBounds(left.removeFromTop(40));
	projectSettings.setBounds(left.removeFromTop(40));
	globalSettings.setBounds(left.removeFromTop(40));
	

	currentContent->setBounds(area.reduced(10));
	
}


void SettingWindows::NewSettingWindows::paint(Graphics& g)
{
	g.fillAll(Colour(bgColour));

	auto area = getLocalBounds().reduced(1);

	
	

	auto top = area.removeFromTop(50);

	auto searchBar = area.removeFromTop(32);
	auto s_ = searchBar.removeFromBottom(4);
	auto shadow = FLOAT_RECTANGLE(s_);


	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF333333)));

	g.fillRect(searchBar);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow.getY(), Colours::transparentBlack, 0.0f, shadow.getBottom(), false));

	g.fillRect(shadow);

	g.setColour(Colour(tabBgColour));

	g.fillRect(top);

	g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));

	g.setColour(Colours::white);
	g.drawText("Settings", FLOAT_RECTANGLE(top), Justification::centred);

	auto bottom = area.removeFromBottom(80);



	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(tabBgColour)));


	g.fillRect(bottom);

	auto s_2 = bottom.removeFromTop(4);

	auto shadow2 = FLOAT_RECTANGLE(s_2);

	g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.2f), 0.0f, shadow2.getY(), Colours::transparentBlack, 0.0f, shadow2.getBottom(), false));

	g.fillRect(shadow2);


	auto left = area.removeFromLeft(120);

	g.setGradientFill(ColourGradient(Colour(tabBgColour), 0.0f, 0.0f, Colour(0xFF222222), 0.0f, (float)getHeight(), false));


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

void SettingWindows::NewSettingWindows::textEditorTextChanged(TextEditor&)
{
	setContent(currentList);
}

void SettingWindows::NewSettingWindows::setContent(SettingList list)
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

void SettingWindows::NewSettingWindows::TabButtonLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown)
{
	
	auto bounds = b.getLocalBounds();

	

	auto s_ = bounds.removeFromBottom(3);

	auto shadow = FLOAT_RECTANGLE(s_);

	if (b.getToggleState())
	{
		g.setColour(Colour(ColourValues::bgColour));
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
	g.drawHorizontalLine(b.getBottom()-3, b.getX(), b.getRight());
}

void SettingWindows::NewSettingWindows::FileBasedValueTree::fillPropertyPanel(PropertyPanel& panel, const String& searchText)
{
	Array<PropertyComponent*> props;

	for (auto c : v)
	{
		auto searchString = c.getProperty("description").toString() + " " + c.getType().toString();
		searchString = searchString.toLowerCase();

		if (searchText.isEmpty() || FuzzySearcher::fitsSearch(searchText, searchString, 0.2))
		{
			addProperty(c, props);
		}
	}

	if (props.size() > 0)
	{
		panel.addSection(getId(), props);

		for (auto p : props)
		{
			auto n = p->getName();

			auto help = SettingDescription::getDescription(n);

			if (help.isNotEmpty())
			{
				MarkdownHelpButton* helpButton = new MarkdownHelpButton();
				helpButton->setFontSize(15.0f);
				helpButton->setHelpText(help);
				helpButton->attachTo(p, MarkdownHelpButton::OverlayLeft);
			}
		}
	}
}

class FileNameValuePropertyComponent : public PropertyComponent,
									   public FilenameComponentListener
{
public:

	FileNameValuePropertyComponent(const String& name, File& initialFile, Value v_) :
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

void SettingWindows::NewSettingWindows::FileBasedValueTree::addProperty(ValueTree& c, Array<PropertyComponent*>& props)
{
	auto value = c.getPropertyAsValue("value", nullptr);
	auto type = c.getProperty("type").toString();
	auto name = getUncamelcasedId(c.getType());
	auto id = c.getType();

	auto items = HiseSettings::Data::getOptionsFor(id);

	if (HiseSettings::Data::isFileId(id))
	{
		auto fpc = new FileNameValuePropertyComponent(name, File(value.toString()), value);

		props.add(fpc);
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

			for (auto s : items)
				choiceValues.add(s);

			props.add(new ChoicePropertyComponent(value, name, items, choiceValues));
		}
	}
	else
	{
		props.add(new TextPropertyComponent(value, name, 1024, name.contains("Extra")));
	}
}

juce::String SettingWindows::NewSettingWindows::FileBasedValueTree::getId() const
{
	return getUncamelcasedId(v.getType());
}

            
hise::SettingWindows::NewSettingWindows::FileBasedValueTree* SettingWindows::NewSettingWindows::getProperlyFormattedValueTree(Identifier s)
{
	return new FileBasedValueTree(s, dataObject.getTreeForSettings(s), dataObject.getFileForSetting(s));
}

void SettingWindows::NewSettingWindows::FileBasedValueTree::save()
{
	for (auto c : v)
	{
		if (c.getProperty("options").toString() == "Yes&#10;No")
		{
			c.setProperty("value", c.getProperty("value")? "Yes" : "No", nullptr);
		}
	}

	ScopedPointer<XmlElement> xml = v.createXml();


	xml->writeToFile(f, "");
}

HiseSettings::Data::Data(MainController* mc_) :
	mc(mc_),
	data("SettingRoot")
{

	data.addChild(ValueTree(SettingFiles::ProjectSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::CompilerSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::GeneralSettings), -1, nullptr);
	data.addChild(ValueTree(SettingFiles::DeviceSettings), -1, nullptr);

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

	else if (id == SettingFiles::DeviceSettings)
		return appDataFolder.getChildFile("DeviceSettings.xml");

	else if (id == SettingFiles::GeneralSettings)
		return appDataFolder.getChildFile("GeneralSettings.xml");
}

void HiseSettings::Data::loadDataFromFiles()
{
	refreshProjectData();

	loadSettingsFromFile(SettingFiles::CompilerSettings);
	//loadSettingsFromFile(SettingFiles::DeviceSettings);
	
	//loadSettingsFromFile(SettingFiles::GeneralSettings);
}

void HiseSettings::Data::refreshProjectData()
{
	loadSettingsFromFile(SettingFiles::ProjectSettings);
	loadSettingsFromFile(SettingFiles::UserSettings);
}

void HiseSettings::Data::loadSettingsFromFile(const Identifier& id)
{
	auto f = getFileForSetting(id);

	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	ValueTree v(id);

	if (xml != nullptr)
	{
		jassert(xml->getTagName() == id.toString());

		v = ValueTree::fromXml(*xml);

		
	}

	data.removeChild(data.getChildWithName(id), nullptr);
	data.addChild(v, -1, nullptr);

	addMissingSettings(v, id);
}

void HiseSettings::Data::addSetting(ValueTree& v, const Identifier& id, const String& defaultValue /*= String()*/)
{
	if (v.getChildWithName(id).isValid())
		return;

	ValueTree child(id);
	child.setProperty("value", defaultValue, nullptr);
	v.addChild(child, -1, nullptr);
}

juce::StringArray HiseSettings::Data::getOptionsFor(const Identifier& id)
{
	if (id == Project::EmbedAudioFiles)
		return { "Yes", "No" };

	if (id == Compiler::UseIPP)
		return { "Yes", "No" };

	if (id == Compiler::VisualStudioVersion)
		return { "Visual Studio 2015", "Visual Studio 2017" };

	return {};
}

bool HiseSettings::Data::isFileId(const Identifier& id)
{
	if (id == Compiler::HisePath)
		return true;

	return false;
}

void HiseSettings::Data::addMissingSettings(ValueTree& v, const Identifier &id)
{
	auto handler = &GET_PROJECT_HANDLER(mc->getMainSynthChain());

	if (id == SettingFiles::ProjectSettings)
	{
		addSetting(v, HiseSettings::Project::Name, handler->getWorkDirectory().getFileName());
		addSetting(v, HiseSettings::Project::Version, "0.1.0");
		addSetting(v, HiseSettings::Project::Description);
		addSetting(v, HiseSettings::Project::BundleIdentifier, "com.myCompany.product");
		addSetting(v, HiseSettings::Project::PluginCode, "Abcd");
		addSetting(v, HiseSettings::Project::EmbedAudioFiles, "No");
		addSetting(v, HiseSettings::Project::AdditionalDspLibraries);
		addSetting(v, HiseSettings::Project::WindowsStaticLibFolder);
		addSetting(v, HiseSettings::Project::OSXStaticLibs);
		addSetting(v, HiseSettings::Project::ExtraDefinitionsWindows);
		addSetting(v, HiseSettings::Project::ExtraDefinitionsOSX);
		addSetting(v, HiseSettings::Project::ExtraDefinitionsIOS);
		addSetting(v, HiseSettings::Project::AppGroupId);
	}
	else if (id == SettingFiles::UserSettings)
	{
		addSetting(v, HiseSettings::User::Company, "My Company");
		addSetting(v, HiseSettings::User::CompanyCode, "Abcd");
		addSetting(v, HiseSettings::User::CompanyCopyright, "(c)2017, Company");
		addSetting(v, HiseSettings::User::CompanyURL, "http://yourcompany.com");
		addSetting(v, HiseSettings::User::TeamDevelopmentID);
	}
	else if (id == SettingFiles::CompilerSettings)
	{
		addSetting(v, HiseSettings::Compiler::HisePath);
		addSetting(v, HiseSettings::Compiler::VisualStudioVersion, "Visual Studio 2017");
		addSetting(v, HiseSettings::Compiler::UseIPP, "Yes");
	}
}

} // namespace hise
