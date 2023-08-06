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

namespace hise { using namespace juce;



// MidiList =====================================================================================================================

struct ScriptingObjects::MidiList::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(MidiList, fill);
	API_VOID_METHOD_WRAPPER_0(MidiList, clear);
	API_METHOD_WRAPPER_1(MidiList, getValue);
	API_METHOD_WRAPPER_1(MidiList, getValueAmount);
	API_METHOD_WRAPPER_1(MidiList, getIndex);
	API_METHOD_WRAPPER_0(MidiList, isEmpty);
	API_METHOD_WRAPPER_0(MidiList, getNumSetValues);
	API_VOID_METHOD_WRAPPER_3(MidiList, setRange);
	API_VOID_METHOD_WRAPPER_2(MidiList, setValue);
	API_VOID_METHOD_WRAPPER_1(MidiList, restoreFromBase64String);
	API_METHOD_WRAPPER_0(MidiList, getBase64String);
};

ScriptingObjects::MidiList::MidiList(ProcessorWithScriptingContent *p) :
ConstScriptingObject(p, 0)
{
	ADD_API_METHOD_1(fill);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_1(getValue);
	ADD_API_METHOD_1(getValueAmount);
	ADD_API_METHOD_1(getIndex);
	ADD_API_METHOD_0(isEmpty);
	ADD_API_METHOD_3(setRange);
	ADD_API_METHOD_0(getNumSetValues);
	ADD_API_METHOD_2(setValue);
	ADD_API_METHOD_1(restoreFromBase64String);
	ADD_API_METHOD_0(getBase64String);

	clear();
}

void ScriptingObjects::MidiList::assign(const int index, var newValue)			 { setValue(index, (int)newValue); }
int ScriptingObjects::MidiList::getCachedIndex(const var &indexExpression) const { return (int)indexExpression; }
var ScriptingObjects::MidiList::getAssignedValue(int index) const				 { return getValue(index); }

void ScriptingObjects::MidiList::fill(int valueToFill)
{
	for (int i = 0; i < 128; i++)
		data[i] = valueToFill;

	numValues = (int)(valueToFill != -1) * 128;
}

void ScriptingObjects::MidiList::clear()
{
	fill(-1);
}

int ScriptingObjects::MidiList::getValue(int index) const
{
	if (isPositiveAndBelow(index, 128))
		return (int)data[index];

	return -1;
}

int ScriptingObjects::MidiList::getValueAmount(int valueToCheck)
{
	if (isEmpty())
		return (int)(valueToCheck == -1) * 128;

	int amount = 0;

	for (int i = 0; i < 128; i++)
		amount += (int)(data[i] == valueToCheck);

	return amount;
}

int ScriptingObjects::MidiList::getIndex(int value) const
{
	if (isEmpty()) 
		return -1;

	for (int i = 0; i < 128; i++)
	{
		if (data[i] == value)
			return i;
	}

	return -1;
}

void ScriptingObjects::MidiList::setValue(int index, int value)
{
	if (isPositiveAndBelow(index, 128))
	{
		auto isClearing = value == -1;
		auto elementIsClear = data[index] == -1;
		auto doSomething = isClearing != elementIsClear;
		numValues += (int)doSomething * ((int)elementIsClear * 2 - 1);
		data[index] = value;
	}
}

void ScriptingObjects::MidiList::setRange(int startIndex, int numToFill, int value)
{
	startIndex = jlimit(0, 127, startIndex);
	numToFill = jmin(numToFill, 127 - startIndex);

	bool isClearing = value == -1;
	int delta = 0;

	for (int i = startIndex; i < numToFill; i++)
	{
		auto elementIsClear = data[i] == -1;
		auto doSomething = isClearing != elementIsClear;
		delta += (int)doSomething * ((int)elementIsClear * 2 - 1);

		data[i] = value;
	}

	numValues += delta;
}

String ScriptingObjects::MidiList::getBase64String() const
{
	MemoryOutputStream stream;
	Base64::convertToBase64(stream, data, sizeof(int) * 128);
	return stream.toString();
}

void ScriptingObjects::MidiList::restoreFromBase64String(String base64encodedValues)
{
	MemoryOutputStream stream(data, sizeof(int) * 128);
	Base64::convertFromBase64(stream, base64encodedValues);
}

void addScriptParameters(ConstScriptingObject* this_, Processor* p)
{
	DynamicObject::Ptr scriptedParameters = new DynamicObject();

	if (ProcessorWithScriptingContent* pwsc = dynamic_cast<ProcessorWithScriptingContent*>(p))
	{
		for (int i = 0; i < pwsc->getScriptingContent()->getNumComponents(); i++)
		{
			scriptedParameters->setProperty(pwsc->getScriptingContent()->getComponent(i)->getName(), var(i));
		}
	}

	this_->addConstant("ScriptParameters", var(scriptedParameters.get()));
}

struct ScriptingObjects::ScriptFile::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptFile, getParentDirectory);
	API_METHOD_WRAPPER_1(ScriptFile, getChildFile);
	API_METHOD_WRAPPER_1(ScriptFile, createDirectory);
	API_METHOD_WRAPPER_0(ScriptFile, getSize);
	API_METHOD_WRAPPER_0(ScriptFile, getBytesFreeOnVolume);
	API_METHOD_WRAPPER_1(ScriptFile, setExecutePermission);
	API_METHOD_WRAPPER_1(ScriptFile, startAsProcess);
	API_METHOD_WRAPPER_0(ScriptFile, getHash);
	API_METHOD_WRAPPER_1(ScriptFile, toString);
	API_METHOD_WRAPPER_0(ScriptFile, loadMidiMetadata);
    API_METHOD_WRAPPER_0(ScriptFile, loadAudioMetadata);
	API_METHOD_WRAPPER_0(ScriptFile, isFile);
	API_METHOD_WRAPPER_0(ScriptFile, isDirectory);
	API_METHOD_WRAPPER_0(ScriptFile, hasWriteAccess);
	API_METHOD_WRAPPER_1(ScriptFile, writeObject);
	API_METHOD_WRAPPER_2(ScriptFile, writeAsXmlFile);
	API_METHOD_WRAPPER_0(ScriptFile, loadFromXmlFile);
	API_METHOD_WRAPPER_1(ScriptFile, writeString);
	API_METHOD_WRAPPER_2(ScriptFile, writeEncryptedObject);
	API_METHOD_WRAPPER_3(ScriptFile, writeAudioFile);
	API_METHOD_WRAPPER_0(ScriptFile, loadAsString);
	API_METHOD_WRAPPER_0(ScriptFile, loadAsObject);
	API_METHOD_WRAPPER_0(ScriptFile, loadAsAudioFile);
	API_METHOD_WRAPPER_0(ScriptFile, getNonExistentSibling);
	API_METHOD_WRAPPER_0(ScriptFile, deleteFileOrDirectory);
	API_METHOD_WRAPPER_1(ScriptFile, loadEncryptedObject);
	API_METHOD_WRAPPER_0(ScriptFile, getRedirectedFolder);
	API_METHOD_WRAPPER_1(ScriptFile, rename);
	API_METHOD_WRAPPER_1(ScriptFile, move);
	API_METHOD_WRAPPER_1(ScriptFile, copy);
	API_METHOD_WRAPPER_2(ScriptFile, isChildOf);
	API_METHOD_WRAPPER_1(ScriptFile, isSameFileAs);
	API_METHOD_WRAPPER_1(ScriptFile, toReferenceString);
	API_METHOD_WRAPPER_1(ScriptFile, getRelativePathFrom);
	API_METHOD_WRAPPER_0(ScriptFile, getNumZippedItems);
	API_VOID_METHOD_WRAPPER_2(ScriptFile, setReadOnly);
	API_VOID_METHOD_WRAPPER_3(ScriptFile, extractZipFile);
	API_VOID_METHOD_WRAPPER_0(ScriptFile, show);
	API_METHOD_WRAPPER_1(ScriptFile, loadAsMidiFile);
	API_METHOD_WRAPPER_2(ScriptFile, writeMidiFile)
};

String ScriptingObjects::ScriptFile::getFileNameFromFile(var fileOrString)
{
	if (fileOrString.isString())
		return fileOrString.toString();

	if (auto asFile = dynamic_cast<ScriptFile*>(fileOrString.getObject()))
	{
		return asFile->f.getFullPathName();
	}

	return {};
}

ScriptingObjects::ScriptFile::ScriptFile(ProcessorWithScriptingContent* p, const File& f_) :
	ConstScriptingObject(p, 4),
	f(f_)
{
	addConstant("FullPath", (int)FullPath);
	addConstant("NoExtension", (int)NoExtension);
	addConstant("Extension", (int)OnlyExtension);
	addConstant("Filename", (int)Filename);

	ADD_API_METHOD_0(getParentDirectory);
	ADD_API_METHOD_1(getChildFile);
	ADD_API_METHOD_1(createDirectory);
	ADD_API_METHOD_0(getSize);
	ADD_API_METHOD_0(getHash);
	ADD_API_METHOD_1(toString);
	ADD_API_METHOD_0(isFile);
	ADD_API_METHOD_0(getBytesFreeOnVolume);
	ADD_API_METHOD_1(setExecutePermission);
	ADD_API_METHOD_1(startAsProcess);
	ADD_API_METHOD_0(isDirectory);
	ADD_API_METHOD_0(deleteFileOrDirectory);
	ADD_API_METHOD_0(hasWriteAccess);
	ADD_API_METHOD_1(writeObject);
	ADD_API_METHOD_1(writeString);
	ADD_API_METHOD_2(writeEncryptedObject);
	ADD_API_METHOD_3(writeAudioFile);
	ADD_API_METHOD_0(loadAsString);
	ADD_API_METHOD_0(loadAsObject);
	ADD_API_METHOD_0(loadAsAudioFile);
	ADD_API_METHOD_1(loadEncryptedObject);
	ADD_API_METHOD_0(loadMidiMetadata);
    ADD_API_METHOD_0(loadAudioMetadata);
	ADD_API_METHOD_1(rename);
	ADD_API_METHOD_1(move);
	ADD_API_METHOD_1(copy);
	ADD_API_METHOD_0(show);
	ADD_API_METHOD_2(isChildOf);
	ADD_API_METHOD_1(isSameFileAs);
	ADD_API_METHOD_0(getNonExistentSibling);
	ADD_API_METHOD_3(extractZipFile);
	ADD_API_METHOD_0(getNumZippedItems);
	ADD_API_METHOD_2(setReadOnly);
	ADD_API_METHOD_1(toReferenceString);
	ADD_API_METHOD_1(getRelativePathFrom);
	ADD_API_METHOD_0(loadFromXmlFile);
	ADD_API_METHOD_2(writeAsXmlFile);
	ADD_API_METHOD_1(loadAsMidiFile);
	ADD_API_METHOD_2(writeMidiFile);
	ADD_API_METHOD_0(getRedirectedFolder);
}

var ScriptingObjects::ScriptFile::getChildFile(String childFileName)
{
	return new ScriptFile(getScriptProcessor(), f.getChildFile(childFileName));
}

var ScriptingObjects::ScriptFile::getParentDirectory()
{
	return new ScriptFile(getScriptProcessor(), f.getParentDirectory());
}

var ScriptingObjects::ScriptFile::createDirectory(String directoryName)
{
	if (!f.getChildFile(directoryName).isDirectory())
		f.getChildFile(directoryName).createDirectory();

	return new ScriptFile(getScriptProcessor(), f.getChildFile(directoryName));
}

int64 ScriptingObjects::ScriptFile::getSize()
{	
	return f.getSize();
}

int64 ScriptingObjects::ScriptFile::getBytesFreeOnVolume()
{
	return f.getBytesFreeOnVolume();
}

bool ScriptingObjects::ScriptFile::setExecutePermission(bool shouldBeExecutable)
{
	return f.setExecutePermission(shouldBeExecutable);
}

juce::var ScriptingObjects::ScriptFile::getNonExistentSibling()
{
	return var(new ScriptFile(getScriptProcessor(), f.getNonexistentSibling(false)));
}

bool ScriptingObjects::ScriptFile::startAsProcess(String parameters)
{
	return f.startAsProcess(parameters);
}

String ScriptingObjects::ScriptFile::getHash()
{
	return SHA256(f).toHexString();
};

bool ScriptingObjects::ScriptFile::hasWriteAccess()
{
	return f.hasWriteAccess();
};


String ScriptingObjects::ScriptFile::toString(int formatType) const
{
	switch (formatType)
	{
	case Format::FullPath: return f.getFullPathName();
	case Format::NoExtension: return f.getFileNameWithoutExtension();
	case Format::OnlyExtension: return f.getFileExtension();
	case Format::Filename: return f.getFileName();
	default:
		reportScriptError("Illegal formatType argument " + String(formatType));
	}

	return {};
}

String ScriptingObjects::ScriptFile::toReferenceString(String folderType)
{
	FileHandlerBase::SubDirectories dirToUse = FileHandlerBase::SubDirectories::numSubDirectories;

	if (!folderType.endsWithChar('/'))
		folderType << '/';

	for (int i = 0; i < FileHandlerBase::SubDirectories::numSubDirectories; i++)
	{
		if (FileHandlerBase::getIdentifier((FileHandlerBase::SubDirectories)i) == folderType)
		{
			dirToUse = (FileHandlerBase::SubDirectories)i;
			break;
		}
	}

	if (dirToUse != FileHandlerBase::numSubDirectories)
	{
		PoolReference ref(getScriptProcessor()->getMainController_(), f.getFullPathName(), dirToUse);
		return ref.getReferenceString();
	}

	reportScriptError("Illegal folder type");
	RETURN_IF_NO_THROW(var());
}

juce::var ScriptingObjects::ScriptFile::getRedirectedFolder()
{
	if (f.existsAsFile())
		reportScriptError("getRedirectedFolder() must be used with a directory");

	if (!f.isDirectory())
		return var(this);

	auto target = FileHandlerBase::getFolderOrRedirect(f);

	if (target == f)
		return var(this);
	else
		return var(new ScriptFile(getScriptProcessor(), target));
}

bool ScriptingObjects::ScriptFile::isFile() const
{
	return f.existsAsFile();
}

bool ScriptingObjects::ScriptFile::isChildOf(var otherFile, bool checkSubdirectories) const
{
	if (auto sf = dynamic_cast<ScriptFile*>(otherFile.getObject()))
	{
		if (checkSubdirectories)
			return f.isAChildOf(sf->f);
		else
			return f.getParentDirectory() == sf->f;
	}

	return false;
}

bool ScriptingObjects::ScriptFile::isSameFileAs(var otherFile) const
{
	if (auto sf = dynamic_cast<ScriptFile*>(otherFile.getObject()))
	{
		return sf->f == f;
	}

	return false;
}

bool ScriptingObjects::ScriptFile::isDirectory() const
{
	return f.isDirectory();
}

bool ScriptingObjects::ScriptFile::deleteFileOrDirectory()
{
	if (!f.isDirectory() && !f.existsAsFile())
		return false;

	return f.deleteRecursively(false);
}

bool ScriptingObjects::ScriptFile::writeObject(var jsonData)
{
	auto text = JSON::toString(jsonData);
	return writeString(text);
}

bool ScriptingObjects::ScriptFile::writeAsXmlFile(var jsonDataToBeXmled, String tagName)
{
	ScopedPointer<XmlElement> xml = new XmlElement(tagName);

	auto v = ValueTreeConverters::convertDynamicObjectToValueTree(jsonDataToBeXmled, Identifier(tagName));

	auto s = v.createXml()->createDocument("");

	return writeString(s);

#if 0
	if (auto obj = jsonDataToBeXmled.getDynamicObject())
	{
		for (const auto& p : obj->getProperties())
		{
			if (p.value.isString())
				xml->setAttribute(p.name, p.value.toString());
			else if (p.value.isInt() || p.value.isInt64())
				xml->setAttribute(p.name, (int)p.value);
			else
				xml->setAttribute(p.name, (double)p.value);
		}
	}

	auto content = xml->createDocument("");
	return writeString(content);
#endif
}



juce::var ScriptingObjects::ScriptFile::loadFromXmlFile()
{
	auto s = loadAsString();

	if (auto xml = XmlDocument::parse(s))
	{
		auto v = ValueTree::fromXml(*xml);
		return ValueTreeConverters::convertValueTreeToDynamicObject(v);
	}

	return var();
}

bool ScriptingObjects::ScriptFile::writeAudioFile(var audioData, double sampleRate, int bitDepth)
{
	if (f.isDirectory())
		reportScriptError("Can't write audio data to a directory target");

	AudioFormatManager afm;
	afm.registerBasicFormats();

	auto fileFormat = f.getFileExtension();

	int numChannels = 1;
	int numSamples = -1;
	int index = 0;
	bool needsBuffering = false;

	auto setIfEqual = [&](int s)
	{
		if (numSamples == -1)
			numSamples = s;
		else if (numSamples != s)
			reportScriptError("Size mismatch at channel " + String(index));

		index++;
	};

	if (audioData.isArray())
	{
		if (audioData[0].isBuffer() || audioData[0].isArray())
		{
			numChannels = audioData.size();

			for (const auto& a : *audioData.getArray())
			{
				if (a.isArray())
				{
					setIfEqual(a.size());
					needsBuffering = true;
				}
				else if (a.isBuffer())
					setIfEqual(a.getBuffer()->size);
			}
		}
		else
		{
			numSamples = audioData.size();
			needsBuffering = true;
		}
	}
	else if (audioData.isBuffer())
	{
		numSamples = audioData.getBuffer()->size;
	}
	
	if (numSamples == -1)
		reportScriptError("Incompatible data");

	if (auto m = afm.findFormatForFileExtension(fileFormat))
	{
		f.deleteFile();
		FileOutputStream* fos = new FileOutputStream(f);

		ScopedPointer<AudioFormatWriter> w = m->createWriterFor(fos, sampleRate, numChannels, bitDepth, {}, 9);
		float** channels = (float**)alloca(sizeof(float**) * numChannels);
		AudioSampleBuffer b;

		if (needsBuffering)
		{
			b = AudioSampleBuffer(numChannels, numSamples);

			if (numChannels == 1)
			{
				for (int i = 0; i < audioData.size(); i++)
				{
					auto value = (float)audioData[i];
					FloatSanitizers::sanitizeFloatNumber(value);
					b.setSample(0, i, value);
				}
			}
			else
			{
				for (int c = 0; c < audioData.size(); c++)
				{
					for (int i = 0; i < audioData.size(); i++)
					{
						auto value = (float)audioData[c][i];
						FloatSanitizers::sanitizeFloatNumber(value);
						b.setSample(c, i, value);
					}
				}
			}
		}
		else
		{
			if (audioData.isBuffer())
			{
				channels[0] = audioData.getBuffer()->buffer.getWritePointer(0);
			}
			else
			{
				for (int i = 0; i < audioData.size(); i++)
				{
					jassert(audioData[i].isBuffer());
					channels[i] = audioData[i].getBuffer()->buffer.getWritePointer(0);
				}
			}

			b = AudioSampleBuffer(channels, numChannels, numSamples);
		}

		return w->writeFromAudioSampleBuffer(b, 0, numSamples);
	}
	else
	{
		reportScriptError("Can't find audio format for file extension " + fileFormat);
		RETURN_IF_NO_THROW(var());
	}
}

bool ScriptingObjects::ScriptFile::writeMidiFile(var eventList, var metadataObject)
{
	if (eventList.isArray())
	{
		Array<HiseEvent> events;

		for (auto e : *eventList.getArray())
		{
			if (auto eh = dynamic_cast<ScriptingMessageHolder*>(e.getObject()))
			{
				events.add(eh->getMessageCopy());
			}
		}

		HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();

		HiseMidiSequence::TimeSignature t;

		if (metadataObject.getDynamicObject() != nullptr)
		{
			auto v = ValueTreeConverters::convertDynamicObjectToValueTree(metadataObject, "TimeSignature");
			t.restoreFromValueTree(v);
		}

        if(t.numBars == 0)
        {
            t.numBars = hmath::ceil((double)events.getLast().getTimeStamp() / (double)HiseMidiSequence::TicksPerQuarter);
        }
        
        newSequence->setLengthFromTimeSignature(t);
		newSequence->setTimeStampEditFormat(HiseMidiSequence::TimestampEditFormat::Ticks);
		MidiPlayer::EditAction::writeArrayToSequence(newSequence, events, 120, 44100.0);

		auto tmpFile = newSequence->writeToTempFile();

		if (f.existsAsFile())
			f.deleteFile();

		return tmpFile.moveFileTo(f);
	}

	return false;
}

juce::var ScriptingObjects::ScriptFile::loadAsMidiFile(int trackIndex)
{
	if (f.existsAsFile() && f.getFileExtension() == ".mid")
	{
		HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();

		FileInputStream fis(f);
		MidiFile mf;
		mf.readFrom(fis);
		newSequence->loadFrom(mf);

		newSequence->setTimeStampEditFormat(HiseMidiSequence::TimestampEditFormat::Ticks);
		newSequence->setCurrentTrackIndex(trackIndex);

		auto v = newSequence->getTimeSignature().exportAsValueTree();

		auto list = newSequence->getEventList(44100.0, 120.0);

		auto obj = newSequence->getTimeSignature().getAsJSON();

		Array<var> eventList;
		eventList.ensureStorageAllocated(list.size());

		for (auto e : list)
		{
			auto eh = new ScriptingMessageHolder(getScriptProcessor());
			eh->setMessage(e);
			eventList.add(var(eh));
		}

		auto returnObj = new DynamicObject();
		returnObj->setProperty("TimeSignature", obj);
		returnObj->setProperty("Events", var(eventList));

		return var(returnObj);
	}

	return var();
}

bool ScriptingObjects::ScriptFile::writeString(String text)
{
	#if JUCE_LINUX
		return f.replaceWithText(text, false, false, "\n");
	#else
		return f.replaceWithText(text);
	#endif	
}

bool ScriptingObjects::ScriptFile::writeEncryptedObject(var jsonData, String key)
{
	auto data = key.getCharPointer().getAddress();
	auto size = jlimit(0, 72, key.length());

	BlowFish bf(data, size);

	auto text = JSON::toString(jsonData, true);

	MemoryOutputStream mos;
	mos.writeString(text);
	mos.flush();
	
	auto out = mos.getMemoryBlock();

	bf.encrypt(out);

	return f.replaceWithText(out.toBase64Encoding());
}

String ScriptingObjects::ScriptFile::loadAsString() const
{
	return f.loadFileAsString();
}

var ScriptingObjects::ScriptFile::loadAsObject() const
{
	var v;

	auto r = JSON::parse(loadAsString(), v);

	if (r.wasOk())
		return v;

	reportScriptError(r.getErrorMessage());

	RETURN_IF_NO_THROW(var());
}

var ScriptingObjects::ScriptFile::loadEncryptedObject(String key)
{
	auto data = key.getCharPointer().getAddress();
	auto size = jlimit(0, 72, key.length());

	BlowFish bf(data, size);

	MemoryBlock in;
	
	in.fromBase64Encoding(f.loadFileAsString());
	bf.decrypt(in);

	var v;

	auto r = JSON::parse(in.toString(), v);

	return v;
}

bool ScriptingObjects::ScriptFile::rename(String newName)
{	
	auto newFile = f.getSiblingFile(newName).withFileExtension(f.getFileExtension());
	return f.moveFileTo(newFile);
}

bool ScriptingObjects::ScriptFile::move(var target)
{	
	if (auto sf = dynamic_cast<ScriptFile*>(target.getObject()))
		return f.moveFileTo(sf->f);
	else
		reportScriptError("target is not a file");

	RETURN_IF_NO_THROW(false);
}

bool ScriptingObjects::ScriptFile::copy(var target)
{	
	if (auto sf = dynamic_cast<ScriptFile*>(target.getObject()))
		return f.copyFileTo(sf->f);
	else
		reportScriptError("target is not a file");

	RETURN_IF_NO_THROW(false);
}

juce::var ScriptingObjects::ScriptFile::loadAsAudioFile() const
{
	double unused = 0;
	auto buffer = hlac::CompressionHelpers::loadFile(f, unused);

	if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
	{
		reportScriptError("No valid audio file");
	}

	if (buffer.getNumChannels() == 1)
	{
		auto bf = new VariantBuffer(buffer.getNumSamples());

		bf->buffer = buffer;
		return var(bf);
	}
	else
	{
		Array<var> channels;

		for (int i = 0; i < buffer.getNumChannels(); i++)
		{
			auto ptr = buffer.getReadPointer(i);
			auto bf = new VariantBuffer(buffer.getNumSamples());
			FloatVectorOperations::copy(bf->buffer.getWritePointer(0), ptr, bf->size);
			channels.add(var(bf));
		}

		return var(channels);
	}
}

juce::var ScriptingObjects::ScriptFile::loadAudioMetadata() const
{
	if (f.existsAsFile())
	{
		AudioFormatManager afm;
		afm.registerBasicFormats();

		auto fis2 = new FileInputStream(f);

		std::unique_ptr<InputStream> fis(static_cast<InputStream*>(fis2));

		if (ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(std::move(fis)))
		{
			DynamicObject::Ptr data = new DynamicObject();

			data->setProperty("SampleRate", reader->sampleRate);
			data->setProperty("NumChannels", reader->numChannels);
			data->setProperty("NumSamples", reader->lengthInSamples);
			data->setProperty("BitDepth", reader->bitsPerSample);
			data->setProperty("Format", reader->getFormatName());
			data->setProperty("File", f.getFullPathName());

			DynamicObject::Ptr meta = new DynamicObject();

			for (const auto& r : reader->metadataValues.getAllKeys())
			{
				meta->setProperty(r, reader->metadataValues[r]);
			}

			data->setProperty("Metadata", meta.get());

			return var(data.get());
		}
	}
    
    return {};
}

juce::var ScriptingObjects::ScriptFile::loadMidiMetadata() const
{
	FileInputStream fis(f);

	MidiFile mf;

	if (f.existsAsFile() && mf.readFrom(fis))
	{
		hise::HiseMidiSequence::Ptr p = new HiseMidiSequence();
		p->loadFrom(mf);

		return p->getTimeSignature().getAsJSON();
	}

	return var();
}

String ScriptingObjects::ScriptFile::getRelativePathFrom(var otherFile)
{
	if (auto sf = dynamic_cast<ScriptFile*>(otherFile.getObject()))
	{
		if (!sf->f.isDirectory())
			reportScriptError("otherFile is not a directory");

		auto rp = f.getRelativePathFrom(sf->f);
		return rp.replaceCharacter('\\', '/');
	}
	else
	{
		reportScriptError("otherFile is not a file");
	}


	return {};
}

void ScriptingObjects::ScriptFile::show()
{
	auto f_ = f;
	MessageManager::callAsync([f_]()
	{
		f_.revealToUser();
	});
}

struct PartUpdater : public Timer
{
	PartUpdater(const std::function<void()>& f_):
		f(f_)
	{
		startTimer(200);
	}

	~PartUpdater()
	{
		ScopedLock sl(lock);
		stopTimer();
	}

	std::function<void()> f;

	void timerCallback() override
	{
		ScopedLock sl(lock);
		f();
	}

	CriticalSection lock;

	bool abortFlag = false;
};

void ScriptingObjects::ScriptFile::extractZipFile(var targetDirectory, bool overwriteFiles, var callback)
{
	File tf;

	if (targetDirectory.isString() && File::isAbsolutePath(targetDirectory.toString()))
		tf = File(targetDirectory.toString());
	else if (auto sf = dynamic_cast<ScriptFile*>(targetDirectory.getObject()))
	{
		tf = sf->f;
	}

	ReferenceCountedObjectPtr<ScriptFile> safeThis(this);

	auto cb = [safeThis, tf, targetDirectory, overwriteFiles, callback](Processor* p)
	{
		if (safeThis == nullptr)
			return SafeFunctionCall::OK;

		juce::ZipFile zipFile(safeThis->f);

		DynamicObject::Ptr data = new DynamicObject();

		double entryProgress = 0.0;

		data->setProperty("Status", 0);
		data->setProperty("Progress", 0.0);
		data->setProperty("TotalBytesWritten", 0);
		data->setProperty("Cancel", false);
		data->setProperty("Target", tf.getFullPathName());
		data->setProperty("CurrentFile", "");
		data->setProperty("Error", "");
		
		int64 numBytesWritten = 0;

		WeakCallbackHolder cb(safeThis.get()->getScriptProcessor(), safeThis.get(), callback, 1);
		//cb.setThisObject(safeThis.get());
		cb.incRefCount();

		if (cb)
		{
			auto c = data->clone();
			cb.call1(var(c.get()));
		}

		data->setProperty("Status", 1);
		int numEntries = zipFile.getNumEntries();
		bool callForEachFile = numEntries < 500;
		
		int64 numBytesToWrite = 0;

		for (int i = 0; i < numEntries; i++)
		{
			numBytesToWrite += zipFile.getEntry(i)->uncompressedSize;
		}

		for (int i = 0; i < numEntries; i++)
		{
			if (Thread::getCurrentThread()->threadShouldExit())
				return SafeFunctionCall::OK;

			if (safeThis == nullptr)
				return SafeFunctionCall::OK;

			auto progress = (double)i / (double)zipFile.getNumEntries();

			safeThis.get()->getScriptProcessor()->getMainController_()->getSampleManager().getPreloadProgress() = progress;

			auto c = data->clone();

			c->setProperty("Progress", progress);
			c->setProperty("TotalBytesWritten", numBytesWritten);
			c->setProperty("CurrentFile", zipFile.getEntry(i)->filename);

			ScopedPointer<PartUpdater> partUpdater;

			auto entrySize = zipFile.getEntry(i)->uncompressedSize;

			auto updateEntryProgress = entrySize > 200 * 1024 * 1024;
			
			if (updateEntryProgress)
			{
				auto f = [&]()
				{
					auto c2 = c->clone();
					auto betterProgress = ((double)numBytesWritten + entryProgress * (double)entrySize) / (double)numBytesToWrite;

					safeThis.get()->getScriptProcessor()->getMainController_()->getSampleManager().getPreloadProgress() = betterProgress;
					c2->setProperty("Progress", betterProgress);

					if (cb)
					{
						cb.call1(var(c2.get()));
					}
				};

				partUpdater = new PartUpdater(f);
			}
			else if (callForEachFile && cb)
				cb.call1(var(c.get()));


			auto result = zipFile.uncompressEntry(i, tf, overwriteFiles, &entryProgress);

			
			partUpdater = nullptr;

			numBytesWritten += entrySize;

			if (result.failed())
			{
				c->setProperty("Error", result.getErrorMessage());

				data = c;

				if (cb)
					cb.call1(var(c.get()));

				break;
			}

			if (c->getProperty("Cancel"))
			{
				c->setProperty("Error", "User abort");

				data = c;

				if (cb)
					cb.call1(var(c.get()));

				break;
			}
		}

		if (cb)
		{
			auto c = data->clone();

			c->setProperty("Status", 2);
			c->setProperty("Progress", 1.0);
			c->setProperty("TotalBytesWritten", numBytesWritten);
			c->setProperty("CurrentFile", "");
			cb.call1(var(c.get()));
		}

		return SafeFunctionCall::OK;
	};

	auto p = dynamic_cast<Processor*>(getScriptProcessor());

	getScriptProcessor()->getMainController_()->getKillStateHandler().killVoicesAndCall(p, cb, MainController::KillStateHandler::SampleLoadingThread);
}

