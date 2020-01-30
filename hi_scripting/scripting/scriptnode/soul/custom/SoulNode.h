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

	struct SoulComponent : public Component
	{
		SoulComponent(SoulNode* n):
			parent(n)
		{
			setSize(200, 30);
		}

		void paint(Graphics& g) override
		{
			
			auto c = parent->ok ? Colours::green : Colours::red;
			c = c.withSaturation(0.2f).withBrightness(0.4f);

			g.setColour(c);
			g.fillAll();

			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);
			g.drawText(parent->currentFileName, getLocalBounds().toFloat(), Justification::centred);
		}

		WeakReference<SoulNode> parent;
	};

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

		void handleDebugMessage(uint64_t sampleCount, const char* endpointName, const char* message)
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

	virtual ~SoulNode() {}

	void process(ProcessData& data)
	{
		SimpleReadWriteLock::ScopedTryReadLock sl(compileLock);

		if (sl.hasLock() && ok && !isBypassed())
		{
			soul::patch::PatchPlayer::RenderContext ctx;
			ctx.inputChannels = data.data;
			ctx.outputChannels = data.data;
			ctx.numOutputChannels = data.numChannels;
			ctx.numInputChannels = data.numChannels;
			ctx.numFrames = data.size;
			ctx.incomingMIDI = midiBuffer;
			ctx.numMIDIMessagesIn = midiPos;

			patchPlayer->render(ctx);

			midiPos = 0;
		}
	}

	virtual void prepare(PrepareSpecs specs) override
	{
		lastConfig.maxFramesPerBlock = specs.blockSize;
		lastConfig.sampleRate = specs.sampleRate;
		channelAmount = specs.numChannels;

		if (patchPlayer != nullptr && patchPlayer->needsRebuilding(lastConfig))
			rebuild();
	}

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
		SimpleReadWriteLock::ScopedTryReadLock sl(compileLock);

		if (sl.hasLock() && ok)
			patchPlayer->reset();
	}

	hise::SimpleReadWriteLock compileLock;

	NodePropertyT<String> codePath;

	valuetree::PropertyListener codeListener;

	soul::patch::MIDIMessage midiBuffer[128];
	int midiPos = 0;
	int channelAmount = 0;
	std::atomic<bool> ok = { false };

	SharedResourcePointer<SoulLibraryHolder> library;

	soul::patch::CompilerCacheFolder::Ptr compilerCache;
	soul::patch::PatchPlayerConfiguration lastConfig;
	soul::patch::PatchPlayer::Ptr patchPlayer;
	soul::patch::PatchInstance::Ptr patch;
	soul::patch::VirtualFile::Ptr currentFile;
	String currentFileName;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SoulNode);
};


}
