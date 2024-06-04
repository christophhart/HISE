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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once




namespace scriptnode {
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::Types;

namespace envelope
{

namespace pimpl
{
template <typename ParameterType> struct envelope_base: public control::pimpl::parameter_node_base<ParameterType>,
														public polyphonic_base
{
    envelope_base(const Identifier& id):
	  control::pimpl::parameter_node_base<ParameterType>(id),
	  polyphonic_base(id, true)
	{}

	virtual ~envelope_base() {};

	template <typename BaseType> void postProcess(BaseType& t, bool wasActive, double lastValue)
	{
		static_assert(std::is_base_of<envelope_base, BaseType>(), "not a base class");

		auto thisActive = t.isActive();

		if (thisActive)
		{
			float mv = (float)t.getModValue();
			FloatSanitizers::sanitizeFloatNumber(mv);
			this->getParameter().template call<0>((double)mv);
		}

		if (thisActive != wasActive)
		{
			this->getParameter().template call<1>((double)(int)thisActive);
			this->getParameter().template call<0>(0.0);
		}
	}

	virtual void initialise(NodeBase* n)
	{
		this->p.initialise(n);

		if constexpr (!ParameterType::isStaticList())
		{
			this->getParameter().numParameters.storeValue(2, nullptr);
			this->getParameter().updateParameterAmount({}, 2);
		}
	}

	static constexpr bool isProcessingHiseEvent() { return true; }

	void resetNoteCounter()
	{
		numKeys = 0;
	}

	void sendGateOffAtReset()
	{
		this->getParameter().template call<1>(0.0);
		this->getParameter().template call<0>(0.0);
	}

	bool handleKeyEvent(HiseEvent& e, bool& newValue)
	{
		if (e.isAllNotesOff())
		{
			numSustainedKeys = 0;
			numKeys = 0;
			newValue = false;
			return true;
		}

		if (e.isControllerOfType(64))
		{
			auto wasPedal = pedal;
			pedal = e.getControllerValue() > 64;

			if (!pedal && wasPedal)
			{
				numKeys = jmax(0, numKeys - numSustainedKeys);
				numSustainedKeys = 0;
				newValue = false;
				return numKeys == 0;
			}
		}

		if (e.isNoteOn())
		{
			numKeys++;
			newValue = true;
			return numKeys == 1;
		}
		if (e.isNoteOff())
		{
			if (pedal)
			{
				numSustainedKeys++;
				return false;
			}
			else
			{
				newValue = false;
				numKeys = jmax(0, numKeys - 1);
				return numKeys == 0;
			}
		}

		return false;
	}

private:

	
	bool pedal = false;
	int numKeys = 0;
	int numSustainedKeys = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(envelope_base);
};

struct ahdsr_base: public mothernode,
				   public data::display_buffer_base<true>
{
	enum Parameters
	{
		Attack,
		AttackLevel,
		Hold,
		Decay,
		Sustain,
		Release,
		AttackCurve,
		Retrigger,
		Gate,
		numParameters
	};

	struct AhdsrRingBufferProperties : public SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 2002;

		int getClassIndex() const override { return PropertyIndex; }

		AhdsrRingBufferProperties(SimpleRingBuffer::WriterBase* b) :
			PropertyObject(b),
			base(getTypedBase<ahdsr_base>())
		{}

		RingBufferComponentBase* createComponent() { return new AhdsrGraph(); }

		bool validateInt(const Identifier& id, int& v) const override;

		Path createPath(Range<int> sampleRange, Range<float> valueRange, Rectangle<float> targetBounds, double) const override;

		void transformReadBuffer(AudioSampleBuffer& b) override
		{
			jassert(b.getNumChannels() == 1);
			jassert(b.getNumSamples() == 9);

			if (base != nullptr)
				FloatVectorOperations::copy(b.getWritePointer(0), base->uiValues, 9);
		}

		WeakReference<ahdsr_base> base;
	};