int ScriptingObjects::ScriptFile::getNumZippedItems()
{
	juce::ZipFile zipFile(f);
	return zipFile.getNumEntries();
}

void ScriptingObjects::ScriptFile::setReadOnly(bool shouldBeReadOnly, bool applyRecursively)
{
	f.setReadOnly(shouldBeReadOnly, applyRecursively);
}

struct ScriptingObjects::ScriptDownloadObject::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptDownloadObject, resume);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, stop);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, abort);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, isRunning);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getProgress);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getFullURL);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getStatusText);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getDownloadedTarget);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getDownloadSpeed);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getNumBytesDownloaded);
	API_METHOD_WRAPPER_0(ScriptDownloadObject, getDownloadSize);
};

ScriptingObjects::ScriptDownloadObject::ScriptDownloadObject(ProcessorWithScriptingContent* pwsc, const URL& url, const String& extraHeader_, const File& targetFile_, var callback_) :
	ConstScriptingObject(pwsc, 3),
	callback(pwsc, this, callback_, 0),
	downloadURL(url),
	targetFile(targetFile_),
	extraHeaders(extraHeader_),
	jp(dynamic_cast<JavascriptProcessor*>(pwsc))
{
	data = new DynamicObject();
	addConstant("data", var(data.get()));

	callback.incRefCount();
	callback.setThisObject(this);
	

	ADD_API_METHOD_0(resume);
	ADD_API_METHOD_0(stop);
	ADD_API_METHOD_0(abort);
	ADD_API_METHOD_0(isRunning);
	ADD_API_METHOD_0(getProgress);
	ADD_API_METHOD_0(getFullURL);
	ADD_API_METHOD_0(getStatusText);
	ADD_API_METHOD_0(getDownloadedTarget);
	ADD_API_METHOD_0(getDownloadSpeed);
	ADD_API_METHOD_0(getNumBytesDownloaded);
	ADD_API_METHOD_0(getDownloadSize);
}

ScriptingObjects::ScriptDownloadObject::~ScriptDownloadObject()
{
	flushTemporaryFile();
}

bool ScriptingObjects::ScriptDownloadObject::abort()
{
	shouldAbort.store(true);
	return stop();
}

bool ScriptingObjects::ScriptDownloadObject::stop()
{
	if (isRunning())
	{
		isWaitingForStop = true;
		return true;
	}

	return false;
}

bool ScriptingObjects::ScriptDownloadObject::stopInternal(bool forceUpdate)
{
	if (isRunning_ || forceUpdate || shouldAbort)
	{
		download = nullptr;
		flushTemporaryFile();

		isRunning_ = false;
		isFinished = false;

		if (shouldAbort)
		{
			isWaitingForStop = false;
			isFinished = true;
			data->setProperty("aborted", true);
			targetFile.deleteFile();
		}

		data->setProperty("success", false);
		data->setProperty("finished", true);
		call(true);

		return true;
	}

	return false;
}

bool ScriptingObjects::ScriptDownloadObject::resume()
{
	if (!isRunning() && !isFinished && !shouldAbort)
	{
		isWaitingForStart = true;
		return true;
	}

	return false;
}



bool ScriptingObjects::ScriptDownloadObject::resumeInternal()
{
	if (!isRunning_)
	{
		if (targetFile.existsAsFile())
		{
			existingBytesBeforeResuming = targetFile.getSize();

			int status = 0;

			auto wis = downloadURL.createInputStream(false, nullptr, nullptr, extraHeaders, 0, nullptr, &status);

			auto numTotal = wis != nullptr ? wis->getTotalLength() : 0;

			bool somethingToDownload = isPositiveAndBelow(existingBytesBeforeResuming, numTotal);

			if (existingBytesBeforeResuming == numTotal && numTotal > 0)
			{
				isFinished = true;
				isRunning_ = false;
				data->setProperty("success", true);
				data->setProperty("finished", true);
				call(true);
				return true;
			}

			if (numTotal > 0 && status == 200 && somethingToDownload)
			{
				wis = nullptr;

				resumeFile = targetFile.getNonexistentSibling(true);

				isRunning_ = true;
				isWaitingForStop = false;

				String rangeHeader;
				rangeHeader << "Range: bytes=" << existingBytesBeforeResuming << "-" << numTotal;

                URL::DownloadTaskOptions options;
                
                options.timeoutMs = HISE_SCRIPT_SERVER_TIMEOUT;
                options.extraHeaders = rangeHeader;
                options.listener = this;
                
				download = downloadURL.downloadToFile(resumeFile, options).release();

				data->setProperty("numTotal", numTotal);
				data->setProperty("numDownloaded", existingBytesBeforeResuming);
				data->setProperty("finished", false);
				data->setProperty("success", false);
			}
			else
			{
				stopInternal();
			}
		}
	}

	return true;
}


bool ScriptingObjects::ScriptDownloadObject::isRunning()
{
	return isRunning_;
}

double ScriptingObjects::ScriptDownloadObject::getProgress() const
{
	auto d = (int)data->getProperty("numDownloaded") + existingBytesBeforeResuming;
	auto t = (int)data->getProperty("numTotal") + existingBytesBeforeResuming;

	if (t != 0)
		return (double)d / double(t);
	else
		return 0.0;
}

int ScriptingObjects::ScriptDownloadObject::getDownloadSpeed()
{
	return isRunning() ? jmax((int)bytesInLastSecond, (int)bytesInCurrentSecond) : 0;
}

double ScriptingObjects::ScriptDownloadObject::getDownloadSize()
{
	return (double)(totalLength_ + existingBytesBeforeResuming);
}

double ScriptingObjects::ScriptDownloadObject::getNumBytesDownloaded()
{
	return (double)(bytesDownloaded_ + existingBytesBeforeResuming);
}

void ScriptingObjects::ScriptDownloadObject::call(bool highPriority)
{
	
	callback.call(nullptr, 0);

#if 0
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		auto type = highPriority ? JavascriptThreadPool::Task::HiPriorityCallbackExecution :
			JavascriptThreadPool::Task::Type::LowPriorityCallbackExecution;

		auto& pool = getScriptProcessor()->getMainController_()->getJavascriptThreadPool();
		Ptr strongPtr = Ptr(this);

		pool.addJob(type, jp, [strongPtr](JavascriptProcessor* p)
		{
			if (auto e = p->getScriptEngine())
			{
				auto r = Result::ok();
				var::NativeFunctionArgs args(var(strongPtr), nullptr, 0);
				e->callExternalFunction(strongPtr->callback, args, &r);
				strongPtr->callbackPending.store(false);
				return r;
			}

			return Result::fail("engine doesn't exist");
		});
	}
#endif
}


String ScriptingObjects::ScriptDownloadObject::getFullURL()
{
	return downloadURL.toString(false);
}

var ScriptingObjects::ScriptDownloadObject::getDownloadedTarget()
{
	return var(new ScriptFile(getScriptProcessor(), targetFile));
}

String ScriptingObjects::ScriptDownloadObject::getStatusText()
{
	if (isRunning_)
		return "Downloading";

	if (shouldAbort)
		return "Aborted";

	if (isFinished)
		return "Completed";

	if (isWaitingForStop)
		return "Paused";

	return "Waiting";
}

void ScriptingObjects::ScriptDownloadObject::finished(URL::DownloadTask*, bool success)
{
	data->setProperty("success", success);
	data->setProperty("finished", true);
	
	isRunning_ = false;
	isFinished = true;

	call(true);
}

void ScriptingObjects::ScriptDownloadObject::flushTemporaryFile()
{
	if (resumeFile.existsAsFile())
	{
		ScopedPointer<FileInputStream> fis = new FileInputStream(resumeFile);
		FileOutputStream fos(targetFile);

		auto numWritten = fos.writeFromInputStream(*fis, -1);

        ignoreUnused(numWritten);
		
		fos.flush();
		fis = nullptr;
		
		download = nullptr;
		auto ok = resumeFile.deleteFile();
        
        if(ok)
            resumeFile = File();
	}
}

void ScriptingObjects::ScriptDownloadObject::progress(URL::DownloadTask*, int64 bytesDownloaded, int64 totalLength)
{
	bytesDownloaded_ = bytesDownloaded;
	totalLength_ = totalLength;

	auto thisTimeMs = Time::getMillisecondCounter();

	bytesInCurrentSecond += (bytesDownloaded + existingBytesBeforeResuming - lastBytesDownloaded);
	lastBytesDownloaded = bytesDownloaded + existingBytesBeforeResuming;

	if ((thisTimeMs - lastSpeedMeasure) > 1000)
	{
		bytesInLastSecond = bytesInCurrentSecond;
		bytesInCurrentSecond = 0;
		lastSpeedMeasure = thisTimeMs;
	}

	data->setProperty("numTotal", totalLength + existingBytesBeforeResuming);
	data->setProperty("numDownloaded", bytesDownloaded + existingBytesBeforeResuming);

	if ((thisTimeMs - lastTimeMs) > 100)
	{
		call(false);
		lastTimeMs = thisTimeMs;
	}
}

void ScriptingObjects::ScriptDownloadObject::start()
{
	isWaitingForStart = false;

	if (targetFile.existsAsFile() && targetFile.getSize() > 0)
	{
		resumeInternal();
		return;
	}

	int status = 0;

	auto wis = downloadURL.createInputStream(false, nullptr, nullptr, extraHeaders, HISE_SCRIPT_SERVER_TIMEOUT, nullptr, &status);

	if (Thread::currentThreadShouldExit())
		return;

	if (status == 200)
	{
		isRunning_ = true;
        
        URL::DownloadTaskOptions options;
        options.listener = this;
        options.timeoutMs = HISE_SCRIPT_SERVER_TIMEOUT;
        
		download = downloadURL.downloadToFile(targetFile, options).release();

		data->setProperty("numTotal", 0);
		data->setProperty("numDownloaded", 0);
		data->setProperty("finished", false);
		data->setProperty("success", false);
		data->setProperty("aborted", false);

		call(true);
	}
	else
	{
		isFinished = true;

		data->setProperty("numTotal", 0);
		data->setProperty("numDownloaded", 0);
		data->setProperty("finished", true);
		data->setProperty("success", false);
		data->setProperty("aborted", false);

		call(true);
	}
}

Component* ScriptingObjects::ScriptComplexDataReferenceBase::createPopupComponent(const MouseEvent& e, Component *c)
{
	if (auto ed = dynamic_cast<Component*>(ExternalData::createEditor(complexObject)))
	{
		ed->setSize(600, 300);
		return ed;
	}
	
	return nullptr;
}

ScriptingObjects::ScriptComplexDataReferenceBase::ScriptComplexDataReferenceBase(ProcessorWithScriptingContent* c, int dataIndex, snex::ExternalData::DataType type_, ExternalDataHolder* otherHolder/*=nullptr*/) :
	ConstScriptingObject(c, 0),
	index(dataIndex),
	type(type_),
	holder(otherHolder != nullptr ? otherHolder : dynamic_cast<ExternalDataHolder*>(c)),
	displayCallback(c, this, var(), 1),
	contentCallback(c, this, var(), 1)
{
	if (holder != nullptr)
	{
		if((complexObject = holder->getComplexBaseType(getDataType(), index)))
			complexObject->getUpdater().addEventListener(this);
	}
}

ScriptingObjects::ScriptComplexDataReferenceBase::~ScriptComplexDataReferenceBase()
{
	if (complexObject != nullptr)
	{
		complexObject->getUpdater().removeEventListener(this);
	}
}

void ScriptingObjects::ScriptComplexDataReferenceBase::setPosition(double newPosition)
{
	if (complexObject != nullptr)
	{
		complexObject->getUpdater().sendDisplayChangeMessage(newPosition, sendNotificationAsync);
	}
}

float ScriptingObjects::ScriptComplexDataReferenceBase::getCurrentDisplayIndexBase() const
{
	if (complexObject != nullptr)
		return complexObject->getUpdater().getLastDisplayValue();
	
	return 0.0f;
}

void ScriptingObjects::ScriptComplexDataReferenceBase::setCallbackInternal(bool isDisplay, var f)
{
	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		auto& cb = isDisplay ? displayCallback : contentCallback;

		cb = WeakCallbackHolder(getScriptProcessor(), this, f, 1);
        cb.incRefCount();
		cb.setThisObject(this);
		cb.addAsSource(this, "onComplexDataEvent");
	}
}

struct ScriptingObjects::ScriptAudioFile::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, loadFile);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getContent);
	API_VOID_METHOD_WRAPPER_0(ScriptAudioFile, update);
	API_VOID_METHOD_WRAPPER_2(ScriptAudioFile, setRange);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getNumSamples);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getSampleRate);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getCurrentlyLoadedFile);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getCurrentlyDisplayedIndex);
	API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, setDisplayCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, setContentCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, linkTo);
};

ScriptingObjects::ScriptAudioFile::ScriptAudioFile(ProcessorWithScriptingContent* pwsc, int index_, ExternalDataHolder* otherHolder) :
	ScriptComplexDataReferenceBase(pwsc, index_, snex::ExternalData::DataType::AudioFile, otherHolder)
{
	ADD_API_METHOD_2(setRange);
	ADD_API_METHOD_1(loadFile);
	ADD_API_METHOD_0(getContent);
	ADD_API_METHOD_0(update);
	ADD_API_METHOD_0(getNumSamples);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_0(getCurrentlyLoadedFile);
	ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
	ADD_API_METHOD_1(setDisplayCallback);
	ADD_API_METHOD_1(setContentCallback);
  ADD_API_METHOD_1(linkTo);
}

void ScriptingObjects::ScriptAudioFile::clear()
{
	if (auto buffer = getBuffer())
		buffer->fromBase64String({});
}

void ScriptingObjects::ScriptAudioFile::setRange(int min, int max)
{
	if (auto buffer = getBuffer())
	{
		int numChannels = buffer->getBuffer().getNumChannels();

		if (numChannels == 0)
		{
			clear();
			return;
		}

		min = jmax(0, min);
		max = jmin(buffer->getBuffer().getNumSamples(), max);

		int size = max - min;

		if (size == 0)
		{
			clear();
			return;
		}

		buffer->setRange({ min, max });
	}
}

void ScriptingObjects::ScriptAudioFile::loadFile(const String& filePath)
{
	if (auto buffer = getBuffer())
		buffer->fromBase64String(filePath);
}

var ScriptingObjects::ScriptAudioFile::getContent()
{
	Array<var> channels;

	if (auto buffer = getBuffer())
	{
		for (int i = 0; i < buffer->getBuffer().getNumChannels(); i++)
			channels.add(buffer->getChannelBuffer(i, false));
	}

	return channels;
}

float ScriptingObjects::ScriptAudioFile::getCurrentlyDisplayedIndex() const
{
	return getCurrentDisplayIndexBase();
}

void ScriptingObjects::ScriptAudioFile::update()
{
	if (auto buffer = getBuffer())
	{
		buffer->getUpdater().sendContentChangeMessage(sendNotificationAsync, -1);
	}
}

int ScriptingObjects::ScriptAudioFile::getNumSamples() const
{
	if (auto buffer = getBuffer())
	{
		return buffer->getBuffer().getNumSamples();
	}

	return 0;
}

double ScriptingObjects::ScriptAudioFile::getSampleRate() const
{
	if (auto buffer = getBuffer())
		return buffer->sampleRate;

	return 0.0;
}

juce::String ScriptingObjects::ScriptAudioFile::getCurrentlyLoadedFile() const
{
	if (auto buffer = getBuffer())
	{
		return buffer->toBase64String();
	}

	return {};
}

void ScriptingObjects::ScriptAudioFile::setDisplayCallback(var displayFunction)
{
	setCallbackInternal(true, displayFunction);
}

void ScriptingObjects::ScriptAudioFile::setContentCallback(var contentFunction)
{
	setCallbackInternal(false, contentFunction);
}

struct ScriptingObjects::ScriptRingBuffer::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptRingBuffer, getReadBuffer);
	API_METHOD_WRAPPER_3(ScriptRingBuffer, createPath);
	API_METHOD_WRAPPER_2(ScriptRingBuffer, getResizedBuffer);
    API_VOID_METHOD_WRAPPER_1(ScriptRingBuffer, setActive);
	API_VOID_METHOD_WRAPPER_1(ScriptRingBuffer, setRingBufferProperties);
	API_VOID_METHOD_WRAPPER_1(ScriptRingBuffer, copyReadBuffer);
};

ScriptingObjects::ScriptRingBuffer::ScriptRingBuffer(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* other/*=nullptr*/):
	ScriptComplexDataReferenceBase(pwsc, index, snex::ExternalData::DataType::DisplayBuffer, other)
{
	ADD_API_METHOD_0(getReadBuffer);
	ADD_API_METHOD_3(createPath);
	ADD_API_METHOD_2(getResizedBuffer);
	ADD_API_METHOD_1(setRingBufferProperties);
	ADD_API_METHOD_1(copyReadBuffer);
    ADD_API_METHOD_1(setActive);
}

var ScriptingObjects::ScriptRingBuffer::getReadBuffer()
{
	auto& rb = getRingBuffer()->getReadBuffer();
	return  var(new VariantBuffer(const_cast<float*>(rb.getArrayOfReadPointers()[0]), rb.getNumSamples()));
}


var ScriptingObjects::ScriptRingBuffer::getResizedBuffer(int numDestSamples, int resampleMode)
{
	if (numDestSamples > 0)
	{
		auto& rb = getRingBuffer()->getReadBuffer();

		if (rb.getNumSamples() == numDestSamples)
			return getReadBuffer();

		VariantBuffer::Ptr b = new VariantBuffer(numDestSamples);

		float stride = (float)rb.getNumSamples() / (float)numDestSamples;

		int dstIndex = 0;

		if (stride < 2.0)
		{
			for (float i = 0.0f; i < (float)rb.getNumSamples(); i += stride)
			{
				auto idx = (int)i;
				auto c = rb.getSample(0, idx);
				b->setSample(dstIndex++, c);
			}
		}
		else
		{
			for (float i = 0.0f; i < (float)rb.getNumSamples(); i += stride)
			{
				auto idx = (int)i;
				auto numThisTime = jmin(rb.getNumSamples() - idx, roundToInt(stride));
				auto v = FloatVectorOperations::findMinAndMax(rb.getReadPointer(0, idx), numThisTime);

				auto c = v.getStart() + v.getLength() * 0.5f;
				b->setSample(dstIndex++, c);
			}
		}

		

		return var(b.get());
	}
	else
		return var(new VariantBuffer(0));
}

var ScriptingObjects::ScriptRingBuffer::createPath(var dstArea, var sourceRange, var startValue)
{
	auto r = Result::ok();

	auto dst = ApiHelpers::getRectangleFromVar(dstArea, &r);

	if (!r.wasOk())
		reportScriptError(r.getErrorMessage());

	auto src = ApiHelpers::getRectangleFromVar(sourceRange, &r);

	if (!r.wasOk())
		reportScriptError(r.getErrorMessage());
	
	auto hToUse = (int)src.getHeight();

	auto sp = new PathObject(getScriptProcessor());

	if (SimpleRingBuffer::Ptr buffer = getRingBuffer())
	{
		auto maxSize = hToUse = getRingBuffer()->getReadBuffer().getNumSamples();

		if (hToUse == -1)
			hToUse = maxSize;

		Range<int> s_range(jmax<int>(0, (int)src.getWidth()), jmin<int>(maxSize, hToUse));
		Range<float> valueRange(jmax<float>(-1.0f, src.getX()), jmin<float>(1.0f, src.getY()));

		SimpleReadWriteLock::ScopedReadLock sl(buffer->getDataLock());

		sp->getPath() = getRingBuffer()->getPropertyObject()->createPath(s_range, valueRange, dst, startValue);
	}

	return var(sp);
}

void ScriptingObjects::ScriptRingBuffer::setActive(bool shouldBeActive)
{
    if (auto obj = getRingBuffer())
    {
        obj->setActive(shouldBeActive);
    }
}

void ScriptingObjects::ScriptRingBuffer::setRingBufferProperties(var propertyData)
{
	if (auto obj = getRingBuffer()->getPropertyObject())
	{
		if (auto dyn = propertyData.getDynamicObject())
		{
			for (auto& nv : dyn->getProperties())
				obj->setProperty(nv.name, nv.value);
		}
	}
}

void ScriptingObjects::ScriptRingBuffer::copyReadBuffer(var targetBuffer)
{
	if (auto obj = getRingBuffer())
	{
		SimpleReadWriteLock::ScopedReadLock sl(obj->getDataLock());

		if (auto tb = targetBuffer.getBuffer())
		{
			auto dst = tb->buffer.getWritePointer(0);
			auto numSamples = tb->size;

			auto& rb = obj->getReadBuffer();

			if (rb.getNumSamples() != numSamples)
			{
				reportScriptError("size mismatch (" + String(numSamples) + "). Expected: " + String(rb.getNumSamples()));
			}
			else
			{
				ScopedLock sl2(obj->getReadBufferLock());
				FloatVectorOperations::copy(dst, rb.getReadPointer(0), numSamples);
			}
		}
        else if (targetBuffer.isArray())
        {
            int numChannels = targetBuffer.size();
            
            auto& rb = obj->getReadBuffer();
            
            if(numChannels != rb.getNumChannels())
                reportScriptError("Illegal channel amount: " + String(numChannels) + ". Expected: " + String(rb.getNumChannels()));
            else
            {
                for(int i = 0; i < numChannels; i++)
                {
                    if(auto tb = targetBuffer[i].getBuffer())
                    {
                        auto dst = tb->buffer.getWritePointer(0);
                        auto numSamples = tb->size;

                        auto& rb = obj->getReadBuffer();

                        if (rb.getNumSamples() != numSamples)
                        {
                            reportScriptError("size mismatch (" + String(numSamples) + "). Expected: " + String(rb.getNumSamples()));
                        }
                        else
                        {
                            ScopedLock sl2(obj->getReadBufferLock());
                            FloatVectorOperations::copy(dst, rb.getReadPointer(i), numSamples);
                        }
                    }
                    else
                    {
                        reportScriptError("Channel " + String(i+1) + " is not a buffer");
                    }
                    
                }
            }
            
        }
	}
	else
		reportScriptError("You need to pass in a Buffer object");
}

struct ScriptingObjects::ScriptTableData::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(ScriptTableData, reset);
	API_VOID_METHOD_WRAPPER_4(ScriptTableData, setTablePoint);
	API_VOID_METHOD_WRAPPER_2(ScriptTableData, addTablePoint);
	API_METHOD_WRAPPER_1(ScriptTableData, getTableValueNormalised);
	API_METHOD_WRAPPER_0(ScriptTableData, getCurrentlyDisplayedIndex);
	API_VOID_METHOD_WRAPPER_1(ScriptTableData, setDisplayCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptTableData, setContentCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptTableData, setTablePointsFromArray);
	API_METHOD_WRAPPER_0(ScriptTableData, getTablePointsAsArray);
    API_VOID_METHOD_WRAPPER_1(ScriptTableData, linkTo);
};

ScriptingObjects::ScriptTableData::ScriptTableData(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* otherHolder):
	ScriptComplexDataReferenceBase(pwsc, index, snex::ExternalData::DataType::Table, otherHolder)
{
	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(addTablePoint);
	ADD_API_METHOD_4(setTablePoint);
	ADD_API_METHOD_1(getTableValueNormalised);
	ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
	ADD_API_METHOD_1(setDisplayCallback);
	ADD_API_METHOD_1(setContentCallback);
	ADD_API_METHOD_1(setTablePointsFromArray);
	ADD_API_METHOD_0(getTablePointsAsArray);
    ADD_API_METHOD_1(linkTo);
}

Component* ScriptingObjects::ScriptTableData::createPopupComponent(const MouseEvent& e, Component *c)
{
#if USE_BACKEND
	auto te = dynamic_cast<Component*>(snex::ExternalData::createEditor(getTable()));
	te->setSize(300, 200);
	return te;
#else
	ignoreUnused(e, c);
	return nullptr;
#endif
}

void ScriptingObjects::ScriptTableData::setTablePoint(int pointIndex, float x, float y, float curve)
{
	if(auto table = getTable())
		table->setTablePoint(pointIndex, x, y, curve);
}

void ScriptingObjects::ScriptTableData::addTablePoint(float x, float y)
{
	if (auto table = getTable())
		table->addTablePoint(x, y);
}

void ScriptingObjects::ScriptTableData::reset()
{
	if (auto table = getTable())
		table->reset();
}

float ScriptingObjects::ScriptTableData::getTableValueNormalised(double normalisedInput)
{
	if (auto st = dynamic_cast<SampleLookupTable*>(getTable()))
	{
		return st->getInterpolatedValue(normalisedInput, sendNotificationAsync);
	}
	
	return 0.0f;
}


float ScriptingObjects::ScriptTableData::getCurrentlyDisplayedIndex() const
{
	return getCurrentDisplayIndexBase();
}

void ScriptingObjects::ScriptTableData::setDisplayCallback(var displayFunction)
{
	setCallbackInternal(true, displayFunction);
}

void ScriptingObjects::ScriptTableData::setContentCallback(var contentFunction)
{
	setCallbackInternal(false, contentFunction);
}

var ScriptingObjects::ScriptTableData::getTablePointsAsArray()
{
	if (auto table = getTable())
	{
        return table->getTablePointsAsVarArray();
	}

	return var();
}

void ScriptingObjects::ScriptTableData::setTablePointsFromArray(var pointList)
{
	if (auto a = pointList.getArray())
	{
		Array<Table::GraphPoint> ngp;
		ngp.ensureStorageAllocated(a->size());

		for (const auto& p : *a)
		{
			if (auto gpa = p.getArray())
			{
				if (gpa->size() != 3)
					reportScriptError("Illegal table point array (must be 3 elements)");

				auto x = jlimit<float>(0.0f, 1.0f, (float)(*gpa)[0]);
				auto y = jlimit<float>(0.0f, 1.0f, (float)(*gpa)[1]);
				auto curve = jlimit<float>(0.0f, 1.0f, (float)(*gpa)[2]);

				ngp.add(Table::GraphPoint(x, y, curve));
			}
		}

		if (ngp.size() >= 2)
		{
			ngp.getReference(0).x = 0.0f;
			ngp.getReference(ngp.size() - 1).x = 1.0f;

			getTable()->setGraphPoints(ngp, a->size(), true);
		}
		else
			reportScriptError("You need at least 2 table points");
	}
}

struct ScriptingObjects::ScriptSliderPackData::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptSliderPackData, setValue);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, setNumSliders);
	API_METHOD_WRAPPER_1(ScriptSliderPackData, getValue);
	API_METHOD_WRAPPER_0(ScriptSliderPackData, getNumSliders);
	API_VOID_METHOD_WRAPPER_3(ScriptSliderPackData, setRange);
	API_METHOD_WRAPPER_0(ScriptSliderPackData, getCurrentlyDisplayedIndex);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, setDisplayCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, setContentCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, setUsePreallocatedLength);
    API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, linkTo);
};

ScriptingObjects::ScriptSliderPackData::ScriptSliderPackData(ProcessorWithScriptingContent* pwsc, int dataIndex, ExternalDataHolder* otherHolder) :
	ScriptComplexDataReferenceBase(pwsc, dataIndex, snex::ExternalData::DataType::SliderPack, otherHolder)
{
	ADD_API_METHOD_2(setValue);
	ADD_API_METHOD_1(setNumSliders);
	ADD_API_METHOD_1(getValue);
	ADD_API_METHOD_0(getNumSliders);
	ADD_API_METHOD_3(setRange);
	ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
	ADD_API_METHOD_1(setDisplayCallback);
	ADD_API_METHOD_1(setContentCallback);
    ADD_API_METHOD_1(setUsePreallocatedLength);
    ADD_API_METHOD_1(linkTo);
}

var ScriptingObjects::ScriptSliderPackData::getStepSize() const
{
	if(auto data = getSliderPackData())
		return data->getStepSize();

	return 0.0;
}

void ScriptingObjects::ScriptSliderPackData::setNumSliders(var numSliders)
{
	if (auto data = getSliderPackData())
		data->setNumSliders(numSliders);
}

int ScriptingObjects::ScriptSliderPackData::getNumSliders() const
{
	if (auto data = getSliderPackData())
		return data->getNumSliders();
	
	return 0;
}

void ScriptingObjects::ScriptSliderPackData::setUsePreallocatedLength(int numUsed)
{
    if(auto data = getSliderPackData())
        data->setUsePreallocatedLength(32);
}

void ScriptingObjects::ScriptSliderPackData::setValue(int sliderIndex, float value)
{
	if(auto data = getSliderPackData())
		data->setValue(sliderIndex, value, sendNotification);
}

float ScriptingObjects::ScriptSliderPackData::getValue(int index) const
{
	if(auto data = getSliderPackData())
		return data->getValue((int)index);

	return 0.0f;
}

void ScriptingObjects::ScriptSliderPackData::setRange(double minValue, double maxValue, double stepSize)
{
	if(auto data = getSliderPackData())
		return data->setRange(minValue, maxValue, stepSize);
}

float ScriptingObjects::ScriptSliderPackData::getCurrentlyDisplayedIndex() const
{
	return getCurrentDisplayIndexBase();
}

