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
namespace dyncomp {
using namespace juce;

struct Factory;
struct Base;

/** This is the data model that is used by the dynamic container.
 *
 *  It holds a ValueTree for the data and one for the values.
 */	
struct Data: public ReferenceCountedObject,
			 public ControlledObject
{
	enum class TreeType
	{
		Data,
		Values
	};

	enum class RefreshType
	{
		repaint,
		changed,
		updateValueFromProcessorConnection,
		loseFocus,
		resetValueToDefault,
		numRefreshTypes
	};

	static RefreshType getRefreshType(const var& t);

	using Ptr = ReferenceCountedObjectPtr<Data>;
	using ValueCallback = std::function<void(const Identifier&, const var&)>;

	using ImageProvider = std::function<Image(const String&)>;
	using FontProvider = std::function<Font(const String&, const String&)>;

	Data(MainController* mc, const var& obj, Rectangle<int> position);

	~Data() override;

	void setValues(const var& valueObject);

	Image getImage(const String& ref);

	simple_css::StyleSheet::Collection::DataProvider* createDataProvider() const;

	/** Converts a JSON object coming from HISE's interface designer to a ValueTree. */
	static ValueTree fromJSON(const var& obj, Rectangle<int> position);

	ReferenceCountedObjectPtr<Base> create(const ValueTree& v);

	void onValueChange(const Identifier& id, const var& newValue, bool useUndoManager);

	const ValueTree& getValueTree(TreeType t) const;

	void setValueCallback(const ValueCallback& v);
	void setDataProviderCallbacks(const ImageProvider& ip_, const FontProvider& fp_);

	var getFloatingTileData(const Identifier& id)
	{
		return floatingTileData->getProperty(id);
	}

	LambdaBroadcaster<ValueTree, RefreshType, bool> refreshBroadcaster;

	ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject> createGraphicsObject(const ValueTree& dataTree, ConstScriptingObject* obj);

	DrawActions::Handler* getDrawHandler(const ValueTree& dataTree) const;

	Factory* getFactory();

	void onChildAdd(const ValueTree& v, bool wasAdded);

private:

	/** A subclass that performs additional data handling of a child component.
	 *
	 *  This subclass will react to both child add / remove events with the given type
	 *	as well as value changes. All callbacks are executed synchronously so you can adapt the
	 *	data model. 
	 */
	struct SubDataHandler: public ControlledObject
	{
		SubDataHandler(Data* d, const ValueTree& v);

		~SubDataHandler()
		{
			valueListener.shutdown();
		}

		bool matches(const ValueTree& v) const { return componentData == v; }

		// call this in your constructor
		void initValueListener();
		
		/** Override this and store the data. This is called whenever the value changes. */
		virtual void onValue(const Identifier& id, const var& newValue);

	protected:

		const Identifier id;

		ValueTree componentData;
		ValueTree values;
		valuetree::PropertyListener valueListener;
	};

	/** A data handler that manages the drag container connection to a FX chain. It performs these steps
	 *
	 *  - on value change it will switch the order of the FX processing
	 *	- on child add / remove it will call HotswappableProcessor::setEffect() with the `text` property of
	 *	  the child element (or an empty string if removed). It will also bypass the FX that are not active
	 */
	struct DragContainerHandler: public SubDataHandler
	{
		struct EffectChainManager;

		DragContainerHandler(Data* d, const ValueTree& v);
		void onValue(const Identifier& id, const var& newValue) override;
		EffectProcessorChain* getDragContainerEffectChain();
		void onChildAdd(ValueTree v, bool wasAdded);

	private:

		Range<int> dragEffectRange;
		var lastValue;
		valuetree::ChildListener connectionListener;
		valuetree::PropertyListener valueListener;
	};

	/** A data handler that connects to a given complex data type (sliderpack, table, audiofile) and
	 *  stores / restores the state using the base64 representation of the data type. */
	struct ComplexDataHandler: public SubDataHandler,
						       public ComplexDataUIUpdaterBase::EventListener,
						       public AsyncUpdater
	{
		ComplexDataHandler(Data* d, const ValueTree& v_, const ExternalData::DataType& dt_);
		~ComplexDataHandler() override;

		void handleAsyncUpdate() override;
		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;
		void onValue(const Identifier& id, const var& newValue) override;
		void onConnectionChange(const Identifier& , const var& );

	private:

		bool recursive = false;
		const ExternalData::DataType dt;
		valuetree::PropertyListener connectionListener;
		ComplexDataUIBase::Ptr complexData;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComplexDataHandler);
	};

