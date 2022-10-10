#include <atomic>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#ifndef __FAUST_BASE_WRAPPER_H
#define __FAUST_BASE_WRAPPER_H

namespace scriptnode {
namespace faust {

// wrapper struct for faust types to avoid name-clash
struct faust_base_wrapper {

	faust_base_wrapper(::faust::dsp* faustDsp):
		faustDsp(faustDsp),  // Unless faustDsp is set to a non-nullptr value in a subclass, several methods will cause a segfault
		sampleRate(0),
		_nChannels(0),
		_nFramesMax(0)
	{
		// faustDsp will be instantiated in templated class here
	}

	~faust_base_wrapper()
	{
	}

	// std::string code;
	int sampleRate;
	::faust::dsp *faustDsp;
	faust_ui ui;

	// audio buffer
	int _nChannels;
	int _nFramesMax;
	std::vector<float> inputBuffer;
	std::vector<float*> inputChannelPointers;
	std::vector<float*> outputChannelPointers;

	// This method assumes faustDsp to be initialized correctly
	virtual bool setup() {
		faustDsp->buildUserInterface(&ui);

		DBG("Faust parameters:");
		for (auto p : ui.getParameterLabels()) {
			DBG(p);
		}

		init();
		return true;
	}

	void prepare(PrepareSpecs specs)
	{
		// recompile if sample rate changed
		int newSampleRate = (int)specs.sampleRate;
		if (newSampleRate != sampleRate) {
			sampleRate = newSampleRate;
			// init samplerate
			init();
		}

		if (_nChannels != specs.numChannels || _nFramesMax != specs.blockSize) {
			DBG("Faust: Resizing buffers: nChannels=" << _nChannels << ", blockSize=" << _nFramesMax);
			_nChannels = specs.numChannels;
			_nFramesMax = specs.blockSize;
			resizeBuffer();
		}
	}

	void init() {
		if (faustDsp)
			faustDsp->init(sampleRate);
	}

	void reset()
	{
		if (faustDsp)
			faustDsp->instanceClear();
	}


	String getClassId() {
		return classId;
	}

	// This method assumes faustDsp to be initialized correctly
	void process(ProcessDataDyn& data)
	{
		// we have either a static ::faust::dsp object here, or we hold the jit lock
		// TODO: stable and sane sample format matching
		int n_faust_inputs = faustDsp->getNumInputs();
		int n_faust_outputs = faustDsp->getNumOutputs();
		int n_hise_channels = data.getNumChannels();

		if (n_faust_inputs == n_hise_channels && n_faust_outputs == n_hise_channels) {
			int nFrames = data.getNumSamples();
			float** channel_data = data.getRawDataPointers();
			// copy input data, because even with -inpl not all faust generated code can handle
			// in-place processing
			bufferChannelsData(channel_data, n_hise_channels, nFrames);
			faustDsp->compute(nFrames, getRawInputChannelPointers(), channel_data);
		} else {
			// TODO error indication
		}
	}

	// This method assumes faustDsp to be initialized correctly
	template <class FrameDataType> void processFrame(FrameDataType& data)
	{
		int n_faust_inputs = faustDsp->getNumInputs();
		int n_faust_outputs = faustDsp->getNumOutputs();
		// TODO check validity
		int n_hise_channels = data.size();

		if (n_faust_inputs == n_hise_channels && n_faust_outputs == n_hise_channels) {
			// copy input data, because even with -inpl not all faust generated code can handle
			// in-place processing
			bufferChannelsData(&data[0], n_hise_channels);
			faustDsp->compute(1, getRawInputChannelPointers(), getRawOutputChannelPointers());
		} else {
			// TODO error indication
		}
	}

	float** getRawInputChannelPointers() {
		return &inputChannelPointers[0];
	}

	float** getRawOutputChannelPointers() {
		return &outputChannelPointers[0];
	}

	void resizeBuffer()
	{
		inputBuffer.resize(_nChannels * _nFramesMax);
		// setup new pointers
		inputChannelPointers.resize(_nChannels);
		inputChannelPointers.clear();
		for (int i=0; i<inputBuffer.size(); i+=_nFramesMax) {
			inputChannelPointers.push_back(&inputBuffer[i]);
		}
		// resize output pointers for processFrame()
		outputChannelPointers.resize(_nChannels);
	}

	void bufferChannelsData(float* channels, int nChannels)
	{
		jassert(nChannels == _nChannels);
		jassert(_nFramesMax > 0);
		for (int i=0; i<nChannels; i++) {
			// buffer channel sample
			inputChannelPointers[i][0] = channels[i];
			// store pointer to original value to write back to
			outputChannelPointers[i] = &channels[i];
		}
	}

	void bufferChannelsData(float** channels, int nChannels, int nFrames)
	{
		jassert(nChannels == _nChannels);
		jassert(nFrames <= _nFramesMax);

		for (int i=0; i<nChannels; i++) {
			memcpy(inputChannelPointers[i], channels[i], nFrames * sizeof(float));
		}
	}

private:
	String classId;
};

} // namespace faust
} // namespace scriptnode

#endif // __FAUST_BASE_WRAPPER_H
