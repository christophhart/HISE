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

namespace scriptnode {
using namespace juce;
using namespace hise;


namespace routing
{

/* TODO: Ideas for routing:

	- make popup that shows all routing destination / targets OK
	- make debug popup OK
	- add debug popup to module browser
	- use connection range from script component
	- implement code goto
	- allow set from receive OK
	- make scripting layer for cables / signals OK
	- attach scripting callback to the value send (with sync / async option)... OK
	- increase margin in cable editor OK
	- make global 64 block processing
	- throw error if network is set to compileable (perhaps make compile-check system based on a `AllowCompilation` property listener) OK
*/

struct GlobalRoutingManager: public ReferenceCountedObject
{
	

	using Ptr = ReferenceCountedObjectPtr<GlobalRoutingManager>;

	using IdList = StringArray;

	struct SlotBase;

	struct Helpers
	{
		static GlobalRoutingManager::Ptr getOrCreate(MainController* mc);

		static Component* createDebugViewer(MainController* mc);

		static Colour getColourFromId(const String& id);

		static void addGotoTargetCallback(Button* b, SlotBase* slot);

		static int getOSCMessageIndex(const String& id);

		static StringArray getCableIds(const OSCMessage& m, const String& domain);
	};

	struct SelectableTargetBase
	{
		using List = Array<WeakReference<SelectableTargetBase>>;

		virtual ~SelectableTargetBase() {};

		virtual void selectCallback(Component* rootEditor) = 0;

		virtual String getTargetId() const = 0;

		

		JUCE_DECLARE_WEAK_REFERENCEABLE(SelectableTargetBase);
	};

	struct CableTargetBase: public SelectableTargetBase
	{
		using List = Array<WeakReference<CableTargetBase>>;

		virtual ~CableTargetBase() {};

		virtual void sendValue(double v) = 0;

		virtual Path getTargetIcon() const = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(CableTargetBase);
	};

	struct RoutingIcons : public PathFactory
	{
		Path createPath(const String& url) const override;;
	};

	struct EditorBase: public RoutingIcons
	{
		EditorBase(GlobalRoutingManager::Ptr m) :
			manager(m)
		{}

		virtual ~EditorBase() {};
	
		juce::Component* asComponent() { return dynamic_cast<Component*>(this); }

		GlobalRoutingManager::Ptr manager;
	};

	struct SlotBase: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<SlotBase>;
		using List = ReferenceCountedArray<SlotBase>;

		enum class SlotType
		{
			Cable,
			Signal,
			numTypes
		};

		SlotBase(const String& id_, SlotType t) :
			id(id_),
			type(t)
		{};

		virtual ~SlotBase() {};

		virtual bool cleanup() = 0;
		virtual bool isConnected() const = 0;
		virtual SelectableTargetBase::List getTargetList() const = 0;
		
		const String id;
		const SlotType type;

		SimpleReadWriteLock lock;
	};

	struct Cable : public SlotBase
	{
		Cable(const String& id_);;

		SelectableTargetBase::List getTargetList() const override;

		bool cleanup() final override;
		bool isConnected() const final override { return !targets.isEmpty(); }

		bool containsTarget(CableTargetBase* n) const;

		void addTarget(CableTargetBase* n);
		void removeTarget(CableTargetBase* n);

		void sendValue(CableTargetBase* source, double v);
		double getLastValue() const { return lastValue; }

		double lastValue = 0.0;
		CableTargetBase::List targets;
	};

	struct Signal: public SlotBase
	{
		Signal(const String& id_);

		bool isConnected() const final override { return sendNode != nullptr && !targetNodes.isEmpty(); }

		void removeTarget(NodeBase* targetNode);

		SelectableTargetBase::List getTargetList() const override;

		Error matchesSourceSpecs(PrepareSpecs targetSpecs);

		Result addTarget(NodeBase* targetNode, PrepareSpecs p);

		bool cleanup() final override;
		void clearSignal();

		Result setSource(NodeBase* newSendNode, PrepareSpecs p);

		void push(ProcessDataDyn& data, float value);
		int pop(ProcessDataDyn& data, float value, int offset);

		Result setConnection(NodeBase* n, bool shouldAdd, PrepareSpecs ps, bool isSource);

		PrepareSpecs sourceSpecs;
		span<float*, NUM_MAX_CHANNELS> channelData;
		span<float, NUM_MAX_CHANNELS> signalPeaks;
		heap<float> buffer;
		ProcessDataDyn lastData;

		NodeBase::Ptr sendNode;
		NodeBase::List targetNodes;
	};

	struct DebugComponent;

	/** Connects the global routing manager to an OSC port. */
	bool connectToOSC(OSCConnectionData::Ptr data);

	void removeUnconnectedSlots(SlotBase::SlotType type);

	IdList getIdList(SlotBase::SlotType type)
	{
		IdList sa;

		auto& lToUse = type == SlotBase::SlotType::Cable ? cables : signals;

		for (auto c : lToUse)
			sa.addIfNotAlreadyThere(c->id);
			
		return sa;
	}

	ReferenceCountedObjectPtr<SlotBase> getSlotBase(const String& id, SlotBase::SlotType t);

	SlotBase::List signals, cables;

	LambdaBroadcaster<SlotBase::SlotType, IdList> listUpdater;

	WeakErrorHandler::Ptr oscErrorHandler;

	LambdaBroadcaster<OSCConnectionData::Ptr> oscListeners;

	void sendOSCError(const String& r);

	void setOSCErrorHandler(WeakErrorHandler* errorHandler)
	{
		oscErrorHandler = errorHandler;
	}

	Array<OSCAddressPattern> scriptCallbackPatterns;

	bool sendOSCMessageToOutput(const String& subAddress, const var& data);

	void handleParsingError(const char* data, int dataSize);

	void addOSCTarget(SlotBase::Ptr p);

	struct OSCBase: public ReferenceCountedObject
	{
		OSCBase(GlobalRoutingManager* p) :
			parent(p)
		{};

		using Ptr = ReferenceCountedObjectPtr<OSCBase>;

		virtual ~OSCBase() {};

		virtual bool matches(const String& url, int port) const = 0;

		bool ok = false;
		WeakReference<GlobalRoutingManager> parent;
	};

	scriptnode::OSCConnectionData::Ptr lastData;

	OSCBase::Ptr sender;
	OSCBase::Ptr receiver;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalRoutingManager);
};

}
}
