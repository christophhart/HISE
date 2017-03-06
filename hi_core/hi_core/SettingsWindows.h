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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#ifndef SETTINGSWINDOWS_H_INCLUDED
#define SETTINGSWINDOWS_H_INCLUDED



/** Contains all Setting windows that can popup and edit a specified XML file. */
struct SettingWindows
{
	enum class Settings
	{
		Project,
		User,
		Compiler,
		numSettings
	};

	/** Use this helper function to get the actual settings.
	*
	*	In order to be of any usage, you must use the appropriate Attributes enum values of each subclass
	*
	*/
	static String getSettingValue(int attributeIndex, ProjectHandler *handler = nullptr);

	static void checkAllSettings(ProjectHandler *handler);

private:

	static File getFileForSettingsWindow(Settings s, ProjectHandler *handler = nullptr);

	class BaseSettingsWindow : public ThreadWithAsyncProgressWindow
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
			ThreadWithAsyncProgressWindow(settingName + " Properties")
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
			CustomToolbarClassName,
			OSXStaticLibs,
			WindowsStaticLibFolder,
			ExtraDefinitionsWindows,
			ExtraDefinitionsOSX,
			ExtraDefinitionsIOS,
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


#endif  // SETTINGSWINDOWS_H_INCLUDED
