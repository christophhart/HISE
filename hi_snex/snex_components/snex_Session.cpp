namespace snex
{
namespace ui
{

std::pair<bool, String> SnexSession::FileCodeProvider::getContent(const String& uri) const
{
	if (uri == getUri() && file.existsAsFile())
		return { true, file.loadFileAsString() };

	return { false, "Unknown SNEX source: " + uri };
}

std::pair<bool, String> SnexSession::DocumentCodeProvider::getContent(const String& requestedUri) const
{
	if (requestedUri == uri)
		return { true, document.getAllContent() };

	return { false, "Unknown SNEX source: " + requestedUri };
}

SnexSession::ValueCodeProvider::ValueCodeProvider(Value& source, const String& uri_, const Identifier& instanceId_) :
	uri(uri_),
	instanceId(instanceId_)
{
	value.referTo(source);
}

std::pair<bool, String> SnexSession::ValueCodeProvider::getContent(const String& requestedUri) const
{
	if (requestedUri == uri)
		return { true, value.toString() };

	return { false, "Unknown SNEX source: " + requestedUri };
}

SnexSession::SnexSession(const String& processorId_, std::unique_ptr<CodeProvider> source_, int numChannels_) :
	processorId(processorId_),
	source(std::move(source_)),
	numChannels(numChannels_)
{
	jassert(source != nullptr);
	jassert(processorId.isNotEmpty());
	jassert(numChannels > 0);
}

std::pair<bool, String> SnexSession::getUriContent(const String& uri) const
{
	return source->getContent(uri);
}

hise::UriLocation SnexSession::getBaseLocation() const
{
	hise::UriLocation location;
	location.processorId = processorId;
	location.uri = source->getUri();
	return location;
}

Result SnexSession::compileFromSource()
{
	auto content = getUriContent(source->getUri());

	if (content.first)
		return compile(content.second);

	lastResult = Result::fail(content.second);
	clearUriErrors(hise::UriError::SourceType::Compilation);
	reportCompilationError(lastResult.getErrorMessage());
	return lastResult;
}

Result SnexSession::compile(const String& sourceSnapshot)
{
	clearUriErrors(hise::UriError::SourceType::Compilation);

	jit::Compiler compiler(scope);
	Types::SnexObjectDatabase::registerObjects(compiler, numChannels);

	auto compiledObject = compiler.compileJitObject(sourceSnapshot);
	lastResult = compiler.getCompileResult();

	if (lastResult.wasOk())
	{
		jit::ComplexType::Ptr compiledClass;
		auto instanceId = source->getInstanceId();

		if (instanceId.isValid())
		{
			auto classId = NamespacedIdentifier(instanceId);
			compiledClass = compiler.getComplexType(classId);

			for (const auto& templateType : compiler.getNamespaceHandler().getTemplateClassTypes())
			{
				if (templateType.id.id != classId)
					continue;

				int voiceAmount = 1;

				if (auto ph = scope.getPolyHandler())
					if (ph->isEnabled())
						voiceAmount = NUM_POLYPHONIC_VOICES;

				compiledClass = compiler.getComplexType(classId, { jit::TemplateParameter(voiceAmount) }, true);
				break;
			}
		}

		object = compiledObject;
		mainClassType = compiledClass;
	}
	else
	{
		reportCompilationError(lastResult.getErrorMessage());
	}

	return lastResult;
}

void SnexSession::reportCompilationError(const String& message)
{
	hise::UriError error;
	error.errorType = hise::UriError::ErrorType::Error;
	error.errorMessage = message;
	error.fillLocation(getBaseLocation());

	if (message.startsWith("Line"))
	{
		error.line = message.fromFirstOccurrenceOf("Line", false, false).getIntValue() - 1;
		error.col = message.fromFirstOccurrenceOf("(", false, false).getIntValue() - 1;

		if (message.contains(":"))
			error.errorMessage = message.fromFirstOccurrenceOf(":", false, false).trim();
	}

	logUriError(hise::UriError::SourceType::Compilation, error);
}

int SnexSession::getNumDataObjects(ExternalData::DataType type) const
{
	if (auto holder = externalData.get())
		return holder->getNumDataObjects(type);

	return 0;
}

Table* SnexSession::getTable(int index)
{
	if (auto holder = externalData.get())
		return holder->getTable(index);

	return nullptr;
}

SliderPackData* SnexSession::getSliderPack(int index)
{
	if (auto holder = externalData.get())
		return holder->getSliderPack(index);

	return nullptr;
}

MultiChannelAudioBuffer* SnexSession::getAudioFile(int index)
{
	if (auto holder = externalData.get())
		return holder->getAudioFile(index);

	return nullptr;
}

FilterDataObject* SnexSession::getFilterData(int index)
{
	if (auto holder = externalData.get())
		return holder->getFilterData(index);

	return nullptr;
}

SimpleRingBuffer* SnexSession::getDisplayBuffer(int index)
{
	if (auto holder = externalData.get())
		return holder->getDisplayBuffer(index);

	return nullptr;
}

bool SnexSession::removeDataObject(ExternalData::DataType type, int index)
{
	if (auto holder = externalData.get())
		return holder->removeDataObject(type, index);

	return false;
}

void SnexSession::linkTo(ExternalData::DataType type, ExternalDataHolder& src, int srcIndex, int dstIndex)
{
	if (auto holder = externalData.get())
		holder->linkTo(type, src, srcIndex, dstIndex);
}

} // namespace ui
} // namespace snex
