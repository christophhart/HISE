/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#define COMPRESSION_BLOCK_SIZE 4096

#include "HiseLosslessAudioFormat.h"

class StdLogger : public Logger
{
public:

	void logMessage(const String &message) override
	{
		NewLine nl;
		std::cout << message << nl;
	}
};

int main(int argc, char **argv)
{
	

	ScopedPointer<Logger> l = new StdLogger();

	Logger::setCurrentLogger(l);

	if (argc != 2)
	{
		Logger::writeToLog("HISE Lossless Audio Codec Test tool");
		Logger::writeToLog("-----------------------------------");
		Logger::writeToLog("Usage: hlac_tool [FOLDER_WITH_TEST_FILES]");
		Logger::writeToLog("(put '_' before filename to skip samples)");
		Logger::setCurrentLogger(nullptr);

		return 1;
	}

	UnitTestRunner runner;
	runner.setAssertOnFailure(false);

	//runner.runAllTests();

	File root(argv[1]);

	Array<File> testSamples;

	root.findChildFiles(testSamples, File::findFiles, true);

	StringArray files;

	files.add("akk1.wav");
	files.add("akk2.wav");
	files.add("piano.wav");
	files.add("snare1.aif");
	files.add("snare2.aif");
	files.add("psal1.wav");
	files.add("psal2.wav");
	files.add("harp1.wav");
	files.add("harp2.wav");
	files.add("harp3.wav");
	files.add("epiano1.aif");
	//files.add("epiano2.aif");
	files.add("epiano3.aif");
	files.add("flute1.wav");
	files.add("organ.wav");
	files.add("wurlie2.aif");

	files.add("tabla.wav");
	//files.add("doubleBass.wav");
	files.add("cello_rel.wav");
	files.add("frendo1.wav");
	files.add("frendo2.wav");
	//files.add("frendo3.wav");
	files.add("frendo4.wav");
	files.add("stage1.aif");
	files.add("stage2.aif");
	files.add("stage3.aif");
	//files.add("suit.aif");
	//files.add("suit2.aif");
	files.add("ep1.aif");
	files.add("ep2.aif");
	files.add("ep3.aif");
	files.add("ep4.aif");

	float blockRatio = 0.0f;
	float deltaRatio = 0.0f;
	float diffRatio = 0.0f;
	float flacRatio = 0.0f;

	double blockSpeed = 0.0;
	double flacSpeed = 0.0;
	double pcmSpeed = 0.0;
	double deltaSpeed = 0.0;

	float r;
	double s;

	for (auto f : testSamples)
	{
		if (f.getFileName().startsWith("_"))
			continue;

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

		auto fr = CompressionHelpers::getFLACRatio(f, s);

		flacSpeed += s;
		flacRatio += fr;

		Logger::writeToLog("Compressing with FLAC:  " + String(fr, 3));

		MemoryOutputStream mos;
		HiseLosslessAudioFormat hlaf;
		HiseLosslessAudioFormat::CompressorOptions wholeBlock;

		wholeBlock.fixedBlockWidth = 512;
		wholeBlock.removeDcOffset = false;
		wholeBlock.useDeltaEncoding = false;
		wholeBlock.useDiffEncodingWithFixedBlocks = false;

		hlaf.setOptions(wholeBlock);
		hlaf.compress(b, mos);
		mos.flush();
		r = hlaf.getCompressionRatio();

		AudioSampleBuffer b2(1, b.getNumSamples());
		MemoryInputStream mis(mos.getMemoryBlock(), true);

		hlaf.setupForDecompression();
		hlaf.decompress(b2, mis);
		blockSpeed += hlaf.getDecompressionPerformance();
		auto db = CompressionHelpers::getDifference(b2, b);

		Logger::writeToLog("Compressing with blocks: " + String(r, 3) + " Error: " + String(db, 2) + "dB");

		blockRatio += r;

		mos.reset();

		HiseLosslessAudioFormat::CompressorOptions delta;

		delta.fixedBlockWidth = -1;
		delta.removeDcOffset = false;
		delta.useDeltaEncoding = true;
		delta.useDiffEncodingWithFixedBlocks = false;
		delta.reuseFirstCycleLengthForBlock = true;
		delta.deltaCycleThreshhold = 0.1f;

		hlaf.setOptions(delta);

		hlaf.compress(b, mos);
		mos.flush();
		r = hlaf.getCompressionRatio();

		AudioSampleBuffer b3(1, b.getNumSamples());
		MemoryInputStream mis2(mos.getMemoryBlock(), true);

		hlaf.setupForDecompression();
		hlaf.decompress(b3, mis2);
		deltaSpeed += hlaf.getDecompressionPerformance();

		auto db2 = CompressionHelpers::getDifference(b3, b);

		Logger::writeToLog("Compressing with delta:  " + String(r, 3) + " Error: " + String(db2, 2) + "dB");

		deltaRatio += r;
		mos.reset();
		hlaf.reset();

		HiseLosslessAudioFormat::CompressorOptions diff;

		diff.fixedBlockWidth = 512;
		diff.removeDcOffset = false;
		diff.useDeltaEncoding = false;
		diff.useDiffEncodingWithFixedBlocks = true;

		hlaf.setOptions(diff);

		hlaf.compress(b, mos);

		r = hlaf.getCompressionRatio();

		Logger::writeToLog("Compressing with diff:   " + String(r, 3));

		diffRatio += r;
	}

	flacRatio /= (float)files.size();
	deltaRatio /= (float)files.size();
	diffRatio /= (float)files.size();
	blockRatio /= (float)files.size();

	blockSpeed /= (double)files.size();
	pcmSpeed /= (double)files.size();
	flacSpeed /= (double)files.size();
	deltaSpeed /= (double)files.size();

	Logger::writeToLog("=====================================================");
	Logger::writeToLog("FLAC ratio:\t" + String(flacRatio, 3));
	Logger::writeToLog("Block ratio:\t" + String(blockRatio, 3));
	Logger::writeToLog("Delta ratio:\t" + String(deltaRatio, 3));
	Logger::writeToLog("Diff ratio:\t" + String(diffRatio, 3));
	Logger::writeToLog("=====================================================");
	Logger::writeToLog("PCM speed:\t" + String(pcmSpeed, 1));
	Logger::writeToLog("FLAC speed:\t" + String(flacSpeed, 1));
	Logger::writeToLog("Block speed:\t" + String(blockSpeed, 1));
	Logger::writeToLog("Delta speed:\t" + String(deltaSpeed, 1));

	Logger::setCurrentLogger(nullptr);

	return 0;
}