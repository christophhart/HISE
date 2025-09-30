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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;

class GlobalModulator;
class GlobalModulatorContainer;


struct ScriptingApi::Content::ScriptSlider::MultiMatrixModulatorConnection: public MatrixConnectionBase,
																			public Processor::AttributeListener
{
	MultiMatrixModulatorConnection(ScriptSlider& s, const ValueTree& matrixData, const String& targetId):
	  MatrixConnectionBase(s, matrixData, targetId),
	  AttributeListener(s.getScriptProcessor()->getMainController_()->getRootDispatcher())
	{
		Processor::Iterator<MatrixModulator> iter(getMainController()->getMainSynthChain());

		while(auto mm = iter.getNextProcessor())
		{
			if(mm->getMatrixTargetId() == targetId)
				connectedMods.add(mm);
		}

		if(auto m = connectedMods.getFirst())
		{
			auto mvf = m->getModulationQueryFunction(MatrixModulator::SpecialParameters::Value);
			auto componentIndex = s.getScriptProcessor()->getScriptingContent()->getComponentIndex(&s);

			// if this happens, the component wasn't added yet.
			if(componentIndex == -1)
				componentIndex = s.getScriptProcessor()->getScriptingContent()->getNumComponents();


			s.getScriptProcessor()->setModulationDisplayQueryFunction(componentIndex, m, mvf);
			uint16 indexes[1] = { (uint16)componentIndex };
			addToProcessor(dynamic_cast<Processor*>(s.getScriptProcessor()), indexes, 1, dispatch::sendNotificationSync);
		}
	}

	void onAttributeUpdate(Processor* p, uint16 index) override
	{
		auto v = p->getAttribute(index);

		for(auto m: connectedMods)
			m->setAttribute(MatrixModulator::SpecialParameters::Value, v, sendNotificationAsync);
	}

	SimpleRingBuffer::Ptr getDisplayBuffer(int index) override
	{
		if(auto m = connectedMods.getFirst())
			m->getDisplayBuffer(index);

		return nullptr;
	}

	MatrixIds::Helpers::IntensityTextConverter::ConstructData createIntensityConverter(int sourceIndex) override
	{
		MatrixIds::Helpers::IntensityTextConverter::ConstructData cd;
		cd.parameterIndex = parent->getScriptProcessor()->getScriptingContent()->getComponentIndex(parent);
		cd.p = dynamic_cast<Processor*>(parent->getScriptProcessor());
		auto rng = RangeHelpers::getDoubleRange(parent->getPropertyValueTree(), RangeHelpers::IdSet::ScriptComponents);
		cd.inputRange = rng.rng;
		cd.prettifierMode = parent->getScriptObjectProperty(ScriptSlider::Mode);

		auto con = MatrixIds::Helpers::getConnection(matrixData, sourceIndex, targetId);

		if(con.isValid())
		{
			jassertfalse; //MatrixIds::Helpers::getTargetType();
		}

		return cd;
	}

	

	Array<WeakReference<MatrixModulator>> connectedMods;
};

struct ScriptingApi::Content::ScriptSlider::MatrixCableConnection: public MatrixConnectionBase
{
	using CableType = routing::GlobalRoutingManager::Cable;
	static constexpr auto C = routing::GlobalRoutingManager::SlotBase::SlotType::Cable;

	struct QueryFunction: public ModulationDisplayValue::QueryFunction
	{
		QueryFunction(MatrixCableConnection* c);
		WeakReference<MatrixCableConnection> connection;
		bool onScaleDrag(Processor* p, bool isDown, float delta) override { return true; };
		ModulationDisplayValue getDisplayValue(Processor* p, double nv, NormalisableRange<double> nr) const override;
	};

	struct Target: public routing::GlobalRoutingManager::CableTargetBase,
				   public ReferenceCountedObject,
				   public PooledUIUpdater::SimpleTimer
	{
		
		using List = ReferenceCountedArray<Target>;

		struct AuxTarget: public routing::GlobalRoutingManager::CableTargetBase,
						  public ReferenceCountedObject
		{
			using Ptr = ReferenceCountedObjectPtr<AuxTarget>;

			AuxTarget(Target& p, const String& sourceId);
			~AuxTarget() override;

			Path getTargetIcon() const override { return {}; };

			void selectCallback(Component* rootEditor) override {};
			void sendValue(double v) override;
			double getAuxValue() const;
			String getTargetId() const override { return "CableMatrixAux"; }

			double lastAuxValue;
			double auxIntensity;
			ReferenceCountedObjectPtr<CableType> cable;
			WeakReference<Target> parentTarget;
		};

		Target(MatrixCableConnection& parent_, const ValueTree& connection_);
		~Target() override;

		void selectCallback(Component* rootEditor) override {};
		String getTargetId() const override { return "CableMatrix"; }
		void sendValue(double v) override;
		void timerCallback() override;
		void onPropertyUpdate(const Identifier& id, const var& newValue);

		uint32 lastMs = 0;

		bool isVoiceStart = false;
		double voiceStartValue = 0.0;

		ValueTree connection;
		AuxTarget::Ptr auxTarget;
		SimpleRingBuffer::Ptr rb;

		MatrixCableConnection& parent;
		scriptnode::modulation::TargetMode tm;
		double intensity;
		double lastModValue = 0.0;
		bool inverted = false;
		int sourceIndex;
		ReferenceCountedObjectPtr<CableType> sourceCable;

		Path getTargetIcon() const override { return {}; }
		valuetree::PropertyListener propertyListener;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Target);
	};

	MatrixCableConnection(ScriptSlider& slider, const ValueTree& matrixData, const String& targetId_);;
	~MatrixCableConnection() override;

	void setTargetSlider(ScriptSlider* newParent);
	MatrixIds::Helpers::IntensityTextConverter::ConstructData createIntensityConverter(int sourceIndex) override;
	void rebuildTargets();

	SimpleRingBuffer::Ptr getDisplayBuffer(int sourceIndex) override;
	ModulationDisplayValue getDisplayValue(double nv, NormalisableRange<double> nr);
	void onSourceTargetChange(const ValueTree& v, const Identifier& id);
	void addConnection(const ValueTree& v);
	void removeConnection(const ValueTree& v);
	void onUpdate(const ValueTree& v, bool wasAdded);
	void calculateNewModValue();

	SimpleReadWriteLock listLock;

	Target::List allTargets;
	Target::List scaleTargets;
	Target::List addTargets;
	ModulationDisplayValue mv;

	StringArray sourceNames;
	valuetree::ChildListener connectionListener;
	valuetree::RecursivePropertyListener sourceTargetListener;
	InvertableParameterRange sliderRange;
	Array<var> globalModCables;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(MatrixCableConnection);
};



