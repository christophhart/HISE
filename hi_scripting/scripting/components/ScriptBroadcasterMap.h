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

#pragma once

namespace hise { 
using namespace juce;
namespace ScriptingObjects
{

struct ButtonWithState: public ButtonListener
{
	using StateCallback = std::function<bool(Button*)>;
	using Callback = std::function<void(Button*, bool)>;

	ButtonWithState(const String& name, const PathFactory& f, const Callback& c, const StateCallback& sb) :
		button(name, this, f),
		alignment(Justification::left)
	{
		clickFunction = c;

		if (sb)
		{
			stateFunction = sb;
			button.setToggleModeWithColourChange(true);
			button.setToggleStateAndUpdateIcon(stateFunction(&button));
		}
	}

	void setColour(Colour c)
	{
		button.onColour = c;
		button.offColour = c;
		button.refreshButtonColours();
	}

	void buttonClicked(Button*) override
	{
		if(clickFunction)
			clickFunction(&button, button.isToggleable() ? button.getToggleState() : true);
	}

	bool checkState()
	{
		if (!stateFunction)
			return false;

		auto state = stateFunction(&button);

		if (button.getToggleState() != state)
		{
			button.setToggleStateAndUpdateIcon(state);
			return true;
		}

		return false;
	}

	struct MenuBar : public Component,
					 public Timer
	{
		bool isEmpty() const { return buttons.isEmpty(); };

		void timerCallback() override
		{
			for (auto b : buttons)
				b->checkState();
		}

		void setFactory(PathFactory* ownedFactory)
		{
			factory = ownedFactory;
		}

		void addButton(const String& name, Justification alignment, const Callback& f, const StateCallback& sb = {}, int padding = 0)
		{
			if (factory == nullptr)
			{
				jassertfalse;
				return;
			}

			auto nb = new ButtonWithState(name, *factory, f, sb);

			nb->alignment = alignment;
			nb->padding = padding;

			addAndMakeVisible(nb->button);

			if (isEmpty())
				startTimer(300);

			buttons.add(nb);
		}

        void paint(Graphics& g) override
        {
            g.setFont(GLOBAL_BOLD_FONT());
            g.setColour(textColour);
            g.drawText(getName(), textBounds, Justification::centred);
        }
        
		int checkSize(int preferredWidth)
		{
			if (isEmpty())
			{
				setSize(preferredWidth, 24);
				return 24;
			}

			return 0;
		}

		void resized() override
		{
			auto bounds = getLocalBounds();

			for (auto b : buttons)
			{
				if (b->alignment == Justification::left)
				{
					b->button.setBounds(bounds.removeFromLeft(bounds.getHeight()).reduced(b->margin));
					bounds.removeFromLeft(b->padding);
				}
				else if (b->alignment == Justification::right)
				{
					b->button.setBounds(bounds.removeFromRight(bounds.getHeight()).reduced(b->margin));
					bounds.removeFromRight(b->padding);
				}
				else
					jassertfalse;
			}
            
            textBounds = bounds.toFloat();
		}

        Colour textColour = Colours::white.withAlpha(0.7f);
		ScopedPointer<PathFactory> factory;
		OwnedArray<ButtonWithState> buttons;
        Rectangle<float> textBounds;
	};

	HiseShapeButton button;
	StateCallback stateFunction;
	Callback clickFunction;
	Justification alignment;
	int padding = 0;
	int margin = 3;
};


struct ComponentWithMetadata
{
	struct TagFilterOptions
	{
		bool dimOpacity = false;
		bool useAnd = false;
		bool useNot = false;
		bool showNextSibling = true;
	};

	using Metadata = ScriptBroadcaster::Metadata;

	ComponentWithMetadata(const Metadata& m) :
		metadata(m)
	{};

