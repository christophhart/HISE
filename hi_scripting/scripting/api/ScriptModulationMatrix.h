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

namespace ScriptingObjects
{

struct ScriptModulationMatrix : public ConstScriptingObject,
								public ControlledObject,
								public UserPresetStateManager
{
	enum class ValueMode
	{
		Default = 0,
		Scale = 0x1,
		Unipolar = 0x2,
		Bipolar = 0x3,
		Undefined
	};

	ScriptModulationMatrix(ProcessorWithScriptingContent* p, const String& cid);

	~ScriptModulationMatrix();

	Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptModulationMatrix"); }

	Identifier getUserPresetStateId() const override { RETURN_STATIC_IDENTIFIER("ModulationMatrix"); }

	void resetUserPresetState()
	{
		clearAllConnections();
	}

	ValueTree exportAsValueTree() const override;

	void restoreFromValueTree(const ValueTree &previouslyExportedState) override;

	// =============================================================================================

	/** Sets the amount of modulation targets created for each [voice start, time-variant, envelope] modulation type. */
	void setNumModulationSlots(var numSlotArray);

	/** Adds a global modulator at the given mod chain for sample accurate modulation. */
	void addModulatorTarget(var targetData);

	/** Adds a (low resolution) parameter modulation slot. */
	void addParameterTarget(var targetData);

	/** Adds (or removes) a connection from the source to the target. */
	bool connect(String sourceId, String targetId, bool addConnection);

	/** Get the current modulation value for the connected component. */
	float getModValue(var component);

	/** Get the target ID for the given component. */
	String getTargetId(String componentId);

	/** Get the component ID for the given modulation target ID. */
	String getComponentId(String targetId);

	/** Checks whether the modulation connection can be made. */
	bool canConnect(String source, String target);

	/** Get the connection data for the given component. */
	var getConnectionData(String componentId);

	/** Update the connections in the list. */
	void updateConnectionData(var connectionList);

	/** Update the intensity for the given connection. */
	bool updateIntensity(String source, String target, float intensityValue);

	/** Update the value mode from the combobox item ID. */
	bool updateValueMode(String source, String target, String valueMode);

	/** Creates a Base64 string of all connections. */
	String toBase64();

	/** Loads the state from a previously exported Base64 string. */
	void fromBase64(String b64);

	/** Removes all connections. */
	void clearAllConnections();

	/** Set a callback that will be executed whenever the matrix state changes. */
	void setConnectionCallback(var updateFunction);

	/** Set a callback that will be executed when the user clicks on "Edit connections". */
	void setEditCallback(var editFunction);

	/** Return a list of all sources. */
	var getSourceList() const;

	/** Return a list of all targets. */
	var getTargetList() const;

	/** Creates a range object that can be passed into setTableRowData to update the slider range. */
	var getIntensitySliderData(String sourceId, String targetId);

	/** Creates a JSON object that can be passed into the setTableRowData to update the combobox. */
	var getValueModeData(String sourceId, String targetId);

	/** Enables the undo manager for all modulation edit actions. */
	void setUseUndoManager(bool shouldUseUndoManager);

	// =============================================================================================

private:

	struct MatrixUndoAction : public UndoableAction
	{
		enum class ActionType
		{
			Clear,
			Add,
			Remove,
			Intensity,
			ValueMode,
			Update,
			numActionTypes
		};

		MatrixUndoAction(ScriptModulationMatrix* m, ActionType a, const var oldValue_, const var& newValue_, const String& s = {}, const String& t = {}) :
			UndoableAction(),
			matrix(m),
			type(a),
			oldValue(oldValue_),
			newValue(newValue_),
			source(s),
			target(t)
		{}

		bool perform() override;

		bool undo() override;

		WeakReference<ScriptModulationMatrix> matrix;
		ActionType type;
		var oldValue;
		var newValue;
		String source;
		String target;
	};

	UndoManager* um = nullptr;

	void clearConnectionsInternal();

	void updateConnectionDataInternal(var connectionList);

	bool connectInternal(const String& source, const String& target, bool addConnection);
	 
	bool updateIntensityInternal(String source, String target, float intensityValue);

	bool updateValueModeInternal(String source, String target, String valueMode);

	int getSourceIndex(const String& id) const;

	int getTargetIndex(const String& id) const;

	Modulator* getSourceMod(const String& id) const;

	ReferenceCountedObject* getSourceCable(const String& id) const;

	struct ScopedRefreshDeferrer
	{
		ScopedRefreshDeferrer(ScriptModulationMatrix& p) :
			parent(p)
		{
			prevValue = p.deferRefresh;
			p.deferRefresh = true;
		}

		~ScopedRefreshDeferrer()
		{
			parent.deferRefresh = prevValue;

			if (!prevValue)
			{
				parent.sendUpdateMessage("", "", ConnectionEvent::Rebuild);
				parent.refreshBypassStates();
			}
		}

		bool prevValue;
		ScriptModulationMatrix& parent;
	};

	enum class ConnectionEvent
	{
		Add,
		Delete,
		Update,
		Intensity,
		ValueMode,
		Rebuild,
		numConnectionEvents
	};

	void sendUpdateMessage(String source, String target, ConnectionEvent eventType);



	void refreshBypassStates();

	bool deferRefresh = false;

	struct SourceData;
	struct ParameterTargetCable;

	struct ModulatorTargetData;
	struct ParameterTargetData;

	struct TargetDataBase : public PooledUIUpdater::SimpleTimer
	{
		TargetDataBase(ScriptModulationMatrix* parent_, const var& json, bool isMod_);

		virtual ~TargetDataBase() {};

		virtual float getModValue() const = 0;

		virtual void init(const var& json);

		virtual MacroControlledObject::ModulationPopupData::Ptr getModulationData();

		virtual bool canConnect(const String& sourceId) const = 0;

		virtual bool connect(const String& sourceId, bool addConnection) = 0;

		virtual bool updateIntensity(const String& source, float newValue) = 0;

		virtual var getIntensitySliderData(String sourceId) const = 0;

		virtual var getValueModeData(const String& sourceId) const = 0;

		virtual bool updateValueMode(const String& sourceId, ValueMode newMode) = 0;

		virtual void clear() = 0;

		void timerCallback() override;

		virtual bool queryFunction(int index, bool checkTicked) const = 0;

		virtual bool checkActiveConnections(const String& sourceId) = 0;

		virtual void updateConnectionData(const var& obj) = 0;

		virtual var getConnectionData() const = 0;

		void verifyProperty(const var& json, const Identifier& id);

		void verifyExists(void* obj, const Identifier& id);

		
		WeakReference<Processor> processor;
		int subIndex;
		bool isMod;
		String modId;
		WeakReference<ScriptModulationMatrix> parent;

		String componentId;
		var sc;
		float lastValue = 0.0f;
		

		float componentValue = 0.0f;
		scriptnode::InvertableParameterRange r;

		JUCE_DECLARE_WEAK_REFERENCEABLE(TargetDataBase);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TargetDataBase);
	};

