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


#ifndef SCRIPTINGAPIOBJECTS_H_INCLUDED
#define SCRIPTINGAPIOBJECTS_H_INCLUDED

namespace hise { using namespace juce;



class ApiHelpers
{
public:

	class ModuleHandler
	{
	public:

		ModuleHandler(Processor* parent_, JavascriptProcessor* sp);

		~ModuleHandler();

		bool removeModule(Processor* p);

		Processor* addModule(Chain* c, const String& type, const String& id, int index = -1);

		Modulator* addAndConnectToGlobalModulator(Chain* c, Modulator* globalModulator, const String& modName, bool connectAsStaticMod = false);

		JavascriptProcessor* getScriptProcessor() { return scriptProcessor.get(); };

	private:



		WeakReference<Processor> parent;
		WeakReference<JavascriptProcessor> scriptProcessor;

		Component::SafePointer<Component> mainEditor;
	};

	static constexpr int SyncMagicNumber = 911;
	static constexpr int AsyncMagicNumber = 912;

	static bool isSynchronous(var syncValue)
	{
		if ((int)syncValue == SyncMagicNumber)
			return true;

		if ((int)syncValue == AsyncMagicNumber)
			return false;

		return (bool)syncValue;
	}

	static var getVarFromPoint(Point<float> pos);

	static Point<float> getPointFromVar(const var& data, Result* r = nullptr);

	static var getVarRectangle(Rectangle<float> floatRectangle, Result* r = nullptr);

	static Rectangle<float> getRectangleFromVar(const var& data, Result* r = nullptr);

	static Rectangle<int> getIntRectangleFromVar(const var& data, Result* r = nullptr);

	static String getFileNameFromErrorMessage(const String& errorMessage);

	static StringArray getJustificationNames();

	static Justification getJustification(const String& justificationName, Result* r = nullptr);

	static Array<Identifier> getGlobalApiClasses();

	static void loadPathFromData(Path& p, var data);

	static PathStrokeType createPathStrokeType(var strokeType);

#if USE_BACKEND

	static String getValueType(const var& v);

	static ValueTree getApiTree();

#endif
};


class ScriptCreatedComponentWrapper;
class ScriptContentComponent;
class ScriptedControlAudioParameter;
class AudioProcessorWrapper;
class HotswappableProcessor;

/** This class wrapps all available objects that can be created by a script.
*	@ingroup scripting
*/
namespace ScriptingObjects
{
	class ScriptBuffer : public ConstScriptingObject
	{
	public:

		ScriptBuffer(ProcessorWithScriptingContent* p, int size) :
			ConstScriptingObject(p, 0)
		{
			jassertfalse;
		};

		Identifier getObjectName() const override { return "Buffer"; }

		/** Returns the magnitude in the given range. */
		float getMagnitude(int startSample, int numSamples)
		{
			jassertfalse;
            return 1.0f;
		}

		/** Returns the RMS value in the given range. */
		float getRMSLevel(int startSample, int numSamples)
		{
            return 1.0f;
		}

		/** Normalises the buffer to the given decibel value. */
		void normalise(float gainInDecibels)
		{
			jassertfalse;
		};

		/** Detects the pitch of the given buffer. */
		double detectPitch(double sampleRate, int startSample, int numSamples)
		{
			return 0.0;
		}

		/** Converts a buffer with up to 44100 samples to a Base64 string. */
		String toBase64()
		{
            return {};
		}

		/** Loads the content from the Base64 string (and resizes the buffer if necessary). */
		void fromBase64(String b64String)
		{

		}

		/** Returns the sample index with the highest peak. */
		int indexOfPeak(int startSample, int numSamples)
		{
			jassertfalse;
            return -1;
		}

        /** Returns a char from 0 to 255 with the given length and input range. */
        var toCharString(int numChars, var range);
        
		/** Returns an array with the min and max value in the given range. */
		var getPeakRange(int startSample, int numSamples);
        
        /** Trims a buffer at the start and end and returns a copy of it. */
        var trim(int trimFromStart, int trimFromEnd)
        {
            jassertfalse;
            return {};
        }

	};

	class MidiList : public ConstScriptingObject,
					 public AssignableObject
	{
	public:

		// ============================================================================================================

		MidiList(ProcessorWithScriptingContent *p);
		~MidiList() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MidiList"); }

		void assign(const int index, var newValue);
		int getCachedIndex(const var &indexExpression) const override;
		var getAssignedValue(int index) const override;

		int getNumChildElements() const override { return 128; }

		DebugInformationBase* getChildElement(int index) override
		{
			IndexedValue i(this, index);
			return new LambdaValueInformation(i, i.getId(), {}, DebugInformation::Type::Constant, getLocation());
		}
		// ================================================================================================ API METHODS

		/** Fills the MidiList with a number specified with valueToFill. */
		void fill(int valueToFill);;

		/** Clears the MidiList to -1. */
		void clear();

		/** Returns the value at the given number. */
		int getValue(int index) const;

		/** Returns the number of occurences of 'valueToCheck' */
		int getValueAmount(int valueToCheck);;

		/** Returns the first index that contains this value. */
		int getIndex(int value) const;

		/** Checks if the list contains any data. */
		bool isEmpty() const { return numValues == 0; }

		/** Returns the number of values that are not -1. */
		int getNumSetValues() const { return numValues; }

		/** Sets the number to something between -127 and 128. */
		void setValue(int index, int value);;

		/** Sets a range of items to the same value. */
		void setRange(int startIndex, int numToFill, int value);

		/** Encodes all values into a base64 encoded string for storage. */
		String getBase64String() const;

		/** Restore the values from a String that was created with getBase64String(). */
		void restoreFromBase64String(String base64encodedValues);

		// ============================================================================================================

		struct Wrapper;

		const int* getRawDataPointer() const { return data; }

	private:

		int data[128];
		int numValues = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiList);
		JUCE_DECLARE_WEAK_REFERENCEABLE(MidiList);

		// ============================================================================================================
	};

	

	class ScriptFile : public ConstScriptingObject
	{
	public:

		enum Format
		{
			FullPath,
			NoExtension,
			OnlyExtension,
			Filename
		};

		static String getFileNameFromFile(var fileOrString);

		ScriptFile(ProcessorWithScriptingContent* p, const File& f_);

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("File"); }

		String getDebugValue() const override { return f.getFullPathName(); };

		void doubleClickCallback(const MouseEvent &, Component*) override
		{
			f.revealToUser();
		}

		// ================================================= API calls

		/** Returns a child file if this is a directory. */
		var getChildFile(String childFileName);

		/** Returns the parent directory as File. */
		var getParentDirectory();

		/** Returns the new directory created at the file location, if directory doesn't already exist */
		var createDirectory(String directoryName);

		/** Returns the size of the file in bytes. */
		int64 getSize();
		
		/** Returns the number of bytes free on the drive that this file lives on. */
		int64 getBytesFreeOnVolume();

		/** Changes the execute-permissions of a file. */
		bool setExecutePermission(bool shouldBeExecutable);

		/** Returns a sibling file that doesn't exist. */
		var getNonExistentSibling();

		/** Launches the file as a process. */
		bool startAsProcess(String parameters);
		
		/** Reads a file and generates the hash of its contents. */
		String getHash();

		/** true if it's possible to create and write to this file. If the file doesn't already exist, this will check its parent directory to see if writing is allowed. */
		bool hasWriteAccess();

		/** Returns a String representation of that file. */
		String toString(int formatType) const;
		
		/** Returns a reference string with a wildcard. */
		String toReferenceString(String folderType);

		/** If this file is a folder that contains a HISE redirection file (LinkWindows / LinkOSX file), then it will return the redirection target, otherwise it will return itself. */
		var getRedirectedFolder();

		/** Checks if this file exists and is a file. */
		bool isFile() const;

		/** Checks if this file is a child file of the other file. */
		bool isChildOf(var otherFile, bool checkSubdirectories) const;

		/** Checks if the file matches the other file (the object comparison might not work reliably). */
		bool isSameFileAs(var otherFile) const;

		/** Checks if this file exists and is a directory. */
		bool isDirectory() const;

		/** Deletes the file or directory WITHOUT confirmation. */
		bool deleteFileOrDirectory();

		/** Replaces the file content with the JSON data. */
		bool writeObject(var jsonData);

		/** Replaces the XML file with the JSON content (needs to be convertible). */
		bool writeAsXmlFile(var jsonDataToBeXmled, String tagName);

		/** Loads the XML file and tries to parse it as JSON object. */
		var loadFromXmlFile();

		/** Writes the given data (either a Buffer or Array of Buffers) to a audio file. */
		bool writeAudioFile(var audioData, double sampleRate, int bitDepth);

		/** Writes the array of MessageHolders as MIDI file using the metadataObject to determine time signature, tempo, etc. */
		bool writeMidiFile(var eventList, var metadataObject);

		/** Loads the track (zero-based) of the MIDI file. If successful, it returns an object containing the time signature and a list of all events. */
		var loadAsMidiFile(int trackIndex);

		/** Replaces the file content with the given text. */
		bool writeString(String text);

		/** Encrypts an JSON object using the supplied key. */
		bool writeEncryptedObject(var jsonData, String key);

