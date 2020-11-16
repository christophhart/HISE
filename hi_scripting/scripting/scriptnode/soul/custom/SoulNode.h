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

class SoulNode : public WrapperNode
{
public:

	using PatchPtr = soul::patch::PatchPlayer::Ptr;

	enum class State
	{
		Uninitialised,
		WaitingForPrepare,
		Compiling,
		CompiledOk,
		CompileError,
		numStates
	};

	struct SoulComponent : public Component
	{
		SoulComponent(SoulNode* n):
			parent(n)
		{
			setSize(200, 30);
		}

		void paint(Graphics& g) override
		{
			
			Colour c;

			switch (parent->state.load())
			{
			case State::Uninitialised: c = Colours::white; break;
			case State::CompiledOk: c = Colours::green; break;
			case State::CompileError: c = Colours::red; break;
			case State::Compiling: c = Colours::yellow; break;
			case State::WaitingForPrepare: c = Colours::orange; break;
			}

			c = c.withSaturation(0.2f).withBrightness(0.4f);

			g.setColour(c);
			g.fillAll();

			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);
			g.drawText(parent->currentFileName, getLocalBounds().toFloat(), Justification::centred);
		}

		WeakReference<SoulNode> parent;
	};

	~SoulNode()
	{
		for (int i = 0; i < getNumParameters(); i++)
			getParameter(i)->setCallback([](double) {});

		patchPlayer.forEachVoice([](soul::patch::PatchPlayer::Ptr& p)
		{
			p = nullptr;
		});

		patch = nullptr;
	}

	int getExtraHeight() const override { return 30; }
	int getExtraWidth() const override { return 200; }

	Component* createExtraComponent() override
	{
		return new SoulComponent(this);
	}

	struct DebugHandler : public soul::patch::DebugMessageHandler
	{
		DebugHandler(SoulNode& parent) :
			p(parent)
		{};

		SoulNode& p;

		void handleDebugMessage(uint64_t /*sampleCount*/, const char* /*endpointName*/, const char* message)
		{
			String s;
			s << p.getId() << " | " << message;
			p.logError(s);
		}

		void addRef() noexcept override { ++refCount; }
		void release() noexcept override { if (--refCount == 0) delete this; }
		std::atomic<int> refCount{ 0 };
	};

	soul::patch::DebugMessageHandler::Ptr debugHandler;

	static File getCacheFolder(MainController* mc);
	
	SoulNode(DspNetwork* rootNetwork, ValueTree data);

	void logMessage(const String& s);

	void logError(const String& s);

	static Identifier getStaticId() { return "soul"; };
	static NodeBase* createNode(DspNetwork* n, ValueTree d) { return new SoulNode(n, d); }

	void process(ProcessData& data)
	{
		SimpleReadWriteLock::ScopedTryReadLock sl(compileLock);

		if (sl.hasLock() && (state == State::CompiledOk) && !isBypassed())
		{
			soul::patch::PatchPlayer::RenderContext ctx;
			ctx.inputChannels = data.data;
			ctx.outputChannels = data.data;
			ctx.numOutputChannels = data.numChannels;
			ctx.numInputChannels = duplicateLeftInput ? 1 : data.numChannels;
			
			// Avoid branching like a pro...
			ctx.numInputChannels *= (int)!isInstrument;

			ctx.numFrames = data.size;
			ctx.incomingMIDI = midiBuffer;
			ctx.numMIDIMessagesIn = midiPos;

			auto result = patchPlayer.get()->render(ctx);

			if (result == soul::patch::PatchPlayer::RenderResult::noProgramLoaded)
				state = State::Uninitialised;

			if (result == soul::patch::PatchPlayer::RenderResult::wrongNumberOfChannels)
				state = State::CompileError;

			midiPos = 0;
		}
	}

	virtual void prepare(PrepareSpecs specs) override;

	bool isPolyphonic() const { return true; }

	void rebuild();

	virtual void handleHiseEvent(HiseEvent& e)
	{
		SimpleReadWriteLock::ScopedReadLock sl(compileLock);

		ignoreUnused(e);

		soul::patch::MIDIMessage m;
		
		auto mm = e.toMidiMesage();
		
		auto md = mm.getRawData();

		m.byte0 = md[0];
		m.byte1 = md[1];
		m.byte2 = md[2];
		m.frameIndex = (int)e.getTimeStamp();

		midiBuffer[midiPos] = m;
		midiPos = jmin(128, midiPos + 1);
	}

	void reset()
	{
		if (!patchPlayer.isVoiceRenderingActive())
		{
			SimpleReadWriteLock::ScopedTryReadLock sl(compileLock);

			if (sl.hasLock() && (state == State::CompiledOk))
			{
				patchPlayer.forEachVoice([](soul::patch::PatchPlayer::Ptr& p)
				{
					if(p != nullptr)
						p->reset();
				});
			}
		}
	}

	hise::SimpleReadWriteLock compileLock;

	NodePropertyT<String> codePath;

	static bool getFlagState(const soul::patch::Parameter& param, const char* flagName, bool defaultState)
	{
		if (auto flag = param.getProperty(flagName))
		{
			auto s = flag.toString<juce::String>();

			return s.equalsIgnoreCase("true")
				|| s.equalsIgnoreCase("yes")
				|| s.getIntValue() != 0;
		}

		return defaultState;
	}

	valuetree::PropertyListener codeListener;

	soul::patch::MIDIMessage midiBuffer[128];
	int midiPos = 0;
	int channelAmount = 0;
	bool duplicateLeftInput;
	bool isInstrument = false;

	std::atomic<State> state = { State::Uninitialised };

	SharedResourcePointer<SoulLibraryHolder> library;

	soul::patch::CompilerCacheFolder::Ptr compilerCache;
	soul::patch::PatchPlayerConfiguration lastConfig;
	PolyData<soul::patch::PatchPlayer::Ptr, NUM_POLYPHONIC_VOICES> patchPlayer;
	soul::patch::PatchInstance::Ptr patch;
	soul::patch::VirtualFile::Ptr currentFile;
	String currentFileName;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SoulNode);
};


}
