#pragma once

namespace snex
{

/** Maps positions in preprocessed SNEX code back to the original source. */
struct UriLocationBuilder : public juce::ReferenceCountedObject
{
	using Ptr = juce::ReferenceCountedObjectPtr<UriLocationBuilder>;

	UriLocationBuilder(const juce::String& code, const juce::String& preprocessedCode);

	hise::UriLocation createLocation(int preprocessedCharIndex) const;
	hise::UriLocation createLocationFromLineAndCol(int line, int col) const;

	juce::CodeDocument codeDoc;
	juce::CodeDocument preprocessedDoc;
	bool resolvePreprocessedCode = false;
	hise::UriLocation baseLocation;
};

} // namespace snex
