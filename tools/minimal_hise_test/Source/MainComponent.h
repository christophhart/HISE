/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "DialogLibrary.h"

namespace hise
{
namespace multipage
{
using namespace juce;

/** TODO:
 *
 * - add Add / Remove button OK
 * - allow dragging of pages
 * - add asset browser
 * - fix deleting (set enablement for add/delete correctly)
 */
struct Tree: public Component,
			 public DragAndDropContainer,
			 public PathFactory
{

    
    static var getParentRecursive(const var& parent, const var& childToLookFor)
    {
	    if(parent[mpid::Children].isArray() || parent.isArray())
	    {
            auto ar = parent.getArray();
            if(ar == nullptr)
                ar = parent[mpid::Children].getArray();

		    for(auto& v: *ar)
		    {
			    if(childToLookFor.getObject() == v.getObject())
                    return parent;

                auto cc = getParentRecursive(v, childToLookFor);

                if(cc.isObject())
                    return cc;
		    }
	    }

        return var();
    }

    static bool containsRecursive(const var& parent, const var& child)
    {
        if(parent == child)
            return true;

	    if(parent[mpid::Children].isArray())
	    {
		    for(const auto& v: *parent[mpid::Children].getArray())
		    {
			    if(containsRecursive(v, child))
                    return true;
		    }
	    }

        return false;
    }

    int getPageIndex(const var& child) const
    {
	    jassert(rootObj.isArray());

        for(int i = 0; i < rootObj.size(); i++)
        {
	        if(containsRecursive(rootObj[i], child))
                return i;
        }

        return -1;
    }

    bool removeFromParent(const var& child)
    {
        auto parent = getParentRecursive(rootObj, child);
        
	    if(parent[mpid::Children].isArray())
	    {
			return parent[mpid::Children].getArray()->removeAllInstancesOf(child) > 0;
	    }

        return false;
    }
    
    struct PageItem: public TreeViewItem
    {
        PageItem(Tree& root_, const var& obj_, int idx, bool isPage_):
          obj(obj_),
          index(idx),
          root(root_),
          isPage(isPage_)
        {
            isVisible = root_.currentDialog->findPageBaseForInfoObject(obj) != nullptr;
	        setLinesDrawnForSubItems(true);
            
            auto type = obj[mpid::Type].toString();
            
            if(type.isNotEmpty())
            {
                icon = root.iconFactory.createPath(type);
                typeColour = root.iconFactory.getColourForCategory(type);
            }
            
        }

        bool isAction() const
        {
	        return typeColour == Colour(0xFF9CC05B);
        }
        
        Colour typeColour;

        Path icon;
        
        bool isVisible;

        int getItemHeight() const override
        {
	        return isPage ? 26 : 24;
        }

        var getDragSourceDescription() override
        {
	        return obj;
        }

    	String getTooltip() override
        {
	        String s;

            

            s << obj[mpid::Type].toString();

            auto id = obj[mpid::ID].toString();

            if(id.isNotEmpty())
				s << " (" << id << ')';
            
            return s;
        }

		bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& dragSourceDetails) override
        {
	        return dragSourceDetails.description[mpid::Type].isString() && obj[mpid::Children].isArray();
        }

        void itemDropped (const DragAndDropTarget::SourceDetails& dragSourceDetails, int insertIndex) override
        {
            if(root.removeFromParent(dragSourceDetails.description))
            {
	            obj[mpid::Children].getArray()->insert(insertIndex, dragSourceDetails.description);

                root.currentDialog->refreshCurrentPage();
            }
        }

        void itemClicked(const MouseEvent& e) override
        {
            if(e.mods.isShiftDown())
            {
                auto typeId = obj[mpid::Type].toString();

	            auto id = root.currentDialog->getStringFromModalInput("Please enter the ID of the " + typeId, obj[mpid::ID].toString());

				if(id.isNotEmpty())
				{
					obj.getDynamicObject()->setProperty(mpid::ID, id);
                    root.currentDialog->refreshCurrentPage();
				}

                return;
            }

            if(!isVisible)
                return;

            if(!e.mods.isRightButtonDown())
                return;

            if(obj.hasProperty(mpid::Children))
            {
	            root.currentDialog->containerPopup(obj);
            }
            else
            {
	            root.currentDialog->nonContainerPopup(obj);
            }
        }

        void itemDoubleClicked(const MouseEvent&) override
        {
	        if(!root.currentDialog->showEditor(obj))
	        {
		        auto newIndex = root.getPageIndex(obj);

                root.currentDialog->gotoPage(newIndex);
	        }
        }

        void itemOpennessChanged(bool isNowOpen) override
        {
            clearSubItems();

	        if(isNowOpen)
	        {
                int idx = 0;

                auto tv = obj;

                auto childrenArePages = false;

                if(tv[mpid::Children].isArray())
                {
                    tv = tv[mpid::Children];
                }
                else
                {
	                childrenArePages = true;
                }
                    

                if(tv.isArray())
                {
	                for(const auto& v: *tv.getArray())
			        {
				        addSubItem(new PageItem(root, v, idx++, childrenArePages));
			        }
                }
	        }
        }

        void paintItem (Graphics& g, int width, int height) override
        {
            float alphaVisible = isVisible ? 1.0f : 0.4f;
            
            
            Rectangle<int> b(0, 0, width, height);

            
            PathFactory::scalePath(icon, b.removeFromLeft(height).reduced(2).toFloat());
            
            g.setColour(typeColour);
            g.fillPath(icon);

            g.setColour(root.tree.findColour(TreeView::ColourIds::linesColourId).withMultipliedAlpha(alphaVisible * 0.5f));

            if(isAction())
            {
                auto ab = obj[mpid::CallOnNext] ? b.removeFromRight(b.getHeight()) : b.removeFromLeft(b.getHeight());
	            auto ap = root.createPath("arrow");
                root.scalePath(ap, ab.toFloat().reduced(4));
                g.fillPath(ap);
            }
            
            g.drawRoundedRectangle(b.toFloat().reduced(2.0f), 3.0f, 1.0f);

            g.setColour(typeColour.withAlpha(0.08f));
            g.fillRoundedRectangle(b.toFloat().reduced(4.0f), 2.0f);
            
            if(isSelected())
	        {
                g.setColour(Colours::white.withAlpha(0.05f));
		        g.fillRoundedRectangle(b.toFloat().reduced(4.0f), 2.0f);
	        }

            b = b.reduced(10, 0);

            auto id = obj[mpid::ID].toString();

            

            if(!id.isEmpty() && obj[mpid::Required])
                id << "*";

            auto t = isPage ? "Page" : obj[mpid::Type].toString();
            
            g.setColour(Colours::white.withAlpha(0.8f * alphaVisible));
            g.setFont(GLOBAL_BOLD_FONT());
            g.drawText(t, b.toFloat(), Justification::left);
            g.setColour(Colours::white.withAlpha(0.4f * alphaVisible));
            g.setFont(GLOBAL_MONOSPACE_FONT());
            g.drawText(id, b.toFloat(), Justification::right);
        }

        String getUniqueName() const override
        {
	        return "Child" + String(index);
        }

	    bool mightContainSubItems() override
	    {
		    return obj[mpid::Children].size() > 0;
	    }

        int index;
        var obj;

        Tree& root;
        const bool isPage;
    };
    