void ScriptingObjects::ScriptSliderPackData::setDisplayCallback(var displayFunction)
{
	setCallbackInternal(true, displayFunction);
}

void ScriptingObjects::ScriptSliderPackData::setContentCallback(var contentFunction)
{
	setCallbackInternal(false, contentFunction);
}

struct ScriptingObjects::ScriptingSamplerSound::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptingSamplerSound, setFromJSON);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, get);
	API_VOID_METHOD_WRAPPER_2(ScriptingSamplerSound, set);
	API_VOID_METHOD_WRAPPER_0(ScriptingSamplerSound, deleteSample);
	API_METHOD_WRAPPER_0(ScriptingSamplerSound, duplicateSample);
	API_METHOD_WRAPPER_0(ScriptingSamplerSound, loadIntoBufferArray);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, replaceAudioFile);
	API_METHOD_WRAPPER_0(ScriptingSamplerSound, getSampleRate);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, getRange);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, refersToSameSample);
	API_METHOD_WRAPPER_0(ScriptingSamplerSound, getCustomProperties);
};

ScriptingObjects::ScriptingSamplerSound::ScriptingSamplerSound(ProcessorWithScriptingContent* p, ModulatorSampler* sampler_, ModulatorSamplerSound::Ptr sound_) :
	ConstScriptingObject(p, ModulatorSamplerSound::numProperties),
	sound(sound_),
	sampler(sampler_)
{
	ADD_API_METHOD_1(setFromJSON);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_2(set);
	ADD_API_METHOD_1(getRange);
	ADD_API_METHOD_0(deleteSample);
	ADD_API_METHOD_0(duplicateSample);
	ADD_API_METHOD_0(loadIntoBufferArray);
	ADD_API_METHOD_1(replaceAudioFile);
	ADD_API_METHOD_1(refersToSameSample);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_0(getCustomProperties);

	sampleIds.ensureStorageAllocated(ModulatorSamplerSound::numProperties);
	sampleIds.add(SampleIds::ID);
	sampleIds.add(SampleIds::FileName);
	sampleIds.add(SampleIds::Root);
	sampleIds.add(SampleIds::HiKey);
	sampleIds.add(SampleIds::LoKey);
	sampleIds.add(SampleIds::LoVel);
	sampleIds.add(SampleIds::HiVel);
	sampleIds.add(SampleIds::RRGroup);
	sampleIds.add(SampleIds::Volume);
	sampleIds.add(SampleIds::Pan);
	sampleIds.add(SampleIds::Normalized);
	sampleIds.add(SampleIds::Pitch);
	sampleIds.add(SampleIds::SampleStart);
	sampleIds.add(SampleIds::SampleEnd);
	sampleIds.add(SampleIds::SampleStartMod);
	sampleIds.add(SampleIds::LoopStart);
	sampleIds.add(SampleIds::LoopEnd);
	sampleIds.add(SampleIds::LoopXFade);
	sampleIds.add(SampleIds::LoopEnabled);
	sampleIds.add(SampleIds::LowerVelocityXFade);
	sampleIds.add(SampleIds::UpperVelocityXFade);
	sampleIds.add(SampleIds::SampleState);
	sampleIds.add(SampleIds::Reversed);
	

	for (int i = 1; i < sampleIds.size(); i++)
		addConstant(sampleIds[i].toString(), (int)i);
}

juce::String ScriptingObjects::ScriptingSamplerSound::getDebugValue() const
{
	return sound != nullptr ? sound->getPropertyAsString(SampleIds::FileName) : "";
}

hise::DebugInformation* ScriptingObjects::ScriptingSamplerSound::getChildElement(int index)
{
	std::function<var()> av;

	Identifier id;

	if (isPositiveAndBelow(index, sampleIds.size()))
	{
		id = sampleIds[index];
		ModulatorSamplerSound::Ptr other = sound;

		av = [other, id]()
		{
			if (other != nullptr)
				return other->getSampleProperty(id);

			return var();
		};
	}
	else
	{
		id = Identifier("CustomProperties");

		var obj(customObject);

		av = [obj]()
		{
			return obj;
		};
	}
		
	String cid = "%PARENT%.";
	cid << id;

	return new LambdaValueInformation(av, Identifier(cid), {}, (DebugInformation::Type)getTypeNumber(), getLocation());
}

void ScriptingObjects::ScriptingSamplerSound::assign(const int index, var newValue)
{
	set(index, newValue);
}

var ScriptingObjects::ScriptingSamplerSound::getAssignedValue(int index) const
{
	return get(index);
}

int ScriptingObjects::ScriptingSamplerSound::getCachedIndex(const var &indexExpression) const
{
	if (indexExpression.isString())
	{
		Identifier thisId(indexExpression.toString());

		auto idx = sampleIds.indexOf(thisId);

		if (idx == -1)
			reportScriptError("Can't find property " + thisId.toString());

		return idx;
	}

	return (int)indexExpression;
}

void ScriptingObjects::ScriptingSamplerSound::set(int propertyIndex, var newValue)
{
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_VOID_IF_NO_THROW();
	}

	sound->setSampleProperty(sampleIds[propertyIndex], newValue);
}

void ScriptingObjects::ScriptingSamplerSound::setFromJSON(var object)
{
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_VOID_IF_NO_THROW();
	}

	if (auto dyn = object.getDynamicObject())
	{
		for (auto prop : dyn->getProperties())
		{
			// TODO: maybe defer the update until everything is set ?
			sound->setSampleProperty(prop.name, prop.value);
		}
	}
}

var ScriptingObjects::ScriptingSamplerSound::get(int propertyIndex) const
{
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_IF_NO_THROW(var());
	}

	auto id = sampleIds[propertyIndex];

	auto v = sound->getSampleProperty(id);

	if (id == SampleIds::FileName)
		return v;
	else
		return var((int)v);
}

var ScriptingObjects::ScriptingSamplerSound::getRange(int propertyIndex) const
{
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_IF_NO_THROW(var());
	}

	auto r = sound->getPropertyRange(sampleIds[propertyIndex]);

	Array<var> range;
	range.add(r.getStart());
	range.add(r.getEnd());
	return var(range);
}

var ScriptingObjects::ScriptingSamplerSound::getSampleRate()
{
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_IF_NO_THROW(var());
	}

	return sound->getSampleRate();
}

var ScriptingObjects::ScriptingSamplerSound::loadIntoBufferArray()
{
	Array<var> channelData;

	for (int i = 0; i < sound->getNumMultiMicSamples(); i++)
	{
		ScopedPointer<AudioFormatReader> reader = sound->getReferenceToSound(i)->createReaderForPreview();

		if (reader != nullptr)
		{
			int numSamplesToRead = (int)reader->lengthInSamples;
			bool isStereo = reader->numChannels == 2;

			if (numSamplesToRead > 0)
			{
				if (!isStereo)
				{
					auto l = new VariantBuffer(numSamplesToRead);
					channelData.add(var(l));

					AudioSampleBuffer b;
					float* data[1] = { l->buffer.getWritePointer(0) };
					b.setDataToReferTo(data, 1, numSamplesToRead);
					reader->read(&b, 0, numSamplesToRead, 0, true, false);
				}
				else
				{
					auto l = new VariantBuffer(numSamplesToRead);
					auto r = new VariantBuffer(numSamplesToRead);
					channelData.add(var(l));
					channelData.add(var(r));

					AudioSampleBuffer b;

					float* data[2] = { l->buffer.getWritePointer(0), r->buffer.getWritePointer(0) };

					b.setDataToReferTo(data, 2, numSamplesToRead);

					reader->read(&b, 0, numSamplesToRead, 0, true, true);
				}
			}
		}
	}

	return channelData;
}

ScriptingObjects::ScriptingSamplerSound* ScriptingObjects::ScriptingSamplerSound::duplicateSample()
{
	auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

	auto s = getSampler();
	auto mc = s->getMainController();

	ScopedValueSetter<bool> svs(s->getSampleMap()->getSyncEditModeFlag(), true);

	SuspendHelpers::ScopedTicket ticket(mc);
	mc->getJavascriptThreadPool().killVoicesAndExtendTimeOut(jp, 1000);

	while (mc->getKillStateHandler().isAudioRunning())
	{
		Thread::sleep(100);
	}

	LockHelpers::freeToGo(s->getMainController());
	LockHelpers::SafeLock sl(mc, LockHelpers::Type::SampleLock);

	auto copy = sound->getData().createCopy();

	s->getSampleMap()->addSound(copy);
	s->refreshPreloadSizes();

	auto newSound = dynamic_cast<ModulatorSamplerSound*>(s->getSound(s->getNumSounds() - 1));

	return new ScriptingSamplerSound(getScriptProcessor(), s, newSound);
}

void ScriptingObjects::ScriptingSamplerSound::deleteSample()
{
#if HI_ENABLE_EXPANSION_EDITING
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_VOID_IF_NO_THROW();
	}

	auto handler = getSampler()->getSampleEditHandler();
	auto soundCopy = sound;

	auto f = [handler, soundCopy](Processor* /*s*/)
	{
		handler->getSampler()->getSampleMap()->removeSound(soundCopy.get());

		return SafeFunctionCall::OK;
	};

	handler->getSampler()->killAllVoicesAndCall(f);
#endif
}

juce::String ScriptingObjects::ScriptingSamplerSound::getId(int id) const
{
	return sampleIds[id].toString();
}

bool ScriptingObjects::ScriptingSamplerSound::replaceAudioFile(var audioData)
{
	if (!objectExists())
	{
		reportScriptError("Sound does not exist");
		RETURN_IF_NO_THROW(false);
	}

	if (!audioData.isArray())
	{
		reportScriptError("You need to pass in an array of buffers.");
		RETURN_IF_NO_THROW(false);
	}

	int numChannels = 0;

	for (int i = 0; i < sound->getNumMultiMicSamples(); i++)
	{
		if (sound->getReferenceToSound(i)->isMonolithic())
		{
			reportScriptError("Can't write to monolith files");
			RETURN_IF_NO_THROW(false);
		}

		numChannels += sound->getReferenceToSound(i)->isStereo() ? 2 : 1;
	}

	auto& ar = *audioData.getArray();

	if (ar.size() != numChannels)
	{
		reportScriptError("Channel amount doesn't match.");
		RETURN_IF_NO_THROW(false);
	}

	int currentChannel = 0;

	int length = -1;

	for (int i = 0; i < sound->getNumMultiMicSamples(); i++)
	{
		int numChannelsForThisMicPosition = sound->getReferenceToSound(i)->isStereo() ? 2 : 1;

		float* channels[2] = { nullptr, nullptr };

		if (auto l = ar[currentChannel].getBuffer())
		{
			channels[0] = l->buffer.getWritePointer(0);

			if (length == -1)
				length = l->size;
			else if (length != l->size)
				reportScriptError("channel length mismatch: " + String(l->size) + ", Expected: " + String(length));

		}
		else
			reportScriptError("Invalid channel data at index " + String(currentChannel));

		if (numChannelsForThisMicPosition == 2)
		{
			if (auto r = ar[currentChannel + 1].getBuffer())
			{
				channels[1] = r->buffer.getWritePointer(0);

				if (length != r->size)
					reportScriptError("channel length mismatch: " + String(r->size) + ", Expected: " + String(length));
			}
			else
				reportScriptError("Invalid channel data at index " + String(currentChannel + 1));
		}

		AudioSampleBuffer b;
		b.setDataToReferTo(channels, numChannelsForThisMicPosition, length);
		
		bool ok = sound->getReferenceToSound(i)->replaceAudioFile(b);

		if (!ok)
		{
			debugError(getSampler(), "Error writing sample " + sound->getReferenceToSound(i)->getFileName(true));
			return false;
		}

		currentChannel += numChannelsForThisMicPosition;
	}

	return true;
}

bool ScriptingObjects::ScriptingSamplerSound::refersToSameSample(var otherSample)
{
	if (auto s = dynamic_cast<ScriptingSamplerSound*>(otherSample.getObject()))
	{
		return s->sound.get() == sound.get();
	}

	reportScriptError("refersToSampleSample: otherSample parameter is not a sample object");
	RETURN_IF_NO_THROW(false);
}

juce::var ScriptingObjects::ScriptingSamplerSound::getCustomProperties()
{
	if (customObject.isObject())
		return customObject;

	customObject = var(new DynamicObject());
	return customObject;
}

hise::ModulatorSampler* ScriptingObjects::ScriptingSamplerSound::getSampler() const
{
	auto s = dynamic_cast<ModulatorSampler*>(sampler.get());

	if (s == nullptr)
	{
		reportScriptError("Can't find sampler");
		RETURN_IF_NO_THROW(nullptr);
	}

	return s;
}

// ScriptingModulator ===========================================================================================================

struct ScriptingObjects::ScriptingModulator::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingModulator, setAttribute);
	API_METHOD_WRAPPER_1(ScriptingModulator, getAttribute);
  API_METHOD_WRAPPER_1(ScriptingModulator, getAttributeId);
	API_METHOD_WRAPPER_1(ScriptingModulator, getAttributeIndex);
	API_METHOD_WRAPPER_0(ScriptingModulator, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingModulator, isBypassed);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, setIntensity);
	API_METHOD_WRAPPER_0(ScriptingModulator, getIntensity);
	API_METHOD_WRAPPER_0(ScriptingModulator, getCurrentLevel);
	API_METHOD_WRAPPER_0(ScriptingModulator, exportState);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, restoreState);
	API_VOID_METHOD_WRAPPER_1(ScriptingModulator, restoreScriptControls);
	API_METHOD_WRAPPER_0(ScriptingModulator, exportScriptControls);
	API_METHOD_WRAPPER_3(ScriptingModulator, addModulator);
  API_METHOD_WRAPPER_1(ScriptingModulator, getModulatorChain);
	API_METHOD_WRAPPER_3(ScriptingModulator, addGlobalModulator);
	API_METHOD_WRAPPER_3(ScriptingModulator, addStaticGlobalModulator);
	API_METHOD_WRAPPER_0(ScriptingModulator, asTableProcessor);
	API_METHOD_WRAPPER_0(ScriptingModulator, getId);
	API_METHOD_WRAPPER_0(ScriptingModulator, getType);
	API_METHOD_WRAPPER_2(ScriptingModulator, connectToGlobalModulator);
	API_METHOD_WRAPPER_0(ScriptingModulator, getGlobalModulatorId);
};

ScriptingObjects::ScriptingModulator::ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_) :
ConstScriptingObject(p, m_ != nullptr ? m_->getNumParameters() + 1 : 1),
mod(m_),
m(nullptr),
moduleHandler(m_, dynamic_cast<JavascriptProcessor*>(p))
{
	if (mod != nullptr)
	{
		m = dynamic_cast<Modulation*>(m_);

		setName(mod->getId());

		addScriptParameters(this, mod.get());

		for (int i = 0; i < mod->getNumParameters(); i++)
		{
			addConstant(mod->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Modulator");
	}

	ADD_API_METHOD_0(getId);
	ADD_API_METHOD_0(getType);
	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_1(setIntensity);
	ADD_API_METHOD_0(getIntensity);
  ADD_API_METHOD_1(getAttribute);
  ADD_API_METHOD_1(getAttributeId);
	ADD_API_METHOD_1(getAttributeIndex);
	ADD_API_METHOD_0(getCurrentLevel);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_1(restoreScriptControls);
	ADD_API_METHOD_0(exportScriptControls);
	ADD_API_METHOD_3(addModulator);
  ADD_API_METHOD_1(getModulatorChain);    
	ADD_API_METHOD_3(addGlobalModulator);
	ADD_API_METHOD_3(addStaticGlobalModulator);
	ADD_API_METHOD_0(asTableProcessor);
	ADD_API_METHOD_2(connectToGlobalModulator);
	ADD_API_METHOD_0(getGlobalModulatorId);
}

String ScriptingObjects::ScriptingModulator::getDebugName() const
{
	if (objectExists() && !objectDeleted())
		return mod->getId();

	return String("Invalid");
}

String ScriptingObjects::ScriptingModulator::getDebugValue() const
{
	if (objectExists() && !objectDeleted())
		return String(mod->getOutputValue(), 2);

	return "0.0";
}

int ScriptingObjects::ScriptingModulator::getCachedIndex(const var &indexExpression) const
{
	if (checkValidObject())
	{
		Identifier id(indexExpression.toString());

		for (int i = 0; i < mod->getNumParameters(); i++)
		{
			if (id == mod->getIdentifierForParameterIndex(i)) return i;
		}
		return -1;
	}
	else
	{
		throw String("Modulator does not exist");
	}
}

void ScriptingObjects::ScriptingModulator::assign(const int index, var newValue)
{
	setAttribute(index, (float)newValue);
}

var ScriptingObjects::ScriptingModulator::getAssignedValue(int /*index*/) const
{
	return 1.0; // Todo...
}

String ScriptingObjects::ScriptingModulator::getId() const
{
	if (checkValidObject())
		return mod->getId();

	return String();
}

String ScriptingObjects::ScriptingModulator::getType() const
{
	if (checkValidObject())
		return mod->getType().toString();

	return String();
}

bool ScriptingObjects::ScriptingModulator::connectToGlobalModulator(String globalModulationContainerId, String modulatorId)
{
	if (checkValidObject())
	{
        if(auto gm = dynamic_cast<GlobalModulator*>(mod.get()))
        {
            return gm->connectToGlobalModulator(globalModulationContainerId + ":" + modulatorId);
        }
        else
            reportScriptError("connectToGlobalModulator() only works with global modulators!");
	}
    
    return false;
}

String ScriptingObjects::ScriptingModulator::getGlobalModulatorId()
{
	if (checkValidObject())
	{
		if (mod->getType().toString().startsWith("Global"))
		{
			GlobalModulator *gm = dynamic_cast<GlobalModulator*>(m->getProcessor());
			return gm->getItemEntryFor(gm->getConnectedContainer(), gm->getOriginalModulator());
		}
	}
	return String();
}

void ScriptingObjects::ScriptingModulator::setAttribute(int index, float value)
{
	if (checkValidObject())
		mod->setAttribute(index, value, ProcessorHelpers::getAttributeNotificationType());
}

float ScriptingObjects::ScriptingModulator::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return mod->getAttribute(parameterIndex);
    }

	return 0.0f;
}

String ScriptingObjects::ScriptingModulator::getAttributeId(int parameterIndex)
{
    if (checkValidObject())
        return mod->getIdentifierForParameterIndex(parameterIndex).toString();    
    
    return String();
}

int ScriptingObjects::ScriptingModulator::getAttributeIndex(String parameterId)
{
    if (checkValidObject())
        return mod->getParameterIndexForIdentifier(parameterId);

    return -1;
}

int ScriptingObjects::ScriptingModulator::getNumAttributes() const
{
	if (checkValidObject())
	{
		return mod->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingModulator::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		mod->setBypassed(shouldBeBypassed, sendNotification);
		mod->sendChangeMessage();
	}
}



bool ScriptingObjects::ScriptingModulator::isBypassed() const
{
	if (checkValidObject())
	{
		return mod->isBypassed();
	}

	return false;
}

void ScriptingObjects::ScriptingModulator::doubleClickCallback(const MouseEvent &, Component* componentToNotify)
{
	ignoreUnused(componentToNotify);
}

Component* ScriptingObjects::ScriptingModulator::createPopupComponent(const MouseEvent& e, Component* t)
{
	return DebugableObject::Helpers::showProcessorEditorPopup(e, t, mod);
}

void ScriptingObjects::ScriptingModulator::setIntensity(float newIntensity)
{
	if (checkValidObject())
	{
		auto mode = m->getMode();

		if (mode == Modulation::GainMode)
		{
			const float value = jlimit<float>(0.0f, 1.0f, newIntensity);
			m->setIntensity(value);

			mod.get()->sendChangeMessage();
		}
		else if(mode == Modulation::PitchMode)
		{
			const float value = jlimit<float>(-12.0f, 12.0f, newIntensity);
			const float pitchFactor = value / 12.0f;

			m->setIntensity(pitchFactor);

			mod.get()->sendChangeMessage();
		}
		else
		{
			const float value = jlimit<float>(-1.0f, 1.0f, newIntensity);

			m->setIntensity(value);

			mod.get()->sendChangeMessage();
		}
	}
};



float ScriptingObjects::ScriptingModulator::getIntensity() const
{
	if (checkValidObject())
	{
		if (m->getMode() == Modulation::PitchMode)
		{
			return dynamic_cast<const Modulation*>(mod.get())->getIntensity() * 12.0f;
		}
		else
		{
			return dynamic_cast<const Modulation*>(mod.get())->getIntensity();
		}
	}

	return 0.0f;
}

float ScriptingObjects::ScriptingModulator::getCurrentLevel()
{
	if (checkValidObject())
	{
		return m->getProcessor()->getDisplayValues().outL;
	}
	
	return 0.f;
}

String ScriptingObjects::ScriptingModulator::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mod, false);
	}

	return String();
}

void ScriptingObjects::ScriptingModulator::restoreState(String base64State)
{
	if (checkValidObject())
	{
		auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64State);

		if (!vt.isValid())
		{
			reportScriptError("Can't load module state");
			RETURN_VOID_IF_NO_THROW();
		}

		ProcessorHelpers::restoreFromBase64String(mod, base64State);
	}
}

String ScriptingObjects::ScriptingModulator::exportScriptControls()
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mod.get()) == nullptr)
	{
		reportScriptError("exportScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mod, false, true);
	}

	return String();
}

void ScriptingObjects::ScriptingModulator::restoreScriptControls(String base64Controls)
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mod.get()) == nullptr)
	{
		reportScriptError("restoreScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(mod, base64Controls, true);
	}
}

var ScriptingObjects::ScriptingModulator::addModulator(var chainIndex, var typeName, var modName)
{
	if (checkValidObject())
	{
		ModulatorChain *c = dynamic_cast<ModulatorChain*>(mod->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");
		
		Processor* p = moduleHandler.addModule(c, typeName, modName, -1);

		if (p != nullptr)
		{
			auto newMod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
			return var(newMod);
		}
		
	}

	return var();
}
    
var ScriptingObjects::ScriptingModulator::getModulatorChain(var chainIndex)
{
    if (checkValidObject())
    {
        auto c = dynamic_cast<Modulator*>(mod->getChildProcessor(chainIndex));
        
        if (c == nullptr)
            reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");
            
        auto modChain = new ScriptingModulator(getScriptProcessor(), c);
            
        return var(modChain);
    }
    else
    {
        return var();
    }
}

var ScriptingObjects::ScriptingModulator::addGlobalModulator(var chainIndex, var globalMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(globalMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(mod->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName);

			if (p != nullptr)
			{
				auto newMod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(newMod);
			}
			
		}
	}

	return var();
}

var ScriptingObjects::ScriptingModulator::addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(timeVariantMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(mod->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName, true);

			if (p != nullptr)
			{
				auto newMod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(newMod);
			}

		}
	}

	return var();
}

var ScriptingObjects::ScriptingModulator::asTableProcessor()
{
	if (checkValidObject())
	{
		auto ltp = dynamic_cast<LookupTableProcessor*>(mod.get());

		if (ltp == nullptr)
			return var(); // don't complain here, handle it on scripting level

		auto t = new ScriptingTableProcessor(getScriptProcessor(), ltp);
			return var(t);
	}

	auto t = new ScriptingObjects::ScriptingTableProcessor(getScriptProcessor(), nullptr);
	return var(t);
}

// ScriptingEffect ==============================================================================================================

struct ScriptingObjects::ScriptingEffect::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingEffect, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingEffect, getAttribute);
    API_METHOD_WRAPPER_1(ScriptingEffect, getAttributeId);
		API_METHOD_WRAPPER_1(ScriptingEffect, getAttributeIndex);
	API_METHOD_WRAPPER_0(ScriptingEffect, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingEffect, isBypassed);
	API_METHOD_WRAPPER_0(ScriptingEffect, exportState);
	API_METHOD_WRAPPER_1(ScriptingEffect, getCurrentLevel);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, restoreState);
	API_VOID_METHOD_WRAPPER_1(ScriptingEffect, restoreScriptControls);
	API_METHOD_WRAPPER_0(ScriptingEffect, exportScriptControls);
	API_METHOD_WRAPPER_3(ScriptingEffect, addModulator);
	API_METHOD_WRAPPER_3(ScriptingEffect, addGlobalModulator);
	API_METHOD_WRAPPER_1(ScriptingEffect, getModulatorChain);
	API_METHOD_WRAPPER_3(ScriptingEffect, addStaticGlobalModulator);
	API_METHOD_WRAPPER_0(ScriptingEffect, getId);
};

ScriptingObjects::ScriptingEffect::ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx) :
ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
effect(fx),
moduleHandler(fx, dynamic_cast<JavascriptProcessor*>(p))
{
	if (fx != nullptr)
	{
		setName(fx->getId());

		addScriptParameters(this, effect.get());

		for (int i = 0; i < fx->getNumParameters(); i++)
		{
			addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Effect");
	}

	ADD_API_METHOD_0(getId);
	ADD_API_METHOD_2(setAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(getAttributeId);
		ADD_API_METHOD_1(getAttributeIndex);
	ADD_API_METHOD_1(getCurrentLevel);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_1(restoreScriptControls);
	ADD_API_METHOD_0(exportScriptControls);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_3(addModulator);
    
	ADD_API_METHOD_1(getModulatorChain);
	ADD_API_METHOD_3(addGlobalModulator);
	ADD_API_METHOD_3(addStaticGlobalModulator);
};


Component* ScriptingObjects::ScriptingEffect::createPopupComponent(const MouseEvent& e, Component* t)
{
	return DebugableObject::Helpers::showProcessorEditorPopup(e, t, effect.get());
}

juce::String ScriptingObjects::ScriptingEffect::getId() const
{
	if (checkValidObject())
		return effect->getId();

	return String();
}

void ScriptingObjects::ScriptingEffect::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		effect->setAttribute(parameterIndex, newValue, ProcessorHelpers::getAttributeNotificationType());
	}
}

float ScriptingObjects::ScriptingEffect::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return effect->getAttribute(parameterIndex);
    }

	return 0.0f;
}

String ScriptingObjects::ScriptingEffect::getAttributeId(int parameterIndex)
{
    if (checkValidObject())
        return effect->getIdentifierForParameterIndex(parameterIndex).toString();    
    
    return String();
}

int ScriptingObjects::ScriptingEffect::getAttributeIndex(String parameterId)
{
    if (checkValidObject())
        return effect->getParameterIndexForIdentifier(parameterId);

    return -1;
}

int ScriptingObjects::ScriptingEffect::getNumAttributes() const
{
	if (checkValidObject())
	{
		return effect->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingEffect::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		effect->setBypassed(shouldBeBypassed, sendNotification);
		effect->sendChangeMessage();
	}
}

bool ScriptingObjects::ScriptingEffect::isBypassed() const
{
	if (checkValidObject())
	{
		return effect->isBypassed();
	}

	return false;
}

String ScriptingObjects::ScriptingEffect::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(effect, false);
	}

	return String();
}

void ScriptingObjects::ScriptingEffect::restoreState(String base64State)
{
	if (checkValidObject())
	{
		auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64State);

		if (!vt.isValid())
		{
			reportScriptError("Can't load module state");
			RETURN_VOID_IF_NO_THROW();
		}

		SuspendHelpers::ScopedTicket ticket(effect->getMainController());
		effect->getMainController()->getJavascriptThreadPool().killVoicesAndExtendTimeOut(dynamic_cast<JavascriptProcessor*>(getScriptProcessor()));
		LockHelpers::freeToGo(effect->getMainController());
		ProcessorHelpers::restoreFromBase64String(effect, base64State);
	}
}

String ScriptingObjects::ScriptingEffect::exportScriptControls()
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(effect.get()) == nullptr)
	{
		reportScriptError("exportScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(effect, false, true);
	}

	return String();
}

void ScriptingObjects::ScriptingEffect::restoreScriptControls(String base64Controls)
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(effect.get()) == nullptr)
	{
		reportScriptError("restoreScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(effect, base64Controls, true);
	}
}

float ScriptingObjects::ScriptingEffect::getCurrentLevel(bool leftChannel)
{
	if (checkValidObject())
	{
		return leftChannel ? effect->getDisplayValues().outL : effect->getDisplayValues().outR;
	}

	return 0.0f;
}

var ScriptingObjects::ScriptingEffect::addModulator(var chainIndex, var typeName, var modName)
{
	if (checkValidObject())
	{
		ModulatorChain *c = dynamic_cast<ModulatorChain*>(effect->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

		Processor* p = moduleHandler.addModule(c, typeName, modName, -1);

		if (p != nullptr)
		{
			auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
			return var(mod);
		}

		return var();
	}
	else
	{
		return var();
	}
	
}

var ScriptingObjects::ScriptingEffect::getModulatorChain(var chainIndex)
{
	if (checkValidObject())
	{
		auto c = dynamic_cast<Modulator*>(effect->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

		auto mod = new ScriptingModulator(getScriptProcessor(), c);

		return var(mod);
	}
	else
	{
		return var();
	}
}

var ScriptingObjects::ScriptingEffect::addGlobalModulator(var chainIndex, var globalMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(globalMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(effect->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName);

			if (p != nullptr)
			{
				auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(mod);
			}

			return var();
		}
	}

	return var();
}

var ScriptingObjects::ScriptingEffect::addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(timeVariantMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(effect->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName, true);

			if (p != nullptr)
			{
				auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(mod);
			}

			return var();
		}
	}

	return var();
}

ScriptingObjects::ScriptingEffect::FilterModeObject::FilterModeObject(const ProcessorWithScriptingContent* p) :
	ConstScriptingObject(const_cast<ProcessorWithScriptingContent*>(p), (int)FilterBank::FilterMode::numFilterModes)
{
	

#define ADD_FILTER_CONSTANT(x) addConstant(#x, (int)FilterBank::FilterMode::x)

	ADD_FILTER_CONSTANT(LowPass);
	ADD_FILTER_CONSTANT(HighPass);
	ADD_FILTER_CONSTANT(LowShelf);
	ADD_FILTER_CONSTANT(HighShelf);
	ADD_FILTER_CONSTANT(Peak);
	ADD_FILTER_CONSTANT(ResoLow);
	ADD_FILTER_CONSTANT(StateVariableLP);
	ADD_FILTER_CONSTANT(StateVariableHP);
	ADD_FILTER_CONSTANT(MoogLP);
	ADD_FILTER_CONSTANT(OnePoleLowPass);
	ADD_FILTER_CONSTANT(OnePoleHighPass);
	ADD_FILTER_CONSTANT(StateVariablePeak);
	ADD_FILTER_CONSTANT(StateVariableNotch);
	ADD_FILTER_CONSTANT(StateVariableBandPass);
	ADD_FILTER_CONSTANT(Allpass);
	ADD_FILTER_CONSTANT(LadderFourPoleLP);
	ADD_FILTER_CONSTANT(LadderFourPoleHP);
	ADD_FILTER_CONSTANT(RingMod);
	

#undef ADD_FILTER_CONSTANT
}



