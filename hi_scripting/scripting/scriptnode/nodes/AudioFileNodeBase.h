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

using RefBufferPtr = ScriptingObjects::ScriptAudioFile::RefCountedBuffer::Ptr;

class AudioFileNodeBase : public HiseDspBase,
	public ScriptingObjects::ScriptAudioFile::Listener
{
public:

	struct Listener
	{
		virtual ~Listener() {};
		virtual void sourceChanged(ScriptingObjects::ScriptAudioFile* newReference) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	struct WrappedDisplay;

	AudioFileNodeBase();;

	void contentChanged() override;
	void initialise(NodeBase* n) override;

	void updateFile(Identifier id, var newValue);
	void updateIndex(Identifier id, var newValue);

	void createParameters(Array<ParameterData>& data) override;
	Component* createExtraComponent(PooledUIUpdater* updater) override;

	void addListener(Listener* l);
	void removeListener(Listener* l);

protected:

	SpinLock lock;
	RefBufferPtr currentBuffer;
	ScriptingObjects::ScriptAudioFile::Ptr audioFile;

private:

	
	
	UndoManager* undoManager = nullptr;

	bool recursiveProtection = false;

	ComplexDataHolder* holder;
	Array<WeakReference<Listener>> listeners;
	ProcessorWithScriptingContent* pwsc = nullptr;

	bool isUsingInternalReference = false;
	NodePropertyT<int> index;
	NodePropertyT<String> internalReference;

	JUCE_DECLARE_WEAK_REFERENCEABLE(AudioFileNodeBase);
};


}