    Tree():
	  addButton("add", nullptr, *this),
      deleteButton("delete", nullptr, *this)
    {
        addAndMakeVisible(addButton);
        addAndMakeVisible(deleteButton);

        addButton.setTooltip("Add a new page");
        deleteButton.setTooltip("Remove the current page");

        addButton.onClick = [this]()
        {
			currentDialog->addListPageWithJSON();
        };

        deleteButton.onClick = [this]()
        {
	        currentDialog->removeCurrentPage();
        };

        tree.setColour(TreeView::backgroundColourId, Colour(0xFF262626));
        tree.setColour(TreeView::ColourIds::selectedItemBackgroundColourId, Colours::white.withAlpha(0.0f));
        tree.setColour(TreeView::ColourIds::linesColourId, Colour(0xFF999999));
        tree.setColour(TreeView::ColourIds::dragAndDropIndicatorColourId, Colour(SIGNAL_COLOUR));

        sf.addScrollBarToAnimate(tree.getViewport()->getVerticalScrollBar());

	    addAndMakeVisible(tree);
        
    }

    ScrollbarFader sf;

    Dialog* currentDialog = nullptr;

    void refresh()
    {
	    tree.setRootItem(new PageItem(*this, currentDialog->getPageListVar(), 0, true));
        tree.setRootItemVisible(false);
        tree.setDefaultOpenness(true);
    }

