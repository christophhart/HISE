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
	API_METHOD_WRAPPER_1(ScriptFile, toString);
	API_METHOD_WRAPPER_0(ScriptFile, isFile);
	API_METHOD_WRAPPER_0(ScriptFile, isDirectory);
	API_METHOD_WRAPPER_1(ScriptFile, writeObject);
	API_METHOD_WRAPPER_1(ScriptFile, writeString);
	API_METHOD_WRAPPER_2(ScriptFile, writeEncryptedObject);
	API_METHOD_WRAPPER_3(ScriptFile, writeAudioFile);
	API_METHOD_WRAPPER_0(ScriptFile, loadAsString);
	API_METHOD_WRAPPER_0(ScriptFile, loadAsObject);
	API_METHOD_WRAPPER_0(ScriptFile, deleteFileOrDirectory);
	API_METHOD_WRAPPER_1(ScriptFile, loadEncryptedObject);
	API_VOID_METHOD_WRAPPER_3(ScriptFile, extractZipFile);
	API_VOID_METHOD_WRAPPER_0(ScriptFile, show);
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
	ConstScriptingObject(p, 3),
	f(f_)
{
	addConstant("FullPath", (int)FullPath);
	addConstant("NoExtension", (int)NoExtension);
	addConstant("Extension", (int)OnlyExtension);
	addConstant("Filename", (int)Filename);

	ADD_API_METHOD_0(getParentDirectory);
	ADD_API_METHOD_1(getChildFile);
	ADD_API_METHOD_1(createDirectory);
	ADD_API_METHOD_1(toString);
	ADD_API_METHOD_0(isFile);
	ADD_API_METHOD_0(isDirectory);
	ADD_API_METHOD_0(deleteFileOrDirectory);
	ADD_API_METHOD_1(writeObject);
	ADD_API_METHOD_1(writeString);
	ADD_API_METHOD_2(writeEncryptedObject);
	ADD_API_METHOD_3(writeAudioFile);
	ADD_API_METHOD_0(loadAsString);
	ADD_API_METHOD_0(loadAsObject);
	ADD_API_METHOD_1(loadEncryptedObject);
	ADD_API_METHOD_0(show);
	ADD_API_METHOD_3(extractZipFile);
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

bool ScriptingObjects::ScriptFile::isFile() const
{
	return f.existsAsFile();
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

bool ScriptingObjects::ScriptFile::writeString(String text)
{
	return f.replaceWithText(text);
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

void ScriptingObjects::ScriptFile::show()
{
	auto f_ = f;
	MessageManager::callAsync([f_]()
	{
		f_.revealToUser();
	});
}

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

		data->setProperty("Status", 0);
		data->setProperty("Progress", 0.0);
		data->setProperty("TotalBytesWritten", 0);
		data->setProperty("Cancel", false);
		data->setProperty("Target", tf.getFullPathName());
		data->setProperty("CurrentFile", "");
		data->setProperty("Error", "");
		
		int64 numBytesWritten = 0;

		WeakCallbackHolder cb(safeThis.get()->getScriptProcessor(), callback, 1);
		cb.setThisObject(safeThis.get());
		cb.incRefCount();

		if (cb)
		{
			cb.call1(var(data->clone()));
		}

		data->setProperty("Status", 1);
		int numEntries = zipFile.getNumEntries();
		bool callForEachFile = numEntries < 500;


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

			if (callForEachFile && cb)
				cb.call1(var(c));

			auto result = zipFile.uncompressEntry(i, tf, overwriteFiles);

			numBytesWritten += zipFile.getEntry(i)->uncompressedSize;

			if (result.failed())
			{
				c->setProperty("Error", result.getErrorMessage());

				data = c;

				if (cb)
					cb.call1(var(c));

				break;
			}

			if (c->getProperty("Cancel"))
			{
				c->setProperty("Error", "User abort");

				data = c;

				if (cb)
					cb.call1(var(c));

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
			cb.call1(var(c));
		}

		return SafeFunctionCall::OK;
	};

	auto p = dynamic_cast<Processor*>(getScriptProcessor());

	getScriptProcessor()->getMainController_()->getKillStateHandler().killVoicesAndCall(p, cb, MainController::KillStateHandler::SampleLoadingThread);
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
};

ScriptingObjects::ScriptDownloadObject::ScriptDownloadObject(ProcessorWithScriptingContent* pwsc, const URL& url, const File& targetFile_, var callback_) :
	ConstScriptingObject(pwsc, 3),
	callback(pwsc, callback_, 0),
	downloadURL(url),
	targetFile(targetFile_),
	jp(dynamic_cast<JavascriptProcessor*>(pwsc))
{
	data = new DynamicObject();
	addConstant("data", var(data));

	callback.setThisObject(this);
	callback.incRefCount();

	ADD_API_METHOD_0(resume);
	ADD_API_METHOD_0(stop);
	ADD_API_METHOD_0(abort);
	ADD_API_METHOD_0(isRunning);
	ADD_API_METHOD_0(getProgress);
	ADD_API_METHOD_0(getFullURL);
	ADD_API_METHOD_0(getStatusText);
	ADD_API_METHOD_0(getDownloadedTarget);
	ADD_API_METHOD_0(getDownloadSpeed);
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
	if (isRunning_ || forceUpdate)
	{
		download = nullptr;
		flushTemporaryFile();

		isRunning_ = false;
		isFinished = false;

		if (shouldAbort)
		{
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

			auto wis = downloadURL.createInputStream(false, nullptr, nullptr, String(), 0, nullptr, &status);

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

				download = downloadURL.downloadToFile(resumeFile, rangeHeader, this).release();

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

	auto wis = downloadURL.createInputStream(false, nullptr, nullptr, String(), HISE_SCRIPT_SERVER_TIMEOUT, nullptr, &status);

	if (Thread::currentThreadShouldExit())
		return;

	if (status == 200)
	{
		isRunning_ = true;
		download = downloadURL.downloadToFile(targetFile, {}, this).release();

		data->setProperty("numTotal", 0);
		data->setProperty("numDownloaded", 0);
		data->setProperty("finished", false);
		data->setProperty("success", false);

		call(true);
	}
	else
	{
		isFinished = true;

		data->setProperty("numTotal", 0);
		data->setProperty("numDownloaded", 0);
		data->setProperty("finished", true);
		data->setProperty("success", false);

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
	holder(otherHolder != nullptr ? otherHolder : dynamic_cast<ExternalDataHolder*>(c))
{
	if (holder != nullptr)
	{
		complexObject = holder->getComplexBaseType(getDataType(), index);
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

struct ScriptingObjects::ScriptAudioFile::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptAudioFile, loadFile);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getContent);
	API_VOID_METHOD_WRAPPER_0(ScriptAudioFile, update);
	API_VOID_METHOD_WRAPPER_2(ScriptAudioFile, setRange);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getNumSamples);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getSampleRate);
	API_METHOD_WRAPPER_0(ScriptAudioFile, getCurrentlyDisplayedIndex);
};

ScriptingObjects::ScriptAudioFile::ScriptAudioFile(ProcessorWithScriptingContent* pwsc, int index_, snex::ExternalDataHolder* otherHolder) :
	ScriptComplexDataReferenceBase(pwsc, 0, snex::ExternalData::DataType::AudioFile, otherHolder)
{
	ADD_API_METHOD_2(setRange);
	ADD_API_METHOD_1(loadFile);
	ADD_API_METHOD_0(getContent);
	ADD_API_METHOD_0(update);
	ADD_API_METHOD_0(getNumSamples);
	ADD_API_METHOD_0(getSampleRate);
	ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
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

struct ScriptingObjects::ScriptRingBuffer::Wrapper
{
	API_METHOD_WRAPPER_0(ScriptRingBuffer, getReadBuffer);
	API_METHOD_WRAPPER_3(ScriptRingBuffer, createPath);
	API_METHOD_WRAPPER_2(ScriptRingBuffer, getResizedBuffer);
};

ScriptingObjects::ScriptRingBuffer::ScriptRingBuffer(ProcessorWithScriptingContent* pwsc, int index, snex::ExternalDataHolder* other/*=nullptr*/):
	ScriptComplexDataReferenceBase(pwsc, index, snex::ExternalData::DataType::DisplayBuffer, other)
{
	ADD_API_METHOD_0(getReadBuffer);
	ADD_API_METHOD_3(createPath);
	ADD_API_METHOD_2(getResizedBuffer);
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

		

		return var(b);
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
	
	auto b = *getReadBuffer().getBuffer();

	auto hToUse = (int)src.getHeight();

	if (hToUse == -1)
		hToUse = b.size;

	Range<int> s_range(jmax<int>(0, (int)src.getWidth()), jmin<int>(b.size, hToUse));
	Range<float> valueRange(jmax<float>(-1.0f, src.getX()), jmin<float>(1.0f, src.getY()));

	int numValues = s_range.getLength();
	int numPixels = dst.getWidth();
	auto stride = roundToInt((float)numValues / (float)numPixels);

	auto sp = new PathObject(getScriptProcessor());
	
	auto& p = sp->getPath();

	auto startv = valueRange.getEnd() - (double)startValue * valueRange.getLength();

	p.startNewSubPath(0.0f, valueRange.getStart());
	p.startNewSubPath(0.0f, valueRange.getEnd());
    p.startNewSubPath(0.0f, startv);

	for (int i = 0; i < numValues; i += stride)
	{
		int numToLook = jmin(stride, numValues - i);

		auto avg = FloatVectorOperations::findMinAndMax(b.buffer.getReadPointer(0, i), numToLook);

		auto value = avg.getEnd();

		if(std::abs(avg.getStart()) > std::abs(avg.getEnd()))
			value = avg.getStart();

		p.lineTo((float)i, valueRange.getEnd() - valueRange.clipValue(value));
	};

	p.lineTo(numValues, startv);

	PathFactory::scalePath(p, dst);

	return var(sp);
}

struct ScriptingObjects::ScriptTableData::Wrapper
{
	API_VOID_METHOD_WRAPPER_0(ScriptTableData, reset);
	API_VOID_METHOD_WRAPPER_4(ScriptTableData, setTablePoint);
	API_VOID_METHOD_WRAPPER_2(ScriptTableData, addTablePoint);
	API_METHOD_WRAPPER_1(ScriptTableData, getTableValueNormalised);
	API_METHOD_WRAPPER_0(ScriptTableData, getCurrentlyDisplayedIndex);
};

ScriptingObjects::ScriptTableData::ScriptTableData(ProcessorWithScriptingContent* pwsc, int index, snex::ExternalDataHolder* otherHolder):
	ScriptComplexDataReferenceBase(pwsc, index, snex::ExternalData::DataType::Table, otherHolder)
{
	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(addTablePoint);
	ADD_API_METHOD_4(setTablePoint);
	ADD_API_METHOD_1(getTableValueNormalised);
	ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
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
		return st->getInterpolatedValue((double)SAMPLE_LOOKUP_TABLE_SIZE * normalisedInput, sendNotificationAsync);
	}

	return 0.0f;
}


float ScriptingObjects::ScriptTableData::getCurrentlyDisplayedIndex() const
{
	return getCurrentDisplayIndexBase();
}

struct ScriptingObjects::ScriptSliderPackData::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptSliderPackData, setValue);
	API_VOID_METHOD_WRAPPER_1(ScriptSliderPackData, setNumSliders);
	API_METHOD_WRAPPER_1(ScriptSliderPackData, getValue);
	API_METHOD_WRAPPER_0(ScriptSliderPackData, getNumSliders);
	API_VOID_METHOD_WRAPPER_3(ScriptSliderPackData, setRange);
	API_METHOD_WRAPPER_0(ScriptSliderPackData, getCurrentlyDisplayedIndex);
};

