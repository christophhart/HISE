namespace hise { using namespace juce;


StringArray MachFiveImporter::getLayerNames(const File &machFiveFile)
{
	ScopedPointer<XmlElement> rootXml = XmlDocument::parse(machFiveFile);

	if (rootXml != nullptr)
	{

		ValueTree root = ValueTree::fromXml(*rootXml);
		ValueTree layers = root.getChildWithName("Program").getChildWithName("Layers");

		StringArray layerNames;

		for (int i = 0; i < layers.getNumChildren(); i++)
		{
			layerNames.add(layers.getChild(i).getProperty("DisplayName"));
		}

		return layerNames;
	}

	return StringArray();
}

ValueTree MachFiveImporter::getSampleMapValueTree(const File &machFiveFile, const String &layerName)
{
	ScopedPointer<XmlElement> rootXml = XmlDocument::parse(machFiveFile);
	
	if (rootXml != nullptr)
	{
		ValueTree root = ValueTree::fromXml(*rootXml);

		ValueTree layers = root.getChildWithName("Program").getChildWithName("Layers");

		ValueTree layer;

		for (int i = 0; i < layers.getNumChildren(); i++)
		{
			if (layers.getChild(i).getProperty("DisplayName") == layerName)
			{
				layer = layers.getChild(i);
				break;
			}
		}

		if (layer.isValid())
		{
			ValueTree sampleMap("samplemap");

			sampleMap.setProperty("ID", layer.getProperty("DisplayName"), nullptr);

			ValueTree keyGroups = layer.getChildWithName("Keygroups");

			for (int i = 0; i < keyGroups.getNumChildren(); i++)
			{
				ValueTree keyGroup = keyGroups.getChild(i);
				ValueTree samplePlayer = keyGroup.getChildWithName("Oscillators").getChildWithName("SamplePlayer");

				ValueTree sample = ValueTree("sample");
				sample.setProperty("ID", i, nullptr);
				sample.setProperty("FileName", samplePlayer.getProperty("SamplePath"), nullptr);
				sample.setProperty(SampleIds::Root,
					samplePlayer.getProperty("BaseNote"), nullptr);

				sample.setProperty(SampleIds::LoKey,
					keyGroup.getProperty("LowKey"), nullptr);

				sample.setProperty(SampleIds::HiKey,
					keyGroup.getProperty("HighKey"), nullptr);

				sample.setProperty(SampleIds::LoVel,
					keyGroup.getProperty("LowVelocity"), nullptr);

				sample.setProperty(SampleIds::HiVel,
					keyGroup.getProperty("HighVelocity"), nullptr);

				sample.setProperty(SampleIds::RRGroup,
					1, nullptr); // TODO

				sample.setProperty(SampleIds::Volume,
					Decibels::gainToDecibels<double>((double)keyGroup.getProperty("Gain")), nullptr);

				sample.setProperty(SampleIds::Pan,
					keyGroup.getProperty("Pan"), nullptr);

				sample.setProperty(SampleIds::Pitch,
					0, nullptr); // TODO

				sample.setProperty(SampleIds::SampleStart,
					samplePlayer.getChildWithName("PlaybackOptions").getProperty("Start"), nullptr);

				sample.setProperty(SampleIds::SampleEnd,
					samplePlayer.getChildWithName("PlaybackOptions").getProperty("Stop"), nullptr);

				sampleMap.addChild(sample, -1, nullptr);
			}

			return sampleMap;
		}		
	}

	return ValueTree();
}

} // namespace hise