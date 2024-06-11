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

namespace scriptnode
{
using namespace juce;
using namespace hise;
using namespace snex;

namespace core
{

/** A general purpose node with all callbacks.
    @ingroup snex_nodes
 
    This node is the most comprehensive one and offers the complete set of callbacks.
    Depending on your use case, it might be easier to use one of the more specialised callbacks.
*/
struct snex_node : public SnexSource
{
	SN_NODE_ID("snex_node");
	SN_GET_SELF_AS_OBJECT(snex_node);
	SN_DESCRIPTION("A generic SNEX node with the complete callback set");

	static constexpr bool isPolyphonic() { return false; }
	static constexpr bool isProcessingHiseEvent() { return true; };
	static constexpr bool isNormalisedModulation() { return true; };

	SN_EMPTY_CREATE_PARAM;

	struct NodeCallbacks : public SnexSource::CallbackHandlerBase
	{
		NodeCallbacks(SnexSource& s, ObjectStorageType& o) :
			CallbackHandlerBase(s, o)
		{};

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

			for (int i = 0; i < ScriptnodeCallbacks::OptionalOffset; i++)
				f[i] = {};

			ok = false;
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override
		{
			FunctionData nf[(int)ScriptnodeCallbacks::numTotalFunctions-1];

			auto ids = ScriptnodeCallbacks::getIds({});

			auto r = Result::ok();

			for (auto id : ids)
			{
				auto i = (int)ScriptnodeCallbacks::getCallbackId(id);
				nf[i] = getFunctionAsObjectCallback(id.toString());

				if (!nf[i].isResolved())
				{
					for (int i = 0; i < (int)ScriptnodeCallbacks::OptionalOffset; i++)
						f[i] = {};

					return Result::fail(id.toString() + " wasn't found");
				}
			}

			bool thisModDefined = false;
			auto modFunction = getFunctionAsObjectCallback("handleModulation", false);

			if (modFunction.isResolved())
			{
				auto sigMatch = modFunction.returnType == TypeInfo(Types::ID::Integer);
				
				if (!sigMatch)
					return Result::fail("wrong signature for " + modFunction.getSignature());

				nf[ScriptnodeCallbacks::HandleModulation - 1] = modFunction;
				thisModDefined = true;
			}

			bool thisPlotDefined = false;

			auto plotFunction = getFunctionAsObjectCallback("getPlotValue", false);

			if(plotFunction.isResolved())
			{
				auto code = parent.getWorkbench()->getCode();

				if(!code.contains("data::filter_node_base"))
				{
					return Result::fail("You need to derive from data::filter_node_base if you want to use getPlotValue()");
				}

				if(!code.contains("SNEX_INIT_FILTER"))
				{
					return Result::fail("You need to call SNEX_INIT_FILTER(externalData, index) in your setExternalData() callback if you want to use getPlotValue()");
				}

				auto sigMatch = plotFunction.returnType == TypeInfo(Types::ID::Double);

				if(!sigMatch)
					return Result::fail("wrong signature for " + plotFunction.getSignature());

				nf[ScriptnodeCallbacks::GetPlotValue - 1] = plotFunction;
				thisPlotDefined = true;
			}
			
			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
				
				for (int i = 0; i < (int)ScriptnodeCallbacks::numTotalFunctions-1; i++)
					f[i] = nf[i];

				modDefined = thisModDefined;
				plotDefined = thisPlotDefined;

				if(plotDefined && filterHandler == nullptr)
				{
					auto obj = dynamic_cast<snex_node*>(&parent);
					filterHandler = new data::filterT(obj);
					
				}

				ok = r.wasOk();
			}

			prepare(lastSpecs);

			return Result::ok();
		}

		void runHiseEventTest(HiseEvent& e) override
		{
			handleHiseEvent(e);
		}

		void runPrepareTest(PrepareSpecs ps) override
		{
			prepare(ps);
			resetFunc();
		}

		void runProcessTest(ProcessDataDyn& d) override
		{
			process(d);
		}
		
		bool runRootTest() const override { return true; };

#if SNEX_MIR_BACKEND
#define CALL_SNEX_VOID callVoidUncheckedWithObject
#else
#define CALL_SNEX_VOID callVoidUncheckedWithObject
    
#endif
        
		void resetFunc()
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::ResetFunction].CALL_SNEX_VOID();
		}

