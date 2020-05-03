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

namespace scriptnode
{
using namespace juce;
using namespace hise;


struct AudioFileNodeBase::WrappedDisplay: public Component,
										  public Listener
{
	WrappedDisplay(AudioFileNodeBase* parent_) :
		parent(parent_)
	{
		parent->addListener(this);
		setSize(256, 100);

	}

	void sourceChanged(ScriptingObjects::ScriptAudioFile* newReference)
	{
		addAndMakeVisible(display = new ScriptAudioFileBufferComponent(newReference));
		resized();
	}

	void resized() override
	{
		display->setBounds(getLocalBounds());
	}

	~WrappedDisplay()
	{
		parent->removeListener(this);
	}

	ScopedPointer<ScriptAudioFileBufferComponent> display;
	WeakReference<AudioFileNodeBase> parent;
};

AudioFileNodeBase::AudioFileNodeBase() :
	index(PropertyIds::SampleIndex, -1),
	internalReference(PropertyIds::File, "")
{
	currentBuffer = new ScriptingObjects::ScriptAudioFile::RefCountedBuffer();
}

void AudioFileNodeBase::contentChanged()
{
	if (audioFile == nullptr)
		return;

	{
		SpinLock::ScopedLockType sl(lock);
		currentBuffer = audioFile->getBuffer();
	}
	
	internalReference.getPropertyTree().setProperty(PropertyIds::Value, audioFile->getCurrentlyLoadedFile(), undoManager);
}

void AudioFileNodeBase::initialise(NodeBase* n)
{
	undoManager = n->getUndoManager();
	pwsc = n->getScriptProcessor();
	holder = dynamic_cast<ComplexDataHolder*>(pwsc);

	index.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(AudioFileNodeBase::updateIndex));
	index.initialise(n);

	internalReference.setAdditionalCallback(BIND_MEMBER_FUNCTION_2(AudioFileNodeBase::updateFile));
	internalReference.initialise(n);
}

void AudioFileNodeBase::updateFile(Identifier id, var newValue)
{
	auto newRef = newValue.toString();

	if (audioFile != nullptr)
		audioFile->loadFile(newRef);
	else
		jassertfalse;
}

void AudioFileNodeBase::updateIndex(Identifier id, var newValue)
{
	int newIndex = (int)newValue;

	if (newIndex == -1 && isUsingInternalReference)
		return;

	if (audioFile != nullptr)
		audioFile->removeListener(this);

	if (newIndex == -1)
		audioFile = new ScriptingObjects::ScriptAudioFile(pwsc);
	else
		audioFile = holder->addOrReturnAudioFile(newIndex);

	isUsingInternalReference = newIndex == -1;

	audioFile->addListener(this, true);

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->sourceChanged(audioFile);
	}
}

void AudioFileNodeBase::createParameters(Array<ParameterData>& )
{
}

void AudioFileNodeBase::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
	l->sourceChanged(audioFile);
}

void AudioFileNodeBase::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

}