	enum InternalChains
	{
		AttackTimeChain = 0,
		AttackLevelChain,
		DecayTimeChain,
		SustainLevelChain,
		ReleaseTimeChain,
		numInternalChains
	};

	ahdsr_base();

	virtual ~ahdsr_base() {};

	/** @internal The container for the envelope state. */
	struct state_base
	{
		/** The internal states that this envelope has */
		enum EnvelopeState
		{
			ATTACK, ///< attack phase (isPlaying() returns \c true)
			HOLD, ///< hold phase
			DECAY, ///< decay phase
			SUSTAIN, ///< sustain phase (isPlaying() returns \c true)
			RETRIGGER, ///< retrigger phase (monophonic only)
			RELEASE, ///< attack phase (isPlaying() returns \c true)
			IDLE ///< idle state (isPlaying() returns \c false.
		};
		 
		state_base();;

		/** Calculate the attack rate for the state. If the modulation value is 1.0f, they are simply copied from the envelope. */
		void setAttackRate(float rate);;

		/** Calculate the decay rate for the state. If the modulation value is 1.0f, they are simply copied from the envelope. */
		void setDecayRate(float rate);;

		/** Calculate the release rate for the state. If the modulation value is 1.0f, they are simply copied from the envelope. */
		void setReleaseRate(float rate);;

		float tick();

		float getUIPosition(double delta);

		void refreshAttackTime();
		void refreshDecayTime();
		void refreshReleaseTime();

		void updateAfterSampleRateChange()
		{
			refreshAttackTime();
			refreshDecayTime();
			refreshReleaseTime();
		}

		const ahdsr_base* envelope = nullptr;

		/// the uptime
		int holdCounter;
		float current_value = 0.0f;

		int leftOverSamplesFromLastBuffer = 0;

		/** The ratio in which the attack time is altered. This is calculated by the internal ModulatorChain attackChain*/
		float modValues[5];

		float attackTime;

		float attackLevel = 0.5f;
		float attackCoef = 0.0f;
		float attackBase = 1.0f;

		float decayTime;
		float decayCoef = 0.0f;
		float decayBase = 1.0f;

		float releaseTime;
		float releaseCoef = 0.0f;
		float releaseBase = 1.0f;
		float release_delta;

		float lastSustainValue;
		bool active = false;

		EnvelopeState current_state;
	};

	void calculateCoefficients(float timeInMilliSeconds, float base, float maximum, float &stateBase, float &stateCoeff) const;

	void setBaseSampleRate(double sr)
	{
		sampleRate = sr;
	}

	void setDisplayValue(int index, float value, bool convertDbValues=true)
	{
		if (convertDbValues && (index == 1 || index == 4))
		{
			value = Decibels::gainToDecibels(jlimit(0.0f, 1.0f, value));
		}
			
		if(rb != nullptr)
			rb->getUpdater().sendContentChangeMessage(sendNotificationAsync, index);

		uiValues[index] = value;
	}

	float getSampleRateForCurrentMode() const;

	void refreshUIPath(Path& p, Point<float>& position);

	void registerPropertyObject(SimpleRingBuffer::Ptr rb) override
	{
		rb->registerPropertyObject<AhdsrRingBufferProperties>();
	}

	void setAttackRate(float rate);
	void setDecayRate(float rate);
	void setReleaseRate(float rate);
	void setSustainLevel(float level);
	void setHoldTime(float holdTimeMs);
	void setTargetRatioA(float targetRatio);
	void setTargetRatioDR(float targetRatio);

	float calcCoef(float rate, float targetRatio) const;

	void setAttackCurve(float newValue);
	void setDecayCurve(float newValue);

	double sampleRate = 44100.0;
	float inputValue;
	float attack;
	float attackLevel;
	float attackCurve;
	float decayCurve;
	float hold;
	float holdTimeSamples;
	float attackBase;
	float decay;
	float decayCoef;
	float decayBase;
	float targetRatioDR;
	float sustain;
	float release;
	float releaseCoef;
	float releaseBase;
	float release_delta;

