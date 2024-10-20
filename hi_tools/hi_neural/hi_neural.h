#pragma once

namespace hise
{
using namespace juce;

/** A wrapper around the RTNeural framework.
 
    - Load a model using the model.txt file created from your Pytorch model
    - Load the weights created from the Pytorch script
    - Run inference by calling process() with the number of inputs / outputs you need.

    ```python
    # Define this somehow
    model = MyModel()
    # Load the previously exported weights (if you haven't trained the model in this session)
	model.load_state_dict(torch.load("weights.pth"))

	# Save the model structure to a text file, then pass this into parseModelJSON to get
	# a JSON object describing the network layout. This also does some error checking to ensure
	# that you don't use unsupported layers etc...
	with open('model.txt', 'w') as f:
		print(model, file=f)

	# Dump the weights as JSON and pass this into loadWeights()
	# (The EncodeTensor class is a helper class supplied by the RTNeural script that can be
	# found here: https://github.com/jatinchowdhury18/RTNeural/blob/main/python/conv1d_torch.py
	# (there you can also find all required import statements)
	with open('parameters.json', 'w') as json_file:
        json.dump(model.state_dict(), json_file,cls=EncodeTensor)
	```
 */
struct NeuralNetwork: public ReferenceCountedObject,
                      public runtime_target::source_base
{
	/** This POD will hold the information about the expected processing mode for the given network
	 *  Usually you will set these properties after loading a model and then the neural network will
	 *	be processed accordingly.
	 */
	struct ProcessingContext
	{
		// the number of samples that will be fed into the network
		// at once. Usually this is 1 so the network will process each sample, but you can increase that
		// number in order to send chunks of audio signals at once into the network.
		int numSamplesPerCall = 1;
		
		// if this is zero the network assumes that each channel should be processed independently and
		// create internal copies for each channel. If you need the network to process multichannel information
		// set this to the amount of channels that you expect.
		// Note: If the network expects multiple frames of multichannel data (by setting both this to > 0 and
		// numSamplesPerCall to > 1, then it will expect the data to be interleaved channel audio.
		int fixChannelSize = 0;

		bool shouldCloneChannels() const { return fixChannelSize == 0; }

		/** This returns the number of input parameters that are expected to be the audio signal.
		 *
		 *  This does not have to be exactly the amount of inputs into the neural network (and the remaining
		 *	input parameters will be considered as dynamically assignable parameters).
		 */
		int getSignalInputSize() const
		{
			if (fixChannelSize)
				return fixChannelSize * numSamplesPerCall;
			else
				return numSamplesPerCall;
		}
	};

	using Ptr = ReferenceCountedObjectPtr<NeuralNetwork>;
	using List = ReferenceCountedArray<NeuralNetwork>;

	struct ModelBase
	{
        ModelBase() {};
		virtual ~ModelBase() {};

		virtual void reset() = 0;
		virtual void process(const float* input, float* output) = 0;
		virtual int getNumInputs() const = 0;
		virtual int getNumOutputs() const = 0;
		virtual ModelBase* clone() = 0;
		virtual Result loadWeights(const String& jsonData) = 0;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelBase);
	};

	struct CppBuilder
	{
		CppBuilder(const Identifier& id_, const var& modelJson);;

		String createCppModel() const;

	private:

		Identifier id;
		Array<var> layers;
	};

	struct Factory
	{
		Factory();

		// Use this to register statically compiled models
		template <typename T> void registerModel()
		{
			registeredModels.add({T::getStaticId(), T::create});
		}

		ModelBase* create(const Identifier& id);

	private:

		using CreateFunction = std::function<ModelBase*()>;
		CreateFunction defaultFunction;
		Array<std::pair<Identifier, CreateFunction>> registeredModels;
	};

	struct Holder
	{
        Holder()
        {
            f = new Factory();
        }
        
        ~Holder()
        {
            f = nullptr;
        }
        
		/** Creates a network with a unique ID or returns a reference if it already exists. */
		Ptr getOrCreate(const Identifier& id);

		/** Returns a list of all created networks. */
		StringArray getIdList() const;

        Factory* getFactory() { return f; }
        
	private:

        ScopedPointer<Factory> f;
        
		List networks;
	};

	NeuralNetwork(const Identifier& id, Factory* f);

	Identifier getId() const { return id; }

    int getRuntimeHash() const override { return id.toString().hashCode(); }
    
    runtime_target::RuntimeTarget getType() const override
    {
        return runtime_target::RuntimeTarget::NeuralNetwork;
    }
    
    NeuralNetwork::Ptr clone(int numNetworks);
    
	~NeuralNetwork();

	/** Parses the model layout from the print() output of the Pytorch model. */
	static var parseModelJSON(const File& modelFile);
	
	int getNumInputs() const;
	int getNumOutputs() const;
	void reset(int networkIndex=-1);
	void process(int networkIndex, const float* input, float* output);
	void clearModel();

	/* Loads a model with trained weights from Tensorflow. */
	Result loadTensorFlowModel(const var& jsonData);

	/** Loads a model with trained weights from Pytorch. */
	Result loadPytorchModel(const var& jsonData);

	/** Build a model from the JSON layout. */
	Result build(const var& modelJSON);

	/** Load the weights. */
	Result loadWeights(const String& jsonData);

	var getModelJSON() const;

	/** This lets you create a number of copies of the same network that you then can address with the network index in both the reset and process call.
	 *
	 *  This is used for implementing multi-channel processing and polyphony. It's called dynamically by the scriptnode `math.neural` node to match the processing context.
	 *
	 *	If your network needs to process multiple channels at once, you can call setCloneChannels(false) and then it will be fed the multichannel frame.
	 *	(you can still override that using forceClone which is required for polyphonic processing. */
	void setNumNetworks(int numNetworks, bool forceClone=false);

	/** Returns the number of internal networks clones. */
	int getNumNetworks() const noexcept { return currentModels.size(); }

	/** Get the number of input parameters that are not fed into the audio signal. */
	int getNumAssignableParameters() const { return jmax(0, getNumInputs() - context.getSignalInputSize()); }

	// Contains the processing information for the network
	ProcessingContext context;

private:
	
    Factory* factory = nullptr;
    
	mutable hise::SimpleReadWriteLock lock;

	const Identifier id;

	OwnedArray<ModelBase> currentModels;
};


} // namespace hise} // namespace hise