    Factory iconFactory;
    
    Path createPath(const String& url) const override
    {
	    Path p;

        static const unsigned char arrowData[] = { 110,109,0,128,7,68,16,41,174,67,108,0,128,7,68,96,215,141,67,108,148,229,37,68,96,215,141,67,108,148,229,37,68,200,168,95,67,108,1,128,69,68,64,0,158,67,108,148,229,37,68,168,43,204,67,108,148,229,37,68,16,41,174,67,108,0,128,7,68,16,41,174,67,99,101,
		0,0 };

        LOAD_EPATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
        LOAD_EPATH_IF_URL("delete", EditorIcons::deleteIcon);

        LOAD_PATH_IF_URL("arrow", arrowData);
        

        return p;
    }

    void setRoot(Dialog& d)
    {
        rootObj = d.getPageListVar();
        currentDialog = &d;

        currentDialog->refreshBroadcaster.addListener(*this, [](Tree& t, int pageIndex)
        {
	        t.refresh();
        }, false);

        refresh();
    }

    void paint(Graphics& g)
    {
	    g.fillAll(Colour(0xFF262626));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(10);

        auto bb = b.removeFromBottom(32);

	    tree.setBounds(b);

        addButton.setBounds(bb.removeFromLeft(bb.getHeight()).reduced(6));
        deleteButton.setBounds(bb.removeFromRight(bb.getHeight()).reduced(6));
    }

    var rootObj;
	juce::TreeView tree;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Tree);

    HiseShapeButton addButton, deleteButton;
};





struct Asset: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<Asset>;
    using List = ReferenceCountedArray<Asset>;

	enum class Type
	{
		Image,
        File,
        Text,
        Archive,
        numTypes
	};

    enum class Filemode
    {
	    Absolute,
        Relative
    };

    static Type getType(const File& f)
    {
	    auto extension = f.getFileExtension();

        if(auto format = ImageFileFormat::findImageFormatForFileExtension(f))
	        return Type::Image;
        else if(extension == ".txt" || extension == ".md")
	        return Type::Text;
        else if(extension == ".zip")
            return Type::Archive;
        else
			return Type::File;
    }

    Image toImage() const
    {
	    if(type == Type::Image)
	    {
		    MemoryInputStream mis(data, false);
			return ImageFileFormat::loadFrom(mis);
	    }
        else
            return {};
    }

    String toText() const
    {
	    if(type == Type::Text)
            return data.toString();
        else
            return {};
    }

    String toCppLiteral() const
    {
	    String c;
        c << "static const char " << id << "[" << String(data.getSize()) << "] = \"";
        c << data.toBase64Encoding() << "\";";
        return c;
    }

    var toJSON() const
    {
	    auto v = new DynamicObject();

        v->setProperty(mpid::Type, (int)type);
        v->setProperty("ID", id);
        v->setProperty("Data", var(data));
    }

    bool writeToFile(const File& targetFile) const
    {
	    MemoryInputStream mis(data, false);
        mis.skipNextBytes(sizeof(int64));
        FileOutputStream fos(targetFile);
        auto numToWrite = mis.getTotalLength() - sizeof(int64);
        return fos.writeFromInputStream(mis, numToWrite) == numToWrite;
    }

    static Asset::Ptr fromFile(const File& f)
    {
	    auto type = getType(f);

        return new Asset(f, type);
    }

    static Asset::Ptr fromMemory(MemoryBlock&& mb, Type t, const String& id)
    {
	    return new Asset(std::move(mb), t, id);
    }

    Asset(MemoryBlock&& mb, Type t, const String& id_):
      type(t),
      data(mb),
      id(id_)
    {
	    
    };

    Asset(const File& f, Type t):
      type(t),
      id("asset_" + String(f.getFullPathName().hash())),
      filename(f.getFullPathName())
    {

	    MemoryOutputStream mos;
        FileInputStream fis(f);

        auto hash = f.getFullPathName().hashCode64();

        mos.writeInt64(hash);
        mos.writeFromInputStream(fis, fis.getTotalLength());
        data = std::move(mos.getMemoryBlock());
    };
    
    const Type type;
    MemoryBlock data;
    String id;
    String filename;
};