// ScriptingSlotFX ==============================================================================================================


struct ScriptingObjects::ScriptingSlotFX::Wrapper
{
    API_METHOD_WRAPPER_1(ScriptingSlotFX, setEffect);
    API_VOID_METHOD_WRAPPER_0(ScriptingSlotFX, clear);
	API_METHOD_WRAPPER_1(ScriptingSlotFX, swap);
	API_METHOD_WRAPPER_0(ScriptingSlotFX, getCurrentEffect);
	API_METHOD_WRAPPER_0(ScriptingSlotFX, getModuleList);
    API_METHOD_WRAPPER_0(ScriptingSlotFX, getParameterProperties);
    API_METHOD_WRAPPER_0(ScriptingSlotFX, getCurrentEffectId);
};

ScriptingObjects::ScriptingSlotFX::ScriptingSlotFX(ProcessorWithScriptingContent *p, EffectProcessor *fx) :
ConstScriptingObject(p, fx != nullptr ? fx->getNumParameters()+1 : 1),
slotFX(fx)
{
    if (fx != nullptr)
    {
        setName(fx->getId());
        
        addScriptParameters(this, slotFX.get());
        
        for (int i = 0; i < fx->getNumParameters(); i++)
        {
            addConstant(fx->getIdentifierForParameterIndex(i).toString(), var(i));
        }
    }
    else
    {
        setName("Invalid Effect");
    }
    
    ADD_API_METHOD_1(setEffect);
	ADD_API_METHOD_0(getCurrentEffect);
    ADD_API_METHOD_0(clear);
	ADD_API_METHOD_1(swap);
	ADD_API_METHOD_0(getModuleList);
    ADD_API_METHOD_0(getParameterProperties);
    ADD_API_METHOD_0(getCurrentEffectId);
};



void ScriptingObjects::ScriptingSlotFX::clear()
{
	if (auto slot = getSlotFX())
	{
        slot->clearEffect();
	}
	else
	{
		reportScriptError("Invalid Slot");
	}
}


ScriptingObjects::ScriptingEffect* ScriptingObjects::ScriptingSlotFX::setEffect(String effectName)
{
	if (effectName == "undefined")
	{
		reportScriptError("Invalid effectName");
		RETURN_IF_NO_THROW(new ScriptingEffect(getScriptProcessor(), nullptr))
	}

	if(auto slot = getSlotFX())
    {
		auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor());

		{
			SuspendHelpers::ScopedTicket ticket(slotFX->getMainController());

			slotFX->getMainController()->getJavascriptThreadPool().killVoicesAndExtendTimeOut(jp);

			LockHelpers::freeToGo(slotFX->getMainController());
			slot->setEffect(effectName, false);
		}

		return new ScriptingEffect(getScriptProcessor(), dynamic_cast<EffectProcessor*>(slot->getCurrentEffect()));
    }
	else
	{
		reportScriptError("Invalid Slot");
		RETURN_IF_NO_THROW(new ScriptingEffect(getScriptProcessor(), nullptr))
	}
}

ScriptingObjects::ScriptingEffect* ScriptingObjects::ScriptingSlotFX::getCurrentEffect()
{
	if (auto slot = getSlotFX())
	{
		if (auto fx = slot->getCurrentEffect())
		{
			return new ScriptingEffect(getScriptProcessor(), dynamic_cast<EffectProcessor*>(fx));
		}
	}

	return {};
}

bool ScriptingObjects::ScriptingSlotFX::swap(var otherSlot)
{
	if (auto t = getSlotFX())
	{
		if (auto sl = dynamic_cast<ScriptingSlotFX*>(otherSlot.getObject()))
		{
			if (auto other = sl->getSlotFX())
			{
				return t->swap(other);
			}
			else
			{
				reportScriptError("Target Slot is invalid");
			}
		}
		else
		{
			reportScriptError("Target Slot does not exist");
		}
	}
	else
	{
		reportScriptError("Source Slot is invalid");
	}
    
    RETURN_IF_NO_THROW(false);
}

juce::var ScriptingObjects::ScriptingSlotFX::getModuleList()
{
	Array<var> list;

	if (auto slot = getSlotFX())
	{
		auto sa = slot->getModuleList();
		
		for (const auto& s : sa)
			list.add(var(s));
	}

	return var(list);
}

String ScriptingObjects::ScriptingSlotFX::getCurrentEffectId()
{
    if (auto slot = getSlotFX())
    {
        return slot->getCurrentEffectId();
    }
    
    return "";
}

var ScriptingObjects::ScriptingSlotFX::getParameterProperties()
{
    if(auto slot = getSlotFX())
        return slot->getParameterProperties();
    
    return var();
}

HotswappableProcessor* ScriptingObjects::ScriptingSlotFX::getSlotFX()
{
	return dynamic_cast<HotswappableProcessor*>(slotFX.get());
}

struct ScriptingObjects::ScriptRoutingMatrix::Wrapper
{
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, addConnection);
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, removeConnection);
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, addSendConnection);
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, removeSendConnection);
	API_VOID_METHOD_WRAPPER_0(ScriptRoutingMatrix, clear);
	API_METHOD_WRAPPER_1(ScriptRoutingMatrix, getSourceGainValue);
	API_VOID_METHOD_WRAPPER_1(ScriptRoutingMatrix, setNumChannels);
	API_METHOD_WRAPPER_1(ScriptRoutingMatrix, getSourceChannelsForDestination);
	API_METHOD_WRAPPER_1(ScriptRoutingMatrix, getDestinationChannelForSource);
};

ScriptingObjects::ScriptRoutingMatrix::ScriptRoutingMatrix(ProcessorWithScriptingContent *p, Processor *processor):
	ConstScriptingObject(p, 2),
	rp(processor)
{
	ADD_API_METHOD_2(addConnection);
	ADD_API_METHOD_2(removeConnection);
	ADD_API_METHOD_2(addSendConnection);
	ADD_API_METHOD_2(removeSendConnection);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_1(getSourceGainValue);
	ADD_API_METHOD_1(setNumChannels);
	ADD_API_METHOD_1(getSourceChannelsForDestination);
	ADD_API_METHOD_1(getDestinationChannelForSource);

	if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
	{
		addConstant("NumInputs", r->getMatrix().getNumSourceChannels());
		addConstant("NumOutputs", r->getMatrix().getNumDestinationChannels());
	}
	else
	{
		jassertfalse;
		addConstant("NumInputs", -1);
		addConstant("NumOutputs", -1);
	}
}

void ScriptingObjects::ScriptRoutingMatrix::setNumChannels(int numSourceChannels)
{
	if (!isPositiveAndBelow(numSourceChannels, NUM_MAX_CHANNELS + 1))
	{
		reportScriptError("illegal channel amount: " + String(numSourceChannels));
		RETURN_VOID_IF_NO_THROW();
	}

	if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
	{
		if (!r->getMatrix().resizingIsAllowed())
		{
			reportScriptError("Can't resize this matrix");
			RETURN_VOID_IF_NO_THROW();
		}

		r->getMatrix().setNumSourceChannels(numSourceChannels);
		r->getMatrix().setNumAllowedConnections(numSourceChannels);
	}
}

bool ScriptingObjects::ScriptRoutingMatrix::addConnection(int sourceIndex, int destinationIndex)
{
	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			return r->getMatrix().addConnection(sourceIndex, destinationIndex);
		}
		else
			return false;
	}

	return false;
}

bool ScriptingObjects::ScriptRoutingMatrix::addSendConnection(int sourceIndex, int destinationIndex)
{
	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			return r->getMatrix().addSendConnection(sourceIndex, destinationIndex);
		}
		else
			return false;
	}

	return false;
}

bool ScriptingObjects::ScriptRoutingMatrix::removeSendConnection(int sourceIndex, int destinationIndex)
{
	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			return r->getMatrix().removeSendConnection(sourceIndex, destinationIndex);
		}
		else
			return false;
	}

	return false;
}

bool ScriptingObjects::ScriptRoutingMatrix::removeConnection(int sourceIndex, int destinationIndex)
{
	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			return r->getMatrix().removeConnection(sourceIndex, destinationIndex);
		}
		else
			return false;
	}

	return false;
}

void ScriptingObjects::ScriptRoutingMatrix::clear()
{
	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			r->getMatrix().resetToDefault();
			r->getMatrix().removeConnection(0, 0);
			r->getMatrix().removeConnection(1, 1);
		}
		
	}
}

float ScriptingObjects::ScriptRoutingMatrix::getSourceGainValue(int channelIndex)
{
	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			if (isPositiveAndBelow(channelIndex, r->getMatrix().getNumSourceChannels()))
			{
				return r->getMatrix().getGainValue(channelIndex, true);
			}
		}

	}

	return 0.0f;
}

var ScriptingObjects::ScriptRoutingMatrix::getSourceChannelsForDestination(var destinationIndex) const
{
	if (destinationIndex.isArray())
	{
		Array<var> returnValues;

		for (auto r : *destinationIndex.getArray())
			returnValues.add(getSourceChannelsForDestination(r));

		return var(returnValues);
	}

	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			Array<var> channels;

			for (int i = 0; i < r->getMatrix().getNumSourceChannels(); i++)
			{
				auto thisDest = r->getMatrix().getConnectionForSourceChannel(i);

				if (thisDest == (int)destinationIndex)
					channels.add(i);
			}

			if (channels.isEmpty())
				return -1;
			else if (channels.size() == 1)
				return channels.getFirst();
			else
				return channels;
		}
	}

	return -1;
}

var ScriptingObjects::ScriptRoutingMatrix::getDestinationChannelForSource(var sourceIndex) const
{
	if (sourceIndex.isArray())
	{
		Array<var> returnArray;

		for (auto r : *sourceIndex.getArray())
			returnArray.add(getDestinationChannelForSource(r));

		return var(returnArray);
	}

	if (checkValidObject())
	{
		if (auto r = dynamic_cast<RoutableProcessor*>(rp.get()))
		{
			return r->getMatrix().getConnectionForSourceChannel(sourceIndex);
		}
	}

	return -1;
}

// ScriptingSynth ==============================================================================================================

struct ScriptingObjects::ScriptingSynth::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingSynth, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingSynth, getAttribute);
    API_METHOD_WRAPPER_1(ScriptingSynth, getAttributeId);
		API_METHOD_WRAPPER_1(ScriptingSynth, getAttributeIndex);
	API_METHOD_WRAPPER_0(ScriptingSynth, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingSynth, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingSynth, isBypassed);
	API_METHOD_WRAPPER_1(ScriptingSynth, getChildSynthByIndex);
	API_METHOD_WRAPPER_0(ScriptingSynth, exportState);
	API_METHOD_WRAPPER_1(ScriptingSynth, getCurrentLevel);
	API_VOID_METHOD_WRAPPER_1(ScriptingSynth, restoreState);
	API_METHOD_WRAPPER_3(ScriptingSynth, addModulator);
	API_METHOD_WRAPPER_1(ScriptingSynth, getModulatorChain);
	API_METHOD_WRAPPER_3(ScriptingSynth, addGlobalModulator);
	API_METHOD_WRAPPER_3(ScriptingSynth, addStaticGlobalModulator);
	API_METHOD_WRAPPER_0(ScriptingSynth, asSampler);
	API_METHOD_WRAPPER_0(ScriptingSynth, getRoutingMatrix);
	API_METHOD_WRAPPER_0(ScriptingSynth, getId);
};

ScriptingObjects::ScriptingSynth::ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_) :
	ConstScriptingObject(p, synth_ != nullptr ? synth_->getNumParameters() + 1 : 1),
	synth(synth_),
	moduleHandler(synth_, dynamic_cast<JavascriptProcessor*>(p))
{
	if (synth != nullptr)
	{
		setName(synth->getId());

		addScriptParameters(this, synth.get());

		for (int i = 0; i < synth->getNumParameters(); i++)
		{
			addConstant(synth->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Effect");
	}

	ADD_API_METHOD_0(getId);
	ADD_API_METHOD_2(setAttribute);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(getAttributeId);
		ADD_API_METHOD_1(getAttributeIndex);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_1(getChildSynthByIndex);
	ADD_API_METHOD_1(getCurrentLevel);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_3(addModulator);
	ADD_API_METHOD_1(getModulatorChain);
	ADD_API_METHOD_3(addGlobalModulator);
	ADD_API_METHOD_3(addStaticGlobalModulator);
	ADD_API_METHOD_0(asSampler);
	ADD_API_METHOD_0(getRoutingMatrix);
};


Component* ScriptingObjects::ScriptingSynth::createPopupComponent(const MouseEvent& e, Component* t)
{
	return DebugableObject::Helpers::showProcessorEditorPopup(e, t, synth);
}

String ScriptingObjects::ScriptingSynth::getId() const
{
	if (checkValidObject())
		return synth->getId();

	return String();
}

void ScriptingObjects::ScriptingSynth::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
        
        
		synth->setAttribute(parameterIndex, newValue, ProcessorHelpers::getAttributeNotificationType());
	}
}

float ScriptingObjects::ScriptingSynth::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return synth->getAttribute(parameterIndex);
    }

	return 0.0f;
}

String ScriptingObjects::ScriptingSynth::getAttributeId(int parameterIndex)
{
    if (checkValidObject())
        return synth->getIdentifierForParameterIndex(parameterIndex).toString();    
    
    return String();
}

int ScriptingObjects::ScriptingSynth::getAttributeIndex(String parameterId)
{
    if (checkValidObject())
        return synth->getParameterIndexForIdentifier(parameterId);

    return -1;
}

int ScriptingObjects::ScriptingSynth::getNumAttributes() const
{
	if (checkValidObject())
	{
		return synth->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingSynth::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		synth->setBypassed(shouldBeBypassed, sendNotification);
		synth->sendChangeMessage();
	}
}

bool ScriptingObjects::ScriptingSynth::isBypassed() const
{
	if (checkValidObject())
	{
		return synth->isBypassed();
	}

	return false;
}

ScriptingObjects::ScriptingSynth* ScriptingObjects::ScriptingSynth::getChildSynthByIndex(int index)
{
	if (getScriptProcessor()->objectsCanBeCreated())
	{
		if (Chain* c = dynamic_cast<Chain*>(synth.get()))
		{
			if (index >= 0 && index < c->getHandler()->getNumProcessors())
			{
				return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), dynamic_cast<ModulatorSynth*>(c->getHandler()->getProcessor(index)));
			}
		}

		return new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr);
	}
	else
	{
		reportIllegalCall("getChildSynth()", "onInit");
		RETURN_IF_NO_THROW(new ScriptingObjects::ScriptingSynth(getScriptProcessor(), nullptr))
	}
}

String ScriptingObjects::ScriptingSynth::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(synth, false);
	}

	return String();
}

void ScriptingObjects::ScriptingSynth::restoreState(String base64State)
{
	if (checkValidObject())
	{
		auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64State);

		if (!vt.isValid())
		{
			reportScriptError("Can't load module state");
			RETURN_VOID_IF_NO_THROW();
		}

		ProcessorHelpers::restoreFromBase64String(synth, base64State);
	}
}

float ScriptingObjects::ScriptingSynth::getCurrentLevel(bool leftChannel)
{
	if (checkValidObject())
	{
		return leftChannel ? synth->getDisplayValues().outL : synth->getDisplayValues().outR;
	}

	return 0.0f;
}

var ScriptingObjects::ScriptingSynth::addModulator(var chainIndex, var typeName, var modName)
{
	if (checkValidObject())
	{
		ModulatorChain *c = dynamic_cast<ModulatorChain*>(synth->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

		Processor* p = moduleHandler.addModule(c, typeName, modName, -1);

		if (p != nullptr)
		{
			auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), dynamic_cast<Modulator*>(p));
			return var(mod);
		}
	}

	return var();
}

var ScriptingObjects::ScriptingSynth::getModulatorChain(var chainIndex)
{
	if (checkValidObject())
	{
		auto c = dynamic_cast<Modulator*>(synth->getChildProcessor(chainIndex));

		if (c == nullptr)
			reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

		auto mod = new ScriptingModulator(getScriptProcessor(), c);

		return var(mod);
	}
	else
	{
		return var();
	}
}

var ScriptingObjects::ScriptingSynth::addGlobalModulator(var chainIndex, var globalMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(globalMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(synth->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName);

			if (p != nullptr)
			{
				auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(mod);
			}
		}
	}

	return var();
}

var ScriptingObjects::ScriptingSynth::addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName)
{
	if (checkValidObject())
	{
		if (auto gm = dynamic_cast<ScriptingModulator*>(timeVariantMod.getObject()))
		{
			ModulatorChain *c = dynamic_cast<ModulatorChain*>(synth->getChildProcessor(chainIndex));

			if (c == nullptr)
				reportScriptError("Modulator Chain with index " + chainIndex.toString() + " does not exist");

			auto p = moduleHandler.addAndConnectToGlobalModulator(c, gm->getModulator(), modName, true);

			if (p != nullptr)
			{
				auto mod = new ScriptingObjects::ScriptingModulator(getScriptProcessor(), p);
				return var(mod);
			}
		}
	}

	return var();
}

var ScriptingObjects::ScriptingSynth::asSampler()
{
	if (checkValidObject())
	{
		auto sampler = dynamic_cast<ModulatorSampler*>(synth.get());

		if (sampler == nullptr)
			return var(); // don't complain here, handle it on scripting level

		auto t = new ScriptingApi::Sampler(getScriptProcessor(), sampler);
		return var(t);
	}

	auto t = new ScriptingApi::Sampler(getScriptProcessor(), nullptr);
	return var(t);
}

var ScriptingObjects::ScriptingSynth::getRoutingMatrix()
{
	auto r = new ScriptRoutingMatrix(getScriptProcessor(), synth.get());
	return var(r);
}

// ScriptingMidiProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingMidiProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingMidiProcessor, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingMidiProcessor, getAttribute);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, getNumAttributes);
	API_METHOD_WRAPPER_1(ScriptingMidiProcessor, getAttributeId);
	API_METHOD_WRAPPER_1(ScriptingMidiProcessor, getAttributeIndex);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, isBypassed);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, exportState);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, restoreState);
	API_VOID_METHOD_WRAPPER_1(ScriptingMidiProcessor, restoreScriptControls);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, exportScriptControls);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, getId);
	API_METHOD_WRAPPER_0(ScriptingMidiProcessor, asMidiPlayer);
	
	
};

ScriptingObjects::ScriptingMidiProcessor::ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_) :
ConstScriptingObject(p, mp_ != nullptr ? mp_->getNumParameters()+1 : 1),
mp(mp_)
{
	if (mp != nullptr)
	{
		setName(mp->getId());

		addScriptParameters(this, mp.get());

		for (int i = 0; i < mp->getNumParameters(); i++)
		{
			addConstant(mp->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid MidiProcessor");
	}

	ADD_API_METHOD_2(setAttribute);
    ADD_API_METHOD_1(getAttribute);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_0(exportState);
	ADD_API_METHOD_1(restoreState);
	ADD_API_METHOD_0(getId);
	ADD_API_METHOD_1(restoreScriptControls);
	ADD_API_METHOD_0(exportScriptControls);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_1(getAttributeId);
	ADD_API_METHOD_1(getAttributeIndex);
	ADD_API_METHOD_0(asMidiPlayer);
	
}

Component* ScriptingObjects::ScriptingMidiProcessor::createPopupComponent(const MouseEvent& e, Component* t)
{
	return DebugableObject::Helpers::showProcessorEditorPopup(e, t, mp);
}

int ScriptingObjects::ScriptingMidiProcessor::getCachedIndex(const var &indexExpression) const
{
	if (checkValidObject())
	{
		Identifier id(indexExpression.toString());

		for (int i = 0; i < mp->getNumParameters(); i++)
		{
			if (id == mp->getIdentifierForParameterIndex(i)) return i;
		}
	}

	return -1;
}

void ScriptingObjects::ScriptingMidiProcessor::assign(const int index, var newValue)
{
	setAttribute(index, (float)newValue);
}

var ScriptingObjects::ScriptingMidiProcessor::getAssignedValue(int /*index*/) const
{
	return 1.0; // Todo...
}

String ScriptingObjects::ScriptingMidiProcessor::getId() const
{
	if (checkValidObject())
		return mp->getId();

	return String();
}

void ScriptingObjects::ScriptingMidiProcessor::setAttribute(int index, float value)
{
	if (checkValidObject())
	{
		mp->setAttribute(index, value,  ProcessorHelpers::getAttributeNotificationType());
	}
}

float ScriptingObjects::ScriptingMidiProcessor::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return mp->getAttribute(parameterIndex);
    }

	return 0.0f;
}

int ScriptingObjects::ScriptingMidiProcessor::getNumAttributes() const
{
	if (checkValidObject())
	{
		return mp->getNumParameters();
	}

	return 0;
}

String ScriptingObjects::ScriptingMidiProcessor::getAttributeId(int parameterIndex)
{
    if (checkValidObject())
        return mp->getIdentifierForParameterIndex(parameterIndex).toString();    
    
    return String();
}

int ScriptingObjects::ScriptingMidiProcessor::getAttributeIndex(String parameterId)
{
    if (checkValidObject())
        return mp->getParameterIndexForIdentifier(parameterId);

    return -1;
}

void ScriptingObjects::ScriptingMidiProcessor::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		mp->setBypassed(shouldBeBypassed, sendNotification);
		mp->sendChangeMessage();
	}
}

bool ScriptingObjects::ScriptingMidiProcessor::isBypassed() const
{
	if (checkValidObject())
	{
		return mp->isBypassed();
	}

	return false;
}

String ScriptingObjects::ScriptingMidiProcessor::exportState()
{
	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mp, false, false);
	}

	return String();
}

void ScriptingObjects::ScriptingMidiProcessor::restoreState(String base64State)
{
	if (checkValidObject())
	{
		auto vt = ProcessorHelpers::ValueTreeHelpers::getValueTreeFromBase64String(base64State);

		if (!vt.isValid())
		{
			reportScriptError("Can't load module state");
			RETURN_VOID_IF_NO_THROW();
		}

		ProcessorHelpers::restoreFromBase64String(mp, base64State, false);
	}
}

String ScriptingObjects::ScriptingMidiProcessor::exportScriptControls()
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mp.get()) == nullptr)
	{
		reportScriptError("exportScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		return ProcessorHelpers::getBase64String(mp, false, true);
	}

	return String();
}

void ScriptingObjects::ScriptingMidiProcessor::restoreScriptControls(String base64Controls)
{
	if (dynamic_cast<ProcessorWithScriptingContent*>(mp.get()) == nullptr)
	{
		reportScriptError("restoreScriptControls can only be used on Script Processors");
	}

	if (checkValidObject())
	{
		ProcessorHelpers::restoreFromBase64String(mp, base64Controls, true);
	}
}

var ScriptingObjects::ScriptingMidiProcessor::asMidiPlayer()
{
	if (auto player = dynamic_cast<MidiPlayer*>(mp.get()))
	{
		return var(new ScriptedMidiPlayer(getScriptProcessor(), player));
	}

	reportScriptError("The module is not a MIDI player");
	RETURN_IF_NO_THROW(var());
}

// ScriptingAudioSampleProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingAudioSampleProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingAudioSampleProcessor, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, getAttribute);
    API_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, getAttributeId);
		API_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, getAttributeIndex);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, getNumAttributes);
	API_VOID_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, setBypassed);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, isBypassed);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, getSampleLength);
	API_VOID_METHOD_WRAPPER_2(ScriptingAudioSampleProcessor, setSampleRange);
	API_VOID_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, setFile);
	API_METHOD_WRAPPER_1(ScriptingAudioSampleProcessor, getAudioFile);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, getFilename);
	API_METHOD_WRAPPER_0(ScriptingAudioSampleProcessor, getSampleStart);
};


ScriptingObjects::ScriptingAudioSampleProcessor::ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, Processor *sampleProcessor) :
ConstScriptingObject(p, dynamic_cast<Processor*>(sampleProcessor) != nullptr ? dynamic_cast<Processor*>(sampleProcessor)->getNumParameters() : 0),
audioSampleProcessor(dynamic_cast<Processor*>(sampleProcessor))
{
	if (audioSampleProcessor != nullptr)
	{
		setName(audioSampleProcessor->getId());

		for (int i = 0; i < audioSampleProcessor->getNumParameters(); i++)
		{
			addConstant(audioSampleProcessor->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Processor");
	}

	ADD_API_METHOD_2(setAttribute);
    ADD_API_METHOD_1(getAttribute);
    ADD_API_METHOD_1(getAttributeId);
		ADD_API_METHOD_1(getAttributeIndex);
	ADD_API_METHOD_0(getNumAttributes);
	ADD_API_METHOD_1(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_0(getSampleLength);
	ADD_API_METHOD_2(setSampleRange);
	ADD_API_METHOD_1(setFile);
	ADD_API_METHOD_1(getAudioFile);
	ADD_API_METHOD_0(getFilename);
	ADD_API_METHOD_0(getSampleStart);
}



void ScriptingObjects::ScriptingAudioSampleProcessor::setAttribute(int parameterIndex, float newValue)
{
	if (checkValidObject())
	{
		audioSampleProcessor->setAttribute(parameterIndex, newValue, sendNotification);
	}
}

float ScriptingObjects::ScriptingAudioSampleProcessor::getAttribute(int parameterIndex)
{
    if (checkValidObject())
    {
        return audioSampleProcessor->getAttribute(parameterIndex);
    }

	return 0.0f;
}

String ScriptingObjects::ScriptingAudioSampleProcessor::getAttributeId(int parameterIndex)
{
    if (checkValidObject())
        return audioSampleProcessor->getIdentifierForParameterIndex(parameterIndex).toString();    
    
    return String();
}

int ScriptingObjects::ScriptingAudioSampleProcessor::getAttributeIndex(String parameterId)
{
    if (checkValidObject())
        return audioSampleProcessor->getParameterIndexForIdentifier(parameterId);

    return -1;
}

int ScriptingObjects::ScriptingAudioSampleProcessor::getNumAttributes() const
{
	if (checkValidObject())
	{
		return audioSampleProcessor->getNumParameters();
	}

	return 0;
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setBypassed(bool shouldBeBypassed)
{
	if (checkValidObject())
	{
		audioSampleProcessor->setBypassed(shouldBeBypassed, sendNotification);
		audioSampleProcessor->sendChangeMessage();
	}
}

bool ScriptingObjects::ScriptingAudioSampleProcessor::isBypassed() const
{
	if (checkValidObject())
	{
		return audioSampleProcessor->isBypassed();
	}

	return false;
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setFile(String fileName)
{
	if (checkValidObject())
	{

#if USE_BACKEND
		auto pool = audioSampleProcessor->getMainController()->getCurrentAudioSampleBufferPool();

		if (!fileName.contains("{EXP::") && !pool->areAllFilesLoaded())
		{
			PoolReference ref(getScriptProcessor()->getMainController_(), fileName, FileHandlerBase::AudioFiles);

			if (ref.getReferenceString().contains("{PROJECT_FOLDER}"))
			{
				reportScriptError("You must call Engine.loadAudioFilesIntoPool() before using this method");
			}
		}
			
#endif

		auto p = dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get());
		jassert(p != nullptr);
		p->getAudioFile(0)->fromBase64String(fileName);
	}
}

String ScriptingObjects::ScriptingAudioSampleProcessor::getFilename()
{
	if (checkValidObject())
	{
		if (checkValidObject())
		{
            return dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get())->getAudioFile(0)->toBase64String();
		}
	}

	return {};
}

var ScriptingObjects::ScriptingAudioSampleProcessor::getSampleStart()
{
	if (checkValidObject())
		return dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get())->getAudioFile(0)->getCurrentRange().getStart();

	return 0;
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setSampleRange(int start, int end)
{
	if (checkValidObject())
        return dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get())->getAudioFile(0)->setRange(Range<int>(start, end));
}

var ScriptingObjects::ScriptingAudioSampleProcessor::getAudioFile(int slotIndex)
{
	if (checkValidObject())
	{
		if (auto ed = dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get()))
			return var(new ScriptAudioFile(getScriptProcessor(), slotIndex, ed));
	}

	reportScriptError("Not a valid object");
	RETURN_IF_NO_THROW(var());
}

int ScriptingObjects::ScriptingAudioSampleProcessor::getSampleLength() const
{
	if (checkValidObject())
	{
        return dynamic_cast<ProcessorWithExternalData*>(audioSampleProcessor.get())->getAudioFile(0)->getCurrentRange().getLength();
	}
	else return 0;
}

// ScriptingTableProcessor ==============================================================================================================

struct ScriptingObjects::ScriptingTableProcessor::Wrapper
{
	API_VOID_METHOD_WRAPPER_3(ScriptingTableProcessor, addTablePoint);
	API_VOID_METHOD_WRAPPER_1(ScriptingTableProcessor, reset);
	API_VOID_METHOD_WRAPPER_5(ScriptingTableProcessor, setTablePoint);
	API_METHOD_WRAPPER_1(ScriptingTableProcessor, exportAsBase64);
	API_VOID_METHOD_WRAPPER_2(ScriptingTableProcessor, restoreFromBase64);
	API_METHOD_WRAPPER_1(ScriptingTableProcessor, getTable);
};



ScriptingObjects::ScriptingTableProcessor::ScriptingTableProcessor(ProcessorWithScriptingContent *p, ExternalDataHolder *tableProcessor_) :
ConstScriptingObject(p, dynamic_cast<Processor*>(tableProcessor_) != nullptr ? dynamic_cast<Processor*>(tableProcessor_)->getNumParameters() : 0),
tableProcessor(dynamic_cast<Processor*>(tableProcessor_))
{
	if (tableProcessor != nullptr)
	{
		setName(tableProcessor->getId());

		for (int i = 0; i < tableProcessor->getNumParameters(); i++)
		{
			addConstant(tableProcessor->getIdentifierForParameterIndex(i).toString(), var(i));
		}
	}
	else
	{
		setName("Invalid Processor");
	}

	ADD_API_METHOD_3(addTablePoint);
	ADD_API_METHOD_1(reset);
	ADD_API_METHOD_5(setTablePoint);
	ADD_API_METHOD_1(exportAsBase64);
	ADD_API_METHOD_2(restoreFromBase64);
	ADD_API_METHOD_1(getTable);
}



void ScriptingObjects::ScriptingTableProcessor::setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve)
{
	if (tableProcessor != nullptr)
	{
		if(auto table = dynamic_cast<ExternalDataHolder*>(tableProcessor.get())->getTable(tableIndex))
		{
			table->setTablePoint(pointIndex, x, y, curve);
			return;
		}
	}

	reportScriptError("No table");
}


