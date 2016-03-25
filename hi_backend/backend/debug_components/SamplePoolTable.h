/*
  ==============================================================================

    SamplePoolTable.h
    Created: 1 Nov 2014 8:05:53pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef SAMPLEPOOLTABLE_H_INCLUDED
#define SAMPLEPOOLTABLE_H_INCLUDED

class ModulatorSamplerSoundPool;

/** A table component containing all samples of a ModulatorSampler.
*	@ingroup debugComponents
*/
class SamplePoolTable      : public Component,
                             public TableListBoxModel,
							 public SafeChangeListener
{
public:

	enum ColumnId
	{
		FileName = 1,
		Memory,
		State,
		References,
		numColumns
	};

	SamplePoolTable(ModulatorSamplerSoundPool *globalPool) ;

	~SamplePoolTable();

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		setName(getHeadline());
		table.updateContent();
		if(getParentComponent() != nullptr) getParentComponent()->repaint();
	}

    int getNumRows() override;

    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;

	void selectedRowsChanged(int /*lastRowSelected*/) override;

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;
    
	String getHeadline() const;

    void resized() override;

	void mouseDown(const MouseEvent &e) override;

private:
    TableListBox table;     // the table component itself
    Font font;

	ModulatorSamplerSoundPool *pool;
	ScopedPointer<TableHeaderLookAndFeel> laf;
    int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePoolTable)
};

class MainController;

/** A table component containing all samples of a ModulatorSampler.
*	@ingroup components
*/
template <class FileType> class ExternalFileTable      : public Component,
                             public TableListBoxModel,
							 public SafeChangeListener,
							 public DragAndDropContainer
{
public:

	enum ColumnId
	{
		FileName = 1,
		Memory,
		Type,
		References,
		numColumns
	};

	ExternalFileTable(Pool<FileType> *pool) ;

	~ExternalFileTable();

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		setName(getHeadline());
		table.updateContent();
		if(getParentComponent() != nullptr) getParentComponent()->repaint();
	}

    int getNumRows() override;

    void paintRowBackground (Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override;

	void selectedRowsChanged(int /*lastRowSelected*/) override;

    void paintCell (Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool /*rowIsSelected*/) override;
    
	String getHeadline() const;

    void resized() override;

	String getTextForTableCell(int rowNumber, int columnNumber);

	var getDragSourceDescription(const SparseSet< int > &set) override
	{
		var id;

		if(set.getNumRanges() > 0)
		{
			const int index = set[0];

			Identifier name = pool->getIdForIndex(index);

			String x = pool->getFileNameForId(name);

			id = x;
			
		}

		return id;
	};

private:
    TableListBox table;     // the table component itself
    Font font;

	int selectedRow;

	var currentlyDraggedId;

	Pool<FileType> *pool;

	
	ScopedPointer<TableHeaderLookAndFeel> laf;
    int numRows;            // The number of rows of data we've got

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExternalFileTable)
};







#endif  // SAMPLEPOOLTABLE_H_INCLUDED