ScriptingObjects::ScriptSliderPackData::ScriptSliderPackData(ProcessorWithScriptingContent* pwsc, int dataIndex, snex::ExternalDataHolder* otherHolder) :
	ScriptComplexDataReferenceBase(pwsc, dataIndex, snex::ExternalData::DataType::SliderPack, otherHolder)
{
	ADD_API_METHOD_2(setValue);
	ADD_API_METHOD_1(setNumSliders);
	ADD_API_METHOD_1(getValue);
	ADD_API_METHOD_0(getNumSliders);
	ADD_API_METHOD_3(setRange);
	ADD_API_METHOD_0(getCurrentlyDisplayedIndex);
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

struct ScriptingObjects::ScriptingSamplerSound::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptingSamplerSound, setFromJSON);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, get);
	API_VOID_METHOD_WRAPPER_2(ScriptingSamplerSound, set);
	API_VOID_METHOD_WRAPPER_0(ScriptingSamplerSound, deleteSample);
	API_METHOD_WRAPPER_0(ScriptingSamplerSound, duplicateSample);
	API_METHOD_WRAPPER_0(ScriptingSamplerSound, loadIntoBufferArray);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, replaceAudioFile);
	API_METHOD_WRAPPER_1(ScriptingSamplerSound, refersToSameSample);
};

