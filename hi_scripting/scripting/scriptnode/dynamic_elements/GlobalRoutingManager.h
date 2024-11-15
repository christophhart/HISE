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

struct LocalCableHelpers
{
	static void replaceAllLocalCables(ValueTree& networkTree);
	static void explode(ValueTree nodeTree, UndoManager* um);
	static void create(DspNetwork* network, const ValueTree& connectionData);
	static void showAllOccurrences(DspNetwork* network, const String& variableName);
	static Array<ValueTree> getListOfConnectedNodeTrees(const ValueTree& networkTree, const String& variableName);
	static Array<WeakReference<NodeBase>> getListOfConnectedNodes(DspNetwork* network, const ValueTree& nodeTreeToSkip, const String& variableName);
	static StringArray getListOfLocalVariableNames(const ValueTree& networkTree);
	
};

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

    struct RuntimeTargetBase: public CableTargetBase
    {
        Path getTargetIcon() const override { return {}; };
        
        void sendValue(double v) override
        {
            target->onValue(v);
        }
        
        runtime_target::target_base<double>* target;
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

	struct Cable : public SlotBase,
                   public runtime_target::source_base
	{
        using TargetType = runtime_target::target_base<double>;
        
		Cable(const String& id_);;

		SelectableTargetBase::List getTargetList() const override;

        int getRuntimeHash() const override
        {
            return id.hashCode();
        };
        
        runtime_target::RuntimeTarget getType() const override
        {
            return runtime_target::RuntimeTarget::GlobalCable;
        }
        
        static void setValueStatic(source_base* sb, double newValue)
        {
            auto c = static_cast<Cable*>(sb);
            c->sendValue(nullptr, newValue);
        }
        
        
        
        template <bool Add> static bool connectStatic(runtime_target::source_base* sb, TargetType* target)
        {
			auto c = dynamic_cast<Cable*>(sb);

            auto& rt = c->initRuntimeTarget();
            
            if(Add)
            {
                return rt.runtimeTargets.addIfNotAlreadyThere(target);
            }
            else
                return rt.runtimeTargets.removeAllInstancesOf(target) != 0;

        }
        
        runtime_target::connection createConnection() const override
        {
            auto c = source_base::createConnection();
            
            c.connectFunction = (void*)connectStatic<true>;
            c.disconnectFunction = (void*)connectStatic<false>;
            c.sendBackFunction = (void*)setValueStatic;
            
            return c;
        }
        
		bool cleanup() final override;
		bool isConnected() const final override { return !targets.isEmpty(); }

		bool containsTarget(CableTargetBase* n) const;

		void addTarget(CableTargetBase* n);
		void removeTarget(CableTargetBase* n);

        
        
		void sendValue(CableTargetBase* source, double v);
		double getLastValue() const { return lastValue; }

		double lastValue = 0.0;
		CableTargetBase::List targets;
        
        struct RuntimeTarget: public CableTargetBase
        {
            void sendValue(double v) override
            {
                for(auto t: runtimeTargets)
                    t->onValue(v);
            }

            Path getTargetIcon() const override
            {
                static const unsigned char pathData[] = { 110,109,32,134,18,68,159,68,182,67,98,134,187,11,68,128,157,172,67,0,128,7,68,236,110,161,67,0,128,7,68,236,110,161,67,98,0,128,7,68,236,110,161,67,12,130,23,68,184,97,125,67,104,76,38,68,68,117,126,67,98,100,126,40,68,48,158,126,67,122,185,42,68,138,
                31,128,67,156,238,44,68,168,115,129,67,108,109,61,41,68,238,213,136,67,98,220,94,40,68,187,117,136,67,215,119,39,68,13,67,136,67,233,138,38,68,13,67,136,67,98,133,132,31,68,13,67,136,67,120,208,25,68,41,171,147,67,120,208,25,68,241,183,161,67,98,120,
                208,25,68,181,145,163,67,206,233,25,68,213,95,165,67,232,25,26,68,249,28,167,67,108,32,134,18,68,159,68,182,67,99,109,253,159,55,68,252,131,139,67,98,182,145,63,68,76,101,149,67,0,128,69,68,139,162,161,67,0,128,69,68,139,162,161,67,108,180,223,68,68,
                68,114,163,67,98,180,223,68,68,68,114,163,67,164,215,55,68,19,8,195,67,232,103,38,68,82,6,196,67,98,161,13,35,68,56,55,196,67,240,216,31,68,229,13,195,67,16,219,28,68,191,13,193,67,108,254,10,33,68,227,173,184,67,98,68,181,34,68,92,71,186,67,201,146,
                36,68,212,44,187,67,233,138,38,68,212,44,187,67,98,77,145,45,68,212,44,187,67,91,69,51,68,185,196,175,67,91,69,51,68,241,183,161,67,98,91,69,51,68,176,199,157,67,159,210,50,68,167,12,154,67,226,5,50,68,26,184,150,67,108,253,159,55,68,252,131,139,67,99,
                109,100,111,59,68,191,47,90,67,108,64,249,62,68,47,87,104,67,108,252,154,17,68,32,232,206,67,108,32,17,14,68,105,212,199,67,108,100,111,59,68,191,47,90,67,99,101,0,0 };

                Path path;
                path.loadPathFromData (pathData, sizeof (pathData));

                return path;
            }
            
            void selectCallback(Component* rootEditor) {};

            String getTargetId() const { return "RuntimeTargets (" + String(runtimeTargets.size()) + ")"; }

            
            Array<TargetType*> runtimeTargets;
            
        };
        
        RuntimeTarget& initRuntimeTarget()
        {
            if(runtimeTarget == nullptr)
            {
                runtimeTarget = new RuntimeTarget();
                addTarget(runtimeTarget);
            }
            
            return *runtimeTarget;
        }
        
        ScopedPointer<RuntimeTarget> runtimeTarget;

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

    void connectToRuntimeTargets(OpaqueNode& on, bool shouldAdd)
    {
        for(auto c: cables)
        {
            auto con = dynamic_cast<Cable*>(c)->createConnection();
            on.connectToRuntimeTarget(shouldAdd, con);
        }
    }
    
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

	hise::AdditionalEventStorage additionalEventStorage;

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