	static ComponentWithMetadata* getSelfOrFirstChildIfMetadata(ComponentWithPreferredSize* p)
	{
		if (auto cm = dynamic_cast<ComponentWithMetadata*>(p))
			return cm;

		if (auto cm = dynamic_cast<ComponentWithMetadata*>(p->children.getFirst()))
			return cm;

		return nullptr;
	}

	void addNeighboursFrom(ComponentWithMetadata* m)
	{
		addNeighbourData(m->metadata);

		for (const auto& n : m->neighbourData)
			addNeighbourData(n);
	}

	void addNeighbourData(const Metadata& m)
	{
		neighbourData.addIfNotAlreadyThere(m);
	}

	virtual ~ComponentWithMetadata() {};

	bool matchesTag(int64 hash, const TagFilterOptions& options)
	{
		for (const auto& id : metadata.tags)
		{
			if (id.toString().hashCode64() == hash)
				return true;
		}

		if (options.showNextSibling)
		{
			for (const auto& other : neighbourData)
			{
				for (const auto& id : other.tags)
				{
					if (id.toString().hashCode64() == hash)
						return true;
				}
			}
		}

		return false;
	}

	Metadata metadata;
	Array<Metadata> neighbourData;
};


/** TODO:
	
	BUGFIX:

	- fix circles being too big with many outlets
	- fix broadcaster appearing when mouse hover

	SOURCES:

	TARGETS:

	DONE:

	- add source type icons (complex data stuff: take from scriptnode) OK
	- fix popup menu clickablility OK
	- make API for individual components ok
	- toolbuttons: watch, bookmarks, enable bang, zoom fit, OK
	- add "Hide all broadcasters and only show when a message is sent" OK
	- add current values as child of broadcaster node (with editable label that sends a new value) OK
	- minimum size to text width OK
	- add bang system to update the UI (with a flash) OK

	- add API for tool buttons of EntryBase OK
	- add goto where possible + display function names if not anonymous OK
	- add bypass button to broadcaster & items / listeners OK

	- add tags with display ok
	- add async / delay / queue / realtime icons
	- add calls to sendMessage() as listener sources (with location?) ok
	- show error message at location when something went wrong
	- do not use `currentExpression` but only the ID (maybe add comments & grouping via setMetadata() ok
	- improve target description ok

	- add a few "native" targets
	  - component properties OK
	  - value set 
	  - complex data
	  - repaint

	- make CodeTemplate creator
	- show saveInPreset and automationId for all script component value related map items
*/
class ScriptBroadcasterMap : public Component,
							  public ComponentWithPreferredSize,
							  public ControlledObject,
							  public ProcessorHelpers::ObjectWithProcessor,
							  public GlobalScriptCompileListener,
							  public AsyncUpdater
{
public:

	struct MessageWatcher : public Timer
	{
		MessageWatcher(ScriptBroadcasterMap& map);

		void timerCallback() override;

		struct LastTime
		{
			LastTime(ScriptBroadcaster* b);

			bool hasChanged();
			String getName() const { return bc->currentExpression; }
			WeakReference<ScriptBroadcaster> bc;
			uint32 prevTime;
		};

		Array<LastTime> times;
		ScriptBroadcasterMap& parent;
	};

	ScopedPointer<MessageWatcher> currentMessageWatcher;

	static constexpr int PinWidth = 20;
	static constexpr int ButtonHeight = 20;

	Processor* getProcessor() override { return dynamic_cast<Processor*>(p.get()); };

	static Colour getColourFromString(const String& s)
	{
		auto h = static_cast<uint32>(s.hashCode());
		return Colour(h).withSaturation(0.6f).withAlpha(1.0f).withBrightness(0.7f);
	}

	using BroadcasterList = Array<WeakReference<ScriptBroadcaster>>;

	struct CommentDisplay : public Component,
							public ComponentWithPreferredSize,
							public ComponentWithMetadata
	{
		CommentDisplay(const ScriptBroadcaster::Metadata& m, Justification pos_):
			ComponentWithMetadata(m),
			r(m.comment),
			pos(pos_)
		{
			auto sd = r.getStyleData();
			sd.fontSize = 14.0f;
			sd.headlineColour = m.c;
			r.setStyleData(sd);
			r.parse();

			h = (int)r.getHeightForWidth(w, false) + 20;

			if (pos == Justification::top || pos == Justification::bottom)
				h += 30;
			else
				w += 30;
		}

		int getPreferredWidth() const { return w; };
		int getPreferredHeight() const { return h; };

		void drawArrow(Graphics& g, Rectangle<float> area, float rotateFactor)
		{
			area = area.withSizeKeepingCentre(14.0f, 10.0f);
			g.setColour(r.getStyleData().headlineColour);

			Path p;
			p.startNewSubPath(0.0f, 1.0f);
			p.lineTo(0.5, 0.0f);
			p.lineTo(1.0, 1.0);

			p.applyTransform(AffineTransform::rotation(float_Pi / 2.0f * rotateFactor));

			PathFactory::scalePath(p, area);

			g.strokePath(p, PathStrokeType(2.0f));
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat().reduced(3.0f);

			if (pos == Justification::top)		  drawArrow(g, b.removeFromBottom(30.0f).removeFromLeft(30.0f), 0.0f);
			else if(pos == Justification::bottom) drawArrow(g, b.removeFromTop(30.0f).removeFromLeft(30.0f), 2.0f); 
			else if(pos == Justification::left)   drawArrow(g, b.removeFromRight(30.0f).removeFromTop(30.0f), 3.0f); 
			else if(pos == Justification::right)  drawArrow(g, b.removeFromLeft(30.0f).removeFromTop(30.0f), 1.0f); 

			auto c = r.getStyleData().headlineColour;

			g.setColour(c.withAlpha(0.08f));
			g.fillRect(b);
			g.setColour(c.withAlpha(0.2f));
			g.fillRect(b.withWidth(4.0f));

			r.draw(g, b.reduced(7.0f));
		}

		static ComponentWithPreferredSize* attachComment(ComponentWithPreferredSize* item, const ScriptBroadcaster::Metadata& m, Justification position)
		{
			if (m.comment.isNotEmpty())
			{
				auto c = new CommentDisplay(m, position);

				if (auto cm = ComponentWithMetadata::getSelfOrFirstChildIfMetadata(item))
					c->addNeighboursFrom(cm);

				if (position == Justification::top)
				{
					auto r = new Column();
                    r->stretchChildren = false;
					r->addChildWithPreferredSize(c);
					r->addChildWithPreferredSize(item);
					return r;
				}
				else if (position == Justification::right)
				{
					auto r = new Row();
                    r->stretchChildren = false;
					r->addChildWithPreferredSize(item);
					r->addChildWithPreferredSize(c);
					return r;
				}
				else if (position == Justification::left)
				{
					auto r = new Row();
					r->addChildWithPreferredSize(c);
					r->addChildWithPreferredSize(item);
					return r;
				}
				else if (position == Justification::bottom)
				{
					auto r = new Column();
					r->addChildWithPreferredSize(item);
					r->addChildWithPreferredSize(c);
					return r;
				}
			}

			return item;
		}
		
		int w = 300;
		int h = 0;
		MarkdownRenderer r;
		Justification pos;
	};

	struct TagItem : public Component,
					 public ComponentWithPreferredSize,
					 public ComponentWithMetadata
	{
		

		struct TagButton: public Component
		{
			using List = OwnedArray<TagButton>;

			TagButton(const Identifier& id_, Colour c_, float height=12.0f) :
				id(id_),
				c(c_),
				hash(id_.toString().hashCode64())
			{
				f = GLOBAL_BOLD_FONT().withHeight(height);
				w = f.getStringWidth(id.toString()) + 20;
				setRepaintsOnMouseActivity(true);
			}

			int getTagWidth() const
			{
				return w;
			}

			void sendMessage(bool newValue)
			{
				if (on != newValue)
				{
					on = newValue;

					if (map != nullptr)
						map->setEnableTag(hash, on);

					repaint();
				}
			}

			void mouseDown(const MouseEvent& e)
			{
				if (e.mods.isShiftDown())
				{
					jassert(parentList != nullptr);

					auto shouldBeOn = !on;

					for (auto b : *parentList)
						b->sendMessage(shouldBeOn);
				}
				else
				{
					sendMessage(!on);
				}

				repaint();
			}

			void paint(Graphics& g) override
			{
				auto alpha = 0.1f;

				if (on)
					alpha += 0.6f;

				if (isMouseOver(true))
					alpha += 0.2f;

				if (isMouseButtonDown(true))
					alpha += 0.1f;

				g.setColour(c.withAlpha(alpha));
				auto b = getLocalBounds().toFloat().reduced(3.0f);
				g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

				g.setColour(on ? Colours::black.withAlpha(0.5f) : Colours::white.withAlpha(0.1f));
				g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);

				g.setColour(on ? Colours::black : Colours::white.withAlpha(0.45f));
				g.setFont(f);
				g.drawText(id.toString(), b, Justification::centred);
			}

			void setParentList(List* lToUse)
			{
				parentList = lToUse;
			}

			static void update(TagButton& b, const Array<int64>& currentTags)
			{
				b.on = currentTags.contains(b.hash);
				b.repaint();
			}

			void setBroadcasterMap(ScriptBroadcasterMap* parentMap)
			{
				map = parentMap;
				map->tagBroadcaster.addListener(*this, TagButton::update, true);
			}

			bool on = false;
			Font f;
			Colour c;
			const Identifier id;
			int64 hash;
			int w;

			List* parentList;

			ScriptBroadcasterMap* map;
			JUCE_DECLARE_WEAK_REFERENCEABLE(TagButton);
		};

		TagItem(const ScriptBroadcaster::Metadata& m);

		int getPreferredWidth() const override { return 250; }

		int getPreferredHeight() const override { return 24; };

		void paint(Graphics& g) override;

		void resized() override;

		static ComponentWithPreferredSize* attachTags(ComponentWithPreferredSize* item, const ScriptBroadcaster::Metadata& m)
		{
			if (!m.tags.isEmpty())
			{
				auto c = new Column();
				c->addChildWithPreferredSize(item);

				auto ni = new TagItem(m);

				if (auto cm = ComponentWithMetadata::getSelfOrFirstChildIfMetadata(item))
					ni->addNeighboursFrom(cm);

				c->addChildWithPreferredSize(ni);
				return c;
			}

			return item;
		}

		TagButton::List tags;
		int numRows = 0;

		Path tagIcon;
	};

