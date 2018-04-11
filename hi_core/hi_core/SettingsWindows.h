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


#ifndef SETTINGSWINDOWS_H_INCLUDED
#define SETTINGSWINDOWS_H_INCLUDED

namespace hise { using namespace juce;

#define DECLARE_ID(name)      const Identifier name (#name)

namespace HiseSettings {

namespace SettingFiles
{
DECLARE_ID(ProjectSettings);
DECLARE_ID(UserSettings);
DECLARE_ID(CompilerSettings);
DECLARE_ID(GeneralSettings);
DECLARE_ID(AudioSettings);
DECLARE_ID(MidiSettings);
DECLARE_ID(ScriptingSettings);
DECLARE_ID(OtherSettings);

}

namespace Project
{
DECLARE_ID(Name);
DECLARE_ID(Version);
DECLARE_ID(Description);
DECLARE_ID(BundleIdentifier);
DECLARE_ID(PluginCode);
DECLARE_ID(EmbedAudioFiles);
DECLARE_ID(AdditionalDspLibraries);
DECLARE_ID(OSXStaticLibs);
DECLARE_ID(WindowsStaticLibFolder);
DECLARE_ID(ExtraDefinitionsWindows);
DECLARE_ID(ExtraDefinitionsOSX);
DECLARE_ID(ExtraDefinitionsIOS);
DECLARE_ID(AppGroupID);

} // Project

namespace Compiler
{
DECLARE_ID(HisePath);
DECLARE_ID(VisualStudioVersion);
DECLARE_ID(UseIPP);

} // Compiler

namespace User
{
DECLARE_ID(Company);
DECLARE_ID(CompanyCode);
DECLARE_ID(CompanyURL);
DECLARE_ID(CompanyCopyright);
DECLARE_ID(TeamDevelopmentID);

} // User

namespace Scripting
{
DECLARE_ID(EnableCallstack);
DECLARE_ID(GlobalScriptPath);
DECLARE_ID(CompileTimeout);
DECLARE_ID(CodeFontSize);
DECLARE_ID(EnableDebugMode);
}

namespace Other
{
DECLARE_ID(EnableAutosave);
DECLARE_ID(AutosaveInterval);
}

namespace Audio
{
DECLARE_ID(Driver);
DECLARE_ID(Device);
DECLARE_ID(Output);
DECLARE_ID(Samplerate);
DECLARE_ID(BufferSize);
}

namespace Midi
{
DECLARE_ID(MidiInput);
DECLARE_ID(MidiChannels);
}


struct ConversionHelpers
{
	static ValueTree loadValueTreeFromFile(const File& f, const Identifier& settingid);

	static ValueTree loadValueTreeFromXml(XmlElement* xml, const Identifier& settingId);

	static XmlElement* getConvertedXml(const ValueTree& v);

	static Array<int> getBufferSizesForDevice(AudioIODevice* currentDevice);

	static Array<double> getSampleRates(AudioIODevice* currentDevice);

	static StringArray getChannelList();
};


struct Data
{
	Data(MainController* mc_);

	File getFileForSetting(const Identifier& id) const;

	void loadDataFromFiles();
	void refreshProjectData();
	void loadSettingsFromFile(const Identifier& id);
	
	var getSetting(const Identifier& id) const
	{
		for (const auto& c : data)
		{
			auto prop = c.getChildWithName(id);

			static const Identifier va("value");

			if (prop.isValid())
			{
				auto value = prop.getProperty(va);

				if (value == "Yes")
					return var(true);

				if (value == "No")
					return var(false);

				return value;
			}
		}

		return var();
	}

	void initialiseAudioDriverData(bool forceReload=false);

	StringArray getOptionsFor(const Identifier& id);

	static bool isFileId(const Identifier& id);

	MainController* getMainController() { return mc; }
	const MainController* getMainController() const { return mc; }

	var getDefaultSetting(const Identifier& id);

	ValueTree data;

private:

	

	void addSetting(ValueTree& v, const Identifier& id);

	

	void addMissingSettings(ValueTree& v, const Identifier &id);

	

	MainController* mc;
	
};



#undef DECLARE_ID


} // SettingIds



/** Contains all Setting windows that can popup and edit a specified XML file. */
struct SettingWindows: public Component,
		public ButtonListener,
		public QuasiModalComponent,
		public TextEditor::Listener,
		private ValueTree::Listener
{
public:

	enum ColourValues
	{
		bgColour = 0xFF444444,
		barColour = 0xFF2B2B2B,
		tabBgColour = 0xFF333333,

		overColour = 0xFF464646
	};

	using SettingList = Array<Identifier>;

	SettingWindows(HiseSettings::Data& data);

	~SettingWindows();

	void buttonClicked(Button* b) override;

	void resized() override;

	void paint(Graphics& g) override;

	void textEditorTextChanged(TextEditor&) override;

	void activateSearchBox()
	{
		fuzzySearchBox.grabKeyboardFocus();
	}

	

	
	

private:

	struct Refresher : public AsyncUpdater
	{
		Refresher(SettingWindows* p_) :
			p(p_)
		{}

		void handleAsyncUpdate() override
		{
			if (p != nullptr)
			{
				p->setContent(p->currentList);
			}
		}

		Component::SafePointer<SettingWindows> p;
	};

	Refresher refresher;

	struct TestFunctions
	{
		static bool isValidNumberBetween(var value, Range<float> range);
	};

	Result checkInput(const Identifier& id, const var& newValue);

	void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged,
		const Identifier& property);

	void settingWasChanged(const Identifier& id, const var& newValue);

	void valueTreeChildAdded(ValueTree& , ValueTree& ) override;;

	void valueTreeChildRemoved(ValueTree& , ValueTree& , int ) override {};

	void valueTreeChildOrderChanged(ValueTree& ,int , int ) override {};

	void valueTreeParentChanged(ValueTree& ) override {}

	struct FileBasedValueTree
	{
		FileBasedValueTree(Identifier s_, File f_, SettingWindows* p_) :
			s(s_),
			f(f_),
			p(p_)
		{};

		void fillPropertyPanel(PropertyPanel& panel, const String& searchText);

		void addProperty(ValueTree& c, Array<PropertyComponent*>& props);

		String getId() const;

		ValueTree getValueTree() const
		{
			return p->dataObject.data.getChildWithName(s);
		}

		void save();

		Identifier s;
		
		File f;
		SettingWindows* p;

		BlackTextButtonLookAndFeel blaf;

	};

	FileBasedValueTree* createFileBasedValueTreeObject(Identifier s);

	OwnedArray<FileBasedValueTree> settings;

	class TabButtonLookAndFeel : public LookAndFeel_V3
	{
		void drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown) override;
	};

	HiseSettings::Data& dataObject;

	TabButtonLookAndFeel tblaf;

	AlertWindowLookAndFeel alaf;

	class Content;
	void setContent(SettingList s);

	ProjectHandler* handler;

	ToggleButton projectSettings;
	ToggleButton developmentSettings;
	ToggleButton audioSettings;
	ToggleButton allSettings;

	TextButton applyButton;
	TextButton cancelButton;
	TextButton undoButton;

	ScopedPointer<Content> currentContent;

	SettingList currentList;

	TextEditor fuzzySearchBox;

	bool saveOnDestroy = false;

	UndoManager undoManager;
	
};

} // namespace hise

#endif  // SETTINGSWINDOWS_H_INCLUDED
