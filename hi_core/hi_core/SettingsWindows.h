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
DECLARE_ID(DeviceSettings);
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
DECLARE_ID(AppGroupId);

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


struct Data
{
	Data(MainController* mc_);

	File getFileForSetting(const Identifier& id) const;

	void loadDataFromFiles();
	void refreshProjectData();
	void loadSettingsFromFile(const Identifier& id);
	
	ValueTree getTreeForSettings(const Identifier& id)
	{
		auto v = data.getChildWithName(id);

		return v;
	}

	var getSetting(const Identifier& id) const
	{
		for (const auto& c : data)
			if (c.hasProperty(id))
				return c.getProperty(id);

		return var();
	}

	static StringArray getOptionsFor(const Identifier& id);

	static bool isFileId(const Identifier& id);

private:

	static void addSetting(ValueTree& v, const Identifier& id, const String& defaultValue = String());

	

	void addMissingSettings(ValueTree& v, const Identifier &id);

	MainController* mc;
	ValueTree data;
};



#undef DECLARE_ID


} // SettingIds



/** Contains all Setting windows that can popup and edit a specified XML file. */
struct SettingWindows
{
	
	

	/** Use this helper function to get the actual settings.
	*
	*	In order to be of any usage, you must use the appropriate Attributes enum values of each subclass
	*
	*/
	static String getSettingValue(int attributeIndex, ProjectHandler *handler = nullptr);

	static void checkAllSettings(ProjectHandler *handler);

	class NewSettingWindows : public Component,
		public ButtonListener,
		public QuasiModalComponent,
		public TextEditor::Listener
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

		NewSettingWindows(HiseSettings::Data& data);

		~NewSettingWindows();

		void buttonClicked(Button* b) override
		{
			if (b == &allSettings) setContent({ HiseSettings::SettingFiles::ProjectSettings, HiseSettings::SettingFiles::UserSettings, HiseSettings::SettingFiles::CompilerSettings});
			if (b == &projectSettings) setContent({ HiseSettings::SettingFiles::ProjectSettings, HiseSettings::SettingFiles::UserSettings });
			if (b == &globalSettings) setContent({ HiseSettings::SettingFiles::CompilerSettings });
			if (b == &applyButton)
			{
				saveOnDestroy = true;
				destroy();
			}
			if (b == &cancelButton) destroy();
		}

		void resized() override;

		void paint(Graphics& g) override;

		void textEditorTextChanged(TextEditor&) override;

		void activateSearchBox()
		{
			fuzzySearchBox.grabKeyboardFocus();
		}


	private:

		struct FileBasedValueTree
		{
			FileBasedValueTree(Identifier s_, ValueTree v_, File f_) :
				s(s_),
				v(v_),
				f(f_)
			{};

			void fillPropertyPanel(PropertyPanel& panel, const String& searchText);

			void addProperty(ValueTree& c, Array<PropertyComponent*>& props);

			String getId() const;


			void save();

			Identifier s;
			ValueTree v;
			File f;

			BlackTextButtonLookAndFeel blaf;

		};

        FileBasedValueTree* getProperlyFormattedValueTree(Identifier s);
		
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
		ToggleButton globalSettings;
		ToggleButton allSettings;

		TextButton applyButton;
		TextButton cancelButton;

		ScopedPointer<Content> currentContent;

		SettingList currentList;

		TextEditor fuzzySearchBox;

		bool saveOnDestroy = false;

	};

	

private:

	

	

public:

	
};

} // namespace hise

#endif  // SETTINGSWINDOWS_H_INCLUDED