	struct EntryBase : public Component,
					   public ComponentWithPreferredSize
	{
		EntryBase()
		{
			setInterceptsMouseClicks(false, true);
			addAndMakeVisible(menubar);
		}

		int addPinWidth(int innerWidth) const;

		using List = Array<WeakReference<EntryBase>>;

		void paintBackground(Graphics& g, Colour c, bool fill = true);

		void connectToOutput(EntryBase* source);

		Rectangle<int> getBodyBounds() const
		{
			auto b = getLocalBounds();
			b.removeFromLeft(marginLeft);
			b.removeFromRight(marginRight);

			return b.removeFromTop(marginTop);
		}

		Rectangle<int> getContentBounds(bool includeTopMargin) const
		{
			auto b = getLocalBounds();

			b.removeFromLeft(marginLeft);
			b.removeFromRight(marginRight);
			
			b.removeFromBottom(marginBottom);

			if(!includeTopMargin)
				b.removeFromTop(marginTop);

			return b;
		}

		void resized() override
		{
			if (!menubar.isEmpty())
			{
				auto cb = getContentBounds(true);
				menubar.setBounds(cb.removeFromTop(24).translated(0, 3));
			}

			resizeChildren(this);
		}

		void setCurrentError(const String& e);

		virtual ~EntryBase() {};

		List inputPins, outputPins;

		String currentError;

		ButtonWithState::MenuBar menubar;
		bool hasErrorButton = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(EntryBase);
	};