		void process(ProcessDataDyn& data)
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::ProcessFunction].CALL_SNEX_VOID(&data);
		}

		template <typename T> void processFrame(T& d)
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::ProcessFrameFunction].CALL_SNEX_VOID(&d);
		}

		void prepare(PrepareSpecs ps)
		{
			lastSpecs = ps;

			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::PrepareFunction].CALL_SNEX_VOID(&lastSpecs);
		}
		
		void handleHiseEvent(HiseEvent& e)
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::HandleEventFunction].CALL_SNEX_VOID(&e);
		}

		bool handleModulation(double& value)
		{
			if (modDefined)
			{
				if (auto s = ScopedCallbackChecker(*this))
				{
					auto v = (void*)&value;
					return f[(int)ScriptnodeCallbacks::HandleModulation - 1].callUncheckedWithObject<int>(v);
				}
			}

			return false;
		}

		data::filter_base* getFilterDataObject() const override { return filterHandler.get(); }

		double getPlotValue(bool getMagnitude, double fNorm)
		{
			jassert(plotDefined);

			if(auto s = ScopedCallbackChecker(*this))
			{
				return f[(int)ScriptnodeCallbacks::GetPlotValue - 1].callUncheckedWithObject<double>((int)getMagnitude, fNorm);
			}
			
			return 0.0;
		}

		FunctionData f[(int)ScriptnodeCallbacks::numTotalFunctions-1];
		
		bool modDefined = false;
		bool plotDefined = false;
		
		PrepareSpecs lastSpecs;

		ScopedPointer<data::filterT> filterHandler;
		

	} callbacks;

	snex_node() :
		SnexSource(),
		callbacks(*this, object)
	{
		setCallbackHandler(&callbacks);
	};

	String getEmptyText(const Identifier& id) const override;
	virtual Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("snex_node"); }

	bool preprocess(String& code) override;

	SnexTestBase* createTester() override
	{
		return new SnexSource::Tester<NodeCallbacks>(*this);
	}

    /** This function will be called whenever the processing specifications change. You can use this to setup your processing. */
	void prepare(PrepareSpecs ps)
	{
		rebuildCallbacksAfterChannelChange(ps.numChannels);
		callbacks.prepare(ps);
	}

	static double getPlotValueStatic(void* obj, bool getMagnitude, double fNorm)
	{
		auto typed = static_cast<snex_node*>(obj);

		return typed->callbacks.getPlotValue(getMagnitude, fNorm);
	}

    /** This callback will be called whenever a HiseEvent (=MIDI event on steroids) should be executed. Note that the execution of HiseEvents depends on the surrounding context. */
	void handleHiseEvent(HiseEvent& e)
	{
		callbacks.handleHiseEvent(e);
	}

    /** This callback will be called whenever the processing pipeline needs to be resetted (eg. after unbypassing an effect or starting a polyphonic voice).*/
	void reset()
	{
		callbacks.resetFunc();
	}

    /** This callback will be executed periodically in the audio thread and should contain your DSP code. */
    template <int C> void process(ProcessData<C>& d)
    {
        jassertfalse;
    }
    
	void process(ProcessDataDyn& data)
	{
		callbacks.process(data);
	}

    /** This callback will be executed if the node is inside a frame processing context. */
	template <typename FrameDataType> void processFrame(FrameDataType& data)
	{
		callbacks.processFrame(data);
	}

	bool handleModulation(double& value)
	{
		return callbacks.handleModulation(value);
	}

	struct editor : public ScriptnodeExtraComponent<snex_node>,
					public SnexSource::SnexSourceListener
	{
		editor(snex_node* n, PooledUIUpdater* updater):
			ScriptnodeExtraComponent(n, updater),
			menubar(n),
			dragger(updater)
			
		{
			n->addCompileListener(this);
			addAndMakeVisible(dragger);
			
			addAndMakeVisible(menubar);
			checkDragger();
			setSize(256, 24 + UIValues::NodeMargin + 28);
			stop();
		}

		~editor()
		{
			if(auto obj = getObject())
				obj->removeCompileListener(this);
		}

		void checkDragger()
		{
			auto showMod = getObject()->callbacks.modDefined;
			dragger.setVisible(showMod);
		}

		void resized() override
		{
			auto b = getLocalBounds();

			menubar.setBounds(b.removeFromTop(24));

			b.removeFromTop(UIValues::NodeMargin);
			
			if (dragger.isVisible())

				dragger.setBounds(b);
		}

		void timerCallback() override {};

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			return new editor(static_cast<snex_node*>(obj), updater);
		}

		void wasCompiled(bool ok) override
		{
			if (ok)
			{
				checkDragger();
				resized();
			}
		}

		ModulationSourceBaseComponent dragger;

		SnexMenuBar menubar;
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(snex_node);
};

}

}