void ScriptingObjects::ScriptingTableProcessor::addTablePoint(int tableIndex, float x, float y)
{
	if (tableProcessor != nullptr)
	{
		if (auto table = dynamic_cast<ExternalDataHolder*>(tableProcessor.get())->getTable(tableIndex))
		{
			table->addTablePoint(x, y);
			return;
		}
	}

	reportScriptError("No table");
}


void ScriptingObjects::ScriptingTableProcessor::reset(int tableIndex)
{
	if (tableProcessor != nullptr)
	{
		if (auto table = dynamic_cast<ExternalDataHolder*>(tableProcessor.get())->getTable(tableIndex))
		{
			table->reset();
			return;
		}
	}

	reportScriptError("No table");
}

void ScriptingObjects::ScriptingTableProcessor::restoreFromBase64(int tableIndex, const String& state)
{
	if (tableProcessor != nullptr)
	{
		if (auto table = dynamic_cast<ExternalDataHolder*>(tableProcessor.get())->getTable(tableIndex))
		{
			table->restoreData(state);
			return;
		}
	}

	reportScriptError("No table");
}

juce::String ScriptingObjects::ScriptingTableProcessor::exportAsBase64(int tableIndex) const
{
	if (tableProcessor != nullptr)
	{
		if (auto table = dynamic_cast<ExternalDataHolder*>(tableProcessor.get())->getTable(tableIndex))
			return table->exportData();
	}

	reportScriptError("No table");
	RETURN_IF_NO_THROW("");
}

var ScriptingObjects::ScriptingTableProcessor::getTable(int tableIndex)
{
	if (checkValidObject())
	{
		if (auto ed = dynamic_cast<ProcessorWithExternalData*>(tableProcessor.get()))
			return var(new ScriptTableData(getScriptProcessor(), tableIndex, ed));
	}

	reportScriptError("Not a valid object");
	RETURN_IF_NO_THROW(var());
}

struct ScriptingObjects::ScriptSliderPackProcessor::Wrapper
{
	API_METHOD_WRAPPER_1(ScriptSliderPackProcessor, getSliderPack);
};

ScriptingObjects::ScriptSliderPackProcessor::ScriptSliderPackProcessor(ProcessorWithScriptingContent* p, ExternalDataHolder* h) :
	ConstScriptingObject(p, 0),
	sp(dynamic_cast<Processor*>(h))
{
	ADD_API_METHOD_1(getSliderPack);
}

var ScriptingObjects::ScriptSliderPackProcessor::getSliderPack(int sliderPackIndex)
{
	if (checkValidObject())
	{
		if (auto ed = dynamic_cast<ProcessorWithExternalData*>(sp.get()))
			return var(new ScriptSliderPackData(getScriptProcessor(), sliderPackIndex, ed));
	}

	reportScriptError("Not a valid object");
	RETURN_IF_NO_THROW(var());
}



// TimerObject ==============================================================================================================

struct ScriptingObjects::TimerObject::Wrapper
{
	API_METHOD_WRAPPER_0(TimerObject, isTimerRunning);
	API_VOID_METHOD_WRAPPER_1(TimerObject, startTimer);
	API_VOID_METHOD_WRAPPER_0(TimerObject, stopTimer);
	API_VOID_METHOD_WRAPPER_1(TimerObject, setTimerCallback);
	API_METHOD_WRAPPER_0(TimerObject, getMilliSecondsSinceCounterReset);
	API_VOID_METHOD_WRAPPER_0(TimerObject, resetCounter);
};

ScriptingObjects::TimerObject::TimerObject(ProcessorWithScriptingContent *p) :
	ConstScriptingObject(p, 0),
	ControlledObject(p->getMainController_(), true),
	it(this),
	tc(p, this, {}, 0)
{
	ADD_API_METHOD_0(isTimerRunning);
	ADD_API_METHOD_1(startTimer);
	ADD_API_METHOD_0(stopTimer);
	ADD_API_METHOD_1(setTimerCallback);
	ADD_API_METHOD_0(resetCounter);
	ADD_API_METHOD_0(getMilliSecondsSinceCounterReset);
}


ScriptingObjects::TimerObject::~TimerObject()
{
	it.stopTimer();
}

void ScriptingObjects::TimerObject::timerCallback()
{
	if (tc)
		tc.call(nullptr, 0);
	else
		it.stopTimer();
}

hise::DebugInformationBase* ScriptingObjects::TimerObject::getChildElement(int index)
{
	if (index == 0)
	{
		WeakReference<TimerObject> safeThis(this);

		auto vf = [safeThis]()
		{
			if (safeThis != nullptr)
			{
				return var(safeThis->getMilliSecondsSinceCounterReset());
			}

			return var(0);
		};

		Identifier id("%PARENT%.durationSinceReset");
		return new LambdaValueInformation(vf, id, {}, (DebugInformation::Type)getTypeNumber(), getLocation());
	}

	if (index == 1)
	{
		return tc.createDebugObject("timerCallback");
	}
	
    return nullptr;
}

void ScriptingObjects::TimerObject::startTimer(int intervalInMilliSeconds)
{
	if (intervalInMilliSeconds > 10)
	{
		it.startTimer(intervalInMilliSeconds);
		resetCounter();
	}
	else
		throw String("Go easy on the timer");
}

void ScriptingObjects::TimerObject::stopTimer()
{
	it.stopTimer();
}

void ScriptingObjects::TimerObject::setTimerCallback(var callbackFunction)
{
	tc = WeakCallbackHolder(getScriptProcessor(), this, callbackFunction, 0);
	tc.incRefCount();
	tc.setThisObject(this);
	tc.addAsSource(this, "onTimerCallback");
}


bool ScriptingObjects::TimerObject::isTimerRunning() const
{
	return it.isTimerRunning();
}

var ScriptingObjects::TimerObject::getMilliSecondsSinceCounterReset()
{
	auto now = Time::getMillisecondCounter();
	return now - milliSecondCounter;
}

void ScriptingObjects::TimerObject::resetCounter()
{
	milliSecondCounter = Time::getMillisecondCounter();
}

struct ScriptingObjects::ScriptingMessageHolder::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setNoteNumber);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setVelocity);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setControllerNumber);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setControllerValue);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getNoteNumber);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getVelocity);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getControllerNumber);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getControllerValue);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, ignoreEvent);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getEventId);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getChannel);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setType);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setChannel);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setTransposeAmount);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getTransposeAmount);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setCoarseDetune);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getCoarseDetune);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setFineDetune);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getFineDetune);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setGain);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getGain);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, getTimestamp);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setTimestamp);
	API_VOID_METHOD_WRAPPER_1(ScriptingMessageHolder, setStartOffset);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, isNoteOn);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, isNoteOff);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, isController);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, clone);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, dump);
}; 

ScriptingObjects::ScriptingMessageHolder::ScriptingMessageHolder(ProcessorWithScriptingContent* pwsc) :
	ConstScriptingObject(pwsc, (int)HiseEvent::Type::numTypes)
{
	ADD_API_METHOD_1(setNoteNumber);
	ADD_API_METHOD_1(setVelocity);
	ADD_API_METHOD_1(setControllerNumber);
	ADD_API_METHOD_1(setControllerValue);
	ADD_API_METHOD_0(getControllerNumber);
	ADD_API_METHOD_0(getControllerValue);
	ADD_API_METHOD_0(getNoteNumber);
	ADD_API_METHOD_0(getVelocity);
	ADD_API_METHOD_1(ignoreEvent);
	ADD_API_METHOD_0(getEventId);
	ADD_API_METHOD_0(getChannel);
	ADD_API_METHOD_1(setChannel);
	ADD_API_METHOD_0(getGain);
	ADD_API_METHOD_1(setGain);
	ADD_API_METHOD_1(setType);
	ADD_API_METHOD_1(setTransposeAmount);
	ADD_API_METHOD_0(getTransposeAmount);
	ADD_API_METHOD_1(setCoarseDetune);
	ADD_API_METHOD_0(getCoarseDetune);
	ADD_API_METHOD_1(setFineDetune);
	ADD_API_METHOD_0(getFineDetune);
	ADD_API_METHOD_0(getTimestamp);
	ADD_API_METHOD_1(setTimestamp);
	ADD_API_METHOD_0(isNoteOn);
	ADD_API_METHOD_0(isNoteOff);
	ADD_API_METHOD_0(isController);
	ADD_API_METHOD_1(setStartOffset);
	ADD_API_METHOD_0(clone);
	ADD_API_METHOD_0(dump);

	addConstant("Empty", 0);
	addConstant("NoteOn", 1);
	addConstant("NoteOff", 2);
	addConstant("Controller", 3);
	addConstant("PitchBend", 4);
	addConstant("Aftertouch", 5);
	addConstant("AllNotesOff", 6);
	addConstant("SongPosition", 7);
	addConstant("MidiStart", 8);
	addConstant("MidiStop", 9);
	addConstant("VolumeFade", 10);
	addConstant("PitchFade", 11);
	addConstant("TimerEvent", 12);
	addConstant("ProgramChange", 13);
}

int ScriptingObjects::ScriptingMessageHolder::getNoteNumber() const { return (int)e.getNoteNumber(); }
var ScriptingObjects::ScriptingMessageHolder::getControllerNumber() const 
{ 
	return (int)e.getControllerNumber();
}
var ScriptingObjects::ScriptingMessageHolder::getControllerValue() const 
{ 
	if (e.isPitchWheel())
		return e.getPitchWheelValue();
	else
		return (int)e.getControllerValue(); 
}
int ScriptingObjects::ScriptingMessageHolder::getChannel() const { return (int)e.getChannel(); }
void ScriptingObjects::ScriptingMessageHolder::setChannel(int newChannel) { e.setChannel(newChannel); }
void ScriptingObjects::ScriptingMessageHolder::setNoteNumber(int newNoteNumber) { e.setNoteNumber(newNoteNumber); }
void ScriptingObjects::ScriptingMessageHolder::setVelocity(int newVelocity) { e.setVelocity((uint8)newVelocity); }
void ScriptingObjects::ScriptingMessageHolder::setControllerNumber(int newControllerNumber) 
{ 
	if (newControllerNumber == HiseEvent::AfterTouchCCNumber)
		e.setType(HiseEvent::Type::Aftertouch);
	else if (newControllerNumber == HiseEvent::PitchWheelCCNumber)
		e.setType(HiseEvent::Type::PitchBend);
	else
		e.setControllerNumber(newControllerNumber);
}
void ScriptingObjects::ScriptingMessageHolder::setControllerValue(int newControllerValue) 
{ 
	if (e.isPitchWheel())
		e.setPitchWheelValue(newControllerValue);
	else
		e.setControllerValue(newControllerValue); 
}

void ScriptingObjects::ScriptingMessageHolder::setType(int type)
{
	if(isPositiveAndBelow(type, (int)HiseEvent::Type::numTypes))
		e.setType((HiseEvent::Type)type);
	else
		reportScriptError("Unknown Type: " + String(type));
}


int ScriptingObjects::ScriptingMessageHolder::getVelocity() const { return e.getVelocity(); }
void ScriptingObjects::ScriptingMessageHolder::ignoreEvent(bool shouldBeIgnored /*= true*/) { e.ignoreEvent(shouldBeIgnored); }
int ScriptingObjects::ScriptingMessageHolder::getEventId() const { return (int)e.getEventId(); }
void ScriptingObjects::ScriptingMessageHolder::setTransposeAmount(int tranposeValue) { e.setTransposeAmount(tranposeValue); }

juce::var ScriptingObjects::ScriptingMessageHolder::clone()
{
	auto no = new ScriptingMessageHolder(getScriptProcessor());
	no->setMessage(e);
	return var(no);
}

int ScriptingObjects::ScriptingMessageHolder::getTransposeAmount() const { return (int)e.getTransposeAmount(); }
void ScriptingObjects::ScriptingMessageHolder::setCoarseDetune(int semiToneDetune) { e.setCoarseDetune(semiToneDetune); }
int ScriptingObjects::ScriptingMessageHolder::getCoarseDetune() const { return (int)e.getCoarseDetune(); }
void ScriptingObjects::ScriptingMessageHolder::setFineDetune(int cents) { e.setFineDetune(cents); }
int ScriptingObjects::ScriptingMessageHolder::getFineDetune() const { return (int)e.getFineDetune(); }
void ScriptingObjects::ScriptingMessageHolder::setGain(int gainInDecibels) { e.setGain(gainInDecibels); }
int ScriptingObjects::ScriptingMessageHolder::getGain() const { return (int)e.getGain(); }
int ScriptingObjects::ScriptingMessageHolder::getTimestamp() const { return (int)e.getTimeStamp(); }
void ScriptingObjects::ScriptingMessageHolder::setTimestamp(int timestampSamples) { e.setTimeStamp(timestampSamples);}
void ScriptingObjects::ScriptingMessageHolder::addToTimestamp(int deltaSamples) { e.addToTimeStamp((int16)deltaSamples); }
void ScriptingObjects::ScriptingMessageHolder::setStartOffset(int offset) { e.setStartOffset((uint16)offset); }
bool ScriptingObjects::ScriptingMessageHolder::isNoteOn() const { return e.isNoteOn(); }
bool ScriptingObjects::ScriptingMessageHolder::isNoteOff() const { return e.isNoteOff(); }
bool ScriptingObjects::ScriptingMessageHolder::isController() const { return e.isController() || e.isPitchWheel() || e.isAftertouch(); }

String ScriptingObjects::ScriptingMessageHolder::dump() const
{

	String x;
	x << "Type: " << e.getTypeAsString() << ", ";
	x << "Channel: " << String(e.getChannel()) << ", ";

	if (e.isPitchWheel())
	{
		x << "Value: " << String(e.getPitchWheelValue()) << ", ";
	}
	else
	{
		x << "Number: " << String(e.getNoteNumber()) << ", ";
		x << "Value: " << String(e.getVelocity()) << ", ";
		x << "EventId: " << String(e.getEventId()) << ", ";
	}

	
	
	x << "Timestamp: " << String(e.getTimeStamp()) << ", ";

	return x;
}



ApiHelpers::ModuleHandler::ModuleHandler(Processor* parent_, JavascriptProcessor* sp) :
	parent(parent_),
	scriptProcessor(sp)
{
#if USE_BACKEND

	auto console = parent != nullptr ? parent->getMainController()->getConsoleHandler().getMainConsole() : nullptr;

	if (console)
		mainEditor = GET_BACKEND_ROOT_WINDOW(console);

#else
	mainEditor = nullptr;
#endif
}

ApiHelpers::ModuleHandler::~ModuleHandler()
{
	
}




bool ApiHelpers::ModuleHandler::removeModule(Processor* p)
{
	if (p == nullptr)
		return true;

	if (p->getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::AudioThread)
	{
		throw String("Effects can't be removed from the audio thread!");
	}

	if (p != nullptr)
	{
		auto removeFunction = [](Processor* p)
		{
			auto c = dynamic_cast<Chain*>(p->getParentProcessor(false));

			jassert(c != nullptr);

			if (c == nullptr)
				return SafeFunctionCall::cancelled;

			// Remove it but don't delete it
			c->getHandler()->remove(p, false);

			return SafeFunctionCall::OK;
		};

		parent->getMainController()->getGlobalAsyncModuleHandler().removeAsync(p, removeFunction);
		return true;
	}
	else
		return false;
}

Processor* ApiHelpers::ModuleHandler::addModule(Chain* c, const String& type, const String& id, int index /*= -1*/)
{
	WARN_IF_AUDIO_THREAD(true, IllegalAudioThreadOps::HeapBlockAllocation);

	for (int i = 0; i < c->getHandler()->getNumProcessors(); i++)
	{
		if (c->getHandler()->getProcessor(i)->getId() == id)
		{
			return c->getHandler()->getProcessor(i);
		}
	}

	SuspendHelpers::ScopedTicket ticket(parent->getMainController());

	parent->getMainController()->getJavascriptThreadPool().killVoicesAndExtendTimeOut(getScriptProcessor());

	LockHelpers::freeToGo(parent->getMainController());

	ScopedPointer<Processor> newProcessor = parent->getMainController()->createProcessor(c->getFactoryType(), type, id);

	if (newProcessor == nullptr)
		throw String("Module with type " + type + " could not be generated.");

	// Now we're safe...
	Processor* pFree = newProcessor.release();

	auto addFunction = [c, index](Processor* p)
	{
		if (c == nullptr)
		{
			delete p; // Rather bad...
			jassertfalse;
			return SafeFunctionCall::OK;
		}

		if (index >= 0 && index < c->getHandler()->getNumProcessors())
		{
			Processor* sibling = c->getHandler()->getProcessor(index);
			c->getHandler()->add(p, sibling);
		}
		else
			c->getHandler()->add(p, nullptr);

		return SafeFunctionCall::OK;
	};
	




	parent->getMainController()->getGlobalAsyncModuleHandler().addAsync(pFree, addFunction);

	// will be owned by the job, then by the handler...
	return pFree;
}

hise::Modulator* ApiHelpers::ModuleHandler::addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator, const String& modName, bool connectAsStaticMod/*=false*/)
{
	if (globalModulator == nullptr)
		throw String("Global Modulator does not exist");

	if (auto container = dynamic_cast<GlobalModulatorContainer*>(ProcessorHelpers::findParentProcessor(globalModulator, true)))
	{
		GlobalModulator* m = nullptr;

		if (dynamic_cast<VoiceStartModulator*>(globalModulator) != nullptr)
		{
			auto vMod = addModule(c, GlobalVoiceStartModulator::getClassType().toString(), modName);
			m = dynamic_cast<GlobalModulator*>(vMod);
		}
		else if (dynamic_cast<TimeVariantModulator*>(globalModulator) != nullptr)
		{
			if (connectAsStaticMod)
			{
				auto tMod = addModule(c, GlobalStaticTimeVariantModulator::getClassType().toString(), modName);
				m = dynamic_cast<GlobalModulator*>(tMod);
			}
			else
			{
				auto tMod = addModule(c, GlobalTimeVariantModulator::getClassType().toString(), modName);
				m = dynamic_cast<GlobalModulator*>(tMod);
			}
		}
		else
			throw String("Not a global modulator");

		if (m == nullptr)
			throw String("Global modulator can't be created");

		auto entry = container->getId() + ":" + globalModulator->getId();

		m->connectToGlobalModulator(entry);

		if (!m->isConnected())
		{
			throw String("Can't connect to global modulator");
		}

		auto returnMod = dynamic_cast<Modulator*>(m);

#if USE_BACKEND
		returnMod->sendChangeMessage();
#endif

		return returnMod;
	}
	else
		throw String("The modulator you passed in is not a global modulator. You must specify a modulator in a Global Modulator Container");
}

struct ScriptingObjects::ScriptedMidiPlayer::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getPlaybackPosition);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setPlaybackPosition);
	API_METHOD_WRAPPER_1(ScriptedMidiPlayer, getNoteRectangleList);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, connectToPanel);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setRepaintOnPositionChange);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, flushMessageList);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getEventList);
	API_METHOD_WRAPPER_2(ScriptedMidiPlayer, convertEventListToNoteRectangles);
	API_METHOD_WRAPPER_2(ScriptedMidiPlayer, saveAsMidiFile);
	API_VOID_METHOD_WRAPPER_0(ScriptedMidiPlayer, reset);
	API_VOID_METHOD_WRAPPER_0(ScriptedMidiPlayer, undo);
	API_VOID_METHOD_WRAPPER_0(ScriptedMidiPlayer, redo);
	API_METHOD_WRAPPER_1(ScriptedMidiPlayer, play);
	API_METHOD_WRAPPER_1(ScriptedMidiPlayer, stop);
	API_METHOD_WRAPPER_1(ScriptedMidiPlayer, record);
	API_METHOD_WRAPPER_3(ScriptedMidiPlayer, setFile);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setTrack);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setSequence);
	API_VOID_METHOD_WRAPPER_3(ScriptedMidiPlayer, create);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getMidiFileList);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, isEmpty);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getNumTracks);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getNumSequences);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getTicksPerQuarter);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setUseTimestampInTicks);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getTimeSignature);
	API_METHOD_WRAPPER_1(ScriptedMidiPlayer, setTimeSignature);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getLastPlayedNotePosition);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setSyncToMasterClock);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setSequenceCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setAutomationHandlerConsumesControllerEvents);
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, asMidiProcessor);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setGlobalPlaybackRatio);
	API_VOID_METHOD_WRAPPER_2(ScriptedMidiPlayer, setPlaybackCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setRecordEventCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, setUseGlobalUndoManager);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiPlayer, connectToMetronome);
};

ScriptingObjects::ScriptedMidiPlayer::ScriptedMidiPlayer(ProcessorWithScriptingContent* p, MidiPlayer* player_):
	MidiPlayerBaseType(player_),
	ConstScriptingObject(p, 0),
	updateCallback(p, this, var(), 1)
{
	ADD_API_METHOD_0(getPlaybackPosition);
	ADD_API_METHOD_1(setPlaybackPosition);
	ADD_API_METHOD_1(getNoteRectangleList);
	ADD_API_METHOD_1(connectToPanel);
	ADD_API_METHOD_1(setRepaintOnPositionChange);
	ADD_API_METHOD_0(getEventList);
	ADD_API_METHOD_1(flushMessageList);
	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_0(undo);
	ADD_API_METHOD_0(redo);
	ADD_API_METHOD_1(play);
	ADD_API_METHOD_2(convertEventListToNoteRectangles);
	ADD_API_METHOD_1(stop);
	ADD_API_METHOD_1(record);
	ADD_API_METHOD_3(setFile);
	ADD_API_METHOD_2(saveAsMidiFile);
	ADD_API_METHOD_0(getMidiFileList);
	ADD_API_METHOD_1(setTrack);
	ADD_API_METHOD_1(setSequence);
	ADD_API_METHOD_0(isEmpty);
	ADD_API_METHOD_3(create);
	ADD_API_METHOD_0(getNumTracks);
	ADD_API_METHOD_0(getNumSequences);
	ADD_API_METHOD_0(getTimeSignature);
	ADD_API_METHOD_1(setTimeSignature);
	ADD_API_METHOD_1(setSyncToMasterClock);
	ADD_API_METHOD_1(setUseTimestampInTicks);
	ADD_API_METHOD_0(getTicksPerQuarter);
	ADD_API_METHOD_0(getLastPlayedNotePosition);
	ADD_API_METHOD_1(setAutomationHandlerConsumesControllerEvents);
	ADD_API_METHOD_1(setSequenceCallback);
	ADD_API_METHOD_0(asMidiProcessor);
	ADD_API_METHOD_1(setGlobalPlaybackRatio);
	ADD_API_METHOD_2(setPlaybackCallback);
	ADD_API_METHOD_1(setRecordEventCallback);
	ADD_API_METHOD_1(setUseGlobalUndoManager);
	ADD_API_METHOD_1(connectToMetronome);
}

ScriptingObjects::ScriptedMidiPlayer::~ScriptedMidiPlayer()
{
	cancelUpdates();
	connectedPanel = nullptr;
	recordEventProcessor = nullptr;
	playbackUpdater = nullptr;
}

juce::String ScriptingObjects::ScriptedMidiPlayer::getDebugValue() const
{
	if (!sequenceValid())
		return {};

	return String(getPlayer()->getPlaybackPosition(), 2);
}

void ScriptingObjects::ScriptedMidiPlayer::sequencesCleared()
{
	callUpdateCallback();

	if (auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(connectedPanel.get()))
	{
		panel->repaint();
	}
}

void ScriptingObjects::ScriptedMidiPlayer::timerCallback()
{
	if (repaintOnPlaybackChange && ((double)getPlaybackPosition() != lastPlaybackChange))
	{
		lastPlaybackChange = getPlaybackPosition();

		if (auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(connectedPanel.get()))
		{
			panel->repaint();
		}
	}
}



var ScriptingObjects::ScriptedMidiPlayer::getNoteRectangleList(var targetBounds)
{
	if (!sequenceValid())
		return {};

	Result r = Result::ok();

	auto rect = ApiHelpers::getRectangleFromVar(targetBounds, &r);

	auto list = getSequence()->getRectangleList(rect);

	Array<var> returnArray;

	for (auto re : list)
		returnArray.add(ApiHelpers::getVarRectangle(re, &r));

	return var(returnArray);
}

juce::var ScriptingObjects::ScriptedMidiPlayer::convertEventListToNoteRectangles(var eventList, var targetBounds)
{
	if (auto holderList = eventList.getArray())
	{
		HiseMidiSequence::Ptr dummySequence = new HiseMidiSequence();

		dummySequence->setTimeStampEditFormat(getSequence()->getTimestampEditFormat());
		dummySequence->createEmptyTrack();

		Array<HiseEvent> eventList;

		for (const auto& h : *holderList)
		{
			if (auto mh = dynamic_cast<ScriptingMessageHolder*>(h.getObject()))
				eventList.add(mh->getMessageCopy());
		}

		MidiPlayer::EditAction::writeArrayToSequence(dummySequence, eventList, 120, 44100.0, getSequence()->getTimestampEditFormat());

		auto r = Result::ok();

		auto targetArea = ApiHelpers::getRectangleFromVar(targetBounds, &r);

		if (!r.wasOk())
			reportScriptError(r.getErrorMessage());

		auto list = dummySequence->getRectangleList(targetArea);

		Array<var> returnArray;

		for (auto re : list)
			returnArray.add(ApiHelpers::getVarRectangle(re, &r));

		dummySequence = nullptr;

		return var(returnArray);
	}

	return var();
}

void ScriptingObjects::ScriptedMidiPlayer::setPlaybackPosition(var newPosition)
{
	if (!sequenceValid())
		return;

	getPlayer()->setAttribute(MidiPlayer::CurrentPosition, jlimit<float>(0.0f, 1.0f, (float)newPosition), sendNotification);

}

var ScriptingObjects::ScriptedMidiPlayer::getPlaybackPosition()
{
	if (!sequenceValid())
		return 0.0;

	return getPlayer()->getPlaybackPosition();
}

juce::var ScriptingObjects::ScriptedMidiPlayer::getLastPlayedNotePosition() const
{
	if (getPlayer()->getPlayState() == MidiPlayer::PlayState::Stop)
		return -1;

	if (auto seq = getPlayer()->getCurrentSequence())
	{
		return seq->getLastPlayedNotePosition();
	}

	return 0;
}

void ScriptingObjects::ScriptedMidiPlayer::setSyncToMasterClock(bool shouldSyncToMasterClock)
{
	if (shouldSyncToMasterClock && !getScriptProcessor()->getMainController_()->getMasterClock().isGridEnabled())
	{
		reportScriptError("You have to enable the master clock before using this method");
	}
	else
	{
		getPlayer()->setSyncToMasterClock(shouldSyncToMasterClock);
	}
}

void ScriptingObjects::ScriptedMidiPlayer::setRepaintOnPositionChange(var shouldRepaintPanel)
{
	if ((bool)shouldRepaintPanel != repaintOnPlaybackChange)
	{
		repaintOnPlaybackChange = (bool)shouldRepaintPanel;

		if (repaintOnPlaybackChange)
			startTimer(50);
		else
			stopTimer();
	}
}

void ScriptingObjects::ScriptedMidiPlayer::setUseGlobalUndoManager(bool shouldUseGlobalUndoManager)
{
	if (shouldUseGlobalUndoManager)
		getPlayer()->setExternalUndoManager(getScriptProcessor()->getMainController_()->getControlUndoManager());
	else
		getPlayer()->setExternalUndoManager(nullptr);
}

void ScriptingObjects::ScriptedMidiPlayer::setRecordEventCallback(var recordEventCallback)
{
	if (auto co = dynamic_cast<WeakCallbackHolder::CallableObject*>(recordEventCallback.getObject()))
	{
		if (!co->isRealtimeSafe())
			reportScriptError("This callable object is not realtime safe!");

		recordEventProcessor = nullptr;
		recordEventProcessor = new ScriptEventRecordProcessor(*this, recordEventCallback);
	}
	else
	{
		reportScriptError("You need to pass in an inline function");
	}
}

void ScriptingObjects::ScriptedMidiPlayer::connectToPanel(var panel)
{
	if (auto p = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(panel.getObject()))
	{
		connectedPanel = dynamic_cast<ConstScriptingObject*>(p);
	}
	else
		reportScriptError("Invalid panel");
}

void ScriptingObjects::ScriptedMidiPlayer::connectToMetronome(var metronome)
{
	if (metronome.isString())
	{
		auto m = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), metronome.toString());

		if (auto typed = dynamic_cast<MidiMetronome*>(m))
			typed->connectToPlayer(getPlayer());
		else
			reportScriptError("Can't find metronome FX with ID " + metronome.toString());
	}
}

var ScriptingObjects::ScriptedMidiPlayer::getEventList()
{
	Array<var> eventHolders;

	if (!sequenceValid())
		return var(eventHolders);

	auto sr = getPlayer()->getSampleRate();
	auto bpm = getPlayer()->getMainController()->getBpm();

	getPlayer()->getCurrentSequence()->setTimeStampEditFormat(useTicks ? HiseMidiSequence::TimestampEditFormat::Ticks : HiseMidiSequence::TimestampEditFormat::Samples);

	auto list = getPlayer()->getCurrentSequence()->getEventList(sr, bpm);

	for (const auto& e : list)
	{
		ScopedPointer<ScriptingMessageHolder> holder = new ScriptingMessageHolder(getScriptProcessor());
		holder->setMessage(e);
		eventHolders.add(holder.release());
	}

	return var(eventHolders);
}