	struct VarEntry;
	struct BroadcasterEntry;
	struct ListenerEntry;
	struct TargetEntry;
	struct BroadcasterRow;
	struct EmptyDisplay;

	void handleAsyncUpdate() override { rebuild(); }

	ScriptBroadcasterMap(JavascriptProcessor* p_, bool active_);

	~ScriptBroadcasterMap()
	{
		getMainController()->removeScriptListener(this);
	}
	
	void rebuild();

	void resized() override
	{
		resizeChildren(this);
	}

#if 0
	void mouseDown(const MouseEvent& e)
	{
		if (e.mods.isRightButtonDown())
		{
			PopupMenu m;

			m.addSectionHeader("Show Broadcaster");

			int idx = 1;

			for (auto b : allBroadcasters)
			{
				m.addItem(idx++, b->currentExpression, true, !filteredBroadcasters.contains(b->currentExpression));
			}

			m.addSeparator();

			m.addItem(900000, "Hide all", true, filteredBroadcasters.size() == allBroadcasters.size());
			m.addItem(900001, "Hide until message is sent", true, currentMessageWatcher != nullptr);

			if (auto result = m.show())
			{
				if (result == 900000)
				{
					if (filteredBroadcasters.isEmpty())
					{
						for (auto b : allBroadcasters)
							filteredBroadcasters.add(b->currentExpression);
					}
					else
						filteredBroadcasters.clear();
				}
				else if (result == 900001)
				{
					if (currentMessageWatcher != nullptr)
						currentMessageWatcher = nullptr;
					else
						currentMessageWatcher = new MessageWatcher(*this);
				}
				else
				{
					auto bToToggle = allBroadcasters[result - 1]->currentExpression;

					if (filteredBroadcasters.contains(bToToggle))
						filteredBroadcasters.removeString(bToToggle);
					else
						filteredBroadcasters.addIfNotAlreadyThere(bToToggle);
				}

				rebuild();
				findParentComponentOfClass<ZoomableViewport>()->zoomToRectangle(getLocalBounds());
			}
		}
	}
#endif

