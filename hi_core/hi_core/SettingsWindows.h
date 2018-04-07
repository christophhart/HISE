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





/** Contains all Setting windows that can popup and edit a specified XML file. */
struct SettingWindows
{
	enum class Settings
	{
		Project,
		User,
		Compiler,
		Audio,
		Midi,
		Global,
		Script,
		numSettings
	};

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

		using SettingList = Array<Settings>;

		NewSettingWindows(ProjectHandler* handler_);

		~NewSettingWindows();

		void buttonClicked(Button* b) override
		{
			if (b == &allSettings) setContent({ Settings::Project, Settings::User, Settings::Compiler, Settings::Audio, Settings::Global });
			if (b == &projectSettings) setContent({ Settings::Project, Settings::User });
			if (b == &globalSettings) setContent({ Settings::Compiler });
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
			FileBasedValueTree(Settings s_, ValueTree v_, File f_) :
				s(s_),
				v(v_),
				f(f_)
			{};

			void fillPropertyPanel(PropertyPanel& panel, const String& searchText);

			void addProperty(ValueTree& c, Array<PropertyComponent*>& props);

			String getId() const;


			void save();


			Settings s;
			ValueTree v;
			File f;

			BlackTextButtonLookAndFeel blaf;

		};

        FileBasedValueTree* getProperlyFormattedValueTree(Settings s);
		

		
		OwnedArray<FileBasedValueTree> settings;

		class TabButtonLookAndFeel : public LookAndFeel_V3
		{
			void drawToggleButton(Graphics& g, ToggleButton& b, bool isMouseOverButton, bool isButtonDown) override;
		};

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

	static File getFileForSettingsWindow(Settings s, ProjectHandler *handler = nullptr);

	

	class BaseSettingsWindow : public DialogWindowWithBackgroundThread
	{
	public:

		enum class ValueTypes
		{
			Text,
			List,
			File,
			numValueTypes
		};

		BaseSettingsWindow(const String &settingName) :
			DialogWindowWithBackgroundThread(settingName + " Properties")
		{}

		/** Saves the file to the disk. */
		void run() override;

		void threadFinished() override;

		/** Reveals the file in the explorer / finder. */
		void resultButtonClicked(const String &name);

		
		/** This method will be called before the XML file is saved.
		*
		*	You can add some checks here and return an error message if something is wrong.
		*	If everything is OK, just return an empty String. */
		virtual String sanityCheck(const XmlElement& /*xmlSettings*/) { return String(); }

	protected:

		static String getNameForValueType(ValueTypes t);

		static ValueTypes getValueType(XmlElement *);

		/** Overwrite this method and create a XML object with the following structure:
		*
		*	<TagName>
		*		<option1 Name="Name" value="Value" description="SomeDescription" options="Option1\nOption2\nOption3"/>
		*	</TagName>
		*
		*	Ideally you don't create these manually but use the helper functions addChildElement() and addChildElementWithOptions()
		*/
		virtual XmlElement *createNewSettingsFile() const = 0;

		/** Overwrite this method and return the file you want to use. */
		virtual File getFile() const = 0;

		virtual String getAttributeName(int attribute) const = 0;

		/** Creates a child element in the specified XML with the given data. */
		void addChildElement(XmlElement &element, int attributeIndex, const String &childValue, const String &description) const;

		/** Creates a child element in the specified XML with the given data and an option list as lines*/
		void addChildElementWithOptions(XmlElement &element, int attributeIndex, const String &childValue, const String &description, const String &optionsAsLines) const;

		void addFileAsChildElement(XmlElement &element, int attributeIndex, const String &childValue, const String &description) const;

		/** Call this method in your subclass and point to the file it should use. */
		void setSettingsFile();

	private:

		FilenameComponent *getFileNameComponent(const String &name)
		{
			for (int i = 0; i < fileComponents.size(); i++)
			{
				if (fileComponents[i]->getName() == name) return fileComponents[i];
			}

			return nullptr;
		}

		ScopedPointer<XmlElement> settings;
		File settingsFile;

		OwnedArray<FilenameComponent> fileComponents;

	};

public:

	// ========================================================================================================

	class ProjectSettingWindow : public BaseSettingsWindow
	{
	public:

		enum class Attributes
		{
			Name,
			Version,
			Description,
			BundleIdentifier,
			PluginCode,
			EmbedAudioFiles,
			AdditionalDspLibraries,
			OSXStaticLibs,
			WindowsStaticLibFolder,
			ExtraDefinitionsWindows,
			ExtraDefinitionsOSX,
			ExtraDefinitionsIOS,
            AppGroupId,
			numAttributes
		};

		ProjectSettingWindow(ProjectHandler *handler_) :
			BaseSettingsWindow("Project"),
			handler(handler_)
		{
			setSettingsFile();
		};

		File getFile() const override;

		String getAttributeName(int attribute) const override;

		static String getAttributeNameForSetting(int attribute);

		XmlElement *createNewSettingsFile() const override;

		String sanityCheck(const XmlElement& xmlSettings) override;

	private:

		ProjectHandler *handler;
	};

	// ========================================================================================================

	class CompilerSettingWindow : public BaseSettingsWindow
	{
	public:

		enum class Attributes
		{
			HisePath = (int)ProjectSettingWindow::Attributes::numAttributes,
			VisualStudioVersion,
			UseIPP,
			numCompilerSettingAttributes
		};


		CompilerSettingWindow() :
			BaseSettingsWindow("Compiler")
		{
			setSettingsFile();
		};

		File getFile() const override;

		String getAttributeName(int attribute) const override;

		static String getAttributeNameForSetting(int attribute);

		XmlElement *createNewSettingsFile() const override;

	private:

		ProjectHandler *handler;
	};

	// ========================================================================================================

	class UserSettingWindow : public BaseSettingsWindow
	{
	public:

		enum Attributes
		{
			Company = (int)CompilerSettingWindow::Attributes::numCompilerSettingAttributes,
			CompanyCode,
			CompanyURL,
            CompanyCopyright,
            TeamDevelopmentId,
			numUserSettingAttributes
		};

		UserSettingWindow(ProjectHandler *handler_) :
			BaseSettingsWindow("User"),
			handler(handler_)
		{
			setSettingsFile();
		};

		File getFile() const override;

		String getAttributeName(int attribute) const override;

		static String getAttributeNameForSetting(int attribute);

		XmlElement *createNewSettingsFile() const override;

		ProjectHandler *handler;
	};

};

} // namespace hise

#endif  // SETTINGSWINDOWS_H_INCLUDED