ScriptingObjects::ScriptingSamplerSound::ScriptingSamplerSound(ProcessorWithScriptingContent* p, ModulatorSampler* sampler_, ModulatorSamplerSound::Ptr sound_) :
	ConstScriptingObject(p, ModulatorSamplerSound::numProperties),
	sound(sound_),
	sampler(sampler_)
{
	ADD_API_METHOD_1(setFromJSON);
	ADD_API_METHOD_1(get);
	ADD_API_METHOD_2(set);
	ADD_API_METHOD_0(deleteSample);
	ADD_API_METHOD_0(duplicateSample);
	ADD_API_METHOD_0(loadIntoBufferArray);
	ADD_API_METHOD_1(replaceAudioFile);
	ADD_API_METHOD_1(refersToSameSample);

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
	ModulatorSamplerSound::Ptr other = sound;

	auto id = sampleIds[index];

	auto av = [other, id]()
	{
		if (other != nullptr)
			return other->getSampleProperty(id);

		return var();
	};

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

	return sound->getSampleProperty(sampleIds[propertyIndex]);
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
					VariantBuffer::Ptr l = new VariantBuffer(numSamplesToRead);
					AudioSampleBuffer b;
					float* data[1] = { l->buffer.getWritePointer(0) };
					b.setDataToReferTo(data, 1, numSamplesToRead);
					reader->read(&b, 0, numSamplesToRead, 0, true, false);
					channelData.add(var(l));
				}
				else
				{
					VariantBuffer::Ptr l = new VariantBuffer(numSamplesToRead);
					VariantBuffer::Ptr r = new VariantBuffer(numSamplesToRead);

					AudioSampleBuffer b;

					float* data[2] = { l->buffer.getWritePointer(0), r->buffer.getWritePointer(0) };

					b.setDataToReferTo(data, 2, numSamplesToRead);

					reader->read(&b, 0, numSamplesToRead, 0, true, true);

					channelData.add(var(l));
					channelData.add(var(r));
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
		handler->getSampler()->getSampleMap()->removeSound(soundCopy);

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

void ScriptingObjects::ScriptingModulator::setAttribute(int index, float value)
{
	if (checkValidObject())
		mod->setAttribute(index, value, sendNotification);
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
#if USE_BACKEND
	if (objectExists() && !objectDeleted())
	{
		auto *editor = GET_BACKEND_ROOT_WINDOW(componentToNotify);

		Processor *p = ProcessorHelpers::getFirstProcessorWithName(editor->getMainSynthChain(), mod->getId());

		if (p != nullptr)
		{
			editor->getMainPanel()->setRootProcessorWithUndo(p);
		}
	}
#else 
	ignoreUnused(componentToNotify);
#endif
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
		return ProcessorHelpers::getBase64String(mod);
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
			RETURN_IF_NO_THROW();
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
		effect->setAttribute(parameterIndex, newValue, sendNotification);
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
		return ProcessorHelpers::getBase64String(effect);
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
			RETURN_IF_NO_THROW();
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
	API_VOID_METHOD_WRAPPER_1(ScriptingSlotFX, swap);
	API_METHOD_WRAPPER_0(ScriptingSlotFX, getCurrentEffect);
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
};



void ScriptingObjects::ScriptingSlotFX::clear()
{
	if (auto slot = getSlotFX())
	{
        slot->reset();
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
			SuspendHelpers::ScopedTicket ticket(slot->getMainController());

			slot->getMainController()->getJavascriptThreadPool().killVoicesAndExtendTimeOut(jp);

			LockHelpers::freeToGo(slot->getMainController());
			slot->setEffect(effectName);
		}

		jassert(slot->getCurrentEffect()->getType() == Identifier(effectName));

		return new ScriptingEffect(getScriptProcessor(), slot->getCurrentEffect());
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
			return new ScriptingEffect(getScriptProcessor(), fx);
		}
	}

	return {};
}

void ScriptingObjects::ScriptingSlotFX::swap(var otherSlot)
{
	if (auto t = getSlotFX())
	{
		if (auto sl = dynamic_cast<ScriptingSlotFX*>(otherSlot.getObject()))
		{
			if (auto other = sl->getSlotFX())
			{
				t->swap(other);
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
}

SlotFX* ScriptingObjects::ScriptingSlotFX::getSlotFX()
{
	return dynamic_cast<SlotFX*>(slotFX.get());
}

struct ScriptingObjects::ScriptRoutingMatrix::Wrapper
{
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, addConnection);
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, removeConnection);
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, addSendConnection);
	API_METHOD_WRAPPER_2(ScriptRoutingMatrix, removeSendConnection);
	API_VOID_METHOD_WRAPPER_0(ScriptRoutingMatrix, clear);
	API_METHOD_WRAPPER_1(ScriptRoutingMatrix, getSourceGainValue);
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

// ScriptingSynth ==============================================================================================================

struct ScriptingObjects::ScriptingSynth::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptingSynth, setAttribute);
    API_METHOD_WRAPPER_1(ScriptingSynth, getAttribute);
    API_METHOD_WRAPPER_1(ScriptingSynth, getAttributeId);
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
		synth->setAttribute(parameterIndex, newValue, sendNotification);
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
		return ProcessorHelpers::getBase64String(synth);
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
			RETURN_IF_NO_THROW();
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
		mp->setAttribute(index, value, sendNotification);
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
			RETURN_IF_NO_THROW();
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


ScriptingObjects::ScriptingAudioSampleProcessor::ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, AudioSampleProcessor *sampleProcessor) :
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
			reportScriptError("You must call Engine.loadAudioFilesIntoPool() before using this method");
#endif

		auto asp = dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get());
		asp->getBuffer().fromBase64String(fileName);
	}
}

String ScriptingObjects::ScriptingAudioSampleProcessor::getFilename()
{
	if (checkValidObject())
	{
		if (checkValidObject())
		{
			return dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->getFileName();
		}
	}

	return {};
}

var ScriptingObjects::ScriptingAudioSampleProcessor::getSampleStart()
{
	if (checkValidObject())
		return dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->getBuffer().getCurrentRange().getStart();

	return 0;
}

void ScriptingObjects::ScriptingAudioSampleProcessor::setSampleRange(int start, int end)
{
	if (checkValidObject())
		dynamic_cast<AudioSampleProcessor*>(audioSampleProcessor.get())->getBuffer().setRange(Range<int>(start, end));
}

var ScriptingObjects::ScriptingAudioSampleProcessor::getAudioFile(int slotIndex)
{
	if (checkValidObject())
	{
		if (auto ed = dynamic_cast<ExternalDataHolder*>(audioSampleProcessor.get()))
			return var(new ScriptAudioFile(getScriptProcessor(), slotIndex, ed));
	}

	reportScriptError("Not a valid object");
	RETURN_IF_NO_THROW(var());
}

int ScriptingObjects::ScriptingAudioSampleProcessor::getSampleLength() const
{
	if (checkValidObject())
	{
		return dynamic_cast<const AudioSampleProcessor*>(audioSampleProcessor.get())->getBuffer().getCurrentRange().getLength();
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
		if (auto ed = dynamic_cast<ExternalDataHolder*>(tableProcessor.get()))
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
		if (auto ed = dynamic_cast<ExternalDataHolder*>(sp.get()))
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
	tc(p, {}, 0)
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

	if (index = 1)
	{
		return tc.createDebugObject("timerCallback");
	}
	
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
	tc = WeakCallbackHolder(getScriptProcessor(), callbackFunction, 0);
	tc.setThisObject(this);
	tc.incRefCount();
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
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, isNoteOn);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, isNoteOff);
	API_METHOD_WRAPPER_0(ScriptingMessageHolder, isController);
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
var ScriptingObjects::ScriptingMessageHolder::getControllerNumber() const { return (int)e.getControllerNumber(); }
var ScriptingObjects::ScriptingMessageHolder::getControllerValue() const { return (int)e.getControllerNumber(); }
int ScriptingObjects::ScriptingMessageHolder::getChannel() const { return (int)e.getChannel(); }
void ScriptingObjects::ScriptingMessageHolder::setChannel(int newChannel) { e.setChannel(newChannel); }
void ScriptingObjects::ScriptingMessageHolder::setNoteNumber(int newNoteNumber) { e.setNoteNumber(newNoteNumber); }
void ScriptingObjects::ScriptingMessageHolder::setVelocity(int newVelocity) { e.setVelocity((uint8)newVelocity); }
void ScriptingObjects::ScriptingMessageHolder::setControllerNumber(int newControllerNumber) { e.setControllerNumber(newControllerNumber);}
void ScriptingObjects::ScriptingMessageHolder::setControllerValue(int newControllerValue) { e.setControllerValue(newControllerValue); }

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
bool ScriptingObjects::ScriptingMessageHolder::isNoteOn() const { return e.isNoteOn(); }
bool ScriptingObjects::ScriptingMessageHolder::isNoteOff() const { return e.isNoteOff(); }
bool ScriptingObjects::ScriptingMessageHolder::isController() const { return e.isController(); }

