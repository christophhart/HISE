#ifndef __FAUST_MENU_BAR_H
#define __FAUST_MENU_BAR_H

namespace scriptnode {
namespace faust {



// Additional types for faust_jit_node
struct FaustMenuBar : public Component,
					  public ButtonListener,
					  public ComboBox::Listener,
                      public DspNetwork::FaustManager::FaustListener

{

	FaustMenuBar(faust_jit_node_base *n);

    ~FaustMenuBar();
    
	struct Factory : public PathFactory
	{
		String getId() const override { return {}; }
		juce::Path createPath(const String& url) const override;
	} factory;

	juce::ComboBox classSelector;
	HiseShapeButton addButton;
	HiseShapeButton editButton;
	HiseShapeButton reloadButton;

	bool compilePending = false;

    scriptnode::parameter::ui::dynamic_list_editor dragger;
    
	WeakReference<faust_jit_node_base> node;
	hise::ScriptnodeComboBoxLookAndFeel claf;

	// Define menu options for addButton
	enum MenuOption {
		MENU_OPTION_FIRST = 1,
		NEW_FILE = MENU_OPTION_FIRST,
		IMPORT_FILE,
		IMPORT_LIB,
		RENAME_FILE,
		REMOVE_FILE,
        REBUILD_PARAMETERS,
		// add more options here
		MENU_OPTION_LAST,
		MENU_OPTION_INVALID,
	};

	std::map<int, String> menuOptions = {
		{NEW_FILE, "Create new file"},
		{IMPORT_FILE, "Import file into project"},
		{IMPORT_LIB, "Import library into project"},
		{RENAME_FILE, "Rename file"},
		{REMOVE_FILE, "Remove entry and file"},
        {REBUILD_PARAMETERS, "Rebuild parameters from FAUST file"},
			// add description for more options here
		{MENU_OPTION_INVALID, "Invalid Option (BUG)"}
	};

	String getTextForMenuOptionId(int id)
	{
		if (menuOptions.count(id) > 0) return menuOptions[id];
		return menuOptions[MENU_OPTION_INVALID];
	}

	void createNewFile();

	std::optional<File> promptForDestinationFile(String extension, File& previousDestFile);

	void importFile(String extension);

	void renameFile();

	void removeFile()
	{

	}

    String getModulationOutputName(int index) const
    {
        return node->getFaustModulationOutputName(index);
    }
    
	void paintOverChildren(Graphics& g) override;

	void executeMenuAction(int option);


	void rebuildComboBoxItems();

    virtual void faustFileSelected(const File& f) override;
	virtual void preCompileFaustCode(const File& f) override;

    virtual Result compileFaustCode(const File& f) override;
    virtual void faustCodeCompiled(const File& f, const Result& compileResult) override;
    
	virtual void resized() override;

	virtual void buttonClicked(Button* b) override;

	virtual void comboBoxChanged (ComboBox *comboBoxThatHasChanged) override;

	bool matchesFile(const File& f) const
	{
		auto sourceFile = node->getFaustFile(node->getClassId());
		return sourceFile == f;
	}

};

} // namespace faust
} // namespace scriptnode

#endif // __FAUST_MENU_BAR_H
