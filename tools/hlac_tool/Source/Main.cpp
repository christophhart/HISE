/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

using namespace hlac;

class StdLogger : public Logger
{
public:

	void logMessage(const String &message) override
	{
#if JUCE_DEBUG
		DBG(message);
#else
		NewLine nl;
		std::cout << message << nl;
#endif
	}
};


#define ABORT_WITH_MESSAGE(x) Logger::writeToLog(x); Logger::setCurrentLogger(nullptr); return 1;

void printHelp()
{
	Logger::writeToLog("HISE Lossless Audio Codec Test tool");
	Logger::writeToLog("-----------------------------------");
	Logger::writeToLog("Usage: hlac_tool [MODE] [INPUT] [OUTPUT]");
	Logger::writeToLog("");
	Logger::writeToLog("modes: 'encode' / 'decode'");
	Logger::writeToLog("test-modes: 'unit_test' / 'test_directory', 'memory_map_directory'");
	Logger::writeToLog("(put '_' before filename to skip samples)");
	Logger::setCurrentLogger(nullptr);
}

int encode(File input, File output, HlacEncoder::CompressorOptions option)
{
    AudioFormatManager afm;

	afm.registerBasicFormats();

	ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(input);

	if (reader == nullptr)
	{
		ABORT_WITH_MESSAGE(input.getFileName() + " is not a valid audio file");
	}

	HiseLosslessAudioFormat hlac;

	FileOutputStream* fos = new FileOutputStream(output);

	StringPairArray empty;

	ScopedPointer<HiseLosslessAudioFormatWriter> writer = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(fos, reader->sampleRate, reader->numChannels, 16, empty, 5));

	writer->setOptions(option);

	bool ok = writer->writeFromAudioReader(*reader, 0, reader->lengthInSamples);

	if (ok)
	{
		Logger::writeToLog(output.getFileName() + "successfully encoded");
		Logger::setCurrentLogger(nullptr);
		return 0;
	}
	else
	{
		ABORT_WITH_MESSAGE("Error at encoding " + output.getFileName());
	}
}

void testMemoryMapPerformance(Array<File>& files)
{
	HiseLosslessAudioFormat hlac;

	AudioSampleBuffer testBuffer;

	for (int i = 0; i < files.size(); i++)
	{
		FileInputStream* fis = new FileInputStream(files[i]);

		ScopedPointer<MemoryMappedAudioFormatReader> reader = hlac.createMemoryMappedReader(fis);

		testBuffer.setSize(reader->numChannels, reader->lengthInSamples, true, false, true);

		reader->read(&testBuffer, 0, reader->lengthInSamples, 0, true, true);
	}
}

int decode(File input, File output)
{

	HiseLosslessAudioFormat hlac;

	ScopedPointer<FileInputStream> fis = new FileInputStream(input);

	

	MemoryOutputStream mos;

	mos.writeFromInputStream(*fis, fis->getTotalLength());

	fis = nullptr;

	AudioFormatManager afm;

	afm.registerBasicFormats();

	AudioFormat* formatToUse = afm.findFormatForFileExtension(output.getFileExtension());

	if (formatToUse == nullptr)
	{
		ABORT_WITH_MESSAGE("Can't find a suitable format for " + output.getFileName());
	}

	int x = 0;

	AudioSampleBuffer b;

	MemoryBlock mb;

	mb.setSize(mos.getDataSize());

	mb.copyFrom(mos.getData(), 0, mos.getDataSize());

	while (x < 1)
	{
		if(x % 100 == 0)
		{
			Logger::writeToLog("Profiling: " + String(x/100) + "% complete");
		}

		MemoryInputStream* mis = new MemoryInputStream(mb, false);
		ScopedPointer<HiseLosslessAudioFormatReader> reader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(mis, true));

		if (reader == nullptr)
		{
			ABORT_WITH_MESSAGE("Can't read " + input.getFileName());
		}
		
		b.setSize(reader->numChannels, (int)reader->lengthInSamples, true, true, true);
		
		reader->read(&b, 0, (int)reader->lengthInSamples, 0, true, true);

		x++;
	}

	Logger::setCurrentLogger(nullptr);
	return 0;
}