	float uiValues[9];

	bool retrigger = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ahdsr_base);
};

struct simple_ar_base : public mothernode,
					    public data::display_buffer_base<true>
{
	struct PropertyObject : public hise::SimpleRingBuffer::PropertyObject
	{
		static constexpr int PropertyIndex = 2001;
		PropertyObject(SimpleRingBuffer::WriterBase* p) :
			SimpleRingBuffer::PropertyObject(p),
			parent(getTypedBase<simple_ar_base>())
		{};

		int getClassIndex() const override { return PropertyIndex; }

		RingBufferComponentBase* createComponent() override { return nullptr; }

		bool validateInt(const Identifier& id, int& v) const override;;
		void transformReadBuffer(AudioSampleBuffer& b) override;

		WeakReference<simple_ar_base> parent;
	};

	virtual ~simple_ar_base() {};

	void registerPropertyObject(SimpleRingBuffer::Ptr rb) override
	{
		rb->registerPropertyObject<PropertyObject>();
	}

	void setDisplayValue(int index, double value);

protected:

	struct State
	{
		State() :
			env(10.0f, 10.0f)
		{};

		EnvelopeFollower::AttackRelease env;
		float targetValue = 0.0f;
		float lastValue = 0.0f;
		double linearRampValue = 0.0f;
		bool active = false;
		bool smoothing = false;
		double upRampDelta = 0.0f;
		double downRampDelta = 0.0f;
		float curve = 0.0f;
		
		float tick();

		void setAttack(double attackMs)
		{
			env.setAttackDouble(attackMs);
			recalculateLinearAttackTime();
		}

		void setRelease(double releaseMs)
		{
			env.setReleaseDouble(releaseMs);
			recalculateLinearAttackTime();
		}

		void setSampleRate(double sr)
		{
			env.setSampleRate(sr);
			recalculateLinearAttackTime();
		}

		void recalculateLinearAttackTime();

		void setAttackCurve(double newCurve)
		{
			curve = newCurve;
		}

		void setGate(bool on)
		{
			auto isOn = targetValue == 1.0;

			if (isOn != on)
			{
				targetValue = on ? 1.0f : 0.0f;
				smoothing = true;
			}
		}

		void reset()
		{
			active = false;
			smoothing = false;
			env.reset();
			targetValue = 0.0f;
			lastValue = targetValue;
			linearRampValue = 0.0f;
		}
	};

private:

	double uiValues[3];

	JUCE_DECLARE_WEAK_REFERENCEABLE(simple_ar_base);
};

}



