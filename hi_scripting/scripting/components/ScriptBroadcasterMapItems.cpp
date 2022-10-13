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

namespace hise { using namespace juce;
namespace ScriptingObjects {

struct ScriptBroadcasterMapFactory : public PathFactory
{
    Path createPath(const String& url) const override;
	
};






#if 0
struct JSONBody : public ComponentWithPreferredSize,
				  public Component
{
	JSONBody(const var& v) :
		value(v)
	{
		s = JSON::toString(value, false);
		f = GLOBAL_MONOSPACE_FONT().withHeight(11.0f);
	};

	static ComponentWithPreferredSize* create(Component*, const var& v)
	{
		if (v.getDynamicObject() != nullptr)
			return new JSONBody(v);

		return nullptr;
	}

	Font f;

	int getPreferredWidth() const override { return 20 + roundToInt((float)getNumCol() * f.getStringWidthFloat("M")); }
	int getPreferredHeight() const override { return 20 + getNumLines() * 12; };

	int getNumCol() const
	{
		auto start = s.begin();
		auto end = s.end();

		int maxNumChars = 0;
		int thisNumChars = 0;

		while (start != end)
		{
			if (*start++ == '\n')
			{
				maxNumChars = jmax(thisNumChars, maxNumChars);
				thisNumChars = 0;
			}
			else
				thisNumChars++;
		}

		return jmax(thisNumChars, maxNumChars);
	}

	int getNumLines() const
	{
		auto start = s.begin();
		auto end = s.end();

		int numLines = 1;

		while (start != end)
		{
			if (*start++ == '\n')
				numLines++;
		}

		return numLines;
	}

	void paint(Graphics& g) override
	{
		g.setFont(f);
		g.setColour(Colours::white.withAlpha(0.8f));
		g.drawMultiLineText(s, 10, 20, getPreferredWidth()-20);
	}

	var value;
	String s;
};
#endif

struct ScriptBroadcasterMap::VarEntry : public ScriptBroadcasterMap::EntryBase
{
	VarEntry(BodyFactory& f, const var& v):
		EntryBase()
	{
		if (auto childElement = f.create(v))
		{
			addChildWithPreferredSize(childElement);
		}
	}

	int getPreferredWidth() const override
	{
		if (children.isEmpty() || !dynamic_cast<Component*>(children.getFirst())->isVisible())
			return 0;

		return addPinWidth(!children.isEmpty() ? children.getFirst()->getPreferredWidth() : 80);
	}
	int getPreferredHeight() const override 
	{ 
		if (children.isEmpty() || !dynamic_cast<Component*>(children.getFirst())->isVisible())
			return 0;

		return !children.isEmpty() ? children.getFirst()->getPreferredHeight() : 24; 
	};

	bool hasNestedChild() const;

