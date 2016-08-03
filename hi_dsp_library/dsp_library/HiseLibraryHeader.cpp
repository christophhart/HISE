
#if HI_EXPORT_DSP_LIBRARY

#include "JuceHeader.h"


size_t HelperFunctions::writeString(char* location, const char* content)
{
	strcpy(location, content);

	return strlen(content);
}

String HelperFunctions::createStringFromChar(const char* charFromOtherHeap, size_t length)
{
	std::string s;
	s.reserve(length);

	for (int i = 0; i < length; i++)
		s.push_back(*charFromOtherHeap++);


	return String(s);
}



#endif