/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define DECLARE_ID(x) static const Identifier x(#x);
namespace SettingIds
{
	DECLARE_ID(CurrentFile);
	DECLARE_ID(ShowPreview);
	DECLARE_ID(ShowEditor);
	DECLARE_ID(FileList);
	DECLARE_ID(FontSize);
	DECLARE_ID(RootDirectory);
	DECLARE_ID(Width);
	DECLARE_ID(Height);
    DECLARE_ID(DarkPreview);

	static Array<Identifier> getAllIds()
	{
		static const Array<Identifier> ids = { RootDirectory, CurrentFile, ShowPreview, ShowEditor, FileList, FontSize, Width, Height, DarkPreview};

		return ids;
	}
}
#undef DECLARE_ID



struct KeyCommand
{
    KeyCommand(int keyCode , ModifierKeys m_, const std::function<void()>& f_):
      f(f_),
      kc(keyCode),
      mods(m_)
    {};
    
    bool operator()(const KeyPress& k) const
    {
        auto keymatch = (k.getKeyCode() == kc) ||
                        (kc < 127 && CharacterFunctions::toUpperCase(kc) == k.getKeyCode());
        
        if(keymatch && k.getModifiers() == mods)
        {
            f();
            return true;
        }
        
        return false;
    }
    
    int kc;
    ModifierKeys mods;
    std::function<void()> f;
    
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component,
							   public MarkdownDatabaseHolder,
							   
							   public TopLevelWindowWithOptionalOpenGL,
							   public MenuBarModel
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&) override;
    void resized() override;

	void registerContentProcessor(MarkdownContentProcessor* processor) {}
	void registerItemGenerators() {}

	File getCachedDocFolder() const override {
		return File();
	}
	File getDatabaseRootDirectory() const
	{
		return File();
	}

	

	
    
	bool shouldUseCachedData() const override { return false; }
	
	

	/** This method must return a list of the names of the menus. */
	StringArray getMenuBarNames() override;

	PopupMenu getMenuForIndex(int topLevelMenuIndex,
		const String& menuName) override;

	void menuItemSelected(int menuItemID,
		int topLevelMenuIndex) override;

    bool keyPressed(const KeyPress& k) override
    {
        for(const auto& c: commands)
        {
            if(c(k))
                return true;
        }
        
        return false;
    }
	void saveIfChanged(bool silent = false);

private:

    void addKeyCommand(int k, ModifierKeys m, const std::function<void()>& f)
    {
        commands.add({k, m, f});
    }
    
    Array<KeyCommand> commands;
    
	hise::PopupLookAndFeel plaf;

	juce::MenuBarComponent menuBar;

	var currentSettings;

	void createDefaultValue(const Identifier& id)
	{
		if (!currentSettings.hasProperty(id))
			currentSettings.getDynamicObject()->setProperty(id, getDefaultSetting(id));
	}

	static var getDefaultSetting(const Identifier& id)
	{
		if (id == SettingIds::CurrentFile)
			return "";
		if (id == SettingIds::FileList)
		{
			Array<var> a;
			return var(a);
		}
		if (id == SettingIds::FontSize)
			return 18.0f;
		if (id == SettingIds::ShowEditor)
			return true;
		if (id == SettingIds::ShowPreview)
			return true;
		if (id == SettingIds::RootDirectory)
			return "";
		if (id == SettingIds::Width)
			return 1280;
		if (id == SettingIds::Height)
			return 800;
		if (id == SettingIds::DarkPreview)
            return true;
        
		jassertfalse;
		return var();
	}

	File getSettingsFile(bool editorSettings)
	{
		auto dir = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("HISE MarkdownEditor");
		dir.createDirectory();
		return dir.getChildFile(editorSettings ? "code_editor.json" : "settings.json");
	}

	void applySetting(const Identifier& id, var newValue);

	void toggleSetting(const Identifier& id);

	void loadSettings()
	{
		currentSettings = JSON::parse(getSettingsFile(false));

		if (currentSettings.getDynamicObject() == nullptr)
			currentSettings = var(new DynamicObject());

		for (auto i : SettingIds::getAllIds())
		{
			applySetting(i, getSetting(i));
		}
	}

	void saveSettings()
	{
		auto text = JSON::toString(currentSettings);
		getSettingsFile(false).replaceWithText(text);
	}
	
	var getSetting(const Identifier& id)
	{
		return currentSettings.getProperty(id, getDefaultSetting(id));
	}

	

	void setFile(const File& f);

	File currentFile;
	File rootDirectory;

	CodeDocument doc;
	mcl::TextDocument tdoc;
	
	mcl::FullEditor editor;
	MarkdownPreview preview;
	

	hise::GlobalHiseLookAndFeel laf;

    mcl::MarkdownPreviewSyncer updater;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
