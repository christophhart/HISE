


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
				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::RootNote),
					samplePlayer.getProperty("BaseNote"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::KeyLow),
					keyGroup.getProperty("LowKey"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::KeyHigh),
					keyGroup.getProperty("HighKey"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::VeloLow),
					keyGroup.getProperty("LowVelocity"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::VeloHigh),
					keyGroup.getProperty("HighVelocity"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::RRGroup),
					1, nullptr); // TODO

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::Volume),
					Decibels::gainToDecibels<double>((double)keyGroup.getProperty("Gain")), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::Pan),
					keyGroup.getProperty("Pan"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::Pitch),
					0, nullptr); // TODO

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::SampleStart),
					samplePlayer.getChildWithName("PlaybackOptions").getProperty("Start"), nullptr);

				sample.setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::SampleEnd),
					samplePlayer.getChildWithName("PlaybackOptions").getProperty("Stop"), nullptr);

				sampleMap.addChild(sample, -1, nullptr);
			}

			return sampleMap;
		}		
	}

	return ValueTree();
}
