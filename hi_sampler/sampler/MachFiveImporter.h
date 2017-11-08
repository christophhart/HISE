/*
  ==============================================================================

    MachFiveImporter.h
    Created: 22 Jun 2016 3:23:56pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef MACHFIVEIMPORTER_H_INCLUDED
#define MACHFIVEIMPORTER_H_INCLUDED
namespace hise { using namespace juce;

class MachFiveImporter
{
public:

	/** Gets a list of all layers (= samplemaps in HISE speak). */
	static StringArray getLayerNames(const File &machFiveFile);

	static ValueTree getSampleMapValueTree(const File &machFiveFile, const String &layerName);

private:


	static XmlElement *getLayer(const File &machFiveFile, const String &layerName);

	static XmlElement *getSampleXmlElementForKeyGroup(const XmlElement &keyGroup);
};


} // namespace hise
#endif  // MACHFIVEIMPORTER_H_INCLUDED
