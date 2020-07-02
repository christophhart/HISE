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

namespace scriptnode
{
using namespace juce;

juce::File SoulNode::getCacheFolder(MainController* mc)
{
	auto cacheFolder = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Binaries).getChildFile("soul_cache");

	if (!cacheFolder.isDirectory())
		cacheFolder.createDirectory();

	return cacheFolder;
}

#if 0
NodeComponent* SoulNode::createComponent()
{
	return new SoulNodeComponent(this);
}
#endif

SoulNode::SoulNode(DspNetwork* rootNetwork, ValueTree data) :
	WrapperNode(rootNetwork, data),
	compilerCache(new soul::patch::CompilerCacheFolder(getCacheFolder(rootNetwork->getScriptProcessor()->getMainController_()), 32)),
	debugHandler(new DebugHandler(*this)),
	codePath(PropertyIds::SoulPatch, "{PROJECT_FOLDER}soul/patch.soulpatch")
{
	codePath.init(this, nullptr);
	codePath.setAdditionalCallback([this](const Identifier& id, const var& newValue)
	{
		rebuild();
	});

	rebuild();
}

void SoulNode::logMessage(const String& s)
{

}

void SoulNode::prepare(PrepareSpecs specs)
{
	patchPlayer.prepare(specs);

	lastConfig.maxFramesPerBlock = specs.blockSize;
	lastConfig.sampleRate = specs.sampleRate;
	channelAmount = specs.numChannels;

	auto p = patchPlayer.getCurrentOrFirst();

	if (state == State::WaitingForPrepare || (p != nullptr && p->needsRebuilding(lastConfig)))
	{
		auto tmp = WeakReference<SoulNode>(this);

		getScriptProcessor()->getMainController_()->getKillStateHandler().killVoicesAndCall(dynamic_cast<Processor*>(getScriptProcessor()), [tmp](Processor* )
		{
			if (tmp != nullptr)
				tmp.get()->rebuild();

			return SafeFunctionCall::OK;
		}, MainController::KillStateHandler::SampleLoadingThread);
	}
}