	struct ModulatorTargetData : public TargetDataBase
	{
		enum class TargetMode
		{
			GainMode = Modulation::GainMode,
			PitchMode = Modulation::PitchMode,
			PanMode = Modulation::PanMode,
			GlobalMode = Modulation::GlobalMode,
			FrequencyMode,
			numTargetModes			
		};

		using IteratorFunction = std::function<bool(Modulator*, ModulatorTargetData&, GlobalModulator*)>;
		

		ModulatorTargetData(ScriptModulationMatrix* parent_, const var& json):
			TargetDataBase(parent_, json, true)
		{
			init(json);
		}

		void init(const var& json) override;

		bool queryFunction(int index, bool checkTicked) const override;

		bool forEach(Modulator* sourceMod, const IteratorFunction& f) const;

		float getModValue() const override;

		bool canConnect(const String& sourceId) const override;

		bool connect(const String& sourceId, bool addConnection) override;

		void clear() override;

		var getIntensitySliderData(String sourceId) const override;

		var getValueModeData(const String& sourceId) const override;

		bool updateValueMode(const String& sourceId, ValueMode newMode) override;

		bool updateIntensity(const String& sourceId, float newValue) override;

		bool checkActiveConnections(const String& sourceId) override;

		void updateConnectionData(const var& obj) override;

		var getConnectionData() const override;

		MacroControlledObject::ModulationPopupData::Ptr getModulationData() override;

	private:

		int getTypeIndex(GlobalModulator* gm) const;

		void updateValue();
		
		bool isBipolarFreqMod(GlobalModulator* gm) const;

		String getValueModeValue(GlobalModulator* gm) const;
		double getIntensityValue(GlobalModulator* gm) const;

		WeakReference<Modulator> componentMod;

		TargetMode targetMode = TargetMode::numTargetModes;
		float defaultValue;

		

		Array<WeakReference<Modulator>> modulators[3];

		Array<ValueMode> freqValueModes[3];
		Array<WeakReference<Modulator>> freqAddMods[3];

		JUCE_DECLARE_WEAK_REFERENCEABLE(ModulatorTargetData);
	};

	struct ParameterTargetData : public TargetDataBase
	{
		using IteratorFunction = std::function<bool(ReferenceCountedObject*, ParameterTargetData&, ParameterTargetCable*)>;

		ParameterTargetData(ScriptModulationMatrix* parent_, const var& json):
			TargetDataBase(parent_, json, false)
		{
			init(json);
		}

		void init(const var& json) override;

		

		MacroControlledObject::ModulationPopupData::Ptr getModulationData() override;

		bool queryFunction(int index, bool checkTicked) const override;

		bool forEach(ReferenceCountedObject* cable, const IteratorFunction& f) const;

		float getModValue() const override;

		bool updateIntensity(const String& sourceId, float newValue) override;

		bool updateValueMode(const String& sourceId, ValueMode newMode) override;

		void clear() override;

		bool canConnect(const String& sourceId) const override;

		bool connect(const String& sourceId, bool addConnection) override;

		bool checkActiveConnections(const String& sourceId) override;

		var getIntensitySliderData(String sourceId) const override;

		var getValueModeData(const String& sourceId) const override;

		void updateConnectionData(const var& obj) override;

		var getConnectionData() const override;

	private:

		friend struct ParameterTargetCable;

		void updateValue();

		ValueMode valueMode = ValueMode::Scale;

		Array<var> parameterTargets;

		float lastParameterValue = 0.0f;
		float modValue = 1.0f;
		

		JUCE_DECLARE_WEAK_REFERENCEABLE(ParameterTargetData);
	};

	LambdaBroadcaster<String, String, ConnectionEvent> broadcaster;

	static void onUpdateMessage(ScriptModulationMatrix& m, const String& source, const String& target, ConnectionEvent eventType);

	WeakCallbackHolder connectionCallback;
	WeakCallbackHolder editCallback;

	OwnedArray<TargetDataBase> targetData;

	OwnedArray<SourceData> sourceData;

	int numSlots[3] = { 0, 0, 0 };

	struct Wrapper;

	WeakReference<GlobalModulatorContainer> container;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptModulationMatrix);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptModulationMatrix);
};

}


} // namespace hise