	void updateComplexDataHandler(const ValueTree& v, bool wasAdded, ExternalData::DataType dt);
	void updateDragContainer(const ValueTree& v, bool wasAdded);

	OwnedArray<SubDataHandler> subDataHandlers;
	valuetree::RecursiveTypedChildListener valueInitialiser;
	OwnedArray<std::pair<ValueTree, ReferenceCountedObjectPtr<ScriptingObjects::GraphicsObject>>> registeredDrawHandlers;
	DynamicObject::Ptr floatingTileData;

	ValueCallback valueCallback;
	ImageProvider ip;
	FontProvider fp;
	UndoManager* um = nullptr;

	ValueTree data;
	ValueTree values;

	ReferenceCountedObjectPtr<ReferenceCountedObject> factory;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Data);
};

struct Base: public Component,
		     public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<Base>;
	using WeakPtr = WeakReference<Base>;
	using List = ReferenceCountedArray<Base>;
	using WeakList = Array<WeakPtr>;

	static Base* findBaseParent(Component* c);
	static void onRefreshStatic(Base& b, const ValueTree& v, Data::RefreshType rt, bool isRecursive);

	Base(Data::Ptr d, const ValueTree& v);
	~Base() override;
	

	Identifier getId() const;

	virtual void onValue(const var& newValue) = 0;
	virtual void updateChild(const ValueTree& v, bool wasAdded);

	void updateBasicProperties(const Identifier& id, const var& newValue);
	virtual void updatePosition(const Identifier&, const var&);
	virtual void baseChildChanged(Base::Ptr c, bool wasAdded);
	virtual void resizeChild(Base::Ptr b, Rectangle<int> newBounds);
	virtual void hideChild(Base::Ptr b, bool shouldBeVisible);

	void updateCSSProperties(const Identifier&, const var& );
	void initCSSForChildComponent();

	/** Override this if this component should forward all stuff to its
	 *  first child element. */
	virtual bool forwardToFirstChild() const = 0;

	virtual void onRefresh(Data::RefreshType rt, bool recursive);

	void addChild(Base::Ptr c);

	bool operator==(const Base& other) const noexcept;
	bool operator==(const ValueTree& otherData) const noexcept;

	void writePositionInValueTree(Rectangle<int> tb, bool useUndoManager);

	var getValueOrDefault() const;
	var getPropertyOrDefault(const Identifier& id) const;

	const Component* getContentComponent() const { return forwardToFirstChild() ? getChildComponent(0) : this; }
	Component* getContentComponent() { return forwardToFirstChild() ? getChildComponent(0) : this; }

	ValueTree getDataTree() const { return dataTree; }

	void setColourFromData(const Identifier& id, int colourId);

	// override this in the derived classes if the colour ids don't match
	virtual void updateColours();

	void writeComponentPropertiesToStyleSheet(bool force = false);

protected:

	Data::Ptr data;
	ValueTree dataTree;
	ValueTree valueReference;
	simple_css::StyleSheet::Ptr css;

private:

	valuetree::PropertyListener basicPropertyListener;
	valuetree::PropertyListener positionListener;
	valuetree::PropertyListener valueListener;
	valuetree::ChildListener childListener;
	valuetree::PropertyListener cssListener;

	List children;

	JUCE_DECLARE_WEAK_REFERENCEABLE(Base);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Base);
};

struct Root: public Base
{
	Root(Data::Ptr d);

	void onValue(const var& newValue) override {}

	void paint(Graphics& g) override;

	void updatePosition(const Identifier&, const var&) override
	{
		
	}

	bool forwardToFirstChild() const override { return false; }
};

} // namespace dyncomp
} // namespace hise