struct AssetManager: public Component,
					 public TableListBoxModel,
					 public PathFactory
{
    enum class Columns
    {
	    Type = 1,
        ID,
        Filename,
        Size
    };
    
    AssetManager():
      listbox("Assets", this),
      addButton("add", nullptr, *this),
      deleteButton("delete", nullptr, *this)
    {
        addButton.setTooltip("Add new asset");
        deleteButton.setTooltip("Delete selected asset");

        listbox.setColour(ListBox::ColourIds::backgroundColourId, Colour(0xFF262626));

	    addAndMakeVisible(listbox);
        addAndMakeVisible(addButton);
        addAndMakeVisible(deleteButton);

        auto h = listbox.getRowHeight();

        listbox.getHeader().addColumn("Type", (int)Columns::Type, h, h, h);
        listbox.getHeader().addColumn("ID", (int)Columns::ID, 100, 100, 100);
        listbox.getHeader().addColumn("File", (int)Columns::Filename, 128, 128, 128);
        listbox.getHeader().addColumn("Size", (int)Columns::Size, 50, 50, 50);

        addButton.onClick = [this]()
        {
	        FileChooser fc("Add asset", File());

            if(fc.browseForFileToOpen())
            {
            	assets.add(Asset::fromFile(fc.getResult()));
                listbox.updateContent();
            }
        };
    }

    String createBinaryData() const
    {
	    String s;

        s << "namespace EmbeddedData { \n";
        s << "\n";

        for(auto a: assets)
            s << a->toCppLiteral() << "\n";

        s << "\n}\n";
        return s;
    }

    void paint(Graphics& g) override
    {
	    g.fillAll(Colour(0xFF262626));
    }

	void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
	    if(rowIsSelected)
        {
            g.setColour(Colours::white.withAlpha(0.05f));
            g.fillRect(0, 0, width, height);
        }
    }

    /** This must draw one of the cells.

        The graphics context's origin will already be set to the top-left of the cell,
        whose size is specified by (width, height).

        Note that the rowNumber value may be greater than the number of rows in your
        list, so be careful that you don't assume it's less than getNumRows().
    */void paintCell (Graphics& g, int rowNumber,int columnId, int width, int height, bool rowIsSelected) override
    {
	    if(auto a = assets[rowNumber])
	    {
            const String letters = "IFTA";

            g.setFont(GLOBAL_BOLD_FONT());
            g.setColour(Colours::white.withAlpha(0.5f));

            String s;
            auto typeIndex = (int)a->type;

            switch((Columns)columnId)
            {
            case Columns::Type:
	            g.drawText(letters.substring(typeIndex, typeIndex+1), 0, 0, width, height, Justification::centred);;
                break;
            case Columns::ID:
                g.drawText(a->id, 0, 0, width, height, Justification::centred);;
                break;
            case Columns::Filename:
                g.drawText(File(a->filename).getFileName(), 0, 0, width, height, Justification::centred);;
                break;
            case Columns::Size:
                s << String((double)a->data.getSize() / 1024.0 / 1024.0, 1) << "MB";
                g.drawText(s, 0, 0, width, height, Justification::centred);;
                break;
            default: ;
            }
	    }
    }

    String getCellTooltip(int rowNumber, int columnId) override
    {
	    return {};
    }


	int getNumRows() override { return assets.size(); }

    void resized() override
    {
	    auto b = getLocalBounds();
        auto bb = b.removeFromBottom(32);

	    listbox.setBounds(b);
        addButton.setBounds(bb.removeFromLeft(bb.getHeight()).reduced(6));
        deleteButton.setBounds(bb.removeFromRight(bb.getHeight()).reduced(6));
    }

    Path createPath(const String& url) const override
    {
	    Path p;

        LOAD_EPATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
        LOAD_EPATH_IF_URL("delete", SampleMapIcons::deleteSamples);

        return p;
            
    }

    TableListBox listbox;
    HiseShapeButton addButton, deleteButton;

	Asset::List assets;
};

}
}

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component,
					    public Timer,
					    public MenuBarModel,
					    public CodeDocument::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    bool manualChange = false;

    File getSettingsFile() const
    {
        auto f = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
        
#if JUCE_MAC
        f = f.getChildFile("Application Support");
#endif
        
        return f.getChildFile("HISE").getChildFile("multipage.json");
    }

    void codeDocumentTextInserted (const String& , int ) override
    {
	    manualChange = true;
    }

    
    void codeDocumentTextDeleted (int , int ) override
    {
        manualChange = true;
    }

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override;;

    void setSavePoint();

    void checkSave()
    {
	    if(modified)
	    {
		    if(currentFile.existsAsFile() && AlertWindow::showOkCancelBox(MessageBoxIconType::QuestionIcon, "Save changes", "Do you want to save the changes"))
	        {
		        currentFile.replaceWithText(JSON::toString(c->exportAsJSON()));
                setSavePoint();
	        }
	    }
    }

    void build();

    /** This method must return a list of the names of the menus. */
    StringArray getMenuBarNames()
    {
	    return { "File", "Edit", "View", "Help" };
    }

    enum CommandId
    {
	    FileNew = 1,
        FileLoad,
        FileSave,
        FileSaveAs,
        FileQuit,
        EditUndo,
        EditRedo,
        EditClearState,
        EditToggleMode,
        EditRefreshPage,
        EditAddPage,
        EditRemovePage,
        ViewShowDialog,
        ViewShowJSON,
        ViewShowCpp,
        HelpAbout,
        HelpVersion,
        FileRecentOffset = 9000
    };
    
	PopupMenu getMenuForIndex (int topLevelMenuIndex, const String&) override;

    bool keyPressed(const KeyPress& key) override;

    void menuItemSelected (int menuItemID, int) override;

private:

    int64 prevHash = 0;
    bool modified = false;
    bool firstAfterSave = false;

    File currentFile;

    juce::RecentlyOpenedFilesList fileList;

    void createDialog(const File& f);

    //==============================================================================
    // Your private member variables go here...
    int counter = 0;
	OpenGLContext context;

    multipage::State rt;
    ScopedPointer<multipage::Dialog> c;

    TooltipWindow tooltips;

    juce::CodeDocument doc;
    mcl::TextDocument stateDoc;
    mcl::TextEditor stateViewer;

    multipage::AssetManager assetManager;

    AlertWindowLookAndFeel plaf;
    MenuBarComponent menuBar;

    ScopedPointer<multipage::library::HardcodedDialogWithState> hardcodedDialog;

    ScriptWatchTable watchTable;
    hise::multipage::Tree tree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