		/** Loads the given file as text. */
		String loadAsString() const;

		/** Loads the given file as object. */
		var loadAsObject() const;

		/** Loads the encrypted object using the supplied RSA key pair. */
		var loadEncryptedObject(String key);

		/** Renames the file. */
		bool rename(String newName);

		/** Moves the file. */
		bool move(var target);

		/** Copies the file. */
		bool copy(var target);

		/** Loads the given file as audio file. */
		var loadAsAudioFile() const;

        /** Tries to parse the metadata from the audio file (channel amount, length, samplerate, etc) and returns a JSON object if sucessful. */
        var loadAudioMetadata() const;
        
		/** Tries to parse the metadata of the MIDI file and returns a JSON object if successful. */
		var loadMidiMetadata() const;

		/** Returns a relative path from the given other file. */
		String getRelativePathFrom(var otherFile);

		/** Opens a Explorer / Finder window that points to the file. */
		void show();

		/** Extracts the ZIP archive if this file is a .zip file. */
		void extractZipFile(var targetDirectory, bool overwriteFiles, var callback);

		/** Returns the number of items in the zip file. */
		int getNumZippedItems();

		/** Changes the read/write permission for the given file. */
		void setReadOnly(bool shouldBeReadOnly, bool applyRecursively);

		// ================================================= End of API calls

		File f;

	private:

