#include <atomic>
#include <chrono>
#include <thread>
using namespace std::chrono_literals;

#ifndef __FAUST_BASE_WRAPPER_H
#define __FAUST_BASE_WRAPPER_H

namespace scriptnode {
namespace faust {

/** wrapper struct for faust types to avoid name-clash.

	This is the base class for every faust node - statically compiled C++ faust nodes
	as well as JIT compiled faust nodes.

	It is templated with the voice amount to allow polyphonic versions of faust nodes.
	
	The concept of polyphony is just duplicating the instances of the internal faust classes
	and let the scriptnode polyphonic system work out which instance should be used for the
	currently active voice.

	In order to achieve this, almost every class needed to be converted to a template class
	with the NV integer argument indicating the number of voices.

	Note: By default, HISE uses a NUM_POLYPHONIC_VOICES value of 256, which might be a bit high
	if you're running complex faust patches. So if memory consumption / loading times get too high
	consider lowering this preprocessor (it's used consistently across the entire codebase so you 
	should be able to control the number of voices you need).
 
    The ParameterClass template argument is used for multi-output modulation sources and will be
    populated with zones of hbargraph / vbargraph elements.
*/
template <int NV, class ParameterClass> struct faust_base_wrapper 
{
	static constexpr int NumVoices = NV;

	faust_base_wrapper():
		sampleRate(0),
		_nChannels(0),
		_nFramesMax(0)
	{
		for (auto& fdsp : faustDsp)
			fdsp = nullptr;
	}

	virtual ~faust_base_wrapper()
	{
		for (auto& fdsp : faustDsp)
			fdsp = nullptr;
	}

	// std::string code;
	int sampleRate;

	// This contains a faust instance for each voice
	PolyData<::faust::dsp*, NumVoices> faustDsp;

	// The UI class is templated because it needs to update either a single zone pointer
	// or all zone pointers of a voice
	faust_ui<NV, ParameterClass> ui;

	// audio buffer
	int _nChannels;
	int _nFramesMax;
	std::vector<float> inputBuffer;
	std::vector<float*> inputChannelPointers;
	std::vector<float*> outputChannelPointers;

	bool initialisedOk() const
	{
		return faustDsp.getFirst() != nullptr;
	}

	// This method assumes faustDsp to be initialized correctly
	virtual bool setup() 
	{
		for(auto f: faustDsp)
			f->buildUserInterface(&ui);

		init();
		
		return true;
	}

	virtual void prepare(PrepareSpecs specs)
	{
		ui.prepare(specs);

		faustDsp.prepare(specs);

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
		if (auto first = faustDsp.getFirst())
		{
			auto numInputs = first->getNumInputs();
			auto numOutputs = first->getNumOutputs();
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

	void init() 
	{
		if (initialisedOk() && sampleRate > 0)
		{
			throwErrorIfChannelMismatch();
            
            ScopedZoneSetter szs(this->ui);
            
			for(auto f: this->faustDsp)
				f->init(sampleRate);
		}
	}

	void reset()
	{
		if (initialisedOk())
		{
			for(auto f: faustDsp)
				f->instanceClear();
		}
	}


	String getClassId() {
		return classId;
	}

	void handleHiseEvent(HiseEvent& e)
	{
		ui.handleHiseEvent(e);
	}

       
	// This method assumes faustDsp to be initialized correctly
	void process(ProcessDataDyn& data)
	{
		// we have either a static ::faust::dsp object here, or we hold the jit lock
		
		auto fdsp = faustDsp.get();

		int n_faust_inputs = fdsp->getNumInputs();
		int n_faust_outputs = fdsp->getNumOutputs();
		int n_hise_channels = data.getNumChannels();

		// We allow the input channel amount to be less than the HISE channel count
		auto ok = n_hise_channels == n_faust_outputs && n_faust_inputs <= n_hise_channels;

		if (ok) {
			int nFrames = data.getNumSamples();
			float** channel_data = data.getRawDataPointers();
			// copy input data, because even with -inpl not all faust generated code can handle
			// in-place processing
			bufferChannelsData(channel_data, n_hise_channels, nFrames);
			fdsp->compute(nFrames, getRawInputChannelPointers(), channel_data);
		} else {
			// should be catched by the init check
			jassertfalse;
		}
        
        ui.handleModulationOutputs();
	}

	// This method assumes faustDsp to be initialized correctly
	template <class FrameDataType> void processFrame(FrameDataType& data)
	{
		auto fdsp = faustDsp.get();

		int n_faust_inputs = fdsp->getNumInputs();
		int n_faust_outputs = fdsp->getNumOutputs();
		// TODO check validity
		int n_hise_channels = data.size();

		if (n_faust_inputs == n_hise_channels && n_faust_outputs == n_hise_channels) {
			// copy input data, because even with -inpl not all faust generated code can handle
			// in-place processing
			bufferChannelsData(&data[0], n_hise_channels);
			fdsp->compute(1, getRawInputChannelPointers(), getRawOutputChannelPointers());
		} else {
			// TODO error indication
		}
        
        ui.handleModulationOutputs();
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