void ScriptingObjects::ScriptedMidiPlayer::flushMessageList(var messageList)
{
	if (!sequenceValid())
		return;

	if (auto ar = messageList.getArray())
	{
		Array<HiseEvent> events;

		for (auto e : *ar)
		{
			if (auto holder = dynamic_cast<ScriptingMessageHolder*>(e.getObject()))
				events.add(holder->getMessageCopy());
			else
				reportScriptError("Illegal item in message list: " + e.toString());
		}

		if (auto seq = getPlayer()->getCurrentSequence())
			seq->setTimeStampEditFormat(useTicks ? HiseMidiSequence::TimestampEditFormat::Ticks : HiseMidiSequence::TimestampEditFormat::Samples);

		getPlayer()->flushEdit(events);
	}
	else
		reportScriptError("Input is not an array");
}

void ScriptingObjects::ScriptedMidiPlayer::setUseTimestampInTicks(bool shouldUseTimestamps)
{
	useTicks = shouldUseTimestamps;
}

int ScriptingObjects::ScriptedMidiPlayer::getTicksPerQuarter() const
{
	return HiseMidiSequence::TicksPerQuarter;
}

void ScriptingObjects::ScriptedMidiPlayer::create(int nominator, int denominator, int barLength)
{
	HiseMidiSequence::TimeSignature sig;

	sig.nominator = nominator;
	sig.denominator = denominator;
	sig.numBars = barLength;
	sig.normalisedLoopRange = { 0.0, 1.0 };
	
	HiseMidiSequence::Ptr seq = new HiseMidiSequence();
	seq->setLengthFromTimeSignature(sig);
	seq->createEmptyTrack();
	getPlayer()->addSequence(seq);
}

bool ScriptingObjects::ScriptedMidiPlayer::isEmpty() const
{
	return !sequenceValid();
}

void ScriptingObjects::ScriptedMidiPlayer::reset()
{
	if (!sequenceValid())
		return;

	getPlayer()->resetCurrentSequence();
}

void ScriptingObjects::ScriptedMidiPlayer::undo()
{
	if (!sequenceValid())
		return;

	if (auto um = getPlayer()->getUndoManager())
		um->undo();
	else
		reportScriptError("Undo is deactivated");
}

void ScriptingObjects::ScriptedMidiPlayer::redo()
{
	if (!sequenceValid())
		return;

	if (auto um = getPlayer()->getUndoManager())
		um->redo();
	else
		reportScriptError("Undo is deactivated");
}

bool ScriptingObjects::ScriptedMidiPlayer::play(int timestamp)
{
	if (auto pl = getPlayer())
		return pl->play(timestamp);

	return false;
}

bool ScriptingObjects::ScriptedMidiPlayer::stop(int timestamp)
{
	if (auto pl = getPlayer())
		return pl->stop(timestamp);

	return false;
}

bool ScriptingObjects::ScriptedMidiPlayer::record(int timestamp)
{
	if (auto pl = getPlayer())
		return pl->record(timestamp);

	return false;
}

bool ScriptingObjects::ScriptedMidiPlayer::setFile(var fileName, bool clearExistingSequences, bool selectNewSequence)
{
	if (auto pl = getPlayer())
	{
		if (clearExistingSequences)
			pl->clearSequences(dontSendNotification);

		auto name = ScriptFile::getFileNameFromFile(fileName);

		if (!name.isEmpty())
		{
			PoolReference r(pl->getMainController(), fileName, FileHandlerBase::MidiFiles);
			pl->loadMidiFile(r);
			if (selectNewSequence)
				pl->setAttribute(MidiPlayer::CurrentSequence, (float)pl->getNumSequences(), sendNotification);

			return r.isValid();

		}
		else
		{
			if(selectNewSequence)
				pl->sendSequenceUpdateMessage(sendNotificationAsync);

			// if it's empty, we don't want to load anything, so we "succeeded".
			return true;
		}
	}
		
	return false;
}

bool ScriptingObjects::ScriptedMidiPlayer::saveAsMidiFile(var fileName, int trackIndex)
{
	if (auto pl = getPlayer())
	{
		auto name = ScriptFile::getFileNameFromFile(fileName);

		if (name.isNotEmpty())
			return pl->saveAsMidiFile(name, trackIndex);
		else
			reportScriptError("Can't parse file name");
	}

	return false;	
}

var ScriptingObjects::ScriptedMidiPlayer::getMidiFileList()
{
	auto list = getProcessor()->getMainController()->getCurrentFileHandler().pool->getMidiFilePool().getListOfAllReferences(true);

	Array<var> l;

	for (auto ref : list)
	{
		l.add(ref.getReferenceString());
	}

	return l;
}

void ScriptingObjects::ScriptedMidiPlayer::setTrack(int trackIndex)
{
	if (auto pl = getPlayer())
		pl->setAttribute(MidiPlayer::CurrentTrack, (float)trackIndex, sendNotification);
}

void ScriptingObjects::ScriptedMidiPlayer::setSequence(int sequenceIndex)
{
	if (auto pl = getPlayer())
		pl->setAttribute(MidiPlayer::CurrentSequence, (float)sequenceIndex, sendNotification);
}

int ScriptingObjects::ScriptedMidiPlayer::getNumSequences()
{
	if (auto pl = getPlayer())
		return pl->getNumSequences();

	return 0;
}



var ScriptingObjects::ScriptedMidiPlayer::getTimeSignature()
{
	if (sequenceValid())
	{
		auto sig = getSequence()->getTimeSignature();

		return sig.getAsJSON();
	}

	return {};
}

bool ScriptingObjects::ScriptedMidiPlayer::setTimeSignature(var timeSignatureObject)
{
	if (sequenceValid())
	{
		HiseMidiSequence::TimeSignature sig;

		sig.nominator = timeSignatureObject.getProperty(TimeSigIds::Nominator, 0);
		sig.denominator = timeSignatureObject.getProperty(TimeSigIds::Denominator, 0);
		sig.numBars = timeSignatureObject.getProperty(TimeSigIds::NumBars, 0);

		sig.normalisedLoopRange = { (double)timeSignatureObject.getProperty(TimeSigIds::LoopStart, 0.0),
									(double)timeSignatureObject.getProperty(TimeSigIds::LoopEnd, 1.0) };

		bool valid = sig.numBars > 0 && sig.nominator > 0 && sig.denominator > 0;

		if(valid)
			getSequence()->setLengthFromTimeSignature(sig);

		return valid;
	}

	return false;
}

void ScriptingObjects::ScriptedMidiPlayer::setAutomationHandlerConsumesControllerEvents(bool shouldBeEnabled)
{
	if (auto player = getPlayer())
	{
		player->setMidiControlAutomationHandlerConsumesControllerEvents(shouldBeEnabled);
	}
}

void ScriptingObjects::ScriptedMidiPlayer::setSequenceCallback(var updateFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(updateFunction))
	{
		updateCallback = WeakCallbackHolder(getScriptProcessor(), this, updateFunction, 1);
		updateCallback.incRefCount();
		updateCallback.addAsSource(this, "onMidiSequenceUpdate");

		callUpdateCallback();
	}
}

void ScriptingObjects::ScriptedMidiPlayer::setPlaybackCallback(var newPlaybackCallback, var synchronous)
{
	playbackUpdater = nullptr;

    bool isSync = ApiHelpers::isSynchronous(synchronous);
    
	if (HiseJavascriptEngine::isJavascriptFunction(newPlaybackCallback))
	{
		playbackUpdater = new PlaybackUpdater(*this, newPlaybackCallback, isSync);
	}
}

juce::var ScriptingObjects::ScriptedMidiPlayer::asMidiProcessor()
{
	if (auto p = getPlayer())
	{
		return var(new ScriptingMidiProcessor(getScriptProcessor(), p));
	}

	return var();
}

void ScriptingObjects::ScriptedMidiPlayer::setGlobalPlaybackRatio(double globalRatio)
{
	getScriptProcessor()->getMainController_()->setGlobalMidiPlaybackSpeed(globalRatio);
}

void ScriptingObjects::ScriptedMidiPlayer::callUpdateCallback()
{
	if (updateCallback)
	{
		var thisVar(this);

		updateCallback.call(&thisVar, 1);
	}
}

void ScriptingObjects::ScriptedMidiPlayer::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{
	callUpdateCallback();
	
}

int ScriptingObjects::ScriptedMidiPlayer::getNumTracks()
{
	if (auto pl = getPlayer())
	{
		if (auto seq = pl->getCurrentSequence())
			return seq->getNumTracks();
	}

	return 0;
}




var ApiHelpers::getVarFromPoint(Point<float> pos)
{
	Array<var> p;
	p.add(pos.getX());
	p.add(pos.getY());
	return var(p);
}

juce::Array<juce::Identifier> ApiHelpers::getGlobalApiClasses()
{

	static const Array<Identifier> ids =
	{
		"Engine",
		"Console",
		"Content",
        "Colours",
		"Sampler",
		"Synth",
		"Math",
		"Settings",
		"Server",
		"FileSystem",
		"Message",
		"Date"
	};
	
	return ids;
}

void ApiHelpers::loadPathFromData(Path& p, var data)
{
	
	if (data.isString())
	{
		juce::MemoryBlock mb;
		mb.fromBase64Encoding(data.toString());
		p.clear();
		p.loadPathFromData(mb.getData(), mb.getSize());
	}
	else if (data.isArray())
	{
		p.clear();
		Array<unsigned char> pathData;
		Array<var> *varData = data.getArray();
		const int numElements = varData->size();

		pathData.ensureStorageAllocated(numElements);

		for (int i = 0; i < numElements; i++)
			pathData.add(static_cast<unsigned char>((int)varData->getUnchecked(i)));

		p.loadPathFromData(pathData.getRawDataPointer(), numElements);
	}
	else if (auto sp = dynamic_cast<ScriptingObjects::PathObject*>(data.getObject()))
	{
		p = sp->getPath();
	}
}

juce::PathStrokeType ApiHelpers::createPathStrokeType(var strokeType)
{
	PathStrokeType s(1.0f);

	if (auto obj = strokeType.getDynamicObject())
	{
		static const StringArray endcaps = { "butt", "square", "rounded" };
		static const StringArray jointStyles = { "mitered", "curved","beveled" };

		auto endCap = (PathStrokeType::EndCapStyle)endcaps.indexOf(obj->getProperty("EndCapStyle").toString());
		auto jointStyle = (PathStrokeType::JointStyle)jointStyles.indexOf(obj->getProperty("JointStyle").toString());
		auto thickness = (float)obj->getProperty("Thickness");

		s = PathStrokeType(SANITIZED(thickness), jointStyle, endCap);
	}
	else
	{
		auto t = (float)strokeType;
		s = PathStrokeType(SANITIZED(t));
	}

	return s;
}

#if USE_BACKEND
juce::ValueTree ApiHelpers::getApiTree()
{
	static ValueTree v;

	if (!v.isValid())
		v = ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize);

	return v;
}
#endif



struct ScriptingObjects::ScriptDisplayBufferSource::Wrapper
{
	API_METHOD_WRAPPER_1(ScriptDisplayBufferSource, getDisplayBuffer);
};

ScriptingObjects::ScriptDisplayBufferSource::ScriptDisplayBufferSource(ProcessorWithScriptingContent *p, ProcessorWithExternalData *h):
	ConstScriptingObject(p, 0),
	source(h)
{
	ADD_API_METHOD_1(getDisplayBuffer);
}

var ScriptingObjects::ScriptDisplayBufferSource::getDisplayBuffer(int index)
{
	if (objectExists())
	{
		auto numObjects = source->getNumDataObjects(ExternalData::DataType::DisplayBuffer);

		if (isPositiveAndBelow(index, numObjects))
			return var(new ScriptingObjects::ScriptRingBuffer(getScriptProcessor(), index, dynamic_cast<ProcessorWithExternalData*>(source.get())));

		reportScriptError("Can't find buffer at index " + String(index));
	}
	
	RETURN_IF_NO_THROW({});
}


struct ScriptingObjects::ScriptUnorderedStack::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptUnorderedStack, isEmpty);
	API_METHOD_WRAPPER_0(ScriptUnorderedStack, size);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, asBuffer);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, insert);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, remove);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, removeElement);
	API_METHOD_WRAPPER_0(ScriptUnorderedStack, clear);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, contains);
	API_METHOD_WRAPPER_2(ScriptUnorderedStack, storeEvent);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, removeIfEqual);
	API_METHOD_WRAPPER_1(ScriptUnorderedStack, copyTo);
	API_VOID_METHOD_WRAPPER_2(ScriptUnorderedStack, setIsEventStack);
};

ScriptingObjects::ScriptUnorderedStack::ScriptUnorderedStack(ProcessorWithScriptingContent *p):
	ConstScriptingObject(p, 5),
	compareFunction(p, this, var(), 2)
{
	ADD_API_METHOD_0(isEmpty);
	ADD_API_METHOD_0(size);
	ADD_API_METHOD_1(asBuffer);
	ADD_API_METHOD_1(insert);
	ADD_API_METHOD_1(remove);
	ADD_API_METHOD_1(removeElement);
	ADD_API_METHOD_1(contains);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_2(setIsEventStack);
	ADD_API_METHOD_2(storeEvent);
	ADD_API_METHOD_1(removeIfEqual);
	ADD_API_METHOD_1(copyTo);

	elementBuffer = new VariantBuffer(data.begin(), 0);
	wholeBf = new VariantBuffer(data.begin(), 128);

	addConstant("BitwiseEqual",					 (int)CompareFunctions::BitwiseEqual);
	addConstant("EventId",						 (int)CompareFunctions::EventId);
	addConstant("NoteNumberAndVelocity",		 (int)CompareFunctions::NoteNumberAndVelocity);
	addConstant("NoteNumberAndChannel",			 (int)CompareFunctions::NoteNumberAndChannel);
	addConstant("EqualData",					 (int)CompareFunctions::EqualData);
}

struct ScriptingObjects::ScriptUnorderedStack::Display : public Component,
														 public Timer
{
	static constexpr int CellWidth = 70;
	static constexpr int CellHeight = 22;
	static constexpr int EventCellWidth = 500;

	static constexpr int NumColumns = 8;
	static constexpr int EventNumColumns = 1;
	
	

	void timerCallback() override { repaint(); }

	Display(ScriptUnorderedStack* p):
		parent(p)
	{
		auto isEventStack = parent->isEventStack;

		auto w = isEventStack ? EventCellWidth : CellWidth;
		auto h = CellHeight;
		auto NumColumnsToUse = isEventStack ? EventNumColumns : NumColumns;
		int NumRows = 128 / NumColumnsToUse;

		setSize(NumColumnsToUse * w, NumRows * h);
		setName(isEventStack ? "Event Stack" : "Float Stack");
		startTimer(30);
	}

	void paint(Graphics& g) override
	{
		if (parent.get() == nullptr)
		{
			g.setColour(Colours::white.withAlpha(0.8f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Refresh this window after recompiling", getLocalBounds().toFloat(), Justification::centred);
			return;
		}

		auto isEventStack = parent->isEventStack;

		auto w = isEventStack ? EventCellWidth : CellWidth;
		auto h = CellHeight;
		auto NumColumnsToUse = isEventStack ? EventNumColumns : NumColumns;
		int NumRows = 128 / NumColumnsToUse;

		int index = 0;

		for (int y = 0; y < NumRows; y++)
		{
			for (int x = 0; x < NumColumnsToUse; x++)
			{
				Rectangle<int> ar(x * w, y * h, w, h);
				
				if (index < parent->size())
				{
					g.setColour(Colours::white.withAlpha(0.2f));
					g.fillRect(ar.reduced(1));

					String text;

					if (isEventStack)
					{
						auto e = *(parent->eventData.begin() + index);
						text = e.toDebugString();
					}
					else
					{
						float v = *(parent->data.begin() + index);
						text = String(v, 1);
					}

					g.setColour(Colours::white.withAlpha(0.8f));
					g.setFont(GLOBAL_MONOSPACE_FONT());
					g.drawText(text, ar.toFloat(), Justification::centred);
				}
				else
				{
					g.setColour(Colours::white.withAlpha(0.05f));
					g.fillRect(ar.reduced(1));
				}

				index++;
			}
		}
	}

	WeakReference<ScriptUnorderedStack> parent;
};

Component* ScriptingObjects::ScriptUnorderedStack::createPopupComponent(const MouseEvent& e, Component *c)
{
#if USE_BACKEND
	auto v = new Display(this);

	if (v->getHeight() > 400)
	{
		auto vp = new Viewport();
		vp->setViewedComponent(v, true);
		vp->setSize(v->getWidth() + vp->getScrollBarThickness(), 400);
		vp->setName(v->getName());

		return vp;
	}

	return v;

#else
	ignoreUnused(e, c);
	return nullptr;
#endif
}




bool ScriptingObjects::ScriptUnorderedStack::copyTo(var target)
{
	if (target.isArray())
	{
		target.getArray()->clear();
		target.getArray()->ensureStorageAllocated(size());

		if (isEventStack)
		{
			for (const auto& e : eventData)
			{
				auto m = new ScriptingMessageHolder(getScriptProcessor());
				m->setMessage(e);
				target.append(var(m));
			}
		}
		else
		{
			for (const auto& v : data)
				target.append(var(v));
		}

		return true;
	}

	if (target.isBuffer())
	{
		if (isEventStack)
		{
			reportScriptError("Can't copy event stack to buffer");
			return false;
		}
		else
		{
			auto b = target.getBuffer();

			if (isPositiveAndBelow(data.size(), b->size))
			{
				b->buffer.clear();
				FloatVectorOperations::copy(b->buffer.getWritePointer(0), data.begin(), data.size());
				return true;
			}

			return false;
		}
	}

	if (auto otherStack = dynamic_cast<ScriptUnorderedStack*>(target.getObject()))
	{
		if (isEventStack == otherStack->isEventStack)
		{
			if (isEventStack)
			{
				otherStack->eventData.clearQuick();
				
				for (const auto& e : eventData)
					otherStack->eventData.insertWithoutSearch(e);

				return true;
			}
			else
			{
				otherStack->data.clearQuick();

				for (const auto& v : data)
					otherStack->data.insertWithoutSearch(v);

				return true;
			}
		}
	}

	reportScriptError("No valid container");
	RETURN_IF_NO_THROW(false);
}

bool ScriptingObjects::ScriptUnorderedStack::storeEvent(int index, var holder)
{
	if (!isEventStack)
	{
		reportScriptError("storeEvent does not work with float number stack");
		RETURN_IF_NO_THROW(false);
	}

	if (auto m = dynamic_cast<ScriptingMessageHolder*>(holder.getObject()))
	{
		if (isPositiveAndBelow(index, size()))
		{
			m->setMessage(eventData[index]);
			return true;
		}
		
		return false;
	}
	else
		reportScriptError("holder must be a MessageHolder");

	RETURN_IF_NO_THROW(false);
}

bool ScriptingObjects::ScriptUnorderedStack::removeIfEqual(var holder)
{
	if (!isEventStack)
	{
		reportScriptError("removeIfEqual does not work with float number stack");
		RETURN_IF_NO_THROW(false);
	}

	auto idx = getIndexForEvent(holder);

	if (idx != -1)
	{
		auto eventFromStack = eventData[idx];
		eventData.removeElement(idx);
		dynamic_cast<ScriptingMessageHolder*>(holder.getObject())->setMessage(eventFromStack);
		return true;
	}

	return false;
}

bool ScriptingObjects::ScriptUnorderedStack::insert(var value)
{
	if (isEventStack)
	{
		if (auto m = dynamic_cast<ScriptingMessageHolder*>(value.getObject()))
			return eventData.insert(m->getMessageCopy());

		return false;
	}
	else
	{
		auto ok = data.insert(value);
		updateElementBuffer();
		return ok;
	}
}

int ScriptingObjects::ScriptUnorderedStack::getIndexForEvent(var value) const
{
	if (auto m = dynamic_cast<ScriptingMessageHolder*>(value.getObject()))
	{
		int numUsed = eventData.size();

		if (compareFunctionType == CompareFunctions::Custom)
		{
			var args[2];
			args[0] = var(compareHolder.get());
			args[1] = value;

			for (int i = 0; i < numUsed; i++)
			{
				compareHolder->setMessage(eventData[i]);
				var rv;

				auto cf = const_cast<WeakCallbackHolder*>(&compareFunction);

				auto r = cf->callSync(args, 2, &rv);

				if (!r.wasOk())
					reportScriptError(r.getErrorMessage());

				if ((bool)rv)
					return i;
			}
		}
		else
		{
			auto e1 = m->getMessageCopy();

			for (int i = 0; i < numUsed; i++)
			{
				if (hcf(e1, eventData[i]))
					return i;
			}
		}
	}

	return -1;
}



bool ScriptingObjects::ScriptUnorderedStack::remove(var value)
{
	if (isEventStack)
	{
		auto index = getIndexForEvent(value);

		if (index != -1)
			return eventData.removeElement(index);

		return false;
	}
	else
	{
		auto ok = data.remove(value);
		updateElementBuffer();
		return ok;
	}
}

bool ScriptingObjects::ScriptUnorderedStack::removeElement(int index)
{
	auto ok = isEventStack ? eventData.removeElement(index) : data.removeElement(index);
	updateElementBuffer();
	return ok;
}

bool ScriptingObjects::ScriptUnorderedStack::clear()
{
	auto wasEmpty = isEmpty();

	isEventStack ? eventData.clear() : data.clear();
	updateElementBuffer();

	return !wasEmpty;
}

int ScriptingObjects::ScriptUnorderedStack::size() const
{
	return isEventStack ? eventData.size() : data.size();
}

bool ScriptingObjects::ScriptUnorderedStack::isEmpty() const
{
	return isEventStack ? eventData.isEmpty() : data.isEmpty();
}

bool ScriptingObjects::ScriptUnorderedStack::contains(var value) const
{
	if (isEventStack)
		return getIndexForEvent(value) != -1;
	else
		return data.contains((float)value);
}

var ScriptingObjects::ScriptUnorderedStack::asBuffer(bool getAllElements)
{
	if (isEventStack)
		reportScriptError("Can't use asBuffer on a stack for events");

	if (getAllElements)
		return var(wholeBf.get());
	else
	{
		return var(elementBuffer.get());
	}
}

void ScriptingObjects::ScriptUnorderedStack::setIsEventStack(bool shouldBeEventStack, var eventCompareFunction)
{
	isEventStack = shouldBeEventStack;

	if (eventCompareFunction.isObject())
	{
		compareFunction = WeakCallbackHolder(getScriptProcessor(), this, eventCompareFunction, 2);
		compareFunctionType = CompareFunctions::Custom;

		if (compareFunction)
		{
			compareFunction.incRefCount();
			compareHolder = new ScriptingMessageHolder(getScriptProcessor());
		}
	}
	else
	{
		compareFunctionType = (CompareFunctions)(int)eventCompareFunction;

		switch (compareFunctionType)
		{
		case CompareFunctions::BitwiseEqual:		  hcf = MCF::equals<CompareFunctions::BitwiseEqual>; break;
		case CompareFunctions::EqualData:			  hcf = MCF::equals<CompareFunctions::EqualData>; break;
		case CompareFunctions::EventId:				  hcf = MCF::equals<CompareFunctions::EventId>; break;
		case CompareFunctions::NoteNumberAndChannel:  hcf = MCF::equals<CompareFunctions::NoteNumberAndChannel>; break;
		case CompareFunctions::NoteNumberAndVelocity: hcf = MCF::equals<CompareFunctions::NoteNumberAndVelocity>; break;
		default: reportScriptError("eventCompareFunction is not a valid compare constant");
		}
	}
}



ScriptingObjects::ScriptBackgroundTask::TaskViewer::TaskViewer(ScriptBackgroundTask* t) :
	Component("Task Viewer"),
	ComponentForDebugInformation(t, dynamic_cast<JavascriptProcessor*>(t->getScriptProcessor())),
	SimpleTimer(t->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	cancelButton("Cancel")
{
	setSize(500, 200);
	addAndMakeVisible(cancelButton);
	cancelButton.onClick = [this]()
	{
		if(auto obj = getObject<ScriptBackgroundTask>())
			obj->signalThreadShouldExit();
	};

	cancelButton.setLookAndFeel(&laf);
}

void ScriptingObjects::ScriptBackgroundTask::TaskViewer::timerCallback()
{
	repaint();
}

void ScriptingObjects::ScriptBackgroundTask::TaskViewer::paint(Graphics& g)
{
	g.fillAll(Colours::black.withAlpha(0.2f));
	
	if (auto o = getObject<ScriptBackgroundTask>())
	{
		g.setColour(Colour(0xFFDDDDDD));

		auto b = getLocalBounds().toFloat();

		auto pb = b.removeFromTop(24.0f);

		pb = pb.reduced(4.0f);

		g.drawRoundedRectangle(pb, pb.getHeight() / 2.0f, 2.0f);

		pb = pb.reduced(4.0f);
		pb = pb.removeFromLeft(o->progress * pb.getWidth());
		pb.setWidth(jmax<float>(pb.getWidth(), pb.getHeight()));

		g.fillRoundedRectangle(pb, pb.getHeight() / 2.0f);

		b.removeFromTop(10.0f);

		b.removeFromBottom(cancelButton.getHeight());

		String s;

		s << "**Name: ** " << o->getThreadName() << "  \n";
		s << "**Active: ** " << (o->isThreadRunning() ? "Yes" : "No") << "  \n";

		auto sm = o->getStatusMessage();

		MarkdownRenderer r(s);

		r.parse();
		r.draw(g, b.reduced(10.0f));
	}
}

struct ScriptingObjects::ScriptBackgroundTask::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, sendAbortSignal);
	API_METHOD_WRAPPER_0(ScriptBackgroundTask, shouldAbort);
	API_VOID_METHOD_WRAPPER_2(ScriptBackgroundTask, setProperty);
	API_METHOD_WRAPPER_1(ScriptBackgroundTask, getProperty);
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setFinishCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, callOnBackgroundThread);
	API_METHOD_WRAPPER_1(ScriptBackgroundTask, killVoicesAndCall);
	API_METHOD_WRAPPER_0(ScriptBackgroundTask, getProgress);
	API_VOID_METHOD_WRAPPER_3(ScriptBackgroundTask, runProcess);
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setProgress);
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setTimeOut);
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setStatusMessage);
	API_METHOD_WRAPPER_0(ScriptBackgroundTask, getStatusMessage);
	API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setForwardStatusToLoadingThread);
};

ScriptingObjects::ScriptBackgroundTask::ScriptBackgroundTask(ProcessorWithScriptingContent* p, const String& name) :
	ConstScriptingObject(p, 0),
	Thread(name),
	currentTask(p, this, var(), 1),
	finishCallback(p, this, var(), 2)
{
	dynamic_cast<JavascriptProcessor*>(p)->getScriptEngine()->preCompileListeners.addListener(*this, recompiled, false);

	ADD_API_METHOD_1(sendAbortSignal);
	ADD_API_METHOD_0(shouldAbort);
	ADD_API_METHOD_2(setProperty);
	ADD_API_METHOD_1(getProperty);
	ADD_API_METHOD_3(runProcess);
	ADD_API_METHOD_1(setFinishCallback);
	ADD_API_METHOD_1(callOnBackgroundThread);
	ADD_API_METHOD_1(killVoicesAndCall);
	ADD_API_METHOD_0(getProgress);
	ADD_API_METHOD_1(setProgress);
	ADD_API_METHOD_1(setTimeOut);
	ADD_API_METHOD_1(setStatusMessage);
	ADD_API_METHOD_0(getStatusMessage);
	ADD_API_METHOD_1(setForwardStatusToLoadingThread);
}

void ScriptingObjects::ScriptBackgroundTask::recompiled(ScriptBackgroundTask& task, bool unused)
{
	task.sendAbortSignal(true);
}

void ScriptingObjects::ScriptBackgroundTask::sendAbortSignal(bool blockUntilStopped)
{
	if (isThreadRunning())
	{
		if (blockUntilStopped)
		{
			if (auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine())
			{
				// extend the timeout while we're waiting for the thread to stop
				engine->extendTimeout(timeOut + 10);
			}

			stopThread(timeOut);
		}
		else
			signalThreadShouldExit();
	}
}

bool ScriptingObjects::ScriptBackgroundTask::shouldAbort()
{
	if (auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine())
	{
		engine->extendTimeout(timeOut + 10);
	}
	else
	{
		signalThreadShouldExit();
	}

	return threadShouldExit();
}

void ScriptingObjects::ScriptBackgroundTask::setProperty(String id, var value)
{
	auto i = Identifier(id);
	SimpleReadWriteLock::ScopedWriteLock sl(lock);
	synchronisedData.set(i, value);
}

var ScriptingObjects::ScriptBackgroundTask::getProperty(String id)
{
	auto i = Identifier(id);
	SimpleReadWriteLock::ScopedReadLock sl(lock);
	return synchronisedData.getWithDefault(i, var());
}

void ScriptingObjects::ScriptBackgroundTask::setFinishCallback(var newFinishCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(newFinishCallback))
	{
		finishCallback = WeakCallbackHolder(getScriptProcessor(), this, newFinishCallback, 2);
		finishCallback.incRefCount();
		finishCallback.setThisObject(this);
		
		finishCallback.addAsSource(this, "onTaskFinished");
	}
}

void ScriptingObjects::ScriptBackgroundTask::callOnBackgroundThread(var backgroundTaskFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(backgroundTaskFunction))
	{
		callFinishCallback(false, false);
		stopThread(timeOut);

		childProcessData = nullptr;

		currentTask = WeakCallbackHolder(getScriptProcessor(), this, backgroundTaskFunction, 1);
		currentTask.incRefCount();
		currentTask.addAsSource(this, "backgroundFunction");
		startThread(8);
	}
}