template <int NV, typename ParameterType> struct simple_ar: public pimpl::envelope_base<ParameterType>,
															     public pimpl::simple_ar_base
{
	static constexpr int NumVoices = NV;

	enum Parameters
	{
		Attack,
		Release,
		Gate,
		AttackCurve
	};

	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Attack, simple_ar);
		DEF_PARAMETER(Release, simple_ar);
		DEF_PARAMETER(Gate, simple_ar);
		DEF_PARAMETER(AttackCurve, simple_ar);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

	static constexpr bool isPolyphonic() { return NumVoices > 1; }

	SN_NODE_ID("simple_ar");
	SN_GET_SELF_AS_OBJECT(simple_ar);
	SN_DESCRIPTION("A simple attack / release envelope");

	simple_ar(): pimpl::envelope_base<ParameterType>(getStaticId()) {}

	void setAttack(double ms)
	{
		setDisplayValue(0, ms);

		for (auto& s : states)
			s.setAttack(ms);
	}

	void setRelease(double ms)
	{
		setDisplayValue(1, ms);

		for (auto& s : states)
			s.setRelease(ms);
	}

	void setAttackCurve(double curve)
	{
		curve = jlimit(0.0, 1.0, curve);
		setDisplayValue(2, curve);

		for (auto& s : states)
			s.setAttackCurve(curve);
	}

	void prepare(PrepareSpecs ps)
	{
		states.prepare(ps);

		for (auto& s : states)
			s.setSampleRate(ps.sampleRate);

		reset();
	}

	void reset()
	{
		this->resetNoteCounter();

		for (auto& s : states)
			s.reset();

		this->sendGateOffAtReset();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if constexpr (isPolyphonic())
		{
			if (e.isNoteOnOrOff())
				setGate(e.isNoteOn() ? 1.0 : 0.0);
		}
		else
		{
			bool value;

			if (this->handleKeyEvent(e, value))
				setGate(value ? 1.0 : 0.0);
		}
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		auto& s = states.get();

		auto thisActive = s.active;
		auto& thisValue = s.lastValue;

        thisValue = s.tick();

		for (auto& v : d)
			v *= thisValue;

		this->postProcess(*this, thisActive, thisValue);
	}

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& s = states.get();

		auto thisActive = s.active;
		auto& thisValue = s.lastValue;

		if (d.getNumChannels() == 1)
		{
			for (auto& v : d[0])
            {
                thisValue = s.tick();
                v *= thisValue;
            }

		}
		else
		{
			auto fd = d.template as<ProcessData<2>>().toFrameData();

			while (fd.next())
			{
				auto thisValue = s.tick();
				for (auto& v : fd)
					v *= thisValue;
			}
		}

		this->postProcess(*this, thisActive, thisValue);
	}

	void setGate(double v)
	{
		setDisplayValue(3, v);

		auto a = v > 0.5;

		for (auto& s : states)
			s.setGate(a);
	}

	double getModValue() const
	{
		return states.get().lastValue;
	}

	bool isActive() const
	{
		return states.get().active;
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(simple_ar, Attack);
			p.setRange({ 0.0, 1000.0, 0.1 });
			p.setSkewForCentre(100.0);
			p.setDefaultValue(10.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(simple_ar, Release);
			p.setRange({ 0.0, 1000.0, 0.1 });
			p.setSkewForCentre(100.0);
			p.setDefaultValue(10.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(simple_ar, Gate);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(simple_ar, AttackCurve);
			p.setRange({ 0.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(std::move(p));
		}
	}

	
	
	PolyData<State, NumVoices> states;
};

template <int NV, typename ParameterType> struct ahdsr : public pimpl::envelope_base<ParameterType>,
														 public pimpl::ahdsr_base
{
	

	ahdsr():
		pimpl::envelope_base<ParameterType>(getStaticId())
	{
		for (auto& s : states)
		{
			s.envelope = this;

			// This makes it use the mod value...
			s.modValues[ahdsr_base::AttackTimeChain] = 0.5f;
			s.modValues[ahdsr_base::ReleaseTimeChain] = 0.5f;
			s.modValues[ahdsr_base::DecayTimeChain] = 0.5f;
		}
			
	}

	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("ahdsr");
	SN_GET_SELF_AS_OBJECT(ahdsr);
	SN_DESCRIPTION("The AHDSR envelope from HISE");

	static constexpr bool isProcessingHiseEvent() { return true; }

	void prepare(PrepareSpecs ps)
	{
		states.prepare(ps);

		setBaseSampleRate(ps.sampleRate);
		ballUpdater.limitFromBlockSizeToFrameRate(ps.sampleRate, ps.blockSize);

		for (state_base& s : states)
			s.updateAfterSampleRateChange();
	}

	void reset()
	{
		this->resetNoteCounter();

		for (state_base& s : states)
			s.current_state = pimpl::ahdsr_base::state_base::IDLE;

		if constexpr (!isPolyphonic())
			this->sendGateOffAtReset();
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if constexpr (isPolyphonic())
		{
			if (e.isNoteOnOrOff())
				setGate(e.isNoteOn() ? 1.0 : 0.0);
		}
		else
		{
			bool value;

			auto shouldRetrigger = e.isNoteOn() && retrigger;

			if (this->handleKeyEvent(e, value) || shouldRetrigger)
				setGate((value || shouldRetrigger) ? 1.0 : 0.0);
		}
	}

	template <typename T> void process(T& data)
	{
		auto& s = states.get();

		auto thisActive = s.active;
		auto thisValue = s.current_value;

		if (data.getNumChannels() == 1)
		{
			for (auto& v : data[0])
				v *= s.tick();
		}
		else
		{
			auto fd = data.template as<ProcessData<2>>().toFrameData();

			while (fd.next())
			{
				auto modValue = s.tick();
				for (auto& v : fd)
					v *= modValue;
			}
		}

		this->postProcess(*this, thisActive, thisValue);
		updateBallPosition(data.getNumSamples());
	}

	template <typename T> void processFrame(T& data)
	{
		auto& s = states.get();
		auto thisActive = s.active;
		auto thisValue = s.current_value;

		auto modValue = s.tick();

		for (auto& v : data)
			v *= modValue;

		this->postProcess(*this, thisActive, thisValue);
		updateBallPosition(1);
	}

	void setGate(double v)
	{
		setParameter<Parameters::Gate>(v);
	}

	void updateBallPosition(int numSamples)
	{
		if (ballUpdater.shouldUpdate(numSamples) && rb != nullptr)
		{
			auto& s = states.get();

			if (s.current_state != lastState)
			{
				lastTimeSamples = 0;
				lastState = s.current_state;
			}

			auto delta = 1000.0 * (double)lastTimeSamples / this->sampleRate;
			auto pos = s.getUIPosition(delta);

			rb->sendDisplayIndexMessage(pos);
		}
		
		lastTimeSamples += numSamples;
	}

	bool isActive() const
	{
		return states.get().active;
	}

	double getModValue() const
	{
		return (double)states.get().current_value;
	}

	template <int P> void setParameter(double value)
	{
		auto v = (float)value;

		jassert(std::isfinite(value));

		FloatSanitizers::sanitizeFloatNumber(v);

		setDisplayValue(P, v, true);

		if (P == Parameters::AttackCurve)
		{
			this->setAttackCurve(v);

			for (auto& s : states)
				s.refreshAttackTime();
		}
		else if (P == Parameters::Hold)
		{
			this->setHoldTime(v);
		}
		else if (P == Parameters::Retrigger)
		{
			this->retrigger = value > 0.5;
		}
		else
		{
			for (state_base& s : states)
			{
				switch (P)
				{
				case Parameters::Gate:
				{
					auto on = v > 0.5f;

					if (on)
					{
						if(s.current_state == state_base::IDLE)
							s.current_state = state_base::ATTACK;
						else
							s.current_state = state_base::RETRIGGER;
					}

					if (!on && s.current_state != pimpl::ahdsr_base::state_base::IDLE)
						s.current_state = state_base::RELEASE;

					break;
				}

				case Parameters::Attack:
					// We need to trick it to use the poly state value like this...
					s.setAttackRate(v * 2.0f);
					break;
				case Parameters::AttackLevel:
					s.attackLevel = v;
					s.refreshAttackTime();
					break;
				case Parameters::Decay:
					s.setDecayRate(v * 2.0f);
					break;
				case Parameters::Release:
					s.setReleaseRate(v * 2.0f);
					break;
				case Parameters::Sustain:
					s.modValues[3] = v;
					s.refreshReleaseTime();
					s.refreshDecayTime();
					break;
                default: break;
				}
			}
		}
	}

	SN_FORWARD_PARAMETER_TO_MEMBER(ahdsr);

	void createParameters(ParameterDataList& data)
	{
		InvertableParameterRange timeRange(0.0, 10000.0, 0.1);
		timeRange.setSkewForCentre(300.0);

		{
			DEFINE_PARAMETERDATA(ahdsr, Attack);
			p.setRange(timeRange);
			p.setDefaultValue(10.0);
			data.add(p);
		}
		
		{
			DEFINE_PARAMETERDATA(ahdsr, AttackLevel);
			p.setDefaultValue(1.0);
			data.add(p);
		}

		{
			DEFINE_PARAMETERDATA(ahdsr, Hold);
			p.setRange(timeRange);
			p.setDefaultValue(20.0);
			data.add(p);
		}

		{
			DEFINE_PARAMETERDATA(ahdsr, Decay);
			p.setRange(timeRange);
			p.setDefaultValue(300.0);
			data.add(p);
		}

		{
			DEFINE_PARAMETERDATA(ahdsr, Sustain);
			p.setDefaultValue(0.5);
			data.add(p);
		}

		{
			DEFINE_PARAMETERDATA(ahdsr, Release);
			p.setRange(timeRange);
			p.setDefaultValue(20.0);
			data.add(p);
		}

		{
			DEFINE_PARAMETERDATA(ahdsr, AttackCurve);
			p.setDefaultValue(0.5);
			data.add(p);
		}

		{
			DEFINE_PARAMETERDATA(ahdsr, Retrigger);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(p);
		}
		{
			DEFINE_PARAMETERDATA(ahdsr, Gate);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(0.0);
			data.add(p);
		}
	}

	ExecutionLimiter<DummyCriticalSection> ballUpdater;
	state_base::EnvelopeState lastState = state_base::IDLE;
	int lastTimeSamples = 0;

	PolyData<state_base, NumVoices> states;
};

struct voice_manager_base : public mothernode
{
	struct editor : public Component,
		public PooledUIUpdater::SimpleTimer,
		public hise::PathFactory
	{
		editor(PooledUIUpdater* updater, PolyHandler* ph_) :
			SimpleTimer(updater),
			ph(ph_),
			panicButton("panic", nullptr, *this)
		{
			addAndMakeVisible(panicButton);

			panicButton.setTooltip("Send a reset message for all active voices");
			panicButton.onClick = [this]()
			{
				if (auto vr = getVoiceResetter())
					vr->onVoiceReset(true, -1);
			};

			setSize(256, 32 + 10);
		};

		void timerCallback() override
		{
			auto isOk = getVoiceResetter() != nullptr;

			auto thisVoice = isOk ? getVoiceResetter()->getNumActiveVoices() : 0;

			if (lastVoiceAmount != thisVoice || isOk != ok)
			{
				ok = isOk;
				lastVoiceAmount = thisVoice;
				repaint();
			}
		}

		Path createPath(const String& id) const override;

		static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
		{
			auto t = static_cast<mothernode*>(obj);
			auto t2 = dynamic_cast<voice_manager_base*>(t);

			return new editor(updater, t2->p);
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();
			b.removeFromBottom(10);

			ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, b, true);

			auto alpha = 0.4f;

			if (isMouseOver())
				alpha += 0.1f;

			if (isMouseButtonDown())
				alpha += 0.1f;

			if (lastVoiceAmount != 0)
				alpha += 0.2f;

			g.setColour(Colours::white.withAlpha(alpha));
			g.setFont(GLOBAL_BOLD_FONT());


			String s;

			if(ok)
			{
				s << String(lastVoiceAmount) << " active voice";

				if (lastVoiceAmount != 1)
					s << "s";
			}
			else
			{
				s << "    Add a ScriptnodeVoiceKillerEnvelope.";
			}

			g.drawText(s, b, Justification::centred);
		}

		VoiceResetter* getVoiceResetter()
		{
			return ph != nullptr ? ph->getVoiceResetter() : nullptr;
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromBottom(10);
			panicButton.setBounds(b.removeFromLeft(32).reduced(4));
		}

		int lastVoiceAmount = 0;
		
		PolyHandler* ph;
		bool ok = false;

		HiseShapeButton panicButton;
	};

	virtual ~voice_manager_base() {};

	virtual void prepare(PrepareSpecs ps)
	{
		p = ps.voiceIndex;
	}

	PolyHandler* p = nullptr;
};

template <int NV> struct silent_killer: public voice_manager_base,
                                        public polyphonic_base
{
	enum Parameters
	{
		Threshold,
		Active
	};

	static constexpr int NumVoices = NV;

	SN_POLY_NODE_ID("silent_killer");
	SN_GET_SELF_AS_OBJECT(silent_killer);
	SN_DESCRIPTION("Send a voice reset message as soon when silence is detected");

	SN_EMPTY_INITIALISE;
	SN_EMPTY_MOD;
	SN_EMPTY_RESET;
	
	DEFINE_PARAMETERS
	{
		DEF_PARAMETER(Threshold, silent_killer);
		DEF_PARAMETER(Active, silent_killer);
	}
	SN_PARAMETER_MEMBER_FUNCTION;

    silent_killer():
      polyphonic_base(getStaticId(), false)
    {};
    
	void prepare(PrepareSpecs ps) override
	{
		p = ps.voiceIndex;
		state.prepare(ps);
	}

	template <typename FrameDataType> void processFrame(FrameDataType& d)
	{
		
	}

	

	template <typename ProcessDataType> void process(ProcessDataType& d)
	{
		auto& s = state.get();

		if (active && !s && d.isSilent())
		{
			p->sendVoiceResetMessage(false);
		}
	}

	void handleHiseEvent(HiseEvent& e)
	{
		if (e.isNoteOn())
			state.get() = true;
		if (e.isNoteOff())
			state.get() = false;
	}
	
	void setThreshold(double gainDb)
	{
		threshold = Decibels::decibelsToGain(gainDb);
	}

	void setActive(double a)
	{
		active = a > 0.5;
	}

	void createParameters(ParameterDataList& data)
	{
		{
			DEFINE_PARAMETERDATA(silent_killer, Active);
			p.setRange({ 0.0, 1.0, 1.0 });
			p.setDefaultValue(1.0);
			data.add(std::move(p));
		}

		{
			DEFINE_PARAMETERDATA(silent_killer, Threshold);
			p.setRange({ -120.0, -60, 1.0 });
			p.setDefaultValue(-100.0);
			data.add(std::move(p));
		}
	}

	PolyData<bool, NumVoices> state;
	bool isEmpty = false;
	bool active = false;
	double threshold;
};


struct voice_manager: public voice_manager_base
{
	SN_NODE_ID("voice_manager");
	SN_GET_SELF_AS_OBJECT(voice_manager);
	SN_DESCRIPTION("Sends a voice reset message when `Value < 0.5`");

	static constexpr bool isPolyphonic() { return false; }

	SN_EMPTY_HANDLE_EVENT;
	SN_EMPTY_MOD;
	SN_EMPTY_RESET;
	SN_EMPTY_PROCESS;
	SN_EMPTY_PROCESS_FRAME;
	SN_EMPTY_INITIALISE;

	template <int P> void setParameter(double v)
	{
		auto voiceIndex = p != nullptr ? p->getVoiceIndex() : -1;

		if (P == 0 && v < 0.5 && voiceIndex != -1)
			p->sendVoiceResetMessage(false);

		if (P == 1 && v < 0.5)
			p->sendVoiceResetMessage(true);
	}

	SN_FORWARD_PARAMETER_TO_MEMBER(voice_manager);

	void createParameters(ParameterDataList& data)
	{
		{
			parameter::data d("Kill Voice", { 0.0, 1.0, 1.0 });
			d.callback = parameter::inner<voice_manager, 0>(*this);
			d.setDefaultValue(1.0f);
			data.add(d);
		}
	}
};

};

}
