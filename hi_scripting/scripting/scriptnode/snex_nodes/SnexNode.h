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

struct snex_node : public SnexSource
{
	SN_NODE_ID("snex_node");
	SN_GET_SELF_AS_OBJECT(snex_node);
	SN_DESCRIPTION("A generic SNEX node with the complete callback set");

	static constexpr bool isPolyphonic() { return false; }
	static constexpr bool isProcessingHiseEvent() { return true; };

	SN_EMPTY_CREATE_PARAM;

	struct NodeCallbacks : public SnexSource::CallbackHandlerBase
	{
		NodeCallbacks(SnexSource& s, ObjectStorageType& o) :
			CallbackHandlerBase(s, o)
		{};

		void reset() override
		{
			SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());

			for (int i = 0; i < ScriptnodeCallbacks::numFunctions; i++)
				f[i] = {};

			ok = false;
		}

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override
		{
			FunctionData nf[(int)ScriptnodeCallbacks::numFunctions];

			auto ids = ScriptnodeCallbacks::getIds({});

			auto r = Result::ok();

			for (auto id : ids)
			{
				auto i = (int)ScriptnodeCallbacks::getCallbackId(id);
				nf[i] = getFunctionAsObjectCallback(id.toString());

				if (!nf[i].isResolved())
				{
					for (int i = 0; i < (int)ScriptnodeCallbacks::numFunctions; i++)
						f[i] = {};

					return Result::fail(id.toString() + " wasn't found");
				}
			}

			{
				SimpleReadWriteLock::ScopedWriteLock l(getAccessLock());
				
				for (int i = 0; i < (int)ScriptnodeCallbacks::numFunctions; i++)
					f[i] = nf[i];

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

#if 0
		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			auto wb = static_cast<snex::ui::WorkbenchManager*>(parent.getParentNode()->getScriptProcessor()->getMainController_()->getWorkbenchManager());

			if (auto rwb = wb->getRootWorkbench())
			{
				auto& td = rwb->getTestData();

				td.processTestData(rwb, this);

#if 0
				if (td.testSourceData.getNumSamples() > 0)
				{
					auto& testData = td.testOutputData;

					testData.makeCopyOf(td.testSourceData);

					struct Test
					{
						Test(snex::ui::WorkbenchData::TestData& td) :
							ps(td.getPrepareSpecs()),
							pd(td.testOutputData.getArrayOfWritePointers(), td.testOutputData.getNumSamples(), td.testOutputData.getNumChannels()),
							chunk(nullptr, 0, 0),
							cd(pd)
						{};

						void next()
						{
							auto c = cd.getChunk(jmin(cd.getNumLeft(), ps.blockSize));
							auto& d = c.toData();
							chunk.referTo(d.getRawDataPointers(), d.getNumChannels(), d.getNumSamples());
						}

						PrepareSpecs ps;
						ProcessDataDyn pd;
						ProcessDataDyn chunk;
						ChunkableProcessData<ProcessDataDyn> cd;
					};


					ScopedPointer<Test> t = new Test(td);
					
					prepare(t->ps);
					resetFunc();

					process(t->pd);


					WeakReference<snex::ui::WorkbenchData> safeW(rwb.get());

					auto f = [safeW]()
					{
						if (safeW.get() != nullptr)
							safeW.get()->postPostCompile();
					};

					MessageManager::callAsync(f);
				}
#endif
			}

			return Result::ok();
		}
#endif

		void resetFunc()
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::ResetFunction].callVoidUncheckedWithObject();
		}

		void process(ProcessDataDyn& data)
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::ProcessFunction].callVoidUncheckedWithObject(&data);
		}

		template <typename T> void processFrame(T& d)
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::ProcessFrameFunction].callVoidUncheckedWithObject(&d);
		}

		void prepare(PrepareSpecs ps)
		{
			lastSpecs = ps;

			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::PrepareFunction].callVoidUncheckedWithObject(&lastSpecs);
		}
		
		void handleHiseEvent(HiseEvent& e)
		{
			if (auto s = ScopedCallbackChecker(*this))
				f[(int)ScriptnodeCallbacks::HandleEventFunction].callVoidUncheckedWithObject(&e);
		}
		
		FunctionData f[(int)ScriptnodeCallbacks::numFunctions];
		
		PrepareSpecs lastSpecs;

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

	void prepare(PrepareSpecs ps)
	{
		rebuildCallbacksAfterChannelChange(ps.numChannels);
		callbacks.prepare(ps);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		callbacks.handleHiseEvent(e);
	}

	void reset()
	{
		callbacks.resetFunc();
	}

	void process(ProcessDataDyn& data)
	{
		callbacks.process(data);
	}

	template <typename T> void processFrame(T& data)
	{
		callbacks.processFrame(data);
	}

	struct editor : public ScriptnodeExtraComponent<snex_node>
	{
		editor(snex_node* n, PooledUIUpdater* updater):
			ScriptnodeExtraComponent(n, updater),
			menubar(n)
		{
			addAndMakeVisible(menubar);
			setSize(200, 24);
			stop();
		}

		void resized() override
		{
			menubar.setBounds(getLocalBounds());
		}

		void timerCallback() override {};

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			return new editor(static_cast<snex_node*>(obj), updater);
		}

		SnexMenuBar menubar;
	};

	JUCE_DECLARE_WEAK_REFERENCEABLE(snex_node);
};

}

}