bool ScriptingObjects::ScriptBackgroundTask::killVoicesAndCall(var loadingFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(loadingFunction))
	{
		stopThread(timeOut);
		currentTask = WeakCallbackHolder(getScriptProcessor(), this, loadingFunction, 0);
		currentTask.incRefCount();
		currentTask.addAsSource(this, "backgroundFunction");

		WeakReference<ScriptBackgroundTask> safeThis(this);

		auto f = [safeThis](Processor* p)
		{
			if (safeThis != nullptr)
			{
				auto r = safeThis->currentTask.callSync(nullptr, 0, nullptr);

				if (!r.wasOk())
					debugError(p, r.getErrorMessage());
			}
				
			return SafeFunctionCall::OK;
		};

		return getScriptProcessor()->getMainController_()->getKillStateHandler().killVoicesAndCall(dynamic_cast<Processor*>(getScriptProcessor()), f, MainController::KillStateHandler::SampleLoadingThread);
	}

	return false;
}

ScriptingObjects::ScriptBackgroundTask::ChildProcessData::ChildProcessData(ScriptBackgroundTask& parent_, const String& command_, const var& args_, const var& pf) :
	processLogFunction(parent_.getScriptProcessor(), &parent_, pf, 3),
	parent(parent_)
{
	processLogFunction.incRefCount();
	processLogFunction.setHighPriority();

	args.add(command_);

	if (args_.isArray())
	{
		for (const auto& v : *args_.getArray())
			args.add(v.toString());
	}
	else if (args_.isString())
	{
		args.addArray(StringArray::fromTokens(args_.toString(), " ", "\"\'"));
	}

	args.removeEmptyStrings(true);
	args.trim();
}

void ScriptingObjects::ScriptBackgroundTask::ChildProcessData::run()
{
	if (args.isEmpty())
	{
		debugError(dynamic_cast<Processor*>(parent.getScriptProcessor()), "no args");
		return;
	}

	childProcess.start(args, ChildProcess::StreamFlags::wantStdErr | ChildProcess::StreamFlags::wantStdOut);

	var a[3];

	a[0] = &parent;
	a[1] = false;
	
	String currentLine;

	while (childProcess.isRunning())
	{
		if (parent.shouldAbort())
		{
			childProcess.kill();
			break;
		}
		
		char newChar;

		auto numBytesRead = childProcess.readProcessOutput(&newChar, 1);

		if (numBytesRead == 1)
		{
			currentLine << newChar;

			if (newChar == '\n' || newChar == '\r')
			{
				if (currentLine.trim().isNotEmpty())
				{
					a[2] = var(currentLine);
					callLog(a);
				}

				currentLine = {};

				parent.wait(10);
			}
		}
			
		parent.wait(1);
	}

	currentLine << childProcess.readAllProcessOutput();

	if (!currentLine.isEmpty())
	{
		a[2] = var(currentLine);
		callLog(a);
	}
		

	a[1] = true;
	a[2] = (int)childProcess.getExitCode();

	callLog(a);
}



void ScriptingObjects::ScriptBackgroundTask::ChildProcessData::callLog(var* a)
{
	if (processLogFunction)
	{
		auto ok = processLogFunction.callSync(a, 3);

		if (!ok.wasOk())
			debugError(dynamic_cast<Processor*>(parent.getScriptProcessor()), ok.getErrorMessage());
	}
}

void ScriptingObjects::ScriptBackgroundTask::runProcess(var command, var args, var logFunction)
{
	if (HiseJavascriptEngine::isJavascriptFunction(logFunction))
	{
		callFinishCallback(false, false);
		stopThread(timeOut);

		currentTask.clear();
		childProcessData = new ChildProcessData(*this, command.toString(), args, logFunction);

		startThread(8);
	}
}

void ScriptingObjects::ScriptBackgroundTask::setProgress(double p)
{
	progress.store(jlimit(0.0, 1.0, p));

	if (forwardToLoadingThread)
	{
		auto& flag = getScriptProcessor()->getMainController_()->getSampleManager().getPreloadProgress();
		flag = p;
	}
}

void ScriptingObjects::ScriptBackgroundTask::setStatusMessage(String m)
{
	{
		SimpleReadWriteLock::ScopedWriteLock sl(lock);
		message = m;
	}

	if (forwardToLoadingThread)
	{
		getScriptProcessor()->getMainController_()->getSampleManager().setCurrentPreloadMessage(m);
	}
}

void ScriptingObjects::ScriptBackgroundTask::run()
{
	if (currentTask || childProcessData)
	{
		if (forwardToLoadingThread)
		{
			getScriptProcessor()->getMainController_()->getSampleManager().setPreloadFlag();
		}

		if (childProcessData != nullptr)
		{
			childProcessData->run();
			childProcessData = nullptr;

		}
		else
		{
			var t(this);

			auto r = currentTask.callSync(&t, 1);

#if USE_BACKEND
			if (!r.wasOk())
				getScriptProcessor()->getMainController_()->writeToConsole(r.getErrorMessage(), 1, dynamic_cast<Processor*>(getScriptProcessor()));
#endif
		}

		if (forwardToLoadingThread)
		{
			getScriptProcessor()->getMainController_()->getSampleManager().clearPreloadFlag();
		}
	}

	callFinishCallback(true, threadShouldExit());
}

ScriptingObjects::ScriptFFT::ScriptFFT(ProcessorWithScriptingContent* p) :
	ConstScriptingObject(p, WindowType::numWindowType),
	phaseFunction(p, this, var(), 2),
	magnitudeFunction(p, this, var(), 2)
{
	addConstant("Rectangle", WindowType::Rectangle);
	addConstant("Triangle", WindowType::Triangle);
	addConstant("Hamming", WindowType::Hamming);
	addConstant("Hann", WindowType::Hann);
	addConstant("BlackmanHarris", WindowType::BlackmanHarris);
	addConstant("Kaiser", WindowType::Kaiser);
	addConstant("FlatTop", WindowType::FlatTop);

	ADD_API_METHOD_1(setWindowType);
	ADD_API_METHOD_2(prepare);
	ADD_API_METHOD_1(setOverlap);
	ADD_API_METHOD_1(process);
	ADD_API_METHOD_2(setMagnitudeFunction);
	ADD_API_METHOD_1(setPhaseFunction);
	ADD_API_METHOD_1(setEnableSpectrum2D);
	ADD_API_METHOD_1(setEnableInverseFFT);

	spectrumParameters = new Spectrum2D::Parameters();
}

ScriptingObjects::ScriptFFT::~ScriptFFT()
{

}

struct ScriptingObjects::ScriptFFT::FFTDebugComponent : public Component,
									  public ComponentForDebugInformation,
									  public PooledUIUpdater::SimpleTimer
{
	FFTDebugComponent(ScriptFFT* fft) :
		ComponentForDebugInformation(fft, dynamic_cast<ApiProviderBase::Holder*>(fft->getScriptProcessor())),
		Component("FFT Display"),
		SimpleTimer(fft->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		resizer(this, nullptr)
	{
		addAndMakeVisible(resizer);
		setSize(500, 500);
	};

	void timerCallback() override
	{
		repaint();
	}

	void paint(Graphics& g) override
	{
		if (auto obj = getObject<ScriptFFT>())
		{
			if (obj->enableSpectrum)
			{
				auto b = getLocalBounds().toFloat();

				if (obj->enableInverse)
				{
					auto inArea = b.removeFromTop(b.getHeight() / 2.0f);
					g.drawImage(obj->spectrum, inArea);
					g.drawImage(obj->outputSpectrum, b);
				}
				else
				{
					g.drawImage(obj->spectrum, b);
				}
			}
			else
			{
				g.setColour(Colours::white.withAlpha(0.7f));
				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText("Spectrum is disabled", getLocalBounds().toFloat(), Justification::centred);
			}
		}
	}

	void resized() override
	{
		resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
	}

	ResizableCornerComponent resizer;
};

Component* ScriptingObjects::ScriptFFT::createPopupComponent(const MouseEvent& e, Component *c)
{
	return new FFTDebugComponent(this);
}

void ScriptingObjects::ScriptFFT::setMagnitudeFunction(var newMagnitudeFunction, bool convertDb)
{
	SimpleReadWriteLock::ScopedWriteLock sl(lock);

	if (HiseJavascriptEngine::isJavascriptFunction(newMagnitudeFunction))
	{
		convertMagnitudesToDecibel = convertDb;
		magnitudeFunction = WeakCallbackHolder(getScriptProcessor(), this, newMagnitudeFunction, 2);
		magnitudeFunction.incRefCount();
		reinitialise();
	}
}

void ScriptingObjects::ScriptFFT::setPhaseFunction(var newPhaseFunction)
{
	SimpleReadWriteLock::ScopedWriteLock sl(lock);

	if (HiseJavascriptEngine::isJavascriptFunction(newPhaseFunction))
	{
		phaseFunction = WeakCallbackHolder(getScriptProcessor(), this, newPhaseFunction, 2);
		phaseFunction.incRefCount();
		reinitialise();
	}
}

void ScriptingObjects::ScriptFFT::setWindowType(int windowType_)
{
	currentWindowType = (WindowType)windowType_;
	spectrumParameters->currentWindowType = currentWindowType;

	reinitialise();
}

void ScriptingObjects::ScriptFFT::setOverlap(double percentageOfOverlap)
{
	overlap = jlimit(0.0, 0.99, percentageOfOverlap);
	spectrumParameters->oversamplingFactor = nextPowerOfTwo(1.0 / (1.0 - overlap));
}

void ScriptingObjects::ScriptFFT::prepare(int powerOfTwoSize, int maxNumChannels)
{
	lastSpecs.sampleRate = 44100.0;
	lastSpecs.blockSize = powerOfTwoSize;
	lastSpecs.numChannels = maxNumChannels;

	maxNumChannels = jlimit(1, NUM_MAX_CHANNELS, maxNumChannels);

	if (isPowerOfTwo(powerOfTwoSize))
	{
		windowBuffer.setSize(1, powerOfTwoSize * 2);
		windowBuffer.clear();
		FloatVectorOperations::fill(windowBuffer.getWritePointer(0), 1.0f, powerOfTwoSize);
		FFTHelpers::applyWindow(currentWindowType, windowBuffer, false);

		spectrumParameters->order = log2(powerOfTwoSize);
		spectrumParameters->Spectrum2DSize = powerOfTwoSize;

		maxNumSamples = powerOfTwoSize;

		Array<var> newWorkBuffer;

		for (int i = 0; i < maxNumChannels; i++)
		{
			WorkBuffer wb;
			wb.chunkInput = new VariantBuffer(maxNumSamples * 2);

			if (enableInverse)
				wb.chunkOutput = new VariantBuffer(maxNumSamples * 2);

			if(magnitudeFunction || enableInverse)
				wb.magBuffer = new VariantBuffer(maxNumSamples);

			if (phaseFunction || enableInverse)
				wb.phaseBuffer = new VariantBuffer(maxNumSamples);

			scratchBuffers.add(std::move(wb));
		}
			
		SimpleReadWriteLock::ScopedWriteLock sl(lock);

		fft = new juce::dsp::FFT(log2(maxNumSamples));
	}
	else
	{
		reportScriptError("powerOfTwoSize must be ... a power of two!");
	}
}

var ScriptingObjects::ScriptFFT::process(var dataToProcess)
{
	if (scratchBuffers.isEmpty() || fft == nullptr || maxNumSamples == 0)
		reportScriptError("You must call prepare before process");

	if (enableSpectrum)
	{
		if (dataToProcess.isArray())
		{
			fullBuffer.setSize(dataToProcess.size(), getNumToProcess(dataToProcess));
			int index = 0;

			for (auto& d : *dataToProcess.getArray())
			{
				FloatVectorOperations::copy(fullBuffer.getWritePointer(index++), d.getBuffer()->buffer.getReadPointer(0), fullBuffer.getNumSamples());
			}
		}
		else if (dataToProcess.isBuffer())
		{
			fullBuffer.makeCopyOf(dataToProcess.getBuffer()->buffer);
		}

		Spectrum2D fb(this, fullBuffer);
		fb.parameters = spectrumParameters;
		auto b = fb.createSpectrumBuffer();

		if (b.getNumSamples() > 0)
		{
			spectrum = fb.createSpectrumImage(b);
		}
		else
		{
			spectrum = {};
		}
	}

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (magnitudeFunction || phaseFunction)
	{
		var returnValue;


		int offset = 0;
		int numDelta = roundToInt((double)maxNumSamples * (1.0 - overlap));

		auto numToProcess = getNumToProcess(dataToProcess);
		int numChannels = dataToProcess.isArray() ? dataToProcess.size() : 1;

		if (enableInverse)
		{
			outputData.clear();

			for (int i = 0; i < numChannels; i++)
			{
				outputData.add(new VariantBuffer(numToProcess));
			}

			if (numChannels == 1)
				returnValue = outputData[0];
			else
				returnValue = var(outputData);
		}

		while (offset < numToProcess)
		{
			copyToWorkBuffer(dataToProcess, offset, 0);

			var args[2];
			args[1] = offset;

			applyFFT(numChannels, offset == 0);

			if (magnitudeFunction)
			{
				args[0] = getBufferArgs(true, numChannels);
				auto r = magnitudeFunction.callSync(args, 2);

				if (!r.wasOk())
					reportScriptError(r.getErrorMessage());
			}

			if (phaseFunction)
			{
				args[0] = getBufferArgs(false, numChannels);

				auto r = phaseFunction.callSync(args, 2);

				if (!r.wasOk())
					reportScriptError(r.getErrorMessage());
			}

			applyInverseFFT(numChannels);

			for (int i = 0; i < numChannels; i++)
			{
				copyFromWorkBuffer(offset, i);
			}
			

			offset += numDelta;
		}

		if (enableSpectrum)
		{
			Spectrum2D fb(this, outputData[0].getBuffer()->buffer);
			fb.parameters = spectrumParameters;
			auto b = fb.createSpectrumBuffer();

			if (b.getNumSamples() > 0)
				outputSpectrum = fb.createSpectrumImage(b);
			else
				outputSpectrum = {};
		}

		return returnValue;
	}
	else
		reportScriptError("the process function is not defined");

	return var();
}

void ScriptingObjects::ScriptFFT::setEnableSpectrum2D(bool shouldBeEnabled)
{
	enableSpectrum = shouldBeEnabled;
}

void ScriptingObjects::ScriptFFT::setEnableInverseFFT(bool shouldApplyReverseTransformToInput)
{
	if (enableInverse != shouldApplyReverseTransformToInput)
	{
		enableInverse = shouldApplyReverseTransformToInput;

		reinitialise();
	}
}

var ScriptingObjects::ScriptFFT::getBufferArgs(bool useMagnitude, int numToUse)
{
	if (isPositiveAndBelow(numToUse-1, scratchBuffers.size()))
	{
		thisProcessBuffer.clearQuick();

		for (int i = 0; i < numToUse; i++)
		{
			auto bufferToUse = useMagnitude ? scratchBuffers[i].magBuffer :
											  scratchBuffers[i].phaseBuffer;

			thisProcessBuffer.set(i, var(bufferToUse.get()));
		}

		if (thisProcessBuffer.size() == 1)
			return var(thisProcessBuffer[0]);
		else
			return var(thisProcessBuffer);
	}
	else
		reportScriptError("channel overflow");

	RETURN_IF_NO_THROW(var());
}

int ScriptingObjects::ScriptFFT::getNumToProcess(var inputData)
{
	if (inputData.isArray())
		return getNumToProcess(inputData[0]);
	if (auto b = inputData.getBuffer())
		return b->size;

	return 0;
}

void ScriptingObjects::ScriptFFT::copyToWorkBuffer(var inputData, int offset, int channel)
{
	if (auto a = inputData.getArray())
	{
		if (channel != 0)
		{
			reportScriptError("Illegal nested arrays");
		}

		for (auto& b : *a)
		{
			copyToWorkBuffer(b, offset, channel);
			channel++;
		}
	}
	else if (auto b = inputData.getBuffer())
	{
		if (auto dst = scratchBuffers[channel].chunkInput)
		{
			dst->clear();
			int numToCopy = jmin(b->size - offset, maxNumSamples);
			dst->buffer.copyFrom(0, 0, b->buffer, 0, offset, numToCopy);
		}
		else
		{
			reportScriptError("channel mismatch");
		}
	}
}

void ScriptingObjects::ScriptFFT::copyFromWorkBuffer(int offset, int channel)
{
	if (!enableInverse)
		return;

	if (auto b = scratchBuffers[channel].chunkOutput)
	{
		if (auto out = outputData[channel].getBuffer())
		{
			int numToCopy = jmin(b->size, out->size - offset);
			out->buffer.addFrom(0, offset, b->buffer, 0, 0, numToCopy);
		}
	}
}

void ScriptingObjects::ScriptFFT::applyFFT(int numChannelsThisTime, bool skipFirstWindowHalf)
{
	if (numChannelsThisTime > scratchBuffers.size())
		reportScriptError("Channel amount mismatch");

	for (int i = 0; i < numChannelsThisTime; i++)
	{
		auto wb = scratchBuffers[i];

		// Apply window here...
		
		{
			auto wdst = wb.chunkInput->buffer.getWritePointer(0);
			auto wsrc = windowBuffer.getReadPointer(0);
			auto windowSize = windowBuffer.getNumSamples();

			if (skipFirstWindowHalf)
			{
				auto firstHalf = windowSize / 4;

				//FloatVectorOperations::multiply(wdst, 2.0f, firstHalf);

				wdst += firstHalf;
				wsrc += firstHalf;
				windowSize -= firstHalf;
			}

			FloatVectorOperations::multiply(wdst, wsrc, windowSize);
		}
		

		fft->performRealOnlyForwardTransform(wb.chunkInput->buffer.getWritePointer(0), false);

		if (phaseFunction || enableInverse)
		{
			FFTHelpers::toPhaseSpectrum(wb.chunkInput->buffer, wb.phaseBuffer->buffer);
		}

		if (magnitudeFunction || enableInverse)
		{
            if(wb.magBuffer == nullptr)
                reportScriptError("The magnitude buffer is not prepared. Make sure to call prepare after setMagnitudeFunction");
            
			FFTHelpers::toFreqSpectrum(wb.chunkInput->buffer, wb.magBuffer->buffer);
			FFTHelpers::scaleFrequencyOutput(wb.magBuffer->buffer, convertMagnitudesToDecibel);
		}
	}
}

void ScriptingObjects::ScriptFFT::applyInverseFFT(int numChannelsThisTime)
{
	if (!enableInverse)
		return;

	if (numChannelsThisTime > scratchBuffers.size())
		reportScriptError("Channel amount mismatch");

	for (int i = 0; i < numChannelsThisTime; i++)
	{
		auto wb = scratchBuffers[i];
		
		FFTHelpers::scaleFrequencyOutput(wb.magBuffer->buffer, convertMagnitudesToDecibel, true);
		FFTHelpers::toComplexArray(wb.phaseBuffer->buffer, wb.magBuffer->buffer, wb.chunkOutput->buffer);

		fft->performRealOnlyInverseTransform(wb.chunkOutput->buffer.getWritePointer(0));
	}
}

struct ScriptingObjects::GlobalRoutingManagerReference::Wrapper
{
	API_METHOD_WRAPPER_1(GlobalRoutingManagerReference, getCable);
	API_METHOD_WRAPPER_2(GlobalRoutingManagerReference, connectToOSC);
	API_METHOD_WRAPPER_2(GlobalRoutingManagerReference, sendOSCMessage);
	API_VOID_METHOD_WRAPPER_2(GlobalRoutingManagerReference, addOSCCallback);
};


ScriptingObjects::GlobalRoutingManagerReference::GlobalRoutingManagerReference(ProcessorWithScriptingContent* sp) :
	ConstScriptingObject(sp, 0),
	ControlledObject(sp->getMainController_()),
	errorCallback(sp, this, var(), 1)
{
	auto ptr = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(getMainController());
	manager = ptr.get();

	ADD_API_METHOD_1(getCable);
	ADD_API_METHOD_2(connectToOSC);
	ADD_API_METHOD_2(sendOSCMessage);
	ADD_API_METHOD_2(addOSCCallback);
}

ScriptingObjects::GlobalRoutingManagerReference::~GlobalRoutingManagerReference()
{
	if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
	{
		if (auto r = dynamic_cast<OSCReceiver*>(m->receiver.get()))
			r->removeListener(this);

		for (auto c : callbacks)
			m->scriptCallbackPatterns.removeAllInstancesOf(c->fullAddress);
	}
}

scriptnode::routing::GlobalRoutingManager::Cable* getCableFromVar(const var& v)
{
	if (auto c = v.getObject())
	{
		return static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(c);
	}

	return nullptr;
}

Component* ScriptingObjects::GlobalRoutingManagerReference::createPopupComponent(const MouseEvent& e, Component *c)
{
	return scriptnode::routing::GlobalRoutingManager::Helpers::createDebugViewer(getScriptProcessor()->getMainController_());
}

void ScriptingObjects::GlobalRoutingManagerReference::oscBundleReceived(const OSCBundle& bundle)
{
	for (const auto& element : bundle)
	{
		if (element.isMessage())
			oscMessageReceived(element.getMessage());
		else if (element.isBundle())
			oscBundleReceived(element.getBundle());
	}
}

void ScriptingObjects::GlobalRoutingManagerReference::oscMessageReceived(const OSCMessage& message)
{
	if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
	{
		auto ap = message.getAddressPattern();

		if (!ap.containsWildcards())
		{
			OSCAddress a(ap.toString());

			for (auto c : callbacks)
			{
				if (c->shouldFire(a))
				{
					c->callForMessage(message);
				}
			}
		}
	}
}

juce::var ScriptingObjects::GlobalRoutingManagerReference::getCable(String cableId)
{
	if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
	{
		auto c = m->getSlotBase(cableId, scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType::Cable);

		return new GlobalCableReference(getScriptProcessor(), var(c.get()));
	}

	return var();
}

bool ScriptingObjects::GlobalRoutingManagerReference::connectToOSC(var connectionData, var errorFunction)
{
	
	if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
	{
		if (HiseJavascriptEngine::isJavascriptFunction(errorFunction))
		{
			errorCallback = WeakCallbackHolder(getScriptProcessor(), this, errorFunction, 1);
			errorCallback.incRefCount();

			m->setOSCErrorHandler(this);
		}
		else
		{
			errorCallback = WeakCallbackHolder(getScriptProcessor(), this, var(), 1);
			m->setOSCErrorHandler(nullptr);
		}

		scriptnode::OSCConnectionData::Ptr data = new scriptnode::OSCConnectionData(connectionData);
		auto ok = m->connectToOSC(data);

		if (ok)
		{
			if (auto r = dynamic_cast<juce::OSCReceiver*>(m->receiver.get()))
			{
				r->addListener(this);

				for (auto c : callbacks)
				{
					c->rebuildFullAddress(m->lastData->domain);
					m->scriptCallbackPatterns.addIfNotAlreadyThere(c->fullAddress);
				}
					


			}
		}

	}

	return false;
}

void ScriptingObjects::GlobalRoutingManagerReference::OSCCallback::callForMessage(const OSCMessage& c)
{
	if (c.isEmpty())
		return;

	auto getVar = [](const OSCArgument& a)
	{
		if (a.isFloat32())
			return var(a.getFloat32());
		if (a.isString())
			return var(a.getString());
		if (a.isInt32())
			return var(a.getInt32());

		return var();
	};

	if (c.size() == 1)
		args[1] = getVar(c[0]);
	else
	{
		auto newArray = Array<var>();

		for (auto& a : c)
			newArray.add(getVar(a));

		args[1] = var(newArray);
	}

	callback.call(args, 2);
}

void ScriptingObjects::GlobalRoutingManagerReference::addOSCCallback(String oscSubAddress, var callback)
{
	if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
	{
		auto c = new OSCCallback(this, oscSubAddress, callback);

		if (m->lastData != nullptr)
		{
			c->rebuildFullAddress(m->lastData->domain);
			m->scriptCallbackPatterns.addIfNotAlreadyThere(c->fullAddress);
		}
		
		callbacks.add(c);
	}
}



bool ScriptingObjects::GlobalRoutingManagerReference::sendOSCMessage(String oscSubAddress, var data)
{
	if (auto m = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(manager.getObject()))
	{
		return m->sendOSCMessageToOutput(oscSubAddress, data);
	}

	return false;
}

struct ScriptingObjects::GlobalCableReference::Wrapper
{
	API_METHOD_WRAPPER_0(GlobalCableReference, getValue);
	API_METHOD_WRAPPER_0(GlobalCableReference, getValueNormalised);
	API_VOID_METHOD_WRAPPER_1(GlobalCableReference, setValue);
	API_VOID_METHOD_WRAPPER_1(GlobalCableReference, setValueNormalised);
	API_VOID_METHOD_WRAPPER_2(GlobalCableReference, setRange);
	API_VOID_METHOD_WRAPPER_3(GlobalCableReference, setRangeWithSkew);
	API_VOID_METHOD_WRAPPER_3(GlobalCableReference, setRangeWithStep);
	API_VOID_METHOD_WRAPPER_2(GlobalCableReference, registerCallback);
	API_VOID_METHOD_WRAPPER_3(GlobalCableReference, connectToMacroControl);
    API_VOID_METHOD_WRAPPER_2(GlobalCableReference, connectToGlobalModulator);
    API_VOID_METHOD_WRAPPER_3(GlobalCableReference, connectToModuleParameter);
};

struct ScriptingObjects::GlobalCableReference::DummyTarget : public scriptnode::routing::GlobalRoutingManager::CableTargetBase
{
	DummyTarget(GlobalCableReference& p) :
		parent(p)
	{
		if (auto c = getCableFromVar(parent.cable))
			c->addTarget(this);
	}

	String getTargetId() const override 
	{ 
		String s;
		s << dynamic_cast<Processor*>(parent.getScriptProcessor())->getId();
		s << ".";
		s << parent.getDebugName();
		s << " (Script Reference)";
		return s;
	}

	Path getTargetIcon() const override
	{
		Path path;
		path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor));
		return path;
	}

	void selectCallback(Component* rootEditor) override
	{
#if USE_BACKEND
		auto r = dynamic_cast<BackendRootWindow*>(rootEditor);

		r->gotoIfWorkspace(dynamic_cast<Processor*>(parent.getScriptProcessor()));
#endif
	}

	void sendValue(double v) final override {};

	~DummyTarget()
	{
		if (auto c = getCableFromVar(parent.cable))
		{
			c->removeTarget(this);
		}
	}

	GlobalCableReference& parent;
};

ScriptingObjects::GlobalCableReference::GlobalCableReference(ProcessorWithScriptingContent* ps, var c) :
	ConstScriptingObject(ps, 0),
	cable(c),
	dummyTarget(new DummyTarget(*this)),
	inputRange(0.0, 1.0)
{
	ADD_API_METHOD_0(getValue);
	ADD_API_METHOD_0(getValueNormalised);
	ADD_API_METHOD_1(setValue);
	ADD_API_METHOD_1(setValueNormalised);
	ADD_API_METHOD_2(setRange);
	ADD_API_METHOD_3(setRangeWithSkew);
	ADD_API_METHOD_3(setRangeWithStep);
	ADD_API_METHOD_2(registerCallback);
	ADD_API_METHOD_3(connectToMacroControl);
    ADD_API_METHOD_2(connectToGlobalModulator);
    ADD_API_METHOD_3(connectToModuleParameter);

	inputRange.checkIfIdentity();
}

ScriptingObjects::GlobalCableReference::~GlobalCableReference()
{
	callbacks.clear();
}

double ScriptingObjects::GlobalCableReference::getValue() const
{
	auto v = getValueNormalised();
	return inputRange.convertFrom0to1(v, true);
}

double ScriptingObjects::GlobalCableReference::getValueNormalised() const
{
	if (auto c = getCableFromVar(cable))
		return c->lastValue;
	
	return 0.0;
}

void ScriptingObjects::GlobalCableReference::setValueNormalised(double normalisedInput)
{
	if (auto c = getCableFromVar(cable))
		c->sendValue(nullptr, normalisedInput);
}

void ScriptingObjects::GlobalCableReference::setValue(double inputWithinRange)
{
	auto v = inputRange.convertTo0to1(inputWithinRange, true);
	setValueNormalised(v);
}

void ScriptingObjects::GlobalCableReference::setRange(double min, double max)
{
	inputRange = scriptnode::InvertableParameterRange(min, max);
	inputRange.checkIfIdentity();
}

void ScriptingObjects::GlobalCableReference::setRangeWithSkew(double min, double max, double midPoint)
{
	inputRange = scriptnode::InvertableParameterRange(min, max);
	inputRange.setSkewForCentre(midPoint);
	inputRange.checkIfIdentity();
}

void ScriptingObjects::GlobalCableReference::setRangeWithStep(double min, double max, double stepSize)
{
	inputRange = scriptnode::InvertableParameterRange(min, max, stepSize);
	inputRange.checkIfIdentity();
}

struct ScriptingObjects::GlobalCableReference::Callback: public scriptnode::routing::GlobalRoutingManager::CableTargetBase,
														 public PooledUIUpdater::SimpleTimer
{
	Callback(GlobalCableReference& p, const var& f, bool synchronous) :
		SimpleTimer(p.getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		parent(p),
		sync(synchronous),
		callback(p.getScriptProcessor(), &p, f, 1)
	{
		id << dynamic_cast<Processor*>(p.getScriptProcessor())->getId();
		id << ".";

		auto ilf = dynamic_cast<WeakCallbackHolder::CallableObject*>(f.getObject());

		if (ilf != nullptr && (!synchronous || ilf->isRealtimeSafe()))
		{
			if (auto dobj = dynamic_cast<DebugableObjectBase*>(ilf))
			{
				id << dobj->getDebugName();
				funcLocation = dobj->getLocation();
			}

			callback.incRefCount();
			callback.setHighPriority();

			if (auto c = getCableFromVar(parent.cable))
			{
				c->addTarget(this);
			}

			if (!synchronous)
				start();
			else
				stop();
		}
		else
		{
			stop();
		}
	}

	void timerCallback() override
	{
		if (sync)
			return;

		double nv;

		if (value.getChangedValue(nv))
			callback.call1(nv);
	}

	String getTargetId() const override { return id; }

	Path getTargetIcon() const override
	{
		Path path;
		path.loadPathFromData(HiBinaryData::SpecialSymbols::scriptProcessor, sizeof(HiBinaryData::SpecialSymbols::scriptProcessor));
		return path;
	}

	void selectCallback(Component* rootEditor) override
	{
#if USE_BACKEND
		auto sp = parent.getScriptProcessor();

		auto br = dynamic_cast<BackendRootWindow*>(rootEditor);

		br->gotoIfWorkspace(dynamic_cast<Processor*>(sp));

		auto l = funcLocation;

		BackendPanelHelpers::ScriptingWorkspace::showEditor(br, true);

		auto f = [sp, l]()
		{
			DebugableObject::Helpers::gotoLocation(nullptr, dynamic_cast<JavascriptProcessor*>(sp), l);
		};

		Timer::callAfterDelay(400, f); 
#endif
	}

	void sendValue(double v) override
	{
		v = parent.inputRange.convertFrom0to1(v, false);

		if (sync)
		{
            var a(v);
            callback.callSync(&a, 1);
		}
		else
			value.setModValueIfChanged(v);
	}

	~Callback()
	{
		if (auto c = getCableFromVar(parent.cable))
		{
			c->removeTarget(this);
		}
	}

	GlobalCableReference& parent;

	WeakCallbackHolder callback;
	const bool sync = false;

	ModValue value;

	String id;

	DebugableObject::Location funcLocation;
};

void ScriptingObjects::GlobalCableReference::registerCallback(var callbackFunction, var synchronous)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callbackFunction))
	{
        bool isSync = ApiHelpers::isSynchronous(synchronous);
        
		auto nc = new Callback(*this, callbackFunction, isSync);
		callbacks.add(nc);
	}
}