		struct Wrapper;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptFile);
	};

	struct ScriptErrorHandler : public ConstScriptingObject,
								public OverlayMessageBroadcaster::Listener
	{
		ScriptErrorHandler(ProcessorWithScriptingContent* p);

		~ScriptErrorHandler();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ErrorHandler"); }

		void overlayMessageSent(int state, const String& message) override;

		// =============================================================== API Methods

		/** Sets a function with two arguments (int state, String message) that will be
            notified at error events. 
		*/
		void setErrorCallback(var errorCallback);

		/** Overrides the default HISE error messages with custom text. */
		void setCustomMessageToShow(int state, String messageToShow);

		/** Clears a state. If there is another error, it will send it again. */
		void clearErrorLevel(int stateToClear);

		/** Clear all states. */
		void clearAllErrors();

		/** Returns the current error message. */
		String getErrorMessage() const;

		/** Returns the number of currently active errors. */
		int getNumActiveErrors() const;

		/** Returns the current error level (and -1 if there is no error). */
		int getCurrentErrorLevel() const;

		/** Causes an error event to be sent through the system (for development purposes only). */
		void simulateErrorEvent(int state);

		// =============================================================== API Methods

	private:

		StringArray customErrorMessages;

		void sendErrorForHighestState();

		BigInteger errorStates;

		

		struct Wrapper;

		WeakCallbackHolder callback;

		var args[2];
	};

	

	struct ScriptBackgroundTask : public ConstScriptingObject,
								  public Thread
	{
		struct Wrapper;
		

		ScriptBackgroundTask(ProcessorWithScriptingContent* p, const String& name);

		~ScriptBackgroundTask()
		{
			stopThread(timeOut);
		}

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("BackgroundTask"); }

		struct TaskViewer : public Component,
							public ComponentForDebugInformation,
							public PooledUIUpdater::SimpleTimer
		{
			TaskViewer(ScriptBackgroundTask* t);;

			void timerCallback() override;

			void paint(Graphics& g) override;

			void resized() override
			{
				auto b = getLocalBounds();
				cancelButton.setBounds(b.removeFromBottom(24));
			}

			BlackTextButtonLookAndFeel laf;
			TextButton cancelButton;
		};

		Component* createPopupComponent(const MouseEvent& e, Component *c) override
		{
			return new TaskViewer(this);
		}

		static void recompiled(ScriptBackgroundTask& task, bool unused);

		// ==================================================================================== Start of API Methods

		/** Signal that this thread should exit. */
		void sendAbortSignal(bool blockUntilStopped);

		/** Checks whether the task should be aborted (either because of recompilation or when you called abort(). */
		bool shouldAbort();

		/** Set a property to a thread safe container. */
		void setProperty(String id, var value);

		/** Retrieve a property through a thread safe container. */
		var getProperty(String id);

		/** Set a function that will be called when the task has started / stopped. */
		void setFinishCallback(var newFinishCallback);

		/** Call a function on the background thread. */
		void callOnBackgroundThread(var backgroundTaskFunction);

		/** Kills all voices and calls the given function on the sample loading thread. */
		bool killVoicesAndCall(var loadingFunction);

		/** Spawns a OS process and executes it with the given command line arguments. */
		void runProcess(var command, var args, var logFunction);

		/** Set a progress for this task. */
		void setProgress(double p);

		/** Get the progress for this task. */
		double getProgress() const { return progress.load(); }

		/** Set timeout. */
		void setTimeOut(int ms)
		{
			timeOut = ms;
		}

		/** Returns the current status message. */
		String getStatusMessage()
		{
			SimpleReadWriteLock::ScopedReadLock sl(lock);
			return message;
		}

		/** Sets a status message. */
		void setStatusMessage(String m);

		/** Forward the state of this thread to the sample loading thread notification system. */
		void setForwardStatusToLoadingThread(bool shouldForward)
		{
			forwardToLoadingThread = shouldForward;
		}

		// ==================================================================================== End of API Methods

		void run() override;

	private:

		bool forwardToLoadingThread = false;

		void callFinishCallback(bool isFinished, bool wasCancelled)
		{
			if (finishCallback)
			{
				var args[2] = { var(isFinished), var(wasCancelled) };
				finishCallback.call(args, 2);
			}
		}

		std::atomic<double> progress = { 0.0 };
		String message;
		int timeOut = 500;
		SimpleReadWriteLock lock;
		NamedValueSet synchronisedData;
		WeakCallbackHolder currentTask;
		WeakCallbackHolder finishCallback;

		struct ChildProcessData
		{
			ChildProcessData(ScriptBackgroundTask& parent_, const String& command_, const var& args_, const var& pf);

			void run();

		private:

			void callLog(var* a);

			ScriptBackgroundTask& parent;
			juce::ChildProcess childProcess;
			WeakCallbackHolder processLogFunction;
			StringArray args;
		};

		ScopedPointer<ChildProcessData> childProcessData;
		



        bool realtimeSafe = true;
        
		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptBackgroundTask);
	};

	class ScriptFFT : public ConstScriptingObject,
					  public Spectrum2D::Holder
	{
	public:

		using WindowType = FFTHelpers::WindowType;
		
		struct Wrapper
		{
			API_VOID_METHOD_WRAPPER_1(ScriptFFT, setWindowType);
			API_VOID_METHOD_WRAPPER_2(ScriptFFT, prepare);
			API_VOID_METHOD_WRAPPER_1(ScriptFFT, setOverlap);
			API_METHOD_WRAPPER_1(ScriptFFT, process);
			API_VOID_METHOD_WRAPPER_2(ScriptFFT, setMagnitudeFunction);
			API_VOID_METHOD_WRAPPER_1(ScriptFFT, setPhaseFunction);
			API_VOID_METHOD_WRAPPER_1(ScriptFFT, setEnableSpectrum2D);
			API_VOID_METHOD_WRAPPER_1(ScriptFFT, setEnableInverseFFT);
		};

		ScriptFFT(ProcessorWithScriptingContent* pwsc);

		~ScriptFFT();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FFT"); }

		struct FFTDebugComponent;

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		Spectrum2D::Parameters::Ptr getParameters() const override { return spectrumParameters; }

		// ======================================================================================================= API Methods

		/** Sets a function that will be executed with the amplitude information of the FFT bins. */
		void setMagnitudeFunction(var newMagnitudeFunction, bool convertToDecibels);

		/** Sets a function that will be executed with the phase information of the FFT bins. */
		void setPhaseFunction(var newPhaseFunction);

		/** Sets a window function that will be applied to the data chunks before processing. */
		void setWindowType(int windowType);

		/** Sets an overlap (from 0...1) for the chunks. */
		void setOverlap(double percentageOfOverlap);

		/** Allocates the buffers required for processing. */
		void prepare(int powerOfTwoSize, int maxNumChannels);

		/** Process the given data (either a buffer or a array of buffers. */
		var process(var dataToProcess);

		/** Enables the creation of a 2D spectrograph image. */
		void setEnableSpectrum2D(bool shouldBeEnabled);

		/** This enables the inverse transform that will reconstruct the signal from the
			processed FFT. */
		void setEnableInverseFFT(bool shouldApplyReverseTransformToInput);

		// ======================================================================================================= End of API Methods

	private:

		AudioSampleBuffer windowBuffer;

		var getBufferArgs(bool useMagnitude, int numToUse);

		void reinitialise()
		{
			if (lastSpecs)
			{
				prepare(lastSpecs.blockSize, lastSpecs.numChannels);
			}
		}

		bool convertMagnitudesToDecibel = false;

		PrepareSpecs lastSpecs;

		bool enableInverse = false;
		bool enableSpectrum = false;

		AudioSampleBuffer fullBuffer;
		Image spectrum;
		Image outputSpectrum;
		Spectrum2D::Parameters::Ptr spectrumParameters;
		SimpleReadWriteLock lock;

		static int getNumToProcess(var inputData);

		void copyToWorkBuffer(var inputData, int offset, int channel);

		void copyFromWorkBuffer(int offset, int channel);

		void applyFFT(int numChannelsThisTime, bool skipFirstWindowHalf);

		void applyInverseFFT(int numChannelsThisTime);

		struct WorkBuffer
		{
			VariantBuffer::Ptr chunkInput;
			VariantBuffer::Ptr chunkOutput;
			VariantBuffer::Ptr magBuffer;
			VariantBuffer::Ptr phaseBuffer;

			
		};
		
		Array<WorkBuffer> scratchBuffers;

		Array<var> thisProcessBuffer;

		Array<var> outputData;

		ScopedPointer<juce::dsp::FFT> fft;
		WeakCallbackHolder magnitudeFunction;
		WeakCallbackHolder phaseFunction;

		WindowType currentWindowType = WindowType::Rectangle;
		double overlap = 0.0;
		int maxNumSamples = 0;
	};

	struct ScriptBuilder : public ConstScriptingObject
	{
		ScriptBuilder(ProcessorWithScriptingContent* p);

		~ScriptBuilder();

		// ============================================================================================= API

		/** Creates a module and returns the build index (0=master container). */
		int create(var type, var id, int rootBuildIndex, int chainIndex);

		/** Connects the script processor to an external script. */
		bool connectToScript(int buildIndex, String relativePath);

        /** Clears all child processors of the chain in the module with the given build index*/
        int clearChildren(int buildIndex, int chainIndex);
        
		/** Returns a typed reference for the module with the given build index. */
		var get(int buildIndex, String interfaceType);

		/** Adds the existing module to the internal list and returns the index for refering to it. */
		int getExisting(String processorId);

		/** Set multiple attributes for the given module at once using a JSON object. */
		void setAttributes(int buildIndex, var attributeValues);

		/** WARNING: Clears all child sound generators, effects and MIDI processor (except for this one obviously). */
		void clear();

		/** Sends a rebuild message. Call this after you've created all the processors to make sure that the patch browser is updated accordingly. */
		void flush();

		// ============================================================================================= API

		Identifier getObjectName() const override { return "Builder"; }

	private:

		bool flushed = true;

		struct Wrapper;

		Array<WeakReference<Processor>> createdModules;

		void createJSONConstants();
	};

	struct ScriptDownloadObject : public ConstScriptingObject,
		public URL::DownloadTask::Listener
	{
		using Ptr = ReferenceCountedObjectPtr<ScriptDownloadObject>;

		ScriptDownloadObject(ProcessorWithScriptingContent* pwsc, const URL& url, const String& extraHeader, const File& targetFile, var callback);;

		~ScriptDownloadObject();

		Identifier getObjectName() const override { return "Download"; }

		// ============================================================================================= API

		/** Stops the download. The target file will not be deleted and you can resume the download later. */
		bool stop();

		/** Resumes the download. */
		bool resume();

		/** Aborts the download and deletes the file that was downloaded. */
		bool abort();

		/** Checks if the download is currently active. */
		bool isRunning();

		/** Returns the progress ratio from 0 to 1. */
		double getProgress() const;

		/** Returns the current download speed in bytes / second. */
		int getDownloadSpeed();

		/** Returns the download size in bytes. */
		double getDownloadSize();

		/** Returns the number of bytes downloaded. */
		double getNumBytesDownloaded();

		/** Returns the full URL of the download. */
		String getFullURL();

		/** Returns the target file if the download has succeeded. */
		var getDownloadedTarget();

		/** Returns a descriptive text of the current download state (eg. "Downloading" or "Paused"). */
		String getStatusText();

		// ============================================================================================= End of API

		void finished(URL::DownloadTask*, bool success) override;

		void flushTemporaryFile();

		void progress(URL::DownloadTask*, int64 bytesDownloaded, int64 totalLength) override;

		void call(bool hiPriority);

		void start();

		bool operator==(const ScriptDownloadObject& other) const
		{
			return downloadURL == other.downloadURL;
		}

		std::atomic<bool> isWaitingForStop = { false };
		std::atomic<bool> isWaitingForStart = { true };
		std::atomic<bool> isRunning_ = { false };
		std::atomic<bool> isFinished = { false };
		std::atomic<bool> shouldAbort = { false };

		struct Wrapper;

		bool stopInternal(bool forceUpdate=false);

		void copyCallBackFrom(ScriptDownloadObject* other)
		{
			callback = std::move(other->callback);
			callback.setThisObject(this);
		}

		URL getURL() const { return downloadURL; }

		File getTargetFile() const { return targetFile; }

	private:

		bool resumeInternal();

		int64 bytesInLastSecond = 0;
		int64 bytesInCurrentSecond = 0;
		int64 lastBytesDownloaded = 0;

		int64 bytesDownloaded_ = 0;
		int64 totalLength_ = 0;

		int64 existingBytesBeforeResuming = 0;
		File resumeFile;

		uint32 lastTimeMs = 0;
		uint32 lastSpeedMeasure = 0;

		DynamicObject::Ptr data;

		URL downloadURL;
		File targetFile;

		WeakCallbackHolder callback;

		String extraHeaders;

		ScopedPointer<URL::DownloadTask> download;

		JavascriptProcessor* jp = nullptr;
	};

	class ScriptComplexDataReferenceBase: public ConstScriptingObject,
										  public ComplexDataUIUpdaterBase::EventListener
	{
	public:

		snex::ExternalData::DataType getDataType() const { return type; }

		String getDebugName() const override { return "Script" + snex::ExternalData::getDataTypeName(getDataType()); };
		String getDebugValue() const override { return getDebugName(); };
		

		bool objectDeleted() const override { return complexObject == nullptr; }
		bool objectExists() const override { return complexObject != nullptr; }

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		ScriptComplexDataReferenceBase(ProcessorWithScriptingContent* c, int dataIndex, snex::ExternalData::DataType type, ExternalDataHolder* otherHolder=nullptr);;

		virtual ~ScriptComplexDataReferenceBase();

		void setPosition(double newPosition);

		float getCurrentDisplayIndexBase() const;

		int getIndex() const { return index; }
		ExternalDataHolder* getHolder() { return holder; }

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data)
		{
			if (t == ComplexDataUIUpdaterBase::EventType::DisplayIndex)
            {
                if(displayCallback)
                    displayCallback.call1(data);
			}
			else
			{
                if(contentCallback)
                    contentCallback.call1(data);
			}
		}

	protected:

		void setCallbackInternal(bool isDisplay, var f);

        void linkToInternal(var o)
        {
            auto other = dynamic_cast<ScriptComplexDataReferenceBase*>(o.getObject());
            
            if(other == nullptr)
            {
                reportScriptError("Not a data object");
                return;
            }
            
            if(other->type != type)
            {
                reportScriptError("Type mismatch");
                return;
            }
            
            using PED = hise::ProcessorWithExternalData;
            
            if(auto pdst = holder.get())
            {
                if(auto psrc = other->holder.get())
                {
                    if(auto ex = psrc->getComplexBaseType(type, other->index))
                    {
                        complexObject->getUpdater().removeEventListener(this);

						pdst->linkTo(type, *psrc, other->index, index);
                        complexObject = holder->getComplexBaseType(type, index);
                        complexObject->getUpdater().addEventListener(this);
                    }
                }
            }
            
            return;
        }
        
		WeakReference<ComplexDataUIBase> complexObject;

		WeakCallbackHolder displayCallback;
		WeakCallbackHolder contentCallback;

	private:


		const snex::ExternalData::DataType type;
		WeakReference<ExternalDataHolder> holder;
		const int index;
	};

	class ScriptAudioFile : public ScriptComplexDataReferenceBase
	{
	public:

		ScriptAudioFile(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* otherHolder = nullptr);

		Identifier getObjectName() const override { return Identifier("AudioFile"); }

		// ============================================================================================================

		void clear();

		/** Sets a new sample range. */
		void setRange(int min, int max);

		/** Loads an audio file from the given reference. */
		void loadFile(const String& filePath);

		/** Returns the current audio data as array of channels. */
		var getContent();

		/** Returns the current sample position (from 0 to numSamples). */
		float getCurrentlyDisplayedIndex() const;

		/** Sends an update message to all registered listeners. */
		void update();

		/** returns the amount of samples. */
		int getNumSamples() const;

		/** Returns the samplerate of the audio file. */
		double getSampleRate() const;

		/** Returns the reference string for the currently loaded file. */
		String getCurrentlyLoadedFile() const;

		/** Sets a callback that is being executed when the playback position changes. */
		void setDisplayCallback(var displayFunction);

		/** Sets a callback that is being executed when a new file is loaded (or the sample range changed). */
		void setContentCallback(var contentFunction);

		// ============================================================================================================

        /** Links this audio file to the other*/
        void linkTo(var other)
        {
            linkToInternal(other);
        }
        
	private:

		MultiChannelAudioBuffer* getBuffer() { return static_cast<MultiChannelAudioBuffer*>(complexObject.get()); }
		const MultiChannelAudioBuffer* getBuffer() const { return static_cast<const MultiChannelAudioBuffer*>(complexObject.get()); }

		struct Wrapper;
	};

	class ScriptRingBuffer : public ScriptComplexDataReferenceBase
	{
	public:

		ScriptRingBuffer(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* other=nullptr);

		Identifier getObjectName() const override { return Identifier("ScriptRingBuffer"); }

		// ============================================================================================================

		/** Returns a reference to the internal read buffer. */
		var getReadBuffer();

		/** Resamples the buffer to a fixed size. */
		var getResizedBuffer(int numDestSamples, int resampleMode);

		/** Creates a path objects scaled to the given bounds and sourceRange */
		var createPath(var dstArea, var sourceRange, var normalisedStartValue);

		/** Sets the ring buffer properties from an object (Use the JSON from the Edit Properties popup). */
		void setRingBufferProperties(var propertyData);

		/** Copies the read buffer into a preallocated target buffer. The target buffer must have the same size. */
		void copyReadBuffer(var targetBuffer);

        /** Enables or disables the ring buffer. */
        void setActive(bool shouldBeActive);
        
		// ============================================================================================================

	private:

		SimpleRingBuffer* getRingBuffer() { return static_cast<SimpleRingBuffer*>(complexObject.get()); }
		const SimpleRingBuffer* getRingBuffer() const { return static_cast<SimpleRingBuffer*>(complexObject.get()); }

		struct Wrapper;
	};

	class ScriptTableData : public ScriptComplexDataReferenceBase
	{
	public:

		ScriptTableData(ProcessorWithScriptingContent* pwsc, int index, ExternalDataHolder* externalHolder=nullptr);

		Identifier getObjectName() const override { return Identifier("Table"); }

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		// ============================================================================================================

		/** Sets the point with the given index to the values. */
		void setTablePoint(int pointIndex, float x, float y, float curve);

		/** Adds a new table point (x and y are normalized coordinates). */
		void addTablePoint(float x, float y);

		/** Resets the table with the given index to a 0..1 line. */
		void reset();

		/** Returns the value of the table at the given input (0.0 ... 1.0). */
		float getTableValueNormalised(double normalisedInput);

		/** Returns the current ruler position (from 0 to 1). */
		float getCurrentlyDisplayedIndex() const;

		/** Sets a callback that is being executed when the ruler position changes. */
		void setDisplayCallback(var displayFunction);

		/** Sets a callback that is being executed when a point is added / removed / changed. */
		void setContentCallback(var contentFunction);

		/** Returns an array containing all table points ([[x0, y0, curve0], ...]). */
		var getTablePointsAsArray();

		/** Sets the table points from a multidimensional array ([x0, y0, curve0], ...]). */
		void setTablePointsFromArray(var pointList);

        /** Makes this table refer to the given table. */
        void linkTo(var otherTable)
        {
            linkToInternal(otherTable);
        }
        
		// ============================================================================================================

        
        
	private:

		Table* getTable() { return static_cast<Table*>(complexObject.get()); }
		const Table* getTable() const { return static_cast<const Table*>(complexObject.get()); }

		struct Wrapper;
	};

	

	class ScriptSliderPackData : public ScriptComplexDataReferenceBase
	{
	public:

		ScriptSliderPackData(ProcessorWithScriptingContent* pwsc, int dataIndex, ExternalDataHolder* otherHolder=nullptr);

		~ScriptSliderPackData() {};

		Identifier getObjectName() const override { return Identifier("SliderPackData"); }

		// ============================================================================================================

		/** Returns the step size. */
		var getStepSize() const;

		/** Sets the amount of sliders. */
		void setNumSliders(var numSliders);

		/** Returns the amount of sliders. */
		int getNumSliders() const;

		/** Sets the value at the given position. */
		void setValue(int sliderIndex, float value);

		/** Returns the value at the given position. */
		float getValue(int index) const;

		/** Sets the range. */
		void setRange(double minValue, double maxValue, double stepSize);

		/** Returns the currently displayed slider index. */
		float getCurrentlyDisplayedIndex() const;

		/** Sets a callback that is being executed when the ruler position changes. */
		void setDisplayCallback(var displayFunction);

		/** Sets a callback that is being executed when a point is added / removed / changed. */
		void setContentCallback(var contentFunction);

        /** Sets a preallocated length that will retain values when the slider pack is resized below that limit. */
        void setUsePreallocatedLength(int length);
        
        /** Links the sliderpack to the other slider pack. */
        void linkTo(var other)
        {
            linkToInternal(other);
        }
        
		// ============================================================================================================

	private:

		SliderPackData* getSliderPackData() { return static_cast<SliderPackData*>(complexObject.get()); }
		const SliderPackData* getSliderPackData() const { return static_cast<const SliderPackData*>(complexObject.get()); }

		struct Wrapper;
	};

	class ScriptingSamplerSound : public ConstScriptingObject,
							      public AssignableObject
	{
	public:

		// ============================================================================================================

		ScriptingSamplerSound(ProcessorWithScriptingContent* p, ModulatorSampler* ownerSampler, ModulatorSamplerSound::Ptr sound_);
		~ScriptingSamplerSound() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Sample"); }

		String getDebugName() const override { return "Sample"; };
		String getDebugValue() const override;

		int getNumChildElements() const override { return (int)sampleIds.size() + (int)customObject.isObject(); }

		DebugInformation* getChildElement(int index) override;

		bool objectDeleted() const override { return sound == nullptr; }
		bool objectExists() const override { return sound != nullptr; }

		void assign(const int index, var newValue) override;

		var getAssignedValue(int index) const override;

		int getCachedIndex(const var &indexExpression) const override;

		ModulatorSamplerSound::Ptr getSoundPtr() { return sound; }

		// ============================================================================================================

		/** Sets the sample property. */
		void set(int propertyIndex, var newValue);

		/** Sets the properties from a JSON object. */
		void setFromJSON(var object);

		/** Returns the sample property. */
		var get(int propertyIndex) const;

		/** Returns the value range that the given property can have (eg. the loop end might not go beyond the sample end. */
		var getRange(int propertyIndex) const;

		/*** Returns the samplerate of the sample (!= the processing samplerate). */
		var getSampleRate();

		/** Loads the sample into a array of buffers for analysis. */
		var loadIntoBufferArray();

		/** Duplicates the sample. */
		ScriptingSamplerSound* duplicateSample();

		/** Deletes the sample from the Sampler (not just this reference!). */
		void deleteSample();

		/** Returns the ID of the property (use this with the setFromJSONMethod). */
		String getId(int id) const;

		/** Writes the content of the audio data (array of buffers) into the audio file. This is undoable!. */
		bool replaceAudioFile(var audioData);

		/** Checks if the otherSample object refers to the same sample as this. */
		bool refersToSameSample(var otherSample);

		/** Returns an object that can hold additional properties. */
		var getCustomProperties();

		// ============================================================================================================

	private:

		var customObject;

		ModulatorSampler* getSampler() const;

		Array<Identifier> sampleIds;

		struct Wrapper;

		WeakReference<Processor> sampler;
		ModulatorSamplerSound::Ptr sound;
	};

	class ScriptingMessageHolder : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		ScriptingMessageHolder(ProcessorWithScriptingContent* content);
		~ScriptingMessageHolder() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MessageHolder"); }

		String getDebugName() const override { return "MessageHolder"; };
		String getDebugValue() const override { return dump(); };
		
		// ============================================================================================================

		/** Return the note number. This can be called only on midi event callbacks. */
		int getNoteNumber() const;

		/** returns the controller number or 'undefined', if the message is neither controller nor pitch wheel nor aftertouch.
		*
		*	You can also check for pitch wheel values and aftertouch messages.
		*	Pitchwheel has number 128, Aftertouch has number 129.
		*/
		var getControllerNumber() const;

		/** Returns the value of the controller. */
		var getControllerValue() const;

		/** Returns the MIDI Channel from 1 to 16. */
		int getChannel() const;

		/** Changes the MIDI channel from 1 to 16. */
		void setChannel(int newChannel);

		/** Changes the note number. */
		void setNoteNumber(int newNoteNumber);

		/** Changes the velocity (range 1 - 127). */
		void setVelocity(int newVelocity);

		/** Changes the ControllerNumber. */
		void setControllerNumber(int newControllerNumber);

		/** Changes the controller value (range 0 - 127). */
		void setControllerValue(int newControllerValue);

		/** Sets the type of the event. */
		void setType(int type);

		/** Returns the Velocity. */
		int getVelocity() const;

		/** Ignores the event. */
		void ignoreEvent(bool shouldBeIgnored = true);;

		/** Returns the event id of the current message. */
		int getEventId() const;

		/** Transposes the note on. */
		void setTransposeAmount(int tranposeValue);

		/** Returns a copy of this message holder object. */
		var clone();

		/** Gets the tranpose value. */
		int getTransposeAmount() const;

		/** Sets the coarse detune amount in semitones. */
		void setCoarseDetune(int semiToneDetune);

		/** Returns the coarse detune amount in semitones. */
		int getCoarseDetune() const;

		/** Sets the fine detune amount in cents. */
		void setFineDetune(int cents);

		/** Returns the fine detune amount int cents. */
		int getFineDetune() const;

		/** Sets the volume of the note (-100 = silence). */
		void setGain(int gainInDecibels);

		/** Returns the volume of the note. */
		int getGain() const;

		/** Returns the current timestamp. */
		int getTimestamp() const;

		/** Sets the timestamp in samples. */
		void setTimestamp(int timestampSamples);

		/** Adds the given sample amount to the current timestamp. */
		void addToTimestamp(int deltaSamples);

		/** Returns true if the event is a note-on event. */
		bool isNoteOn() const;

		/** Returns true if the event is a note-off event. */
		bool isNoteOff() const;

		/** Returns true if the event is a CC controller event. */
		bool isController() const;

		/** Creates a info string for debugging. */
		String dump() const;

		/** Sets the start offset. */
		void setStartOffset(int offset);

		// ============================================================================================================

		void setMessage(const HiseEvent &newEvent) { e = HiseEvent(newEvent); }

		HiseEvent getMessageCopy() const { return e; }

	private:

		struct Wrapper;

		HiseEvent e;
	};

	class ScriptUnorderedStack : public ConstScriptingObject,
		public AssignableObject
	{
	public:

		enum class CompareFunctions
		{
			BitwiseEqual,
			EventId,
			NoteNumberAndVelocity,
			NoteNumberAndChannel,
			EqualData,
			Custom
		};

		ScriptUnorderedStack(ProcessorWithScriptingContent *p);
		~ScriptUnorderedStack() {};

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("UnorderedStack"); }

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		void assign(const int index, var newValue) override { reportScriptError("Can't assign via index"); }
		int getCachedIndex(const var &indexExpression) const override { return (int)indexExpression; }
		var getAssignedValue(int index) const override { return var(data.begin()[jlimit(0, 128, index)]); }

		int getNumChildElements() const override { return data.size(); }

		String getDebugValue() const override { return "Used: " + String(size()); }

		DebugInformationBase* getChildElement(int index) override
		{
			IndexedValue i(this, index);
			return new LambdaValueInformation(i, i.getId(), {}, DebugInformation::Type::Constant, getLocation());
		}

		// ============================================================================================================

		/** Copies the stack into the given container. */
		bool copyTo(var target);

		/** Stores the event into the message holder. */
		bool storeEvent(int index, var holder);

		/** Removes the matching event from the stack and puts it in the holder. */
		bool removeIfEqual(var holder);

		/** Inserts a number at the end of the stack. */
		bool insert(var value);

		/** removes the given number and fills the gap. */
		bool remove(var value);

		/** Removes the element at the given number and fills the gap. */
		bool removeElement(int index);

		/** Clears the stack. */
		bool clear();

		/** Returns the number of values in the stack. */
		int size() const;

		/** Checks if any number is present in the stack. */
		bool isEmpty() const;

		/** checks if the number is in the stack. */
		bool contains(var value) const;

		/** Returns a buffer that refers the data. */
		var asBuffer(bool getAllElements);

		/** Sets this stack to hold HISE events rather than floating point numbers. */
		void setIsEventStack(bool shouldBeEventStack, var eventCompareFunction);

		// ============================================================================================================

	private:

		int getIndexForEvent(var eventHolder) const;

		struct MCF
		{
			template <CompareFunctions CompareType> static bool equals(const HiseEvent& e1, const HiseEvent& e2)
			{
				switch (CompareType)
				{
				case CompareFunctions::BitwiseEqual:		  return e1 == e2;
				case CompareFunctions::EventId:				  return e1.getEventId() == e2.getEventId();
				case CompareFunctions::NoteNumberAndChannel:  return e1.getNoteNumber() && e2.getNoteNumber() &&
																	 e1.getChannel() == e2.getChannel();
				case CompareFunctions::NoteNumberAndVelocity: return e1.isNoteOn() && e2.isNoteOn() &&
															         e1.getNoteNumber() == e2.getNoteNumber() &&
																	 e1.getVelocity() == e2.getVelocity();
				default: jassertfalse;						  return false;
				}
			}
		};

		WeakCallbackHolder compareFunction;
		ReferenceCountedObjectPtr<ScriptingMessageHolder> compareHolder;

		CompareFunctions compareFunctionType;
		std::function<bool(const HiseEvent&, const HiseEvent&)> hcf;

		struct Display;

		struct Wrapper;

		void updateElementBuffer()
		{
			if (isEventStack)
				return;

			elementBuffer->referToData(data.begin(), data.size());
		}

		VariantBuffer::Ptr wholeBf, elementBuffer;
		hise::UnorderedStack<float, 128> data;

		hise::UnorderedStack<HiseEvent, 128> eventData;

		bool isEventStack = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptUnorderedStack);
	};
	
    

	/** A scripting objects that wraps an existing Modulator.
	*/
	class ScriptingModulator : public ConstScriptingObject,
							   public AssignableObject
	{
	public:

		ScriptingModulator(ProcessorWithScriptingContent *p, Modulator *m_);;
		~ScriptingModulator() {};

		// ============================================================================================================

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Modulator"); }

		Identifier getObjectName() const override { return getClassName(); }
		bool objectDeleted() const override { return mod.get() == nullptr; }
		bool objectExists() const override { return mod != nullptr;	}

		String getDebugName() const override;
		String getDebugValue() const override;
		String getDebugDataType() const override { return getObjectName().toString(); }
		void doubleClickCallback(const MouseEvent &e, Component* componentToNotify) override;

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		// ============================================================================================================

		int getCachedIndex(const var &indexExpression) const override;
		void assign(const int index, var newValue) override;
		var getAssignedValue(int /*index*/) const override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the modulator. */
		String getId() const;

		/** Returns the Type of the modulator. */
		String getType() const;
		
		/** Connects a receive modulator to a global modulator. */
		bool connectToGlobalModulator(String globalModulationContainerId, String modulatorId);
		
		/** Returns the id of the global modulation container and global modulator this modulator is connected to */
		String getGlobalModulatorId();
		
		/** Sets the attribute of the Modulator. You can look up the specific parameter indexes in the manual. */
		void setAttribute(int index, float value);

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
        /** Returns the ID of the attribute with the given index. */
        String getAttributeId(int index);
				
				/** Returns the index of the attribute with the given ID. */
				int getAttributeIndex(String id);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the Modulator. */
		void setBypassed(bool shouldBeBypassed);;

		/** Checks if the modulator is bypassed. */
		bool isBypassed() const;

		/** Changes the Intensity of the Modulator. Ranges: Gain Mode 0 ... 1, PitchMode -12 ... 12. */
		void setIntensity(float newIntensity);

		/** Returns the intensity of the Modulator. Ranges: Gain: 0...1, Pitch: -12...12. */
		float getIntensity() const;

		/** Returns the current peak value of the modulator. */
		float getCurrentLevel();

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Export the control values (without the script). */
		String exportScriptControls();

		/** Restores the control values for scripts (without recompiling). */
		void restoreScriptControls(String base64Controls);

		/** Adds a modulator to the given chain and returns a reference. */
		var addModulator(var chainIndex, var typeName, var modName);

		/** Adds a and connects a receiver modulator for the given global modulator. */
		var addGlobalModulator(var chainIndex, var globalMod, String modName);

		/** Adds and connects a receiving static time variant modulator for the given global modulator. */
		var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);

		/** Returns a reference as table processor to modify the table or undefined if no table modulator. */
		var asTableProcessor();

        /** Returns the Modulator chain with the given index. */
        var getModulatorChain(var chainIndex);
        
		

		// ============================================================================================================

		struct Wrapper;
		
		Modulator* getModulator() { return mod.get(); }

	private:

		ApiHelpers::ModuleHandler moduleHandler;

		WeakReference<Modulator> mod;
		Modulation *m;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingModulator);

		// ============================================================================================================
	};



	class ScriptingEffect : public ConstScriptingObject
	{
	public:


		class FilterModeObject : public ConstScriptingObject
		{
		public:

			FilterModeObject(const ProcessorWithScriptingContent* p);
			Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("FilterModes"); }

		private:

		};


		// ============================================================================================================

		ScriptingEffect(ProcessorWithScriptingContent *p, EffectProcessor *fx);
		~ScriptingEffect() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("Effect"); }

		Identifier getObjectName() const override { return getClassName(); }
		bool objectDeleted() const override { return effect.get() == nullptr; }
		bool objectExists() const override { return effect != nullptr; }

		// ============================================================================================================ 

		String getDebugName() const override { return effect.get() != nullptr ? effect->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component* ) override {};

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the effect. */
		String getId() const;

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue);

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
        /** Returns the ID of the attribute with the given index. */
        String getAttributeId(int index);
				
				/** Returns the index of the attribute with the given ID. */
				int getAttributeIndex(String id);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the effect. */
		void setBypassed(bool shouldBeBypassed);

		/** Checks if the effect is bypassed. */
		bool isBypassed() const;

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Export the control values (without the script). */
		String exportScriptControls();

		/** Restores the control values for scripts (without recompiling). */
		void restoreScriptControls(String base64Controls);

		/** Returns the current peak level for the given channel. */
		float getCurrentLevel(bool leftChannel);

		/** Adds a modulator to the given chain and returns a reference. */
		var addModulator(var chainIndex, var typeName, var modName);

		/** Returns the Modulator chain with the given index. */
		var getModulatorChain(var chainIndex);

		/** Adds a and connects a receiver modulator for the given global modulator. */
		var addGlobalModulator(var chainIndex, var globalMod, String modName);

		/** Adds and connects a receiving static time variant modulator for the given global modulator. */
		var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);

		// ============================================================================================================

		struct Wrapper;

		EffectProcessor* getEffect() { return dynamic_cast<EffectProcessor*>(effect.get()); }

	private:

		ApiHelpers::ModuleHandler moduleHandler;

		WeakReference<Processor> effect;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingEffect);

		// ============================================================================================================
	};


	class ScriptingSlotFX : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		ScriptingSlotFX(ProcessorWithScriptingContent *p, EffectProcessor *fx);
		~ScriptingSlotFX() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("SlotFX"); }

		Identifier getObjectName() const override { return getClassName(); }
		bool objectDeleted() const override { return slotFX.get() == nullptr; }
		bool objectExists() const override { return slotFX != nullptr; }

		// ============================================================================================================ 

		String getDebugName() const override { return slotFX.get() != nullptr ? slotFX->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component*) override {};

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		
		/** Bypasses the effect. This uses the soft bypass feature of the SlotFX module. */
		void setBypassed(bool shouldBeBypassed);

		/** Clears the slot (loads a unity gain module). */
		void clear();

		/** Loads the effect with the given name and returns a reference to it. */
		ScriptingEffect* setEffect(String effectName);

		/** Returns a reference to the currently loaded effect. */
		ScriptingEffect* getCurrentEffect();

		/** Swaps the effect with the other slot. */
		bool swap(var otherSlot);

		/** Returns the list of all available modules that you can load into the slot (might be empty if there is no compiled dll present). */
		var getModuleList();

        /** Returns a JSON object containing all parameters with their range properties. */
        var getParameterProperties();
        
        /** Returns the ID of the effect that is currently loaded. */
        String getCurrentEffectId();
        
		// ============================================================================================================

		struct Wrapper;

		HotswappableProcessor* getSlotFX();

	private:

		WeakReference<Processor> slotFX;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingSlotFX);

		// ============================================================================================================
	};


	class ScriptRoutingMatrix : public ConstScriptingObject
	{
	public:

		ScriptRoutingMatrix(ProcessorWithScriptingContent *p, Processor *processor);
		~ScriptRoutingMatrix() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("RoutingMatrix"); }

		Identifier getObjectName() const override { return getClassName(); }
		bool objectDeleted() const override { return rp.get() == nullptr; }
		bool objectExists() const override { return rp != nullptr; }

		// ============================================================================================================ 

		String getDebugName() const override { return rp.get() != nullptr ? rp->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component*) override {};

		// ============================================================================================================ 

		/** Sets the amount of channels (if the matrix is resizeable). */
		void setNumChannels(int numSourceChannels);

		/** adds a connection to the given channels. */
		bool addConnection(int sourceIndex, int destinationIndex);

		/** adds a send connection to the given channels. */
		bool addSendConnection(int sourceIndex, int destinationIndex);

		/** removes the send connection. */
		bool removeSendConnection(int sourceIndex, int destinationIndex);

		/** Removes the connection from the given channels. */
		bool removeConnection(int sourceIndex, int destinationIndex);

		/** Removes all connections. */
		void clear();

		/** Gets the current peak value of the given channelIndex. */
		float getSourceGainValue(int channelIndex);

		/** Returns one or multiple input channels that is mapped to the given output channel (or -1). */
		var getSourceChannelsForDestination(var destinationIndex) const;

		/** Returns the output channel that is mapped to the given input channel (or -1). */
		var getDestinationChannelForSource(var sourceIndex) const;

		// ============================================================================================================ 

		RoutableProcessor* getRoutableProcessor() { return dynamic_cast<RoutableProcessor*>(rp.get()); }

		struct Wrapper;

	private:

		WeakReference<Processor> rp;


	};



	class ScriptingSynth : public ConstScriptingObject
	{
	public:

		// ============================================================================================================ 

		ScriptingSynth(ProcessorWithScriptingContent *p, ModulatorSynth *synth_);
		~ScriptingSynth() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("ChildSynth"); }

		Identifier getObjectName() const override { return getClassName(); };
		bool objectDeleted() const override { return synth.get() == nullptr; };
		bool objectExists() const override { return synth != nullptr; };

		// ============================================================================================================ 

		String getDebugName() const override { return synth.get() != nullptr ? synth->getId() : "Invalid"; };
		String getDebugDataType() const override { return getObjectName().toString(); }
		String getDebugValue() const override { return String(synth.get() != nullptr ? dynamic_cast<ModulatorSynth*>(synth.get())->getNumActiveVoices() : 0) + String(" voices"); }
		void doubleClickCallback(const MouseEvent &, Component* ) override {};

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the synth. */
		String getId() const;

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue);;

        /** Returns the attribute with the given index. */
        float getAttribute(int index);

        /** Returns the attribute with the given index. */
        String getAttributeId(int index);
				
				/** Returns the index of the attribute with the given ID. */
				int getAttributeIndex(String id);

		/** Returns the number of attributes. */
		int getNumAttributes() const;
        
		/** Bypasses the synth. */
		void setBypassed(bool shouldBeBypassed);
		
		/** Checks if the synth is bypassed. */
		bool isBypassed() const;

		/** Returns the child synth with the given index. */
		ScriptingSynth* getChildSynthByIndex(int index);

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Returns the current peak level for the given channel. */
		float getCurrentLevel(bool leftChannel);

		/** Adds a modulator to the given chain and returns a reference. */
		var addModulator(var chainIndex, var typeName, var modName);

		/** Returns the modulator chain with the given index. */
		var getModulatorChain(var chainIndex);

		/** Adds a and connects a receiver modulator for the given global modulator. */
		var addGlobalModulator(var chainIndex, var globalMod, String modName);

		/** Adds and connects a receiving static time variant modulator for the given global modulator. */
		var addStaticGlobalModulator(var chainIndex, var timeVariantMod, String modName);

		/** Returns a reference as Sampler or undefined if no Sampler. */
		var asSampler();

		/** Returns a reference to the routing matrix object of the sound generator. */
		var getRoutingMatrix();

		// ============================================================================================================ 

		struct Wrapper;

	private:

		ApiHelpers::ModuleHandler moduleHandler;

		WeakReference<Processor> synth;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingSynth);

		// ============================================================================================================
	};


	class ScriptingMidiProcessor : public ConstScriptingObject,
								   public AssignableObject
	{
	public:

		// ============================================================================================================ 

		ScriptingMidiProcessor(ProcessorWithScriptingContent *p, MidiProcessor *mp_);;
		~ScriptingMidiProcessor() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("MidiProcessor"); }

		Identifier getObjectName() const override { return getClassName(); }
		bool objectDeleted() const override { return mp.get() == nullptr; }
		bool objectExists() const override { return mp != nullptr; }

		String getDebugName() const override { return mp.get() != nullptr ? mp->getId() : "Invalid"; };
		String getDebugValue() const override { return String(); }
		void doubleClickCallback(const MouseEvent &, Component* ) override {};

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		int getCachedIndex(const var &indexExpression) const override;
		void assign(const int index, var newValue) override;
		var getAssignedValue(int /*index*/) const override;

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Returns the ID of the MIDI Processor. */
		String getId() const;

		/** Sets the attribute of the MidiProcessor. If it is a script, then the index of the component is used. */
		void setAttribute(int index, float value);

        /** Returns the attribute with the given index. */
        float getAttribute(int index);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

        /** Returns the ID of the attribute with the given index. */
		String getAttributeId(int index);
		
		/** Returns the index of the attribute with the given ID. */
		int getAttributeIndex(String id);
		
		/** Bypasses the MidiProcessor. */
		void setBypassed(bool shouldBeBypassed);
		
		/** Checks if the MidiProcessor is bypassed. */
		bool isBypassed() const;

		/** Exports the state as base64 string. */
		String exportState();

		/** Restores the state from a base64 string. */
		void restoreState(String base64State);

		/** Export the control values (without the script). */
		String exportScriptControls();

		/** Restores the control values for scripts (without recompiling). */
		void restoreScriptControls(String base64Controls);

		/** Returns a reference of type ScriptedMidiPlayer that can be used to control the playback. */
		var asMidiPlayer();

		// ============================================================================================================

		struct Wrapper;

	private:

		WeakReference<MidiProcessor> mp;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingMidiProcessor)

		// ============================================================================================================
	};

	class ScriptingAudioSampleProcessor : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		ScriptingAudioSampleProcessor(ProcessorWithScriptingContent *p, Processor *sampleProcessor);
		~ScriptingAudioSampleProcessor() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("AudioSampleProcessor"); }

		Identifier getObjectName() const override { return getClassName(); };
		bool objectDeleted() const override { return audioSampleProcessor.get() == nullptr; }
		bool objectExists() const override { return audioSampleProcessor != nullptr; }

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Changes one of the Parameter. Look in the manual for the index numbers of each effect. */
		void setAttribute(int parameterIndex, float newValue);;

        /** Returns the attribute with the given index. */
        float getAttribute(int index);

        /** Returns the attribute with the given index. */
        String getAttributeId(int index);
				
				/** Returns the index of the attribute with the given ID. */
				int getAttributeIndex(String id);
        
		/** Returns the number of attributes. */
		int getNumAttributes() const;

		/** Bypasses the audio sample player. */
		void setBypassed(bool shouldBeBypassed);

		/** Checks if the audio sample player is bypassed. */
		bool isBypassed() const;

		/** loads the file. You can use the wildcard {PROJECT_FOLDER} to get the audio file folder for the current project. */
		void setFile(String fileName);

		/** Returns the filename (including wildcard) for the currently loaded file. */
		String getFilename(); 

		/** Returns the samplerange in the form [start, end]. */
		var getSampleStart();

		/** Returns the length of the current sample selection in samples. */
		int getSampleLength() const;

		/** Sets the length of the current sample selection in samples. */
		void setSampleRange(int startSample, int endSample);

		/** Creates a ScriptAudioFile reference to the given index. */
		var getAudioFile(int slotIndex);

		// ============================================================================================================

		struct Wrapper; 

	private:

		WeakReference<Processor> audioSampleProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingAudioSampleProcessor);

		// ============================================================================================================
	};

	class ScriptSliderPackProcessor : public ConstScriptingObject
	{
	public:

		ScriptSliderPackProcessor(ProcessorWithScriptingContent* p, ExternalDataHolder* h);

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("SliderPackProcessor"); }

		Identifier getObjectName() const override { return getClassName(); };
		bool objectDeleted() const override { return sp.get() == nullptr; }
		bool objectExists() const override { return sp.get() != nullptr; }

		// ============================================================================================================

		/** Creates a data reference to the given index. */
		var getSliderPack(int sliderPackIndex);

		// ============================================================================================================

	private:

		struct Wrapper;

		WeakReference<Processor> sp;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptSliderPackProcessor);
	};

	class ScriptDisplayBufferSource : public ConstScriptingObject
	{
	public:

		ScriptDisplayBufferSource(ProcessorWithScriptingContent *p, ProcessorWithExternalData *h);
		~ScriptDisplayBufferSource() {};

		// =============================================================================================

		/** Returns a reference to the display buffer at the given index. */
		var getDisplayBuffer(int index);

		// =============================================================================================

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("DisplayBufferSource"); };
		bool objectDeleted() const override { return source.get() == nullptr; }
		bool objectExists() const override { return source != nullptr; }

	private:

		struct Wrapper;
		WeakReference<ExternalDataHolder> source;
	};

	class ScriptingTableProcessor : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		ScriptingTableProcessor(ProcessorWithScriptingContent *p, ExternalDataHolder *tableProcessor);
		~ScriptingTableProcessor() {};

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("TableProcessor"); }

		Identifier getObjectName() const override { return getClassName(); };
		bool objectDeleted() const override { return tableProcessor.get() == nullptr; }
		bool objectExists() const override { return tableProcessor != nullptr; }

		// ============================================================================================================ API Methods

		/** Checks if the Object exists and prints a error message on the console if not. */
		bool exists() { return checkValidObject(); };

		/** Sets the point with the given index to the values. */
		void setTablePoint(int tableIndex, int pointIndex, float x, float y, float curve);

		/** Adds a new table point (x and y are normalized coordinates). */
		void addTablePoint(int tableIndex, float x, float y);

		/** Resets the table with the given index to a 0..1 line. */
		void reset(int tableIndex);

		/** Restores the state from a base64 encoded string. */
		void restoreFromBase64(int tableIndex, const String& state);

		/** Exports the state as base64 encoded string. */
		String exportAsBase64(int tableIndex) const;

		/** Creates a ScriptTableData object for the given table. */
		var getTable(int tableIndex);

		// ============================================================================================================
		
		struct Wrapper;

	private:

		WeakReference<Processor> tableProcessor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingTableProcessor);

		// ============================================================================================================
	};


	struct GlobalRoutingManagerReference : public ConstScriptingObject,
										   public ControlledObject,
										   public WeakErrorHandler,
										   public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
										   
	{
		GlobalRoutingManagerReference(ProcessorWithScriptingContent* sp);;

		~GlobalRoutingManagerReference();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("GlobalRoutingManager"); }

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		void handleErrorMessage(const String& error) override
		{
			if(errorCallback)
				errorCallback.call1(error);
		}

		void oscBundleReceived(const OSCBundle& bundle) override;

		void oscMessageReceived(const OSCMessage& message) override;

		// =============================================================================================

		/** Returns a scripted reference to the global cable (and creates a cable with this ID if it can't be found. */
		var getCable(String cableId);

		/** Allows the global routing manager to send and receive OSC messages through the cables. */
		bool connectToOSC(var connectionData, var errorFunction);

		/** Register a scripting callback to be executed when a OSC message that matches the subAddress is received. */
		void addOSCCallback(String oscSubAddress, var callback);

		/** Send an OSC message to the output port. */
		bool sendOSCMessage(String oscSubAddress, var data);

		// =============================================================================================

	private:

		WeakCallbackHolder errorCallback;

		struct OSCCallback: public ReferenceCountedObject
		{
			using List = ReferenceCountedArray<OSCCallback>;

			OSCCallback(GlobalRoutingManagerReference* parent, String& sd, const var& cb) :
				callback(parent->getScriptProcessor(), parent, cb, 2),
				subDomain(sd),
				fullAddress("/*")
			{
				callback.incRefCount();
				callback.setHighPriority();
			};

			WeakCallbackHolder callback;
			const String subDomain;
			OSCAddressPattern fullAddress;

			void rebuildFullAddress(const String& newRoot)
			{
				try
				{
					fullAddress = OSCAddressPattern(newRoot + subDomain);
				}
				catch (OSCFormatError& e)
				{
					throw e.description;
				}
			}

			void callForMessage(const OSCMessage& c);

			var args[2];

			bool shouldFire(const OSCAddress& oscAddress)
			{
				return callback && fullAddress.matches(oscAddress);
			}

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCCallback);
		};

		OSCCallback::List callbacks;

		struct Wrapper;

		var manager;
	};

	/** A wrapper around a global cable. */
	struct GlobalCableReference : public ConstScriptingObject
	{
		GlobalCableReference(ProcessorWithScriptingContent* ps, var c);

		~GlobalCableReference();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("GlobalCable"); }

		// =============================================================================================

		/** Returns the value (converted to the input range). */
		double getValue() const;

		/** Returns the normalised value between 0...1 */
		double getValueNormalised() const;

		/** Sends the normalised value to all targets. */
		void setValueNormalised(double normalisedInput);

		/** Sends the value to all targets (after converting it from the input range. */
		void setValue(double inputWithinRange);
		
		/** Set the input range using a min and max value (no steps / no skew factor). */
		void setRange(double min, double max);

		/** Set the input range using a min and max value and a mid point for skewing the range. */
		void setRangeWithSkew(double min, double max, double midPoint);

		/** Set the input range using a min and max value as well as a step size. */
		void setRangeWithStep(double min, double max, double stepSize);

		/** Registers a function that will be executed whenever a value is sent through the cable. */
		void registerCallback(var callbackFunction, var synchronous);

		/** Connects the cable to a macro control. */
		void connectToMacroControl(int macroIndex, bool macroIsTarget, bool filterRepetitions);

        /** Connects the cable to a global LFO modulation output as source. */
        void connectToGlobalModulator(const String& lfoId, bool addToMod);
        
        /** Connects the cable to a module parameter using a JSON object for defining the range. */
        void connectToModuleParameter(const String& processorId, var parameterIndexOrId, var targetObject);
        
		// =============================================================================================

	private:

		struct DummyTarget;
		struct Wrapper;
		struct Callback;

		var cable;

		ScopedPointer<DummyTarget> dummyTarget;
		OwnedArray<Callback> callbacks;
		scriptnode::InvertableParameterRange inputRange;
	};

	class TimerObject : public ConstScriptingObject,
					    public ControlledObject
	{
	public:

		// ============================================================================================================

		TimerObject(ProcessorWithScriptingContent *p);
		~TimerObject();

		void mainControllerIsDeleted() { stopTimer(); };

		// ============================================================================================================

		Identifier getObjectName() const override { return "Timer"; }
		bool objectDeleted() const override { return false; }
		bool objectExists() const override { return false; }

		void timerCallback();

		int getNumChildElements() const override { return 2; }

		DebugInformationBase* getChildElement(int index) override;

		// ============================================================================================================
		
		/** Starts the timer. */
		void startTimer(int intervalInMilliSeconds);

		/** Stops the timer. */
		void stopTimer();
		
		/** Sets the function that will be called periodically. */
		void setTimerCallback(var callbackFunction);

		/** Checks if the timer is active. */
		bool isTimerRunning() const;

		/** Returns the duration from the last counter reset. */
		var getMilliSecondsSinceCounterReset();

		/** Resets the internal counter. */
		void resetCounter();

		// ============================================================================================================

	private:

		uint32 milliSecondCounter;

		struct Wrapper;

		struct InternalTimer : public Timer
		{
		public:

			InternalTimer(TimerObject* parent_):
				parent(parent_)
			{
				
			}

			void timerCallback()
			{
				parent->timerCallback();
			}

		private:

			TimerObject* parent;
		};

		InternalTimer it;

		WeakCallbackHolder tc;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimerObject)
        JUCE_DECLARE_WEAK_REFERENCEABLE(TimerObject);
	};

	class ScriptedMidiAutomationHandler : public ConstScriptingObject,
									      public SafeChangeListener
	{
	public:

		struct Wrapper;

		ScriptedMidiAutomationHandler(ProcessorWithScriptingContent* sp);

		~ScriptedMidiAutomationHandler();

		void changeListenerCallback(SafeChangeBroadcaster *b) override;

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("MidiAutomationHandler"); };

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MidiAutomationHandler"); }

		// ============================================================================================================ API Methods

		/** Returns an object that contains the MIDI automation data. */
		var getAutomationDataObject();

		/** Sets the MIDI automation from the automation data object. */
		void setAutomationDataFromObject(var automationData);

		/** Sets the numbers that are displayed in the MIDI automation popup. */
		void setControllerNumbersInPopup(var numberArray);

		/** Replaces the names in the popup. */
		void setControllerNumberNames(var ccName, var nameArray);

		/** Enables the "exclusive" mode for MIDI automation (only one active parameter for each controller). */
		void setExclusiveMode(bool shouldBeExclusive);

		/** Set a function (with one parameter containing the automation data as JSON) that will be executed whenever the MIDI automation changes. */
		void setUpdateCallback(var callback);

		/** Sets whether a automated MIDI CC message should be consumed by the automation handler (default is enabled). */
		void setConsumeAutomatedControllers(bool shouldBeConsumed);

		// ============================================================================================================ End of API Methods

	private:

		MidiControllerAutomationHandler* handler;
		WeakCallbackHolder updateCallback;
	};

	class ScriptedMidiPlayer : public MidiPlayerBaseType,
								public ConstScriptingObject,
								public SuspendableTimer
	{
	public:

		ScriptedMidiPlayer(ProcessorWithScriptingContent* p, MidiPlayer* player_);
		~ScriptedMidiPlayer();

		static Identifier getClassName() { RETURN_STATIC_IDENTIFIER("MidiPlayer"); };
		Identifier getObjectName() const override { return getClassName(); }

		String getDebugValue() const override;

		void sequenceLoaded(HiseMidiSequence::Ptr newSequence) override;
		
		void sequencesCleared() override;

		void timerCallback() override;

		// ============================================================================================================ API Methods

		/** Returns an array containing all notes converted to the space supplied with the target bounds [x, y, w, h]. */
		var getNoteRectangleList(var targetBounds);

		/** Converts a given array of Message holders to a rectangle list. */
		var convertEventListToNoteRectangles(var eventList, var targetBounds);

		/** Sets the playback position in the current loop. Input must be between 0.0 and 1.0. */
		void setPlaybackPosition(var newPosition);

		/** Returns the playback position in the current loop between 0.0 and 1.0. */
		var getPlaybackPosition();

		/** Returns the position of the last played note. */
		var getLastPlayedNotePosition() const;

		/** Syncs the playback of this MIDI player to the master clock (external or internal). */
		void setSyncToMasterClock(bool shouldSyncToMasterClock);

		/** If true, the panel will get a repaint() call whenever the playback position changes. 
		
			Otherwise it will only be updated when the sequence changes. */
		void setRepaintOnPositionChange(var shouldRepaintPanel);

		/** If enabled, it uses the global undo manager for all edits (So you can use Engine.undo()). */
		void setUseGlobalUndoManager(bool shouldUseGlobalUndoManager);

		/** Sets a inline function that will process every note that is about to be recorded. */
		void setRecordEventCallback(var recordEventCallback);

		/** Connect this to the panel and it will be automatically updated when something changes. */
		void connectToPanel(var panel);

		/** Connects this MIDI player to the given metronome. */
		void connectToMetronome(var metronome);

		/** Creates an array containing all MIDI messages wrapped into MessageHolders for processing. */
		var getEventList();

		/** Writes the given array of MessageHolder objects into the current sequence. This is undoable. */
		void flushMessageList(var messageList);

		/** Uses Ticks instead of samples when editing the MIDI data. */
		void setUseTimestampInTicks(bool shouldUseTicksAsTimestamps);

		/** Returns the tick resolution for a quarter note. */
		int getTicksPerQuarter() const;

		/** Creates an empty sequence with the given length. */
		void create(int nominator, int denominator, int barLength);

		/** Checks if the MIDI player contains a sequence to read / write. */
		bool isEmpty() const;

		/** Resets the current sequence to the last loaded file. */
		void reset();

		/** Undo the last edit. */
		void undo();

		/** Redo the last edit. */
		void redo();

		/** Starts playing. Use the timestamp to delay the event or use the currents event timestamp for sample accurate playback. */
		bool play(int timestamp);

		/** Starts playing. Use the timestamp to delay the event or use the currents event timestamp for sample accurate playback. */
		bool stop(int timestamp);

		/** Starts recording (not yet implemented). Use the timestamp to delay the event or use the currents event timestamp for sample accurate playback. */
		bool record(int timestamp);

		/** Loads a MIDI file and switches to this sequence if specified. */
		bool setFile(var fileName, bool clearExistingSequences, bool selectNewSequence);

		/** Saves the current sequence into the given file at the track position. */
		bool saveAsMidiFile(var file, int trackIndex);

		/** Returns a list of all MIDI files that are embedded in the plugin. */
		var getMidiFileList();

		/** Sets the track index (starting with one). */
		void setTrack(int trackIndex);

		/** Enables the (previously loaded) sequence with the given index. */
		void setSequence(int sequenceIndex);

		/** Returns the number of tracks in the current sequence. */
		int getNumTracks();

		/** Returns the number of loaded sequences. */
		int getNumSequences();

		/** Returns an object with properties about the length of the current sequence. */
		var getTimeSignature();

		/** Sets the timing information of the current sequence using the given object. */
		bool setTimeSignature(var timeSignatureObject);

		/** This will send any CC messages from the MIDI file to the global MIDI handler. */
		void setAutomationHandlerConsumesControllerEvents(bool shouldBeEnabled);

		/** Attaches a callback that gets executed whenever the sequence was changed. */
		void setSequenceCallback(var updateFunction);

		/** Attaches a callback with two arguments (timestamp, playState) that gets executed when the play state changes. */
		void setPlaybackCallback(var playbackCallback, var synchronous);

		/** Returns a typed MIDI processor reference (for setting attributes etc). */
		var asMidiProcessor();

		/** Sets a global playback ratio (for all MIDI players). */
		void setGlobalPlaybackRatio(double globalRatio);

		// ============================================================================================================

		struct Wrapper;

	private:

		struct ScriptEventRecordProcessor : public MidiPlayer::EventRecordProcessor
		{
			ScriptEventRecordProcessor(ScriptedMidiPlayer& parent_, const var& function):
				parent(parent_),
				eventCallback(parent.getScriptProcessor(), &parent, function, 1),
				mp(parent.getPlayer())
			{
				eventCallback.incRefCount();
				
				
				mp->addEventRecordProcessor(this);

				holder = new ScriptingMessageHolder(parent.getScriptProcessor());
				args = var(holder);
			}

			~ScriptEventRecordProcessor()
			{
				if (mp != nullptr)
					mp->removeEventRecordProcessor(this);

				holder = nullptr;
				args = var();
			}

			void processRecordedEvent(HiseEvent& e) override
			{
				holder->setMessage(e);
				
				var thisObject(&parent);

				eventCallback.callSync(var::NativeFunctionArgs(thisObject, &args, 1));
				e = holder->getMessageCopy();
			}

			ScriptedMidiPlayer& parent;
			WeakCallbackHolder eventCallback;
			var args;
			ScriptingMessageHolder* holder;
			
			WeakReference<MidiPlayer> mp;
		};

		ScopedPointer<ScriptEventRecordProcessor> recordEventProcessor;

		void callUpdateCallback();

		struct PlaybackUpdater : public PooledUIUpdater::SimpleTimer,
								 public MidiPlayer::PlaybackListener
		{
			PlaybackUpdater(ScriptedMidiPlayer& parent_, var f, bool sync_);

			~PlaybackUpdater();

			void timerCallback() override;

			void playbackChanged(int timestamp, MidiPlayer::PlayState newState) override;

			bool dirty = false;
			const bool sync;
			ScriptedMidiPlayer& parent;
			WeakCallbackHolder playbackCallback;

			var args[2];
		};

		ScopedPointer<PlaybackUpdater> playbackUpdater;

		WeakCallbackHolder updateCallback;

		bool useTicks = false;

		bool repaintOnPlaybackChange = false;

		double lastPlaybackChange = 0.0;

		WeakReference<ConstScriptingObject> connectedPanel;

		bool sequenceValid() const { return getPlayer() != nullptr && getSequence() != nullptr; }
		HiseMidiSequence::Ptr getSequence() const { return getPlayer()->getCurrentSequence(); }
	};

	
};



} // namespace hise
#endif  // SCRIPTINGAPIOBJECTS_H_INCLUDED