int main(int argc, char **argv)
{
	ScopedPointer<Logger> l = new StdLogger();
	Logger::setCurrentLogger(l);

	if (argc < 2)
	{
		printHelp();
		return 1;
	}

	String mode(argv[1]);

	if (mode.startsWith("encode") || mode == "decode")
	{
		bool shouldEncode = mode != "decode";

		if (argc < 3)
		{
			printHelp();
			return 1;
		}

		File input = File(argv[2]);

		if (!input.existsAsFile())
		{
			ABORT_WITH_MESSAGE("File " + String(argv[2]) + " does not exist");
		}

		File output;
		
		if (argc == 4)
			output = File(argv[3]);
		else
		{
			if(shouldEncode)
				output = input.getSiblingFile(input.getFileNameWithoutExtension() + ".hlac");
			else
			{
				ABORT_WITH_MESSAGE("You have to specify a output file name");
			}
		}
		
		HlacEncoder::CompressorOptions option = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::Diff);

		if (mode.contains("Block")) option = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::WholeBlock);

		if (output.existsAsFile())
			output.deleteFile();

		return shouldEncode ? encode(input, output, option) : decode(input, output);

	}

	if (mode == "unit_test")
	{
		UnitTestRunner runner;
		runner.setAssertOnFailure(false);
		runner.runAllTests();

		int numTests = runner.getNumResults();
		int numFails = 0;

		for (int i = 0; i < numTests; i++)
		{
			auto result = runner.getResult(i);
			
			numFails += result->failures;
		}

		if (numFails > 0)
		{
			Logger::writeToLog("Unit tests failed");
			Logger::setCurrentLogger(nullptr);
			return 1;
		}
		else
		{
			Logger::writeToLog("All unit tests passed");
			Logger::setCurrentLogger(nullptr);
			return 0;
		}
	}


	if (mode == "memory_map_directory")
	{
		File root(argv[2]);

		Array<File> testSamples;

		root.findChildFiles(testSamples, File::findFiles, true);

		for (int i = 0; i < 100; i++)
		{
			
			Logger::writeToLog(String(i) + "% complete");

			testMemoryMapPerformance(testSamples);
		}

		Logger::setCurrentLogger(nullptr);
		return 0;
	}

	if (mode != "test_directory")
	{
		Logger::writeToLog("Invalid mode");
		Logger::setCurrentLogger(nullptr);
		return 1;
	}

	File root(argv[2]);

	Array<File> testSamples;

	root.findChildFiles(testSamples, File::findFiles, true);

    bool useBlock = true;
	bool useDelta = true;
	bool useDiff = true;
	bool checkWithFlac = true;

	double blockRatio = 0.0f;
	double deltaRatio = 0.0f;
	double diffRatio = 0.0f;
	double flacRatio = 0.0f;

	double blockSpeed = 0.0;
	double flacSpeed = 0.0;
	double pcmSpeed = 0.0;
	double deltaSpeed = 0.0;
	double diffSpeed = 0.0;

	double r;
	double s;

	int numFilesChecked = 0;


	HiseLosslessAudioFormat hlac;


	for (auto f : testSamples)
	{
        if (f.getFileName().startsWith("_") || f.isHidden())
			continue;

		++numFilesChecked;

		Logger::writeToLog("");
		Logger::writeToLog("Compressing file " + f.getFileName());
		Logger::writeToLog("--------------------------------------------------------------------");

		AudioSampleBuffer b;

		try
		{
			b = CompressionHelpers::loadFile(f, s);
		}
		catch (String error)
		{
			Logger::writeToLog(error);
			Logger::setCurrentLogger(nullptr);
			return 1;
		}
		
		pcmSpeed += s;

		if (checkWithFlac)
		{
			auto fr = CompressionHelpers::getFLACRatio(f, s);

			flacSpeed += s;
			flacRatio += fr;

			Logger::writeToLog("Compressing with FLAC:  " + String(fr, 3));

		}

		StringPairArray emptyMetadata;

		if (useBlock)
		{
			MemoryOutputStream* blockMos = new MemoryOutputStream();

			ScopedPointer<HiseLosslessAudioFormatWriter> blockWriter = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(blockMos, 44100, b.getNumChannels(), 16, emptyMetadata, 5));
			
			if (blockWriter == nullptr)
				return 1;

			HlacEncoder::CompressorOptions wholeBlock = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::WholeBlock);

			blockWriter->setOptions(wholeBlock);
			blockWriter->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
			
			r = blockWriter->getCompressionRatioForLastFile();

			blockWriter->flush();

			AudioSampleBuffer b2(b.getNumChannels(), CompressionHelpers::getPaddedSampleSize(b.getNumSamples()));

			MemoryInputStream* blockMis = new MemoryInputStream(blockMos->getMemoryBlock(), true);


			ScopedPointer<HiseLosslessAudioFormatReader> blockReader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(blockMis, false));

			blockReader->read(&b2, 0, b2.getNumSamples(), 0, true, true);

			blockSpeed += blockReader->getDecompressionPerformanceForLastFile();
			CompressionHelpers::checkBuffersEqual(b2, b);

			Logger::writeToLog("Compressing with blocks: " + String(r, 3));

			blockRatio += r;

			blockWriter = nullptr;
			blockReader = nullptr;
		}

		if (useDiff)
		{
			MemoryOutputStream* diffMos = new MemoryOutputStream();

			ScopedPointer<HiseLosslessAudioFormatWriter> diffWriter = dynamic_cast<HiseLosslessAudioFormatWriter*>(hlac.createWriterFor(diffMos, 44100, b.getNumChannels(), 16, emptyMetadata, 5));

			if (diffWriter == nullptr)
				return 1;

			HlacEncoder::CompressorOptions diff = HlacEncoder::CompressorOptions::getPreset(HlacEncoder::CompressorOptions::Presets::Diff);

			diffWriter->setOptions(diff);
			diffWriter->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());

			r = diffWriter->getCompressionRatioForLastFile();

			diffRatio += r;

			diffWriter->flush();

			AudioSampleBuffer b2(b.getNumChannels(), CompressionHelpers::getPaddedSampleSize(b.getNumSamples()));
			
			FloatVectorOperations::fill(b2.getWritePointer(0), 1.0f, b2.getNumSamples());

			MemoryInputStream* diffMis = new MemoryInputStream(diffMos->getMemoryBlock(), true);
			ScopedPointer<HiseLosslessAudioFormatReader> diffReader = dynamic_cast<HiseLosslessAudioFormatReader*>(hlac.createReaderFor(diffMis, false));

			diffReader->read(&b2, 0, b2.getNumSamples(), 0, true, true);

			diffSpeed += diffReader->getDecompressionPerformanceForLastFile();

			CompressionHelpers::checkBuffersEqual(b2, b);

			Logger::writeToLog("Compressing with diff: " + String(r, 3));
		}
	}

	flacRatio /= (float)numFilesChecked;
	deltaRatio /= (float)numFilesChecked;
	diffRatio /= (float)numFilesChecked;
	blockRatio /= (float)numFilesChecked;

	blockSpeed /= (double)numFilesChecked;
	pcmSpeed /= (double)numFilesChecked;
	flacSpeed /= (double)numFilesChecked;
	deltaSpeed /= (double)numFilesChecked;
	diffSpeed /= (double)numFilesChecked;

	Logger::writeToLog("=====================================================");
	if (checkWithFlac) Logger::writeToLog("FLAC ratio:\t" + String(flacRatio, 3));
	if (useBlock) Logger::writeToLog("Block ratio:\t" + String(blockRatio, 3));
	if (useDelta) Logger::writeToLog("Delta ratio:\t" + String(deltaRatio, 3));
	if (useDiff) Logger::writeToLog("Diff ratio:\t" + String(diffRatio, 3));
	Logger::writeToLog("=====================================================");
	Logger::writeToLog("PCM speed:\t" + String(pcmSpeed, 1));
	if (checkWithFlac) Logger::writeToLog("FLAC speed:\t" + String(flacSpeed, 1));
	if (useBlock) Logger::writeToLog("Block speed:\t" + String(blockSpeed, 1));
	if (useDelta) Logger::writeToLog("Delta speed:\t" + String(deltaSpeed, 1));
	if (useDiff) Logger::writeToLog("Diff speed:\t" + String(diffSpeed, 1));

	Logger::setCurrentLogger(nullptr);

	return 0;
}