String ScriptingObjects::ScriptingMessageHolder::dump() const
{
	String x;
	x << "Type: " << e.getTypeAsString() << ", ";
	x << "Channel: " << String(e.getChannel()) << ", ";
	x << "Number: " << String(e.getNoteNumber()) << ", ";
	x << "Value: " << String(e.getVelocity()) << ", ";
	x << "EventId: " << String(e.getEventId()) << ", ";
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
	API_METHOD_WRAPPER_0(ScriptedMidiPlayer, getTimeSignature);
	API_METHOD_WRAPPER_1(ScriptedMidiPlayer, setTimeSignature);
};

ScriptingObjects::ScriptedMidiPlayer::ScriptedMidiPlayer(ProcessorWithScriptingContent* p, MidiPlayer* player_):
	MidiPlayerBaseType(player_),
	ConstScriptingObject(p, 0)
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
}

ScriptingObjects::ScriptedMidiPlayer::~ScriptedMidiPlayer()
{
	connectedPanel = nullptr;
}

juce::String ScriptingObjects::ScriptedMidiPlayer::getDebugValue() const
{
	if (!sequenceValid())
		return {};

	return String(getPlayer()->getPlaybackPosition(), 2);
}

juce::String ScriptingObjects::ScriptedMidiPlayer::getDebugName() const
{

	if (!sequenceValid())
		return {};

	if (auto seq = getPlayer()->getCurrentSequence())
		return seq->getId().toString();

	return "No sequence loaded";
}

void ScriptingObjects::ScriptedMidiPlayer::trackIndexChanged()
{
	if (auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(connectedPanel.get()))
	{
		panel->repaint();
	}
}

void ScriptingObjects::ScriptedMidiPlayer::sequenceIndexChanged()
{
	if (auto panel = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(connectedPanel.get()))
	{
		panel->repaint();
	}
}

void ScriptingObjects::ScriptedMidiPlayer::sequencesCleared()
{
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

void ScriptingObjects::ScriptedMidiPlayer::connectToPanel(var panel)
{
	if (auto p = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(panel.getObject()))
	{
		connectedPanel = dynamic_cast<ConstScriptingObject*>(p);
	}
	else
		reportScriptError("Invalid panel");
}

var ScriptingObjects::ScriptedMidiPlayer::getEventList()
{
	if (!sequenceValid())
		return {};

	auto list = getPlayer()->getCurrentSequence()->getEventList(getPlayer()->getSampleRate(), getPlayer()->getMainController()->getBpm());

	Array<var> eventHolders;

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

		getPlayer()->flushEdit(events);
	}
	else
		reportScriptError("Input is not an array");
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

#define DECLARE_ID(x) static Identifier x(#x);
namespace TimeSigIds
{
DECLARE_ID(Nominator);
DECLARE_ID(Denominator);
DECLARE_ID(NumBars);
DECLARE_ID(LoopStart);
DECLARE_ID(LoopEnd);
}
#undef DECLARE_ID

var ScriptingObjects::ScriptedMidiPlayer::getTimeSignature()
{
	if (sequenceValid())
	{
		auto sig = getSequence()->getTimeSignature();

		DynamicObject::Ptr newObj = new DynamicObject();
		newObj->setProperty(TimeSigIds::Nominator, sig.nominator);
		newObj->setProperty(TimeSigIds::Denominator, sig.denominator);
		newObj->setProperty(TimeSigIds::NumBars, sig.numBars);
		newObj->setProperty(TimeSigIds::LoopStart, sig.normalisedLoopRange.getStart());
		newObj->setProperty(TimeSigIds::LoopEnd, sig.normalisedLoopRange.getEnd());

		return var(newObj);
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

void ScriptingObjects::ScriptedMidiPlayer::sequenceLoaded(HiseMidiSequence::Ptr newSequence)
{

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


struct ScriptingObjects::ScriptedLookAndFeel::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, registerFunction);
	API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, setGlobalFont);
};


ScriptingObjects::ScriptedLookAndFeel::ScriptedLookAndFeel(ProcessorWithScriptingContent* sp) :
	ConstScriptingObject(sp, 0),
	g(new GraphicsObject(sp, this)),
	functions(new DynamicObject())
{
	ADD_API_METHOD_2(registerFunction);
	ADD_API_METHOD_2(setGlobalFont);

	getScriptProcessor()->getMainController_()->setCurrentScriptLookAndFeel(this);
}

ScriptingObjects::ScriptedLookAndFeel::~ScriptedLookAndFeel()
{
	
}

void ScriptingObjects::ScriptedLookAndFeel::registerFunction(var functionName, var function)
{
	if (HiseJavascriptEngine::isJavascriptFunction(function))
	{
		functions.getDynamicObject()->setProperty(Identifier(functionName.toString()), function);
	}
}

void ScriptingObjects::ScriptedLookAndFeel::setGlobalFont(const String& fontName, float fontSize)
{
	f = getScriptProcessor()->getMainController_()->getFontFromString(fontName, fontSize);
}

Array<Identifier> ScriptingObjects::ScriptedLookAndFeel::getAllFunctionNames()
{
	static const Array<Identifier> sa =
	{
		"drawAlertWindow",
		"getAlertWindowMarkdownStyleData",
		"drawAlertWindowIcon",
		"drawPopupMenuBackground",
		"drawPopupMenuItem",
		"drawToggleButton",
		"drawRotarySlider",
		"drawLinearSlider",
		"drawDialogButton",
		"drawComboBox",
		"drawNumberTag",
		"drawPresetBrowserBackground",
		"drawPresetBrowserColumnBackground",
		"drawPresetBrowserListItem",
		"drawPresetBrowserSearchBar",
		"drawPresetBrowserTag",
		"drawTablePath",
		"drawTablePoint",
		"drawTableRuler",
		"drawScrollbar",
		"drawMidiDropper",
		"drawAhdsrBall",
		"drawAhdsrPath"
	};

	return sa;
}

bool ScriptingObjects::ScriptedLookAndFeel::callWithGraphics(Graphics& g_, const Identifier& functionname, var argsObject)
{
	// If this hits, you need to add that id to the array above.
	jassert(getAllFunctionNames().contains(functionname));

	auto f = functions.getProperty(functionname, {});

	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		var args[2];
		
		args[0] = var(g);
		args[1] = argsObject;

		var thisObject(this);
		var::NativeFunctionArgs arg(thisObject, args, 2);
		auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();
		Result r = Result::ok();
		
		try
		{
			engine->callExternalFunctionRaw(f, arg);
		}
		catch (String& errorMessage)
		{
			debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), errorMessage);
		}
		catch (HiseJavascriptEngine::RootObject::Error& e)
		{
			auto p = dynamic_cast<Processor*>(getScriptProcessor());
			debugToConsole(p, e.toString(p) + e.errorMessage);
		}
		
		g->getDrawHandler().flush();

		DrawActions::Handler::Iterator iter(&g->getDrawHandler());

		while (auto action = iter.getNextAction())
		{
			action->perform(g_);
		}

		return true;
	}

	return false;
}

