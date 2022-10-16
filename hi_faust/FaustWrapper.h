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

	virtual ~faust_base_wrapper()
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
        // Skip the prepare call when the processing context
        // isn't valid yet
        if(!specs)
            return;
        
		if (_nChannels != specs.numChannels || _nFramesMax != specs.blockSize) {
			DBG("Faust: Resizing buffers: nChannels=" << _nChannels << ", blockSize=" << _nFramesMax);
			_nChannels = specs.numChannels;
			_nFramesMax = specs.blockSize;
			resizeBuffer();
		}

		// recompile if sample rate changed
		int newSampleRate = (int)specs.sampleRate;
		if (newSampleRate != sampleRate) {
			sampleRate = newSampleRate;
			// init samplerate
			init();
		}
		else
			throwErrorIfChannelMismatch();
	}

	void throwErrorIfChannelMismatch()
	{
		if (faustDsp)
		{
			auto numInputs = faustDsp->getNumInputs();
			auto numOutputs = faustDsp->getNumOutputs();
			auto numHiseChannels = _nChannels;

			if (numInputs > numHiseChannels ||
				numOutputs != numHiseChannels)
			{
				// the error system expects a single integer as "expected value", so we need
				// to encode both input and output channels into a single integer using a magic trick
				auto encodedChannelCount = 1000 * numInputs + numOutputs;

				scriptnode::Error::throwError(Error::IllegalFaustChannelCount, numHiseChannels, encodedChannelCount);
			}
		}
	}

	void init() {
		if (faustDsp && sampleRate > 0)
		{
			throwErrorIfChannelMismatch();
            
            faust_ui::ScopedZoneSetter szs(ui);
            
			faustDsp->init(sampleRate);
		}
	}

	void reset()
	{
		if (faustDsp)
			faustDsp->instanceClear();
	}


	String getClassId() {
		return classId;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		ui.handleHiseEvent(e);
	}

    bool handleModulation(double& v)
    {
        return ui.handleModulation(v);
    }
    
	// This method assumes faustDsp to be initialized correctly
	void process(ProcessDataDyn& data)
	{
		// we have either a static ::faust::dsp object here, or we hold the jit lock
		
		int n_faust_inputs = faustDsp->getNumInputs();
		int n_faust_outputs = faustDsp->getNumOutputs();
		int n_hise_channels = data.getNumChannels();

		// We allow the input channel amount to be less than the HISE channel count
		auto ok = n_hise_channels == n_faust_outputs && n_faust_inputs <= n_hise_channels;

		if (ok) {
			int nFrames = data.getNumSamples();
			float** channel_data = data.getRawDataPointers();
			// copy input data, because even with -inpl not all faust generated code can handle
			// in-place processing
			bufferChannelsData(channel_data, n_hise_channels, nFrames);
			faustDsp->compute(nFrames, getRawInputChannelPointers(), channel_data);
		} else {
			// should be catched by the init check
			jassertfalse;
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