struct MacroCableTarget : public scriptnode::routing::GlobalRoutingManager::CableTargetBase,
						 public ControlledObject
{
	MacroCableTarget(MainController* mc, int index, bool filterReps) :
		ControlledObject(mc),
		macroIndex(index),
		filterRepetitions(filterReps)
	{
		macroData = mc->getMainSynthChain()->getMacroControlData(macroIndex);
	};

	void selectCallback(Component* rootEditor)
	{
		// maybe open the macro panel?
		jassertfalse;
	}

	String getTargetId() const override
	{
		return "Macro " + String(macroIndex + 1);
	}

	void sendValue(double v) override
	{
		if (macroData == nullptr)
			macroData = getMainController()->getMainSynthChain()->getMacroControlData(macroIndex);

		auto newValue = 127.0f * jlimit(0.0f, 1.0f, (float)v);
		
		if ((!filterRepetitions || lastValue != newValue) && macroData != nullptr)
		{
			lastValue = newValue;
			macroData->setValue(newValue);
		}
	}

	Path getTargetIcon() const override
	{
		Path p;
		p.loadPathFromData(HiBinaryData::SpecialSymbols::macros, sizeof(HiBinaryData::SpecialSymbols::macros));
		return p;
	}

	const bool filterRepetitions;
	float lastValue = -1.0f;
	const int macroIndex;
	WeakReference<MacroControlBroadcaster::MacroControlData> macroData;
};


void ScriptingObjects::GlobalCableReference::connectToMacroControl(int macroIndex, bool macroIsTarget, bool filterRepetitions)
{
	if (auto c = getCableFromVar(cable))
	{
		if (macroIsTarget)
		{
			using CableType = scriptnode::routing::GlobalRoutingManager::CableTargetBase;

			for (int i = 0; i < c->getTargetList().size(); i++)
			{
				if (auto m = dynamic_cast<MacroCableTarget*>(c->getTargetList()[i].get()))
				{
					if (macroIndex == -1 || m->macroIndex == macroIndex)
					{
						c->removeTarget(m);
						i--;
						continue;
					}
				}
			}

			if (macroIndex != -1)
				c->addTarget(new MacroCableTarget(getScriptProcessor()->getMainController_(), macroIndex, filterRepetitions));
		}
		else
		{
			// not implemented
			jassertfalse;
		}
	}
}


void ScriptingObjects::GlobalCableReference::connectToGlobalModulator(const String& lfoId, bool addToMod)
{
    auto mc = getScriptProcessor()->getMainController_()->getMainSynthChain();
    
    if(auto p = ProcessorHelpers::getFirstProcessorWithName(mc, lfoId))
    {
        if(auto gc = dynamic_cast<GlobalModulatorContainer*>(p->getParentProcessor(true)))
        {
            gc->connectToGlobalCable(dynamic_cast<Modulator*>(p), cable, addToMod);
        }
    }
}

struct ProcessorParameterTarget : public scriptnode::routing::GlobalRoutingManager::CableTargetBase,
                                  public ControlledObject
{
    ProcessorParameterTarget(Processor* p, int index, const scriptnode::InvertableParameterRange& range, double smoothingTimeMs) :
        ControlledObject(p->getMainController()),
        targetRange(range),
        parameterIndex(index),
        processor(p)
    {
		lastValue.prepare(p->getSampleRate() / (double)p->getLargestBlockSize(), smoothingTimeMs);

        id << processor->getId();
        id << "::";
        id << processor->parameterNames[index].toString();
    };

    void selectCallback(Component* rootEditor)
    {
        // maybe open the macro panel?
        jassertfalse;
    }

    String getTargetId() const override
    {
        return id;
    }

    void sendValue(double v) override
    {
		lastValue.set(v);
        auto newValue = jlimit(0.0f, 1.0f, (float)lastValue.advance());
        auto cv = targetRange.convertFrom0to1(newValue, true);
        processor->setAttribute(parameterIndex, cv, sendNotification);
    }

    Path getTargetIcon() const override
    {
        Path p;
        p.loadPathFromData(HiBinaryData::SpecialSymbols::macros, sizeof(HiBinaryData::SpecialSymbols::macros));
        return p;
    }

    const int parameterIndex;
    const scriptnode::InvertableParameterRange targetRange;
    WeakReference<Processor> processor;
    String id;
	sdouble lastValue;
};

void ScriptingObjects::GlobalCableReference::connectToModuleParameter(const String& processorId, var parameterIndex, var targetRange)
{
    auto mc = getScriptProcessor()->getMainController_()->getMainSynthChain();
    
    if(processorId.isEmpty() && (int)parameterIndex == -1)
    {
        if (auto c = getCableFromVar(cable))
        {
            // Clear all module connections for this cable
            for (int i = 0; i < c->getTargetList().size(); i++)
            {
                if (auto ppt = dynamic_cast<ProcessorParameterTarget*>(c->getTargetList()[i].get()))
                {
                    c->removeTarget(ppt);
                    i--;
                    continue;
                }
            }
        }
    }
    
    if(auto p = ProcessorHelpers::getFirstProcessorWithName(mc, processorId))
    {
        auto indexToUse = -1;
        
        if(parameterIndex.isString())
        {
            Identifier pId(parameterIndex.toString());
            indexToUse = p->parameterNames.indexOf(pId);
            
            if(indexToUse == -1)
                reportScriptError("Can't find parameter ID " + pId.toString());
        }
        else
        {
            indexToUse = (int)parameterIndex;
        }
        
        if (auto c = getCableFromVar(cable))
        {
            for (int i = 0; i < c->getTargetList().size(); i++)
            {
                if (auto ppt = dynamic_cast<ProcessorParameterTarget*>(c->getTargetList()[i].get()))
                {
                    
                    if (p == ppt->processor &&
                        (indexToUse == -1 || ppt->parameterIndex == indexToUse))
                    {
                        c->removeTarget(ppt);
                        i--;
                        continue;
                    }
                }
            }
            
            auto range = scriptnode::RangeHelpers::getDoubleRange(targetRange);
            
			auto smoothing = (double)targetRange.getProperty("SmoothingTime", 0.0);

            if (indexToUse != -1)
                c->addTarget(new ProcessorParameterTarget(p, indexToUse, range, smoothing));
        }
    }
    else
    {
        reportScriptError("Can't find module with ID " + processorId);
    }
}

ScriptingObjects::ScriptedMidiPlayer::PlaybackUpdater::PlaybackUpdater(ScriptedMidiPlayer& parent_, var f, bool sync_) :
	SimpleTimer(parent_.getScriptProcessor()->getMainController_()->getGlobalUIUpdater(), !sync_),
	sync(sync_),
	parent(parent_),
	playbackCallback(parent.getScriptProcessor(), &parent, f, 2)
{
	if (auto mp = parent.getPlayer())
		mp->addPlaybackListener(this);

	playbackCallback.incRefCount();
	playbackCallback.setThisObject(&parent);
}

ScriptingObjects::ScriptedMidiPlayer::PlaybackUpdater::~PlaybackUpdater()
{
	if (auto mp = parent.getPlayer())
		mp->removePlaybackListener(this);
}

void ScriptingObjects::ScriptedMidiPlayer::PlaybackUpdater::timerCallback()
{
	if (dirty)
	{
		playbackCallback.call(args, 2);
		dirty = false;
	}
}

void ScriptingObjects::ScriptedMidiPlayer::PlaybackUpdater::playbackChanged(int timestamp, MidiPlayer::PlayState newState)
{
	args[0] = var(timestamp);
	args[1] = var((int)newState);

	if (sync)
		playbackCallback.callSync(args, 2, nullptr);
	else
		dirty = true;
}

struct ScriptingObjects::ScriptedMidiAutomationHandler::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptedMidiAutomationHandler, getAutomationDataObject);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setAutomationDataFromObject);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setControllerNumbersInPopup);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setExclusiveMode);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setUpdateCallback);
	API_VOID_METHOD_WRAPPER_1(ScriptedMidiAutomationHandler, setConsumeAutomatedControllers);
	API_VOID_METHOD_WRAPPER_2(ScriptedMidiAutomationHandler, setControllerNumberNames);
};

ScriptingObjects::ScriptedMidiAutomationHandler::ScriptedMidiAutomationHandler(ProcessorWithScriptingContent* sp) :
	ConstScriptingObject(sp, 0),
	handler(sp->getMainController_()->getMacroManager().getMidiControlAutomationHandler()),
	updateCallback(getScriptProcessor(), this, var(), 1)
{
	handler->addChangeListener(this);

	ADD_API_METHOD_0(getAutomationDataObject);
	ADD_API_METHOD_1(setAutomationDataFromObject);
	ADD_API_METHOD_1(setControllerNumbersInPopup);
	ADD_API_METHOD_1(setExclusiveMode);
	ADD_API_METHOD_1(setUpdateCallback);
	ADD_API_METHOD_1(setConsumeAutomatedControllers);
	ADD_API_METHOD_2(setControllerNumberNames);
}

ScriptingObjects::ScriptedMidiAutomationHandler::~ScriptedMidiAutomationHandler()
{
	handler->removeChangeListener(this);
}

void ScriptingObjects::ScriptedMidiAutomationHandler::changeListenerCallback(SafeChangeBroadcaster *b)
{
	if (updateCallback)
		updateCallback.call1(getAutomationDataObject());
}

juce::var ScriptingObjects::ScriptedMidiAutomationHandler::getAutomationDataObject()
{
	auto data = handler->exportAsValueTree();
	return var(ValueTreeConverters::convertFlatValueTreeToVarArray(data));
}

void ScriptingObjects::ScriptedMidiAutomationHandler::setAutomationDataFromObject(var automationData)
{
	auto vt = ValueTreeConverters::convertVarArrayToFlatValueTree(automationData, "MidiAutomation", "Controller");
	handler->restoreFromValueTree(vt);
}

void ScriptingObjects::ScriptedMidiAutomationHandler::setControllerNumbersInPopup(var numberArray)
{
	BigInteger bi;

	if (auto a = numberArray.getArray())
	{
		for (auto v : *a)
			bi.setBit((int)v, true);
	}

	handler->setControllerPopupNumbers(bi);
}

void ScriptingObjects::ScriptedMidiAutomationHandler::setControllerNumberNames(var ccName, var nameArray)
{
	handler->setCCName(ccName);
	
	StringArray sa;

	if (auto a = nameArray.getArray())
	{
		for (const auto& v : *a)
		{
			sa.add(v.toString());
		}
	}

	handler->setControllerPopupNames(sa);
}

void ScriptingObjects::ScriptedMidiAutomationHandler::setExclusiveMode(bool shouldBeExclusive)
{
	handler->setExclusiveMode(shouldBeExclusive);
}

void ScriptingObjects::ScriptedMidiAutomationHandler::setUpdateCallback(var callback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(callback))
	{
		updateCallback = WeakCallbackHolder(getScriptProcessor(), this, callback, 1);
		updateCallback.incRefCount();
		updateCallback.addAsSource(this, "onMidiAutomationUpdate");
		updateCallback.setThisObject(this);

		auto obj = getAutomationDataObject();

		auto r = updateCallback.callSync(&obj, 1);

		if (!r.wasOk())
			reportScriptError(r.getErrorMessage());
	}
}

void ScriptingObjects::ScriptedMidiAutomationHandler::setConsumeAutomatedControllers(bool shouldBeConsumed)
{
	handler->setConsumeAutomatedControllers(shouldBeConsumed);
}

struct ScriptingObjects::ScriptBuilder::Wrapper
{
	API_METHOD_WRAPPER_4(ScriptBuilder, create);
	API_METHOD_WRAPPER_2(ScriptBuilder, get);
	API_METHOD_WRAPPER_1(ScriptBuilder, getExisting);
    API_VOID_METHOD_WRAPPER_2(ScriptBuilder, clearChildren);
	API_VOID_METHOD_WRAPPER_2(ScriptBuilder, setAttributes);
	API_VOID_METHOD_WRAPPER_0(ScriptBuilder, clear);
	API_VOID_METHOD_WRAPPER_0(ScriptBuilder, flush);
	API_VOID_METHOD_WRAPPER_2(ScriptBuilder, connectToScript);
};

ScriptingObjects::ScriptBuilder::ScriptBuilder(ProcessorWithScriptingContent* p) :
	ConstScriptingObject(p, 6)
{
	createdModules.add(getScriptProcessor()->getMainController_()->getMainSynthChain());

	createJSONConstants();

	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_4(create);
	ADD_API_METHOD_2(get);
	ADD_API_METHOD_1(getExisting);
	ADD_API_METHOD_2(setAttributes);
	ADD_API_METHOD_0(flush);
    ADD_API_METHOD_2(clearChildren);
	ADD_API_METHOD_2(connectToScript);
}

ScriptingObjects::ScriptBuilder::~ScriptBuilder()
{
	if (!flushed && !createdModules.isEmpty())
	{
		debugError(dynamic_cast<Processor*>(getScriptProcessor()), "forgot to flush() a Builder!");
	}
}

template <typename T> void addScriptProcessorInterfaceID(var& ids)
{
	auto i = T::getClassName();
	ids.getDynamicObject()->setProperty(i, i.toString());
}

void ScriptingObjects::ScriptBuilder::createJSONConstants()
{
	auto root = getScriptProcessor()->getMainController_()->getMainSynthChain();

	auto createObjectForFactory = [](hise::FactoryType* f)
	{
		DynamicObject::Ptr p = new DynamicObject();

		f->setConstrainer(nullptr);

		for (auto& t : f->getAllowedTypes())
			p->setProperty(Identifier(t.type.toString().removeCharacters(" ")), t.type.toString());

		return var(p.get());
	};

	{
		MidiProcessorFactoryType f(root);
		addConstant("MidiProcessors", createObjectForFactory(&f));
	}

	{
		ModulatorChainFactoryType f(NUM_POLYPHONIC_VOICES, Modulation::GainMode, root);
		addConstant("Modulators", createObjectForFactory(&f));
	}

	{
		ModulatorSynthChainFactoryType m(NUM_POLYPHONIC_VOICES, root);
		addConstant("SoundGenerators", createObjectForFactory(&m));
	}

	{
		EffectProcessorChainFactoryType e(NUM_POLYPHONIC_VOICES, root);
		addConstant("Effects", createObjectForFactory(&e));
	}
	{
		var s(new DynamicObject());

		addScriptProcessorInterfaceID<ScriptingMidiProcessor>(s);
		addScriptProcessorInterfaceID<ScriptingModulator>(s);
		addScriptProcessorInterfaceID<ScriptingSynth>(s);
		addScriptProcessorInterfaceID<ScriptingEffect>(s);
		addScriptProcessorInterfaceID<ScriptingAudioSampleProcessor>(s);
		addScriptProcessorInterfaceID<ScriptSliderPackProcessor>(s);
		addScriptProcessorInterfaceID<ScriptingTableProcessor>(s);
		addScriptProcessorInterfaceID<ScriptingApi::Sampler>(s);
		addScriptProcessorInterfaceID<ScriptedMidiPlayer>(s);
		addScriptProcessorInterfaceID<ScriptRoutingMatrix>(s);
		addScriptProcessorInterfaceID<ScriptingSlotFX>(s);

		addConstant("InterfaceTypes", s);
	}
	{
		var chainIds(new DynamicObject());

		chainIds.getDynamicObject()->setProperty("Direct", raw::IDs::Chains::Direct);
		chainIds.getDynamicObject()->setProperty("Midi", raw::IDs::Chains::Midi);
		chainIds.getDynamicObject()->setProperty("Gain", raw::IDs::Chains::Gain);
		chainIds.getDynamicObject()->setProperty("Pitch", raw::IDs::Chains::Pitch);
		chainIds.getDynamicObject()->setProperty("FX", raw::IDs::Chains::FX);
		chainIds.getDynamicObject()->setProperty("GlobalMod", raw::IDs::Chains::GlobalModulatorSlot);

		addConstant("ChainIndexes", chainIds);
	}
}

int ScriptingObjects::ScriptBuilder::create(var type, var id, int rootBuildIndex, int chainIndex)
{
	if (!getScriptProcessor()->getScriptingContent()->interfaceCreationAllowed())
	{
		reportScriptError("You can't use this method after the onInit callback!");
		RETURN_IF_NO_THROW(-1);
	}
	
	if (auto p = createdModules[rootBuildIndex])
	{
		if (auto existing = ProcessorHelpers::getFirstProcessorWithName(p, id.toString()))
		{
			createdModules.add(existing);
			return createdModules.size() - 1;
		}

        
        auto mc = getScriptProcessor()->getMainController_();
        
        MainController::ScopedBadBabysitter sb(mc);
        
		raw::Builder b(mc);

		Identifier t_(type.toString());

		

		auto newP = b.create(p, t_, chainIndex);

		if (newP != nullptr)
		{
			newP->setId(id.toString(), dontSendNotification);
			createdModules.add(newP);
			
			flushed = false;

			return createdModules.size() - 1;
		}
		else
			reportScriptError("Couldn't create module with ID " + t_.toString());
	}
	else
	{
		reportScriptError("Couldn't find parent module with index " + String(rootBuildIndex));
	}

	RETURN_IF_NO_THROW(-1);
}

int ScriptingObjects::ScriptBuilder::clearChildren(int buildIndex, int chainIndex)
{
    if (auto p = createdModules[buildIndex])
    {
        Chain* c = nullptr;
        
        if(chainIndex == -1)
            c = dynamic_cast<Chain*>(p.get());
        else
            c = dynamic_cast<Chain*>(p->getChildProcessor(chainIndex));
        
        if(c == nullptr)
        {
            reportScriptError("Illegal chain index for the module " + p->getId());
        }
        
        auto h = c->getHandler();
        
        auto numBefore = h->getNumProcessors();
        
        if(numBefore != 0)
        {
            for(int i = 0; i < h->getNumProcessors(); i++)
            {
                auto pToDelete = h->getProcessor(i--);
                {
                    MessageManagerLock mm;
                    pToDelete->sendDeleteMessage();
                }
                
                h->remove(pToDelete);
            }
        }
            
        return numBefore;
    }
    else
        reportScriptError("Can't find parent module with index " + String(buildIndex));
    
    return -1;
}

bool ScriptingObjects::ScriptBuilder::connectToScript(int buildIndex, String relativePath)
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(createdModules[buildIndex].get()))
	{
		jp->setConnectedFile(relativePath, true);
		return true;
	}

	return false;
}

juce::var ScriptingObjects::ScriptBuilder::get(int buildIndex, String interfaceType)
{
	if (auto p = createdModules[buildIndex])
	{
		Identifier id(interfaceType);

#define RETURN_IF_MATCH(Type, PType) if(id == Type::getClassName() && dynamic_cast<PType*>(p.get()) != nullptr) return var(new Type(getScriptProcessor(), dynamic_cast<PType*>(p.get())));

		RETURN_IF_MATCH(ScriptingMidiProcessor, hise::MidiProcessor);
		RETURN_IF_MATCH(ScriptingModulator, hise::Modulator);
		RETURN_IF_MATCH(ScriptingSynth, hise::ModulatorSynth);
		RETURN_IF_MATCH(ScriptingEffect, hise::EffectProcessor);
		RETURN_IF_MATCH(ScriptingAudioSampleProcessor, hise::Processor);
		RETURN_IF_MATCH(ScriptSliderPackProcessor, snex::ExternalDataHolder);
		RETURN_IF_MATCH(ScriptingTableProcessor, snex::ExternalDataHolder);
		RETURN_IF_MATCH(ScriptingApi::Sampler, hise::ModulatorSampler);
		RETURN_IF_MATCH(ScriptedMidiPlayer, hise::MidiPlayer);
		RETURN_IF_MATCH(ScriptRoutingMatrix, hise::Processor);
		RETURN_IF_MATCH(ScriptingSlotFX, hise::EffectProcessor);

#undef RETURN_IF_MATCH

	}
	return var();
}

int ScriptingObjects::ScriptBuilder::getExisting(String processorId)
{
	for (auto p : createdModules)
	{
		if (p->getId() == processorId)
			return createdModules.indexOf(p);
	}

	auto pToAdd = ProcessorHelpers::getFirstProcessorWithName(getScriptProcessor()->getMainController_()->getMainSynthChain(), processorId);

	if (pToAdd == nullptr)
	{
		reportScriptError("Can't find processor with ID " + processorId);
	}

	createdModules.add(pToAdd);
	return createdModules.size() - 1;
}

void ScriptingObjects::ScriptBuilder::setAttributes(int buildIndex, var attributeValues)
{
	if (auto p = createdModules[buildIndex])
	{
		Array<Identifier> attributeIds;

		for (int i = 0; i < p->getNumParameters(); i++)
			attributeIds.add(p->getIdentifierForParameterIndex(i));

		if (auto obj = attributeValues.getDynamicObject())
		{
			for (const auto& a : obj->getProperties())
			{
				auto idx = attributeIds.indexOf(a.name);

				if (idx == -1)
				{
					reportScriptError("Can't find attribute " + a.name);
					break;
				}
				else
				{
					auto v = (float)a.value;
					FloatSanitizers::sanitizeFloatNumber(v);

					p->setAttribute(idx, v, dontSendNotification);
				}
			}

			p->sendPooledChangeMessage();
		}
	}
}

void ScriptingObjects::ScriptBuilder::clear()
{
	if (getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::SampleLoadingThread)
	{
		debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "skipping Builder.clear() on project load");
		return;
	}

	auto thisAsP = dynamic_cast<Processor*>(getScriptProcessor());

    auto mc = getScriptProcessor()->getMainController_();
    
    MainController::ScopedBadBabysitter sb(mc);
    
	raw::Builder b(mc);

	auto synthChain = getScriptProcessor()->getMainController_()->getMainSynthChain();
	
	for (int i = 0; i < synthChain->getNumChildProcessors(); i++)
	{
		if (i < ModulatorSynth::numInternalChains)
		{
			auto m = synthChain->getChildProcessor(i);

			for (int j = 0; j < m->getNumChildProcessors(); j++)
			{
				auto cToRemove = m->getChildProcessor(j);

				// please don't kill yourself.
				if (cToRemove == thisAsP)
					continue;

				else
				{
                    {
                        MessageManagerLock mm;
                        cToRemove->sendDeleteMessage();
                    }
					b.remove<Processor>(cToRemove);
					j--;
				}
			}
		}
		else
		{
            auto p = synthChain->getChildProcessor(i--);
            
            {
                MessageManagerLock mm;
                p->sendDeleteMessage();
            }
            
			b.remove<Processor>(p);
		}
	}

	flushed = false;
}



void ScriptingObjects::ScriptBuilder::flush()
{
	flushed = true;

	auto synthChain = getScriptProcessor()->getMainController_()->getMainSynthChain();

	synthChain->sendRebuildMessage(true);

	getScriptProcessor()->getMainController_()->getProcessorChangeHandler().sendProcessorChangeMessage(synthChain, MainController::ProcessorChangeHandler::EventType::RebuildModuleList, false);
}

struct ScriptingObjects::ScriptErrorHandler::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptErrorHandler, setErrorCallback);
	API_VOID_METHOD_WRAPPER_2(ScriptErrorHandler, setCustomMessageToShow);
	API_VOID_METHOD_WRAPPER_1(ScriptErrorHandler, clearErrorLevel);
	API_VOID_METHOD_WRAPPER_0(ScriptErrorHandler, clearAllErrors);
	API_METHOD_WRAPPER_0(ScriptErrorHandler, getErrorMessage);
	API_METHOD_WRAPPER_0(ScriptErrorHandler, getNumActiveErrors);
	API_METHOD_WRAPPER_0(ScriptErrorHandler, getCurrentErrorLevel);
	API_VOID_METHOD_WRAPPER_1(ScriptErrorHandler, simulateErrorEvent);
};

ScriptingObjects::ScriptErrorHandler::ScriptErrorHandler(ProcessorWithScriptingContent* p) :
	ConstScriptingObject(p, OverlayMessageBroadcaster::State::numReasons),
	callback(p, this, var(), 2)
{
	addConstant("AppDataDirectoryNotFound", (int)OverlayMessageBroadcaster::AppDataDirectoryNotFound);
	addConstant("LicenseNotFound", (int)OverlayMessageBroadcaster::LicenseNotFound);
	addConstant("ProductNotMatching", (int)OverlayMessageBroadcaster::ProductNotMatching);
	addConstant("UserNameNotMatching", (int)OverlayMessageBroadcaster::UserNameNotMatching);
	addConstant("EmailNotMatching", (int)OverlayMessageBroadcaster::EmailNotMatching);
	addConstant("MachineNumbersNotMatching", (int)OverlayMessageBroadcaster::MachineNumbersNotMatching);
	addConstant("LicenseExpired", (int)OverlayMessageBroadcaster::LicenseExpired);
	addConstant("LicenseInvalid", (int)OverlayMessageBroadcaster::LicenseInvalid);
	addConstant("CriticalCustomErrorMessage", (int)OverlayMessageBroadcaster::CriticalCustomErrorMessage);
	addConstant("SamplesNotInstalled", (int)OverlayMessageBroadcaster::SamplesNotInstalled);
	addConstant("SamplesNotFound", (int)OverlayMessageBroadcaster::SamplesNotFound);
	addConstant("IllegalBufferSize", (int)OverlayMessageBroadcaster::IllegalBufferSize);
	addConstant("CustomErrorMessage", (int)OverlayMessageBroadcaster::CustomErrorMessage);
	addConstant("CustomInformation", (int)OverlayMessageBroadcaster::CustomInformation);

	p->getMainController_()->addOverlayListener(this);

	// Deactivate the default overlay if you create this object.
	p->getMainController_()->setUseDefaultOverlay(false);

	ADD_API_METHOD_1(setErrorCallback);
	ADD_API_METHOD_2(setCustomMessageToShow);
	ADD_API_METHOD_1(clearErrorLevel);
	ADD_API_METHOD_0(clearAllErrors);
	ADD_API_METHOD_0(getErrorMessage);
	ADD_API_METHOD_0(getNumActiveErrors);
	ADD_API_METHOD_0(getCurrentErrorLevel);
	ADD_API_METHOD_1(simulateErrorEvent);

	for (int i = 0; i < OverlayMessageBroadcaster::State::numReasons; i++)
		customErrorMessages.add({});
}

void ScriptingObjects::ScriptErrorHandler::overlayMessageSent(int state, const String& message)
{
	errorStates.setBit(state, true);

	if (state == OverlayMessageBroadcaster::CustomErrorMessage ||
		state == OverlayMessageBroadcaster::CustomInformation ||
		state == OverlayMessageBroadcaster::CriticalCustomErrorMessage)
	{
		customErrorMessages.set(state, message);
	}

	sendErrorForHighestState();
}

void ScriptingObjects::ScriptErrorHandler::setErrorCallback(var errorCallback)
{
	if (HiseJavascriptEngine::isJavascriptFunction(errorCallback))
	{
		callback = WeakCallbackHolder(getScriptProcessor(), this, errorCallback, 2);
		callback.incRefCount();
		callback.addAsSource(this, "onErrorCallback");
		callback.setThisObject(this);
		callback.setHighPriority();
	}
}

void ScriptingObjects::ScriptErrorHandler::setCustomMessageToShow(int state, String messageToShow)
{
	customErrorMessages.set(state, messageToShow);
}

void ScriptingObjects::ScriptErrorHandler::clearErrorLevel(int stateToClear)
{
	errorStates.clearBit(stateToClear);

	if (!errorStates.isZero())
	{
		sendErrorForHighestState();
	}
}

void ScriptingObjects::ScriptErrorHandler::clearAllErrors()
{
	errorStates.clear();
}

String ScriptingObjects::ScriptErrorHandler::getErrorMessage() const
{
	auto el = getCurrentErrorLevel();

	if (el == -1)
		return {};

	auto m = customErrorMessages[el];

	if (m.isNotEmpty())
		return m;

	return getScriptProcessor()->getMainController_()->getOverlayTextMessage((OverlayMessageBroadcaster::State)el);
}

int ScriptingObjects::ScriptErrorHandler::getNumActiveErrors() const
{
	return errorStates.countNumberOfSetBits();
}

int ScriptingObjects::ScriptErrorHandler::getCurrentErrorLevel() const
{
	if (errorStates.isZero())
		return -1;

	for (int i = 0; i < errorStates.getHighestBit() + 1; i++)
	{
		if (errorStates[i])
			return i;
	}

	jassertfalse;
	return -1;
}

void ScriptingObjects::ScriptErrorHandler::simulateErrorEvent(int state)
{
	getScriptProcessor()->getMainController_()->sendOverlayMessage(state);
}

void ScriptingObjects::ScriptErrorHandler::sendErrorForHighestState()
{
	if (callback)
	{
		args[0] = getCurrentErrorLevel();
		args[1] = getErrorMessage();
		callback.call(args, 2);
	}
}

ScriptingObjects::ScriptErrorHandler::~ScriptErrorHandler()
{
	getScriptProcessor()->getMainController_()->removeOverlayListener(this);
}







} // namespace hise
