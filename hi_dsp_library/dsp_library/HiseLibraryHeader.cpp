
#if HI_EXPORT_DSP_LIBRARY

#include  "JuceHeader.h"


size_t hise::HelperFunctions::writeString(char* location, const char* content)
{
	strcpy(location, content);

	return strlen(content);
}

juce::String hise::HelperFunctions::createStringFromChar(const char* charFromOtherHeap, size_t length)
{
	std::string s;
	s.reserve(length);

	for (size_t i = 0; i < length; i++)
		s.push_back(*charFromOtherHeap++);


	return juce::String(s);
}



#endif