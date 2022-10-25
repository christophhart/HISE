/** ============================================================================
 *
 * MCL Text Editor JUCE module 
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


#pragma once

namespace mcl
{
using namespace juce;





/** A base class for generating syntax trees for a given code document. */
class DocTreeBuilder: public CoallescatedCodeDocumentListener
{
public: 

	/** An item is an element in a tree structure that represents the code structure and can be 
		used for displaying / navigation purposes. 
		
		It is language agnostic and the specific implementation has to be done by subclassing DocTreeBuilder. */
	struct Item: public ReferenceCountedObject
	{
		int lineNumber;
		String name;
		Identifier type;

		Item** begin() const { return const_cast<Item**>(children.begin()); }
		Item** end() const { return const_cast<Item**>(children.end()); }

		/* Removes all children. */
		void clearChildren();

		/** Iterates the item and its children and calls the given function.
		
			If the function returns true, any further items will not be called. */
		bool forEach(const std::function<bool(Item*)>& f);

		/** Converts the item to a ValueTree representation. */
		ValueTree toValueTree() const;

		/** Returns the path to the Item by walking back the parent hierarchy. */
		String getPath() const;

		/** Returns its parent. This can be nullptr if the item is the root item. */
		Item* getParent() const;

		/** Adds an item as child to this item. */
		void addChild(Item* n);

	private:

		ReferenceCountedArray<Item> children;
		WeakReference<Item> parent;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Item);
	};

	using Ptr = ReferenceCountedObjectPtr<Item>;
	using List = ReferenceCountedArray<Item>;

	/** A Listener that will be notified whenever the syntax tree changes. */
	struct Listener
	{
		virtual ~Listener() {};

		/** Will be called on the message thread as soon as the tree was rebuilt. */
		virtual void treeWasRebuilt(Ptr newRoot) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	

	/** Creates a doc tree builder. */
	DocTreeBuilder(TextDocument& t) :
		CoallescatedCodeDocumentListener(t.getCodeDocument()),
		doc(t)
	{};

	

	/** @internal */
	void codeChanged(bool , int , int ) override;

	virtual ~DocTreeBuilder() {};

	/** Override this method and return a Item that contains the structure tree of your code. */
	virtual Ptr createItems() = 0;
	
	/** Add a a listener that will be notified when the items have changed. */
	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

protected:

	CodeDocument::Iterator createIterator() const
	{
		return CodeDocument::Iterator(doc.getCodeDocument());
	}

	TextDocument& doc;

private:

	Array<WeakReference<Listener>> listeners;
	Ptr currentRoot;
};

/** A JUCE TreeView representation of the DocTreeBuilder data. */
struct DocTreeView : public Component,
					 public DocTreeBuilder::Listener
{

	struct DocTreeViewItem : public TreeViewItem
	{
		DocTreeViewItem(DocTreeBuilder::Ptr item_) :
			item(item_)
		{};

		bool mightContainSubItems() override
		{
			return item->begin() != item->end();
		}

		String getUniqueName() const override
		{
			return item->getPath();
		}

		void itemOpennessChanged(bool isNowOpen) override
		{
			if (isNowOpen)
			{
				for (auto c : *item)
					addSubItem(new DocTreeViewItem(c));
			}
			else
				clearSubItems();
		}

		void paintItem(Graphics& g, int width, int height) override
		{
			Font f(Font::getDefaultMonospacedFontName(), 16.0f, Font::plain);
			g.setFont(f);
			g.setColour(Colours::white.withAlpha(0.7f));
			g.drawText(item->name, 0.0f, 0.0f, (float)width, (float)height, Justification::centredLeft);
		}

		DocTreeBuilder::Ptr item;
	};

	void treeWasRebuilt(DocTreeBuilder::Ptr newRoot) override
	{
		tree.setRootItem(nullptr);

		rootItem = new DocTreeViewItem(newRoot);
		tree.setRootItem(rootItem);
		tree.setDefaultOpenness(true);
		tree.setRootItemVisible(false);
		resized();
	}

	DocTreeView(TextDocument& doc)
	{
		addAndMakeVisible(tree);
	}

	void setBuilder(DocTreeBuilder* ownedBuilder)
	{
		builder = ownedBuilder;
		builder->addListener(this);
	}

	void resized() override
	{
		tree.setBounds(getLocalBounds());
	}

	~DocTreeView()
	{
		tree.setRootItem(nullptr);
		rootItem = nullptr;

		if (builder != nullptr)
		{
			builder->removeListener(this);
			builder = nullptr;
		}
	}

	ScopedPointer<DocTreeBuilder> builder;
	TreeView tree;
	ScopedPointer<TreeViewItem> rootItem;
};

}