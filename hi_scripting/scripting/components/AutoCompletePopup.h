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


#ifndef AUTOCOMPLETEPOPUP_H_INCLUDED
#define AUTOCOMPLETEPOPUP_H_INCLUDED



class JavascriptCodeEditor::AutoCompletePopup : public ListBoxModel,
	public Component
{

public:

	// ================================================================================================================

	class AllToTheEditorTraverser : public KeyboardFocusTraverser
	{
	public:
		AllToTheEditorTraverser(JavascriptCodeEditor *editor_) : editor(editor_) {};

		virtual Component* getNextComponent(Component* /*current*/)
		{
			return editor;
		}
		virtual Component* getPreviousComponent(Component* /*current*/)
		{
			return editor;
		}
		virtual Component* getDefaultComponent(Component* /*parentComponent*/)
		{
			return editor;
		}

		JavascriptCodeEditor *editor;
	};

	// ================================================================================================================

	AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor_, Range<int> tokenRange_, const String &tokenText);
	~AutoCompletePopup();

	void createVariableRows();
	void createApiRows(const ValueTree &apiTree);
	void createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText);

	void addCustomEntries(const Identifier &objectId, const ValueTree &apiTree);
	void addApiConstants(const ApiClass* apiClass, const Identifier &objectId);
	void addApiMethods(const ValueTree &classTree, const Identifier &objectId);

	KeyboardFocusTraverser* createFocusTraverser() override;

	int getNumRows() override;
	void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
	void listBoxItemClicked(int row, const MouseEvent &) override;
	void listBoxItemDoubleClicked(int row, const MouseEvent &) override;

	bool handleEditorKeyPress(const KeyPress& k);

	void paint(Graphics& g) override;
	void resized();

	void selectRowInfo(int rowIndex);
	void rebuildVisibleItems(const String &selection);

	bool escapeKeyHandled = false;

	// ================================================================================================================

private:

	SharedResourcePointer<ApiHelpers::Api> api;

	// ================================================================================================================

	struct RowInfo
	{
		enum class Type
		{
			ApiClass = (int)DebugInformation::Type::numTypes,
			ApiMethod,
			numTypes
		};

		bool matchesSelection(const String &selection)
		{
			return name.containsIgnoreCase(selection);
		}

		AttributedString description;
		String codeToInsert, name, typeName, value;
		int type;
	};

	// ================================================================================================================

	class InfoBox : public Component
	{
	public:

		void setInfo(RowInfo *newInfo);
		void paint(Graphics &g);

	private:

		AttributedString infoText;
		RowInfo *currentInfo = nullptr;
	};

	// ================================================================================================================

	OwnedArray<RowInfo> allInfo;
	Array<RowInfo*> visibleInfo;
	StringArray names;
	int fontHeight;
	int currentlySelectedBox = -1;
	ScopedPointer<InfoBox> infoBox;
	ScopedPointer<ListBox> listbox;
	JavascriptProcessor *sp;
	Range<int> tokenRange;
	JavascriptCodeEditor *editor;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoCompletePopup);

	// ================================================================================================================
};





#endif  // AUTOCOMPLETE_H_INCLUDED
