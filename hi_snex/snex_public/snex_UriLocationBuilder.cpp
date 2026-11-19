namespace snex
{

UriLocationBuilder::UriLocationBuilder(const String& code, const String& preprocessedCode)
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

UriLocation UriLocationBuilder::createLocation(int preprocessedCharIndex) const
{
	UriLocation location;
	location.fillLocation(baseLocation);

	if (preprocessedCharIndex < 0)
		return location;

	const auto& source = resolvePreprocessedCode ? preprocessedDoc : codeDoc;
	CodeDocument::Position position(source, preprocessedCharIndex);
	CodeDocument::Position original(codeDoc, position.getLineNumber(), position.getIndexInLine());
	location.fillLocation(original);
	return location;
}

UriLocation UriLocationBuilder::createLocationFromLineAndCol(int line, int col) const
{
	UriLocation location;
	location.fillLocation(baseLocation);

	if (line < 1 || col < 1)
		return location;

	const auto& source = resolvePreprocessedCode ? preprocessedDoc : codeDoc;
	CodeDocument::Position position(source, line - 1, col - 1);
	CodeDocument::Position original(codeDoc, position.getLineNumber(), position.getIndexInLine());
	location.fillLocation(original);
	return location;
}

} // namespace snex
