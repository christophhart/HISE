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

struct SnexOscillator : public SnexSource
{
	struct OscillatorCallbacks : public SnexSource::CallbackHandlerBase
	{
		OscillatorCallbacks(SnexSource& p, ObjectStorageType& o);;

		Result recompiledOk(snex::jit::ComplexType::Ptr objectClass) override;

		Result runTest(snex::ui::WorkbenchData::CompileResult& lastResult) override
		{
			struct OscTestData
			{
				OscTestData(WorkbenchData::Ptr rootWb)
				{
					auto& td = rootWb->getTestData();

					td.testOutputData.makeCopyOf(td.testSourceData);
					d.data.referToRawData(td.testOutputData.getWritePointer(0), td.testOutputData.getNumSamples());
					d.uptime = 0.0;
					d.delta = 0.0;
				}

				OscProcessData d;
			};

			auto wb = static_cast<snex::ui::WorkbenchManager*>(getNodeWorkbench(parent.getParentNode()));

			ScopedPointer<OscTestData> td = new OscTestData(wb->getRootWorkbench());

			ScopedDeactivator sd(*this);
			
			auto f = getFunctionAsObjectCallback("process");

			if (auto realOsc = dynamic_cast<OscillatorCallbacks*>(&parent.getCallbackHandler()))
			{
				td->d.delta = realOsc->lastDelta;
			}

			f.callVoid(&td->d);

			MessageManager::callAsync([wb]()
			{
				wb->getRootWorkbench()->postPostCompile();
			});
			
			return Result::ok();
		}

		void reset() override;
		//float tick(double uptime);
		void process(OscProcessData& d);

		void prepare(PrepareSpecs ps);

		double lastDelta = 0.0;
		//FunctionData tickFunction;
		FunctionData processFunction;
		FunctionData prepareFunction;
		PrepareSpecs lastSpecs;
	};

	using OscTester = SnexSource::Tester<OscillatorCallbacks>;

	SnexOscillator();

	SnexTestBase* createTester() override { return new OscTester(*this); }

	Identifier getTypeId() const override { RETURN_STATIC_IDENTIFIER("snex_osc"); };
	String getEmptyText(const Identifier& id) const override;

	void initialise(NodeBase* n) override;
	float tick(double uptime);
	void process(OscProcessData& d);



	void prepare(PrepareSpecs ps);

	bool preprocess(String& code) final override;

	OscillatorCallbacks callbacks;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexOscillator);
};


struct NewSnexOscillatorDisplay : public ScriptnodeExtraComponent<SnexOscillator>,
								  public SnexSource::SnexSourceListener
{
	struct SnexDisplay : public ComponentWithMiddleMouseDrag,
						 public RingBufferComponentBase
	{
		SnexDisplay() = default;

		Colour getColourForAnalyserBase(int colourId) override { return Colours::transparentBlack; };

		void paint(Graphics& g) override;
		void refresh() override;;
		void resized() override { refresh(); }

		Path p;
		String errorMessage;
	};

	NewSnexOscillatorDisplay(SnexOscillator* osc, PooledUIUpdater* updater);
	~NewSnexOscillatorDisplay();

	void complexDataAdded(snex::ExternalData::DataType t, int index) override;
	void parameterChanged(int snexParameterId, double newValue) override;
	void complexDataTypeChanged() override;
	void wasCompiled(bool ok);

	void timerCallback() override;
	void resized() override;

	static Component* createExtraComponent(void* obj, PooledUIUpdater* u);

private:

	struct SnexOscPropertyObject : public SimpleRingBuffer::PropertyObject
	{
		SnexOscPropertyObject(SimpleRingBuffer::WriterBase* o) :
			PropertyObject(o),
			osc(getTypedBase<SnexOscillator>())
		{};

		RingBufferComponentBase* createComponent() override
		{
			return new SnexDisplay();
		}

		bool validateInt(const Identifier& id, int& v) const override
		{
			if (id == RingBufferIds::BufferLength)
				return SimpleRingBuffer::toFixSize<256>(v);

			if (id == RingBufferIds::NumChannels)
				return SimpleRingBuffer::toFixSize<1>(v);
            
            return true;
		}

		void transformReadBuffer(AudioSampleBuffer& b) override;

		WeakReference<SnexOscillator> osc;
	};

	SnexDisplay display;
	SnexMenuBar menuBar;
};




}
}
