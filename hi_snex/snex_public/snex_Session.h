#pragma once

#include <memory>
#include <utility>

namespace snex
{

/** A stable source and compilation context. Scheduling and test execution live outside this class. */
class SnexSession : public ReferenceCountedObject,
	public hise::UriErrorLogger,
	public ExternalDataHolder
{
public:
	using Ptr = ReferenceCountedObjectPtr<SnexSession>;

	struct CodeProvider
	{
		virtual ~CodeProvider() = default;
		virtual String getUri() const = 0;
		virtual std::pair<bool, String> getContent(const String& uri) const = 0;
		virtual bool saveCode(const String& code) = 0;
		virtual Identifier getInstanceId() const = 0;
	};

	struct FileCodeProvider : public CodeProvider
	{
		explicit FileCodeProvider(const File& sourceFile) : file(sourceFile) {}

		String getUri() const override { return file.getFullPathName(); }
		std::pair<bool, String> getContent(const String& uri) const override;
		bool saveCode(const String& code) override { return file.replaceWithText(code); }
		Identifier getInstanceId() const override { return file.getFileNameWithoutExtension(); }

		File file;
	};

	struct ValueCodeProvider : public CodeProvider
	{
		ValueCodeProvider(Value& source, const String& uri_, const Identifier& instanceId_);

		String getUri() const override { return uri; }
		std::pair<bool, String> getContent(const String& requestedUri) const override;
		bool saveCode(const String& code) override { value.setValue(code); return true; }
		Identifier getInstanceId() const override { return instanceId; }

		Value value;
		String uri;
		Identifier instanceId;
	};

	SnexSession(const String& processorId_, std::unique_ptr<CodeProvider> source_, int numChannels_ = 2);

	Result compile(const String& sourceSnapshot);
	Result compileFromSource();

	void setUseInterpreter(bool shouldUseInterpreter) { useInterpreter = shouldUseInterpreter; }
	bool isUsingInterpreter() const { return useInterpreter; }

	void handleAsyncUpdate() override {}
	String getUriProcessorId() const override { return processorId; }
	std::pair<bool, String> getUriContent(const String& uri) const override;
	hise::UriLocation getBaseLocation() const;

	const JitObject& getObject() const { return object; }
	jit::ComplexType::Ptr getMainClassType() const { return mainClassType; }
	const Result& getLastResult() const { return lastResult; }
	jit::GlobalScope& getGlobalScope() { return scope; }
	CodeProvider& getCodeProvider() const { return *source; }

	void setExternalDataHolder(ExternalDataHolder* holder) { externalData = holder; }
	int getNumDataObjects(ExternalData::DataType type) const override;
	Table* getTable(int index) override;
	SliderPackData* getSliderPack(int index) override;
	MultiChannelAudioBuffer* getAudioFile(int index) override;
	FilterDataObject* getFilterData(int index) override;
	SimpleRingBuffer* getDisplayBuffer(int index) override;
	bool removeDataObject(ExternalData::DataType type, int index) override;
	void linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex) override;

private:
	void reportCompilationError(const String& message, UriLocationBuilder::Ptr locations = nullptr);

	String processorId;
	std::unique_ptr<CodeProvider> source;
	const int numChannels;
	bool useInterpreter = false;
	jit::GlobalScope scope;
	JitObject object;
	jit::ComplexType::Ptr mainClassType;
	Result lastResult = Result::ok();
	WeakReference<ExternalDataHolder> externalData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SnexSession);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnexSession);
};

} // namespace snex