	BroadcasterList allBroadcasters;
	Array<ScriptBroadcaster::Metadata> filteredBroadcasters;
	BodyFactory factory;

	void paint(Graphics& g) override;

	int getPreferredWidth() const override
	{
		return getMaxWidthOfChildComponents(this);
	}

	int getPreferredHeight() const override
	{
		return getSumOfChildComponentHeight(this);
	}

	void setEnableTag(int64 tagHash, bool shouldBeOn)
	{
		if (shouldBeOn)
			currentTags.addIfNotAlreadyThere(tagHash);
		else
			currentTags.removeAllInstancesOf(tagHash);

		tagBroadcaster.sendMessage(sendNotificationAsync, currentTags);
	}

	void updateTagFilter();

	void showAll();

	void zoomToWidth();
	
	StringArray availableTags;

	Array<int64> currentTags;

	ComponentWithMetadata::TagFilterOptions tagFilterOptions;

	LambdaBroadcaster<Array<int64>> tagBroadcaster;

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	void paintOverChildren(Graphics& g) override;

	WeakReference<JavascriptProcessor> p;

	void setShowComments(bool shouldShowComments);

	bool showComments() const { return commentsShown; }

	bool ok = true;
	bool commentsShown = true;
	bool active = false;

	private:

	void paintCablesForOutputs(Graphics& g, EntryBase* b);
	static void forEachDebugInformation(DebugInformationBase::Ptr di, const std::function<void(DebugInformationBase::Ptr)>& f);
	BroadcasterList createBroadcasterList();

	

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptBroadcasterMap);
};




} // namespace ScriptingObjects

} // namespace hise