void SoulNode::rebuild()
{
	if (lastConfig.sampleRate == 0.0 || lastConfig.maxFramesPerBlock == 0 || channelAmount == 0)
	{
		state = State::WaitingForPrepare;
		return;
	}

	try
	{
		state = State::Compiling;

		auto desktop = File::getSpecialLocation(File::userDesktopDirectory);
		auto expectedLocation = desktop.getChildFile(soul::patch::SOULPatchLibrary::getLibraryFileName()).getFullPathName();

		library->ensureLibraryLoaded(expectedLocation);

		if (library->library == nullptr)
			throw String("Can't find Soul DLL at " + expectedLocation);

		auto mc = getScriptProcessor()->getMainController_();
		PoolReference ref(mc, codePath.getValue(), FileHandlerBase::AdditionalSourceCode);
		auto vptr = new VirtualHiseFile(mc, ref);
		
		currentFile = soul::patch::VirtualFile::Ptr(vptr);
		currentFileName = currentFile->getName().toString<String>();

		if (!vptr->isValid())
			throw String("Can't find soul patch " + currentFileName);
			
		auto newPatch = library->createInstance(currentFile);

		auto desc = newPatch->getDescription();

		if (newPatch == nullptr)
			throw String("Can't create SOUL patch from " + currentFileName);

		auto createNew = [this, newPatch](PatchPtr& p)
		{
			PerformanceCounter pc("s");

			pc.start();
			auto pl = newPatch->compileNewPlayer(lastConfig, compilerCache.get(), nullptr, nullptr, debugHandler.get());
			pc.stop();

			SimpleReadWriteLock::ScopedWriteLock sl(compileLock);
			std::swap(p, pl);
		};

		if (isPolyphonic())
			patchPlayer.forEachVoice(createNew);
		else
			createNew(patchPlayer.getFirst());

		auto newPatchPlayer = patchPlayer.getFirst();

		if (newPatchPlayer == nullptr)
			throw String("Can't create SOUL patch player from " + currentFileName);

		auto compileMessages = newPatchPlayer->getCompileMessages();

		for (auto& m : compileMessages)
		{
			if (m.isError)
				logError(m.fullMessage);
		}

		int numInputChannelsInNewPatch = 0;
		int numOutputChannelsInNewPatch = 0;

		for (auto b : newPatchPlayer->getInputBuses())
			numInputChannelsInNewPatch += b.numChannels;

		for (auto b : newPatchPlayer->getOutputBuses())
			numOutputChannelsInNewPatch += b.numChannels;

		if(channelAmount != numOutputChannelsInNewPatch)
		{
			String s;

			s << "Channel mismatch: " << numInputChannelsInNewPatch << "/" << numOutputChannelsInNewPatch << ". Expected: " << channelAmount << "/" << channelAmount;

			throw s;
		}

		if (!newPatchPlayer->isPlayable())
			throw String("Can't play soul patch " + currentFileName);

		StringArray oldParameters;
		StringArray newParameters;

		auto patchParameters = newPatchPlayer->getParameters();

		for (int i = 0; i < getNumParameters(); i++)
			oldParameters.add(getParameter(0)->getId());

		for (auto p : patchParameters)
		{
			if (getFlagState(*p, "hidden", false))
				jassertfalse;

			newParameters.add(p->name);
		}
		
		if (oldParameters != newParameters)
		{
			auto parameterWasAdded = oldParameters.size() < newParameters.size();
			auto parameterWasRemoved = oldParameters.size() > newParameters.size();

			if (parameterWasAdded)
			{
				for (auto p : patchParameters)
				{
					auto pName = p->name.toString<String>();

					if (oldParameters.contains(pName))
						continue;

					ValueTree nt(PropertyIds::Parameter);
					nt.setProperty(PropertyIds::ID, pName, nullptr);

					getParameterTree().addChild(nt, -1, getUndoManager());
					auto np = new Parameter(this, nt);
					addParameter(np);
				}
			}
			else if (parameterWasRemoved)
			{
				for (auto op : oldParameters)
				{
					if (newParameters.contains(op))
						continue;

					if (auto oldParameter = getParameter(op))
					{
						auto index = oldParameter->data.getParent().indexOf(oldParameter->data);
						removeParameter(index);
					}
				}
			}
		}
		
		for (auto p : patchParameters)
		{
			if (auto nodeParameter = getParameter(p->name.toString<String>()))
			{
				NormalisableRange<double> d;
				d.start = p->minValue;
				d.end = p->maxValue;
				d.interval = p->step;
				RangeHelpers::storeDoubleRange(nodeParameter->data, false, d, getUndoManager());

				nodeParameter->setCallback([p](double v)
				{
					p->setValue(v);
				});

				p->setValue(nodeParameter->getValue());
			}
		}
		
		{
			SimpleReadWriteLock::ScopedWriteLock sl(compileLock);

			duplicateLeftInput = numInputChannelsInNewPatch == 1 && numOutputChannelsInNewPatch == 2;
			isInstrument = numInputChannelsInNewPatch == 0 && numOutputChannelsInNewPatch != 0;
			state = State::CompiledOk;

			std::swap(newPatch, patch);
		}

		reset();
	}
	catch (String& s)
	{
		state = State::CompileError;
		logError(s);
	}
	catch (...)
	{
		state = State::CompileError;

		jassertfalse;
	}
}

#if 0
soul::patch::VirtualFile::Ptr SoulNode::getExternalFile(const char* externalVariableName)
{
	auto mc = getScriptProcessor()->getMainController_();
	PoolReference ref(mc, String(externalVariableName), FileHandlerBase::AudioFiles);
	soul::patch::VirtualFile::Ptr ptr(new VirtualHiseFile(mc, ref));
}
#endif

void SoulNode::logError(const String& s)
{
	debugError(dynamic_cast<Processor*>(getScriptProcessor()), s);
}

}