var ScriptingObjects::ScriptedLookAndFeel::callDefinedFunction(const Identifier& functionname, var* args, int numArgs)
{
	// If this hits, you need to add that id to the array above.
	jassert(getAllFunctionNames().contains(functionname));

	auto f = functions.getProperty(functionname, {});

	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		var thisObject(this);
		var::NativeFunctionArgs arg(thisObject, args, numArgs);
		auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();
		Result r = Result::ok();

		try
		{
			return engine->callExternalFunctionRaw(f, arg);
		}
		catch (String& errorMessage)
		{
			debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), errorMessage);
		}
		catch (HiseJavascriptEngine::RootObject::Error& )
		{

		}
	}

	return {};
}

Identifier ScriptingObjects::ScriptedLookAndFeel::Laf::getIdOfParentFloatingTile(Component& c)
{
	if (auto ft = c.findParentComponentOfClass<FloatingTile>())
	{
		return ft->getCurrentFloatingPanel()->getIdentifierForBaseClass();
	}

	return {};
}

bool ScriptingObjects::ScriptedLookAndFeel::Laf::addParentFloatingTile(Component& c, DynamicObject* obj)
{
	auto id = getIdOfParentFloatingTile(c);

	if (id.isValid())
	{
		obj->setProperty("parentType", id.toString());
		return true;
	}

	return false;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAlertBox(Graphics& g_, AlertWindow& w, const Rectangle<int>& ta, TextLayout& tl)
{
	if (functionDefined("drawAlertWindow"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(w.getLocalBounds().toFloat()));
		obj->setProperty("title", w.getName()); 

		addParentFloatingTile(w, obj);

		if (get()->callWithGraphics(g_, "drawAlertWindow", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawAlertBox(g_, w, ta, tl);
}

hise::MarkdownLayout::StyleData ScriptingObjects::ScriptedLookAndFeel::Laf::getAlertWindowMarkdownStyleData()
{
	auto s = MessageWithIcon::LookAndFeelMethods::getAlertWindowMarkdownStyleData();

	if (functionDefined("getAlertWindowMarkdownStyleData"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("textColour", s.textColour.getARGB());
		obj->setProperty("codeColour", s.codeColour.getARGB());
		obj->setProperty("linkColour", s.linkColour.getARGB());
		obj->setProperty("headlineColour", s.headlineColour.getARGB());

		obj->setProperty("headlineFont", s.boldFont.getTypefaceName());
		obj->setProperty("font", s.f.getTypefaceName());
		obj->setProperty("fontSize", s.fontSize);

		var x = var(obj);

		auto nObj = get()->callDefinedFunction("getAlertWindowMarkdownStyleData", &x, 1);

		if (nObj.getDynamicObject() != nullptr)
		{
			s.textColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["textColour"]);
			s.linkColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["linkColour"]);
			s.codeColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["codeColour"]);
			s.headlineColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["headlineColour"]);

			s.boldFont = getMainController()->getFontFromString(nObj.getProperty("headlineFont", "Default"), s.boldFont.getHeight());

			s.fontSize = nObj["fontSize"];
			s.f = getMainController()->getFontFromString(nObj.getProperty("font", "Default"), s.boldFont.getHeight());
		}
	}
	
	return s;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPopupMenuBackground(Graphics& g_, int width, int height)
{
	if (functionDefined("drawPopupMenuBackground"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("width", width);
		obj->setProperty("height", height);

		if (get()->callWithGraphics(g_, "drawPopupMenuBackground", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawPopupMenuBackground(g_, width, height);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPopupMenuItem(Graphics& g_, const Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* textColourToUse)
{
	if (functionDefined("drawPopupMenuItem"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("isSeparator", isSeparator);
		obj->setProperty("isActive", isActive);
		obj->setProperty("isHighlighted", isHighlighted);
		obj->setProperty("isTicked", isTicked);
		obj->setProperty("hasSubMenu", hasSubMenu);
		obj->setProperty("text", text);

		if (get()->callWithGraphics(g_, "drawPopupMenuItem", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawPopupMenuItem(g_, area, isSeparator, isActive, isHighlighted, isTicked, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawToggleButton(Graphics &g_, ToggleButton &b, bool isMouseOverButton, bool isButtonDown)
{
	if (functionDefined("drawToggleButton"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(b.getLocalBounds().toFloat()));
		obj->setProperty("enabled", b.isEnabled());
		obj->setProperty("text", b.getButtonText());
		obj->setProperty("over", isMouseOverButton);
		obj->setProperty("down", isButtonDown);
		obj->setProperty("value", b.getToggleState());

		obj->setProperty("bgColour", b.findColour(HiseColourScheme::ComponentOutlineColourId).getARGB());
		obj->setProperty("itemColour1", b.findColour(HiseColourScheme::ComponentFillTopColourId).getARGB());
		obj->setProperty("itemColour2", b.findColour(HiseColourScheme::ComponentFillBottomColourId).getARGB());
		obj->setProperty("textColour", b.findColour(HiseColourScheme::ComponentTextColourId).getARGB());

		addParentFloatingTile(b, obj);

		if (get()->callWithGraphics(g_, "drawToggleButton", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawToggleButton(g_, b, isMouseOverButton, isButtonDown);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawRotarySlider(Graphics &g_, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	if (functionDefined("drawRotarySlider"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		s.setTextBoxStyle (Slider::NoTextBox, false, -1, -1);

		obj->setProperty("id", s.getComponentID());
		obj->setProperty("enabled", s.isEnabled());
		obj->setProperty("text", s.getName());
		obj->setProperty("area", ApiHelpers::getVarRectangle(s.getLocalBounds().toFloat()));

		obj->setProperty("value", s.getValue());
		
		NormalisableRange<double> range = NormalisableRange<double>(s.getMinimum(), s.getMaximum(), s.getInterval(), s.getSkewFactor());
		obj->setProperty("valueNormalized", range.convertTo0to1(s.getValue()));

		obj->setProperty("valueSuffixString", s.getTextFromValue(s.getValue()));
		obj->setProperty("suffix", s.getTextValueSuffix());
		obj->setProperty("skew", s.getSkewFactor());
		obj->setProperty("min", s.getMinimum());
		obj->setProperty("max", s.getMaximum());

		obj->setProperty("clicked", s.isMouseButtonDown());
		obj->setProperty("hover", s.isMouseOver());

		obj->setProperty("bgColour", s.findColour(HiseColourScheme::ComponentOutlineColourId).getARGB());
		obj->setProperty("itemColour1", s.findColour(HiseColourScheme::ComponentFillTopColourId).getARGB());
		obj->setProperty("itemColour2", s.findColour(HiseColourScheme::ComponentFillBottomColourId).getARGB());
		obj->setProperty("textColour", s.findColour(HiseColourScheme::ComponentTextColourId).getARGB());

		addParentFloatingTile(s, obj);

		if (get()->callWithGraphics(g_, "drawRotarySlider", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawRotarySlider(g_, -1, -1, width, height, -1, -1, -1, s);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawLinearSlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/, const Slider::SliderStyle style, Slider &slider)
{
	if (functionDefined("drawLinearSlider"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		obj->setProperty("id", slider.getComponentID());
		obj->setProperty("enabled", slider.isEnabled());
		obj->setProperty("text", slider.getName());
		obj->setProperty("area", ApiHelpers::getVarRectangle(slider.getLocalBounds().toFloat()));

		obj->setProperty("valueSuffixString", slider.getTextFromValue(slider.getValue()));
		obj->setProperty("suffix", slider.getTextValueSuffix());
		obj->setProperty("skew", slider.getSkewFactor());

		obj->setProperty("style", style);	// Horizontal:2, Vertical:3, Range:9

		// Vertical & Horizontal style slider
		obj->setProperty("min", slider.getMinimum());
		obj->setProperty("max", slider.getMaximum());
		obj->setProperty("value", slider.getValue());
		
		NormalisableRange<double> range = NormalisableRange<double>(slider.getMinimum(), slider.getMaximum(), slider.getInterval(), slider.getSkewFactor());
		obj->setProperty("valueNormalized", range.convertTo0to1(slider.getValue()));

		// Range style slider
		double minv = 0.0;
		double maxv = 1.0;

		if (slider.isTwoValue())
		{
			minv = slider.getMinValue();
			maxv = slider.getMaxValue();
		}

		obj->setProperty("valueRangeStyleMin", minv);
		obj->setProperty("valueRangeStyleMax", maxv);

		obj->setProperty("valueRangeStyleMinNormalized", range.convertTo0to1(minv));
		obj->setProperty("valueRangeStyleMaxNormalized", range.convertTo0to1(maxv));

		obj->setProperty("clicked", slider.isMouseButtonDown());
		obj->setProperty("hover", slider.isMouseOver());

		obj->setProperty("bgColour", slider.findColour(HiseColourScheme::ComponentOutlineColourId).getARGB());
		obj->setProperty("itemColour1", slider.findColour(HiseColourScheme::ComponentFillTopColourId).getARGB());
		obj->setProperty("itemColour2", slider.findColour(HiseColourScheme::ComponentFillBottomColourId).getARGB());
		obj->setProperty("textColour", slider.findColour(HiseColourScheme::ComponentTextColourId).getARGB());

		addParentFloatingTile(slider, obj);

		if (get()->callWithGraphics(g, "drawLinearSlider", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawLinearSlider(g, -1, -1, width, height, -1, -1, -1, style, slider);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawButtonText(Graphics &g_, TextButton &button, bool isMouseOverButton, bool isButtonDown)
{
	if (functionDefined("drawDialogButton"))
		return;

	static const Identifier pb("PresetBrowser");

	if (getIdOfParentFloatingTile(button) == pb)
		PresetBrowserLookAndFeelMethods::drawPresetBrowserButtonText(g_, button, isMouseOverButton, isButtonDown);
	else
		GlobalHiseLookAndFeel::drawButtonText(g_, button, isMouseOverButton, isButtonDown);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawComboBox(Graphics& g_, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb)
{
	if (functionDefined("drawComboBox"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(cb.getLocalBounds().toFloat()));

		auto text = cb.getText();

		if (text.isEmpty())
		{
			if (cb.getNumItems() == 0)
				text = cb.getTextWhenNoChoicesAvailable();
			else
				text = cb.getTextWhenNothingSelected();
		}

		obj->setProperty("text", text);
		obj->setProperty("active", cb.getSelectedId() != 0);
		obj->setProperty("enabled", cb.isEnabled() && cb.getNumItems() > 0);

		obj->setProperty("bgColour", cb.findColour(HiseColourScheme::ComponentOutlineColourId).getARGB());
		obj->setProperty("itemColour1", cb.findColour(HiseColourScheme::ComponentFillTopColourId).getARGB());
		obj->setProperty("itemColour2", cb.findColour(HiseColourScheme::ComponentFillBottomColourId).getARGB());
		obj->setProperty("textColour", cb.findColour(HiseColourScheme::ComponentTextColourId).getARGB());

		addParentFloatingTile(cb, obj);

		if (get()->callWithGraphics(g_, "drawComboBox", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawComboBox(g_, width, height, isButtonDown, buttonX, buttonY, buttonW, buttonH, cb);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::positionComboBoxText(ComboBox &c, Label &labelToPosition)
{
	if (functionDefined("drawComboBox"))
	{
		labelToPosition.setVisible(false);
		return;
	}

	GlobalHiseLookAndFeel::positionComboBoxText(c, labelToPosition);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label)
{
	if (functionDefined("drawComboBox"))
	{
		label.setVisible(false);
		return;
	}

	GlobalHiseLookAndFeel::drawComboBoxTextWhenNothingSelected(g, box, label);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawButtonBackground(Graphics& g_, Button& button, const Colour& bg, bool isMouseOverButton, bool isButtonDown)
{
	if (functionDefined("drawDialogButton"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(button.getLocalBounds().toFloat()));
		obj->setProperty("text", button.getButtonText());
		obj->setProperty("enabled", button.isEnabled());
		obj->setProperty("over", isMouseOverButton);
		obj->setProperty("down", isButtonDown);
		obj->setProperty("value", button.getToggleState());
		obj->setProperty("bgColour", bg.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		addParentFloatingTile(button, obj);

		if (get()->callWithGraphics(g_, "drawDialogButton", var(obj)))
			return;
	}

	static const Identifier pb("PresetBrowser");

	if(getIdOfParentFloatingTile(button) == pb)
		PresetBrowserLookAndFeelMethods::drawPresetBrowserButtonBackground(g_, button, bg, isMouseOverButton, isButtonDown);
	else
		GlobalHiseLookAndFeel::drawButtonBackground(g_, button, bg, isMouseOverButton, isButtonDown);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawNumberTag(Graphics& g_, Colour& c, Rectangle<int> area, int offset, int size, int number)
{
	if (auto l = get())
	{
		if (number != -1)
		{
			DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
			obj->setProperty("macroIndex", number - 1);

			if (l->callWithGraphics(g_, "drawNumberTag", var(obj)))
				return;
		}
	}

	NumberTag::LookAndFeelMethods::drawNumberTag(g_, c, area, offset, size, number);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPresetBrowserBackground(Graphics& g_, PresetBrowser* p)
{
	if (functionDefined("drawPresetBrowserBackground"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(p->getLocalBounds().toFloat()));
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserBackground", var(obj)))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawPresetBrowserBackground(g_, p);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawColumnBackground(Graphics& g_, Rectangle<int> listArea, const String& emptyText)
{
	if (functionDefined("drawPresetBrowserColumnBackground"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(listArea.toFloat()));
		obj->setProperty("text", emptyText);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserColumnBackground", var(obj)))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawColumnBackground(g_, listArea, emptyText);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawListItem(Graphics& g_, int columnIndex, int rowIndex, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode)
{
	if (functionDefined("drawPresetBrowserListItem"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(position.toFloat()));
		obj->setProperty("columnIndex", columnIndex);
		obj->setProperty("rowIndex", rowIndex);
		obj->setProperty("text", itemName);
		obj->setProperty("selected", rowIsSelected);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserListItem", var(obj)))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawListItem(g_, columnIndex, rowIndex, itemName, position, rowIsSelected, deleteMode);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawSearchBar(Graphics& g_, Rectangle<int> area)
{
	if (functionDefined("drawPresetBrowserSearchBar"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		auto p = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(p);

		static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
	103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
	191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
	218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
	95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

		p->getPath().loadPathFromData(searchIcon, sizeof(searchIcon));
		p->getPath().applyTransform(AffineTransform::rotation(float_Pi));
		p->getPath().scaleToFit(6.0f, 5.0f, 18.0f, 18.0f, true);

		obj->setProperty("icon", var(p));

		if (get()->callWithGraphics(g_, "drawPresetBrowserSearchBar", var(obj)))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawSearchBar(g_, area);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTablePath(Graphics& g_, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness)
{
	if (functionDefined("drawTablePath"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(sp);

		sp->getPath() = p;

		obj->setProperty("path", var(sp));

		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("lineThickness", lineThickness);
		obj->setProperty("bgColour", te.findColour(TableEditor::ColourIds::bgColour).getARGB());
		obj->setProperty("itemColour", te.findColour(TableEditor::ColourIds::fillColour).getARGB());
		obj->setProperty("itemColour2", te.findColour(TableEditor::ColourIds::lineColour).getARGB());
		obj->setProperty("textColour", te.findColour(TableEditor::ColourIds::rulerColour).getARGB());

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTablePath", var(obj)))
			return;
	}

	if (auto tl = dynamic_cast<TableEditor::LookAndFeelMethods*>(&te.getLookAndFeel()))
		tl->drawTablePath(g_, te, p, area, lineThickness);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTablePoint(Graphics& g_, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged)
{
	if (functionDefined("drawTablePoint"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		obj->setProperty("tablePoint", ApiHelpers::getVarRectangle(tablePoint));
		obj->setProperty("isEdge", isEdge);
		obj->setProperty("hover", isHover);
		obj->setProperty("clicked", isDragged);
		obj->setProperty("bgColour", te.findColour(TableEditor::ColourIds::bgColour).getARGB());
		obj->setProperty("itemColour", te.findColour(TableEditor::ColourIds::fillColour).getARGB());
		obj->setProperty("itemColour2", te.findColour(TableEditor::ColourIds::lineColour).getARGB());
		obj->setProperty("textColour", te.findColour(TableEditor::ColourIds::rulerColour).getARGB());

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTablePoint", var(obj)))
			return;
	}

	if (auto tl = dynamic_cast<TableEditor::LookAndFeelMethods*>(&te.getLookAndFeel()))
		tl->drawTablePoint(g_, te, tablePoint, isEdge, isHover, isDragged);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableRuler(Graphics& g_, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition)
{
	if (functionDefined("drawTableRuler"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("position", rulerPosition);
		obj->setProperty("lineThickness", lineThickness);
		obj->setProperty("bgColour", te.findColour(TableEditor::ColourIds::bgColour).getARGB());
		obj->setProperty("itemColour", te.findColour(TableEditor::ColourIds::fillColour).getARGB());
		obj->setProperty("itemColour2", te.findColour(TableEditor::ColourIds::lineColour).getARGB());
		obj->setProperty("textColour", te.findColour(TableEditor::ColourIds::rulerColour).getARGB());

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTableRuler", var(obj)))
			return;
	}

	if (auto tl = dynamic_cast<TableEditor::LookAndFeelMethods*>(&te.getLookAndFeel()))
		tl->drawTableRuler(g_, te, area, lineThickness, rulerPosition);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawScrollbar(Graphics& g_, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
	if (functionDefined("drawScrollbar"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		auto fullArea = Rectangle<int>(x, y, width, height).toFloat();

		Rectangle<float> thumbArea;

		if(isScrollbarVertical)
			thumbArea = Rectangle<int>(x, y + thumbStartPosition, width, thumbSize).toFloat();
		else
			thumbArea = Rectangle<int>(x + thumbStartPosition, y, thumbSize, height).toFloat();

		obj->setProperty("area", ApiHelpers::getVarRectangle(fullArea));
		obj->setProperty("handle", ApiHelpers::getVarRectangle(thumbArea));
		obj->setProperty("vertical", isScrollbarVertical);
		obj->setProperty("over", isMouseOver);
		obj->setProperty("down", isMouseDown);
		obj->setProperty("bgColour", scrollbar.findColour(ScrollBar::ColourIds::backgroundColourId).getARGB());
		obj->setProperty("itemColour", scrollbar.findColour(ScrollBar::ColourIds::thumbColourId).getARGB());
		obj->setProperty("itemColour2", scrollbar.findColour(ScrollBar::ColourIds::trackColourId).getARGB());
		
		addParentFloatingTile(scrollbar, obj);

		if (get()->callWithGraphics(g_, "drawScrollbar", var(obj)))
			return;
	}

	GlobalHiseLookAndFeel::drawScrollbar(g_, scrollbar, x, y, width, height, isScrollbarVertical, thumbStartPosition, thumbSize, isMouseOver, isMouseDown);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive)
{
	if (functionDefined("drawAhdsrPath"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		auto p = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(p);

		p->getPath() = s;

		obj->setProperty("isActive", isActive);
		obj->setProperty("path", keeper);
		obj->setProperty("currentState", graph.getCurrentStateIndex());
		obj->setProperty("area", ApiHelpers::getVarRectangle(s.getBounds().toFloat()));
		obj->setProperty("bgColour", graph.findColour(AhdsrGraph::ColourIds::bgColour).getARGB());
		obj->setProperty("itemColour", graph.findColour(AhdsrGraph::ColourIds::fillColour).getARGB());
		obj->setProperty("itemColour2", graph.findColour(AhdsrGraph::ColourIds::lineColour).getARGB());
		obj->setProperty("itemColour3", graph.findColour(AhdsrGraph::ColourIds::outlineColour).getARGB());

		addParentFloatingTile(graph, obj);

		if (get()->callWithGraphics(g, "drawAhdsrPath", var(obj)))
			return;
	}
	
	AhdsrGraph::LookAndFeelMethods::drawAhdsrPathSection(g, graph, s, isActive);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> pos)
{
	if (functionDefined("drawAhdsrBall"))
	{
		DynamicObject::Ptr obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(graph.getLocalBounds().toFloat()));
		obj->setProperty("position", ApiHelpers::getVarFromPoint(pos));
		obj->setProperty("currentState", graph.getCurrentStateIndex());
		obj->setProperty("bgColour", graph.findColour(AhdsrGraph::ColourIds::bgColour).getARGB());
		obj->setProperty("itemColour", graph.findColour(AhdsrGraph::ColourIds::fillColour).getARGB());
		obj->setProperty("itemColour2", graph.findColour(AhdsrGraph::ColourIds::lineColour).getARGB());
		obj->setProperty("itemColour3", graph.findColour(AhdsrGraph::ColourIds::outlineColour).getARGB());

		addParentFloatingTile(graph, obj);

		if (get()->callWithGraphics(g, "drawAhdsrBall", var(obj)))
			return;
	}

	AhdsrGraph::LookAndFeelMethods::drawAhdsrBallPosition(g, graph, pos);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawMidiDropper(Graphics& g_, Rectangle<float> area, const String& text, MidiFileDragAndDropper& d)
{
	if (functionDefined("drawMidiDropper"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("hover", d.hover);
		obj->setProperty("active", d.isActive());
		obj->setProperty("externalDrag", d.externalDrag);

		obj->setProperty("bgColour", d.findColour(HiseColourScheme::ComponentBackgroundColour).getARGB());
		obj->setProperty("itemColour", d.findColour(HiseColourScheme::ComponentOutlineColourId).getARGB());
		obj->setProperty("textColour", d.findColour(HiseColourScheme::ComponentTextColourId).getARGB());
		obj->setProperty("text", text);

		if (get()->callWithGraphics(g_, "drawMidiDropper", var(obj)))
			return;
	}

	MidiFileDragAndDropper::LookAndFeelMethods::drawMidiDropper(g_, area, text, d);
}

juce::Image ScriptingObjects::ScriptedLookAndFeel::Laf::createIcon(PresetHandler::IconType type)
{
	auto img = MessageWithIcon::LookAndFeelMethods::createIcon(type);

	if (auto l = get())
	{
		DynamicObject::Ptr obj = new DynamicObject();
		
		String s;

		switch (type)
		{
		case PresetHandler::IconType::Error:	s = "Error"; break;
		case PresetHandler::IconType::Info:		s = "Info"; break;
		case PresetHandler::IconType::Question: s = "Question"; break;
		case PresetHandler::IconType::Warning:	s = "Warning"; break;
		default: jassertfalse; break;
		}

		obj->setProperty("type", s);
		obj->setProperty("area", ApiHelpers::getVarRectangle({ 0.0f, 0.0f, (float)img.getWidth(), (float)img.getHeight() }));

		Image img2(Image::ARGB, img.getWidth(), img.getHeight(), true);
		Graphics g(img2);

		if (l->callWithGraphics(g, "drawAlertWindowIcon", var(obj)))
		{
			if ((int)obj->getProperty("type") == -1)
				return {};

			return img2;
		}
			
	}

	return img;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTag(Graphics& g_, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position)
{
	if (functionDefined("drawPresetBrowserTag"))
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(position.toFloat()));
		obj->setProperty("text", name);
		obj->setProperty("blinking", blinking);
		obj->setProperty("value", active);
		obj->setProperty("selected", selected);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserTag", var(obj)))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawTag(g_, blinking, active, selected, name, position);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawModalOverlay(Graphics& g_, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command)
{
	if (auto l = get())
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("labelArea", ApiHelpers::getVarRectangle(labelArea.toFloat()));
		obj->setProperty("title", title);
		obj->setProperty("text", command);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (l->callWithGraphics(g_, "drawPresetBrowserDialog", var(obj)))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawModalOverlay(g_, area, labelArea, title, command);
}



bool ScriptingObjects::ScriptedLookAndFeel::Laf::functionDefined(const String& s)
{
	return get() != nullptr && HiseJavascriptEngine::isJavascriptFunction(get()->functions.getProperty(Identifier(s), {}));
}

#if !HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL
LookAndFeel* HiseColourScheme::createAlertWindowLookAndFeel(void* mainController)
{
	if (auto mc = reinterpret_cast<MainController*>(mainController))
	{
		if (mc->getCurrentScriptLookAndFeel() != nullptr)
			return new ScriptingObjects::ScriptedLookAndFeel::Laf(mc);
	}

	return new hise::AlertWindowLookAndFeel();
}
#endif



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
		"Sampler",
		"Synth",
		"Math",
		"Settings",
		"Server",
		"FileSystem",
		"Message",
		"Buffer"
	};
	
	return ids;
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

ScriptingObjects::ScriptDisplayBufferSource::ScriptDisplayBufferSource(ProcessorWithScriptingContent *p, ExternalDataHolder *h):
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
			return var(new ScriptingObjects::ScriptRingBuffer(getScriptProcessor(), index, source));

		reportScriptError("Can't find buffer at index " + String(index));
	}
	
	RETURN_IF_NO_THROW(var());
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
};

ScriptingObjects::ScriptUnorderedStack::ScriptUnorderedStack(ProcessorWithScriptingContent *p):
	ConstScriptingObject(p, 0)
{
	ADD_API_METHOD_0(isEmpty);
	ADD_API_METHOD_0(size);
	ADD_API_METHOD_1(asBuffer);
	ADD_API_METHOD_1(insert);
	ADD_API_METHOD_1(remove);
	ADD_API_METHOD_1(removeElement);
	ADD_API_METHOD_1(contains);
	ADD_API_METHOD_0(clear);

	elementBuffer = new VariantBuffer(data.begin(), 0);
	wholeBf = new VariantBuffer(data.begin(), 128);
}

struct ScriptingObjects::ScriptUnorderedStack::Display : public Component,
														 public Timer
{
	static constexpr int CellWidth = 70;
	static constexpr int CellHeight = 22;
	static constexpr int NumRows = 16;
	static constexpr int NumColumns = 128 / NumRows;

	void timerCallback() override { repaint(); }

	Display(ScriptUnorderedStack* p):
		parent(p)
	{
		setSize(NumColumns * CellWidth, NumRows * CellHeight);
		setName("Unordered Stack Viewer");
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

		int index = 0;

		for (int y = 0; y < NumRows; y++)
		{
			for (int x = 0; x < NumColumns; x++)
			{
				Rectangle<int> ar(x * CellWidth, y * CellHeight, CellWidth, CellHeight);
				
				if (index < parent->size())
				{
					g.setColour(Colours::white.withAlpha(0.2f));
					g.fillRect(ar.reduced(1));

					float v = *(parent->data.begin() + index);
					g.setColour(Colours::white.withAlpha(0.8f));
					g.setFont(GLOBAL_MONOSPACE_FONT());
					g.drawText(String(v, 1), ar.toFloat(), Justification::centred);
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
	return new Display(this);
#else
	ignoreUnused(e, c);
	return nullptr;
#endif
}




} // namespace hise