namespace ScriptingObjects
{

struct ScriptModulationMatrix : public ConstScriptingObject,
								public ControlledObject,
								public UserPresetStateManager
{
	ScriptModulationMatrix(ProcessorWithScriptingContent* p, const String& cid);

	~ScriptModulationMatrix();

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptModulationMatrix"); }

	Identifier getUserPresetStateId() const override { return MatrixIds::MatrixData; }

	void resetUserPresetState() override
	{
		clearAllConnections({});
	}

	ValueTree exportAsValueTree() const override;

	void restoreFromValueTree(const ValueTree &previouslyExportedState) override;

	// =============================================================================================

	/** Adds (or removes) a connection from the source to the target. */
	bool connect(String sourceId, String targetId, bool addConnection);

	/** Get the target ID (either ID of the matrix modulator or matrixTargetId property) for the given component. */
	String getTargetId(var componentOrId);

	/** Get the component reference for the given modulation target ID. */
	var getComponent(String targetId);

    /** Returns a JSON object with the visualisation data for the given modulation target. */
    var getModulationDisplayData(String targetId);

    /** Checks whether the modulation connection can be made. */
	bool canConnect(String source, String target);

	/** Creates a Base64 string of all connections. */
	String toBase64();

	/** Loads the state from a previously exported Base64 string. */
	void fromBase64(String b64);

	/** Removes all connections for the given target (or all connections if no target is specified). */
	void clearAllConnections(String targetId);

	/** Set a callback that will be executed whenever the matrix state changes. */
	void setConnectionCallback(var updateFunction);

	/** Set a callback that will be executed when the user clicks on "Edit connections". */
	void setEditCallback(var menuItems, var editFunction);

	/** Return a list of all sources. */
	var getSourceList() const;

	/** Return a list of all targets. */
	var getTargetList() const;

	/** Sets the currently selected source. */
	void setCurrentlySelectedSource(String sourceId);

	/** Attaches a callback to be notified whenever a new modulation source is selected. */
	void setSourceSelectionCallback(var sourceSelectionCallback);

	/** Attaches a callback to be notified wheneve a modulation connection is being dragged. */
	void setDragCallback(var newDragCallback);

	/** Sets the global properties for the matrix modulation system. */
	void setMatrixModulationProperties(var newProperties);

	/** Sets the property of a modulation connection (with undo). */
	bool setConnectionProperty(String sourceId, String targetId, String propertyId, var value);
	
	/** Returns the property of a modulation connection. */
	var getConnectionProperty(String sourceId, String targetId, String propertyId);
	
	/** Returns a JSON object with the current matrix modulation properties. */
	var getMatrixModulationProperties() const;

	// =============================================================================================

private:

	struct QueryObject
	{
	    ReferenceCountedObject* slider;
		WeakReference<Processor> p;
		ModulationDisplayValue::QueryFunction::Ptr qf;
	};

    std::map<String, QueryObject> queryFunctions;
    
    var getModulationDataFromQueryFunction(const QueryObject& p);

    void callSuspended(const std::function<void(ScriptModulationMatrix&)>& f)
	{
		auto safeThis = WeakReference<ScriptModulationMatrix>(this);
		auto pf = [safeThis, f](Processor* p)
		{
			if(safeThis != nullptr)
				f(*safeThis);

			return SafeFunctionCall::OK;
		};

		auto tt = getMainController()->getKillStateHandler().getCurrentThread();

		// allow it to be executed synchronously in the loading thread
		if(tt != MainController::KillStateHandler::TargetThread::SampleLoadingThread)
			tt = MainController::KillStateHandler::TargetThread::ScriptingThread;

		auto p = dynamic_cast<Processor*>(getScriptProcessor());
		p->getMainController()->getKillStateHandler().killVoicesAndCall(p, pf, tt);
	}
	

	StringArray sourceList;
	StringArray allTargets;
	StringArray parameterTargets;
	StringArray modulatorTargets;

	struct Wrapper;

	UndoManager* um = nullptr;

	WeakCallbackHolder connectionCallback;
	WeakCallbackHolder editCallback;
	WeakCallbackHolder sourceSelectionCallback;
	WeakCallbackHolder dragCallback;

	valuetree::ChildListener connectionListener;


	WeakReference<GlobalModulatorContainer> container;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptModulationMatrix);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptModulationMatrix);
};

}


} // namespace hise