	void paint(Graphics& g) override
	{
		if (!hasNestedChild())
			paintBackground(g, Colour(0xFF444444));
	}
};

struct ScriptBroadcasterMap::BroadcasterEntry : public ScriptBroadcasterMap::EntryBase,
												public ComponentWithMetadata
{
	BroadcasterEntry(BodyFactory& f, ScriptBroadcaster* sb) :
		EntryBase(),
		ComponentWithMetadata(sb->metadata),
		b(sb)
	{
		for (auto t : sb->items)
			addNeighbourData(t->metadata);

		for(auto l: sb->attachedListeners)
			addNeighbourData(l->metadata);

		childLayout = ComponentWithPreferredSize::Layout::ChildrenAreColumns;
        stretchChildren = false;

		int numValues = sb->lastValues.size();

		auto updater = sb->getScriptProcessor()->getMainController_()->getGlobalUIUpdater();

		WeakReference<ScriptBroadcaster> weakSb = sb;

		for (int i = 0; i < numValues; i++)
		{
			addChildWithPreferredSize(new LiveUpdateVarBody(updater, sb->argumentIds[i],
			[weakSb, i]()
			{
				if (weakSb != nullptr)
					return weakSb.get()->lastValues[i];

				return var();
			}));
		}

        menubar.setName(b->metadata.id.toString());
		menubar.setFactory(new ScriptBroadcasterMapFactory());

		menubar.addButton("bypass", Justification::left, [weakSb](Button*, bool value)
		{
			if (weakSb != nullptr)
				weakSb->setBypassed(!value, true, true);
		},
		[weakSb](Button*)
		{
			if (weakSb != nullptr)
				return !weakSb->isBypassed();

			return true;
		});

		menubar.addButton("goto", Justification::right, [weakSb](Button* b, bool)
		{
			if (weakSb != nullptr)
				weakSb->gotoLocationWithDatabaseLookup();
		});

        if(weakSb->enableQueue)
        {
            menubar.addButton("queue", Justification::right, [weakSb](Button*, bool value)
            {
                if (weakSb != nullptr)
                    weakSb->setEnableQueue(value);
            },
            [weakSb](Button*)
            {
                if (weakSb != nullptr)
                    return weakSb->enableQueue;

                return true;
            });
        }
        
        if(weakSb->realtimeSafe)
        {
            menubar.addButton("realtime", Justification::right, [weakSb](Button*, bool value)
            {
                if (weakSb != nullptr)
                    weakSb->setRealtimeMode(value);
            },
            [weakSb](Button*)
            {
                if (weakSb != nullptr)
                    return weakSb->realtimeSafe;

                return true;
            });
        }
		marginTop = 45;
		marginLeft = sb->attachedListeners.isEmpty() ? 0 : PinWidth;
		marginRight = sb->items.isEmpty() ? 0 : PinWidth;
	};

	int getPreferredHeight() const override
	{
		auto maxPins = jmax(inputPins.size(), outputPins.size());

		return jmax(80, 6 + maxPins * PinWidth);
	}

	int getPreferredWidth() const override { return jmax(250, getSumOfChildComponentWidth(this)); };

	void paint(Graphics& g) override
	{
		if (b == nullptr)
			return;

		auto c = b->metadata.c;

		paintBackground(g, c);
	}

	WeakReference<ScriptBroadcaster> b;
};





struct ScriptBroadcasterMap::ListenerEntry : public ScriptBroadcasterMap::EntryBase,
											 public PathFactory,
											 public ComponentWithMetadata
{
	ListenerEntry(BodyFactory& f, ScriptBroadcaster* sb, ScriptBroadcaster::ListenerBase* l) :
		EntryBase(),
		ComponentWithMetadata(l->metadata),
		listener(l)
	{
		id = listener->getItemId().toString();
		font = GLOBAL_BOLD_FONT();
		w = font.getStringWidth(id) + 40;

		addNeighbourData(sb->metadata);

		auto list = l->createChildArray();

		ScopedPointer<BodyFactory> bf = new BodyFactory(f.parent, &f);

		l->registerSpecialBodyItems(*bf);

		childLayout = ComponentWithPreferredSize::Layout::ChildrenAreRows;

		for (auto v : list)
		{
			addChildWithPreferredSize(new VarEntry(*bf, v));
		}

		icon = createPath(l->getItemId().toString().toLowerCase());

		marginTop = 30;
		marginBottom = 5;
		marginLeft = 5;
		marginRight = 5;
	}

	Path createPath(const String& url) const override;

	int getPreferredHeight() const override { return getSumOfChildComponentHeight(this); }
	int getPreferredWidth() const override
	{ 
		return jmax(w, getMaxWidthOfChildComponents(this)); 
	};

	void resized() override
	{
		EntryBase::resized();

		scalePath(icon, getLocalBounds().removeFromTop(30).removeFromLeft(30).reduced(8).toFloat());
	}

	void paint(Graphics& g) override
	{
		if (listener == nullptr)
			return;

		auto c = getColourFromString(id);

        if(metadata.c != Colours::grey)
            c = metadata.c;
        
		paintBackground(g, c, false);

		g.setColour(c.withAlpha(0.8f));
		g.setFont(font);

		g.fillPath(icon);

		auto textArea = getLocalBounds().toFloat().removeFromTop(marginTop);
		textArea.removeFromLeft(icon.getBounds().getRight() + 10);

		g.drawText(id, textArea, Justification::left);
	}

	int w;
	Font font;
	String id;
	Path icon;
	WeakReference<ScriptBroadcaster::ListenerBase> listener;
};



struct ScriptBroadcasterMap::TargetEntry : public ScriptBroadcasterMap::EntryBase,
										   public ComponentWithMetadata
{
	static int getBestRowLength(int totalSize)
	{
		int empty[7];
		empty[0] = 8;

		for (int i = 1; i < 7; i++)
			empty[i] = totalSize % i;

		for (int i = 7; i >= 2; i--)
		{
			if (empty[i] == 0)
				return i;
		}

		return 8;
	}

	TargetEntry(BodyFactory& f, ScriptBroadcaster* sb, ScriptBroadcaster::TargetBase* item_) :
		EntryBase(),
		ComponentWithMetadata(item_->metadata),
		item(item_)
	{
		addNeighbourData(sb->metadata);

		marginTop = 5;
		marginLeft = PinWidth;
		marginRight = 5;
		marginBottom = 5;

		textToDisplay << item->getItemId().toString() << ": " << item->metadata.id.toString();

		minWidth = GLOBAL_BOLD_FONT().getStringWidth(textToDisplay) + 4 * 24 + 10;

		ScopedPointer<BodyFactory> bf = new BodyFactory(f.parent, &f);

		item->registerSpecialBodyItems(*bf);

		auto list = item->createChildArray();

		auto numTargets = list.size();

		if (numTargets > 8)
		{
			auto rowLength = getBestRowLength(numTargets);

			ScopedPointer<Row> currentRow = new Row();

			for (auto v : list)
			{
				currentRow->addChildWithPreferredSize(new VarEntry(*bf, v));
				if (currentRow->children.size() == rowLength)
				{
					addChildWithPreferredSize(currentRow.release());
					currentRow = new Row();
				}
			}

			if (!currentRow->children.isEmpty())
				addChildWithPreferredSize(currentRow.release());

			childLayout = ComponentWithPreferredSize::Layout::ChildrenAreRows;
			lineBreak = true;
		}
		else
		{
			for (const auto& v : list)
			{
				addChildWithPreferredSize(new VarEntry(*bf, v));
			}

			childLayout = ComponentWithPreferredSize::Layout::ChildrenAreColumns;
		}

		WeakReference<ScriptBroadcaster::TargetBase> weakItem(item);

		menubar.setFactory(new ScriptBroadcasterMapFactory());

		menubar.addButton("bypass", Justification::left, [weakItem](Button*, bool value)
		{
			if (weakItem != nullptr)
				weakItem->enabled = value;
		},
		[weakItem](Button*)
		{
			if(weakItem != nullptr)
				return weakItem->enabled;
            
            return false;
		});

        menubar.setName(textToDisplay);
        menubar.textColour = weakItem->metadata.c;
        
		auto jp = dynamic_cast<ScriptBroadcasterMap*>(&f.parent)->p;
		auto loc = weakItem->location;

		menubar.addButton("goto", Justification::right, [jp, loc](Button* b, bool value)
		{
			DebugableObject::Helpers::gotoLocation(b, jp, loc);
		});

		marginTop = 30;
	}

	void paint(Graphics& g) override
	{
		if (item == nullptr)
			return;

		auto c = item->metadata.c;

		paintBackground(g, c, children.isEmpty());
	}

	int getPreferredHeight() const override
	{
		auto numChildren = children.size();

		if (numChildren == 0)
			return 28;
		else if (!lineBreak)
			return getMaxHeightOfChildComponents(this);
		else
			return getSumOfChildComponentHeight(this);
	}

	int getPreferredWidth() const override
	{
		auto numChildren = children.size();

		

		if (numChildren == 0)
			return minWidth;
		else if (!lineBreak)
			return jmax(minWidth, getSumOfChildComponentWidth(this));
		else
			return jmax(minWidth, getMaxWidthOfChildComponents(this));
	};

	bool lineBreak = false;

	String textToDisplay;
	int minWidth;

	WeakReference<ScriptBroadcaster::TargetBase> item;
};

struct ScriptBroadcasterMap::BroadcasterRow : public Component,
											  public ComponentWithPreferredSize
{
	int getPreferredWidth() const override
	{
		return getSumOfChildComponentWidth(this);
	}

	int getPreferredHeight() const override
	{
		return getMaxHeightOfChildComponents(this);
	}

	static void handleError(BroadcasterRow& row, ScriptBroadcaster::ItemBase* item, String error)
	{
		Component::callRecursive<ListenerEntry>(&row, [&](ListenerEntry* e)
		{
			if (e->listener.get() == item)
			{
				e->setCurrentError(error);
				return true;
			}
			return false;
		});

		Component::callRecursive<TargetEntry>(&row, [&](TargetEntry* t)
		{
			if (t->item.get() == item)
			{
				t->setCurrentError(error);
				return true;
			}
			return false;
		});
	}

	BroadcasterRow(BodyFactory& f, ScriptBroadcaster* b):
		broadcaster(b)
	{
		b->metadata.attachCommentFromCallableObject(var(b), true);

		setInterceptsMouseClicks(false, true);

		childLayout = ComponentWithPreferredSize::Layout::ChildrenAreColumns;

		ScopedPointer<Column> sources;
		
		ScopedPointer<Column> targets;

		padding = 30;
        stretchChildren = false;

		auto bcEntry = new BroadcasterEntry(f, b);
		auto bcWithComment = CommentDisplay::attachComment(bcEntry, b->metadata, Justification::top);
		
		b->errorBroadcaster.addListener(*this, BroadcasterRow::handleError, true);

		if (!b->attachedListeners.isEmpty())
		{
			sources = new Column();

			sources->padding = 20;

			for (auto al : b->attachedListeners)
			{
				auto le = new ListenerEntry(f, b, al);

				for (auto c : le->children)
					bcEntry->connectToOutput(dynamic_cast<EntryBase*>(c));

				auto l = TagItem::attachTags(le, al->metadata);

				sources->addChildWithPreferredSize(CommentDisplay::attachComment(l, al->metadata, Justification::left));
			}
		}

		if (!b->items.isEmpty())
		{
			targets = new Column();

			targets->padding = 20;

			for (auto i : b->items)
			{
				auto te = new TargetEntry(f, b, i);
				te->connectToOutput(bcEntry);
				
                auto tar = TagItem::attachTags(te, i->metadata);
				targets->addChildWithPreferredSize(CommentDisplay::attachComment(tar, i->metadata, Justification::right));
			}
		}

		if (sources != nullptr)
			addChildWithPreferredSize(sources.release());

		addChildWithPreferredSize(TagItem::attachTags(bcWithComment, b->metadata));

		if (targets != nullptr)
			addChildWithPreferredSize(targets.release());

		resetSize();
	}

	void resized() override
	{
		resizeChildren(this);
	}

	void setNested()
	{
		marginTop = 10;
		marginLeft = 10;
		marginBottom = 10;
		marginRight = 10;
	}

	WeakReference<ScriptBroadcaster> broadcaster;

	OwnedArray<Column> columns;

	JUCE_DECLARE_WEAK_REFERENCEABLE(BroadcasterRow);
};


bool ScriptBroadcasterMap::VarEntry::hasNestedChild() const
{
	return dynamic_cast<BroadcasterRow*>(children.getFirst()) != nullptr;
}

void ScriptBroadcaster::OtherBroadcasterListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerFunction([&factory](Component* r, const var& v)
	{
		ComponentWithPreferredSize* c = nullptr;

		if (auto sb = dynamic_cast<ScriptBroadcaster*>(v.getObject()))
		{
			if (auto root = dynamic_cast<ComponentWithPreferredSize*>(r))
			{
				for (int i = 0; i < root->children.size(); i++)
				{
					if (auto br = dynamic_cast<ScriptBroadcasterMap::BroadcasterRow*>(root->children[i]))
					{
						if (br->broadcaster == sb)
						{
							br->setNested();
							return root->children.removeAndReturn(i);
						}
					}
				}
			}
		}

		//c = new ScriptBroadcasterMap::BroadcasterRow(factory, sb, true);
		
		return c;
	});
}

struct ScriptBroadcasterMap::EmptyDisplay : public Component
{
	EmptyDisplay(const String& text_):
		text(text_)
	{
		setInterceptsMouseClicks(false, true);
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(text, getLocalBounds().toFloat(), Justification::centred);
	}

	String text;
};



} // namespace ScriptingObjects
} // namespace hise
