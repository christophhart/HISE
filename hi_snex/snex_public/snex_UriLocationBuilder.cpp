namespace snex
{

UriLocationBuilder::UriLocationBuilder(const juce::String& code, const juce::String& preprocessedCode)
{
	codeDoc.setDisableUndo(true);
	codeDoc.replaceAllContent(code);

	if (code != preprocessedCode)
	{
		preprocessedDoc.setDisableUndo(true);
		preprocessedDoc.replaceAllContent(preprocessedCode);
		resolvePreprocessedCode = true;
	}
}

hise::UriLocation UriLocationBuilder::createLocation(int preprocessedCharIndex) const
{
	hise::UriLocation location;
	location.fillLocation(baseLocation);

	if (preprocessedCharIndex < 0)
		return location;

	const auto& source = resolvePreprocessedCode ? preprocessedDoc : codeDoc;
	juce::CodeDocument::Position position(source, preprocessedCharIndex);
	juce::CodeDocument::Position original(codeDoc, position.getLineNumber(), position.getIndexInLine());
	location.fillLocation(original);
	return location;
}

hise::UriLocation UriLocationBuilder::createLocationFromLineAndCol(int line, int col) const
{
	hise::UriLocation location;
	location.fillLocation(baseLocation);

	if (line < 1 || col < 1)
		return location;

	const auto& source = resolvePreprocessedCode ? preprocessedDoc : codeDoc;
	juce::CodeDocument::Position position(source, line - 1, col - 1);
	juce::CodeDocument::Position original(codeDoc, position.getLineNumber(), position.getIndexInLine());
	location.fillLocation(original);
	return location;
}

} // namespace snex
