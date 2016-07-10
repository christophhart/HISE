

void MachFiveImporter::loadIntoSampleMap(SampleMap &map, const File &/*machFiveFile*/, const String &/*layerName*/)
{
	map.clear();

	ScopedPointer<XmlElement> xml = map.exportAsValueTree().createXml();

	DBG(xml->createDocument(""));
}
