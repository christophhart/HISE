#pragma once

namespace snex
{

/** Maps positions in preprocessed SNEX code back to the original source. */
struct UriLocationBuilder : public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<UriLocationBuilder>;

	UriLocationBuilder(const String& code, const String& preprocessedCode);

	UriLocation createLocation(int preprocessedCharIndex) const;
	UriLocation createLocationFromLineAndCol(int line, int col) const;

	CodeDocument codeDoc;
	CodeDocument preprocessedDoc;
	bool resolvePreprocessedCode = false;
	UriLocation baseLocation;
};

} // namespace snex
