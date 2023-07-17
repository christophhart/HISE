#ifndef __FAUST_JIT_NODE
#define __FAUST_JIT_NODE

namespace scriptnode {
namespace faust {

/** This node is a base class for the actual faust JIT node implementations which are templated by their voice amount.

	There are a few virtual function calls that the sub classes implement to allow a common interface between monophonic and polyphonic JIT nodes.
	The Faust menubar (and all other UI components) will use this class and all the processing code which relies on the internal `faust_jit_wrapper`
	is moved to the subclasses.
*/
class faust_jit_node_base: public scriptnode::WrapperNode,
						   public DspNetwork::FaustManager::FaustListener
{
public:

	SN_NODE_ID("faust");
	JUCE_DECLARE_WEAK_REFERENCEABLE(faust_jit_node_base);
	virtual Identifier getTypeId() const { RETURN_STATIC_IDENTIFIER("faust"); }

	faust_jit_node_base(DspNetwork* n, ValueTree v);
    
    ~faust_jit_node_base();
    
    Result getLastResult() const { return compileResult; }
    
    File getFaustRootFile();
	File getFaustFile(String basename);
	std::vector<std::string> getFaustLibraryPaths();

	// pure virtual to set/get the class in faust_jit_node and
	// only get in faust_node<T>, here because of UI code
	virtual String getClassId();
	virtual StringArray getAvailableClassIds();
	virtual bool removeClassId(String classIdToRemove);
	virtual void setClass(const String& newClassId);
	virtual void createSourceAndSetClass(const String newClassId);
	virtual void reinitFaustWrapper();

	// Parameter methods
	void parameterUpdated(ValueTree child, bool wasAdded);
	void addNewParameter(parameter::data p);
	valuetree::ChildListener parameterListener;

	// provide correct pointer to createExtraComponent()
	virtual void* getObjectPtr() override { return (void*)this; }

	void logError(String errorMessage);

    virtual void faustFileSelected(const File& ) override {};
    virtual Result compileFaustCode(const File& f) override;
    virtual void faustCodeCompiled(const File& f, const Result& compileResult) override {};
    
    // Faust Mono / Poly Interface functions ===========================================================
    
    /** Checks if a mod zone is defined. Super poorly named, rename to isUsingModulation() ASAP!!! */
    virtual bool isUsingNormalisation() const = 0;

    /** Returns the internal parameter::dynamic_list object that is using the multiple faust outputs. */
    virtual parameter::dynamic_list* getFaustModulationOutputs() = 0;
    
    virtual String getFaustModulationOutputName(int index) const = 0;
    
    /** Returns the amount of modulation outputs of the faust_ui object. */
    virtual int getNumFaustModulationOutputs() const = 0;
    
    virtual SimpleReadWriteLock& getFaustCompileLock() = 0;
    
protected:
	
	// Checks if the faust_ui object has a given parameter. */
	virtual bool hasFaustParameter(const String& id) const = 0;

	/** Returns the amount of parameters of the faust_ui object. */
	virtual int getNumFaustParameters() const = 0;
    
	/** Connects the parameters to the zone pointers of the faust_ui object. */
	virtual void setupFaustParameters() = 0;

	/** Send the compile data to the templated faust_jit_wrapper. */
	virtual bool setFaustCode(const String& classId, const std::string& code) = 0;

	/** Setup the compiler data for the templated faust_jit_wrapper. */
	virtual bool setupFaust(const std::vector<std::string>& faustLibrary, std::string& error_msg) = 0;

	/** This returns the class ID from the faust object (!= the node property). */
	virtual String getFaustClassId() const = 0;

    
    
	// ===================================================================================================

    void setupParameters();
    void resetParameters();
    
    // the faust parameters need to be wrapped into dynamic_base_holders
    // and they need to be assigned with NodeBase::Parameter::setDynamicParameter()
    // (if a macro or modulation source grabs the dynamic parameter, it might
    // cause a crash when you recompile the faust node because the connection
    // points to a dangling zone pointer.
    parameter::dynamic_base::List parameterHolders;
    
    Result compileResult;
    
	void initialise(NodeBase* n);

	void loadSource();
	NodePropertyT<String> classId;
	void updateClassId(Identifier, var newValue);
};

/** This implements all the interactions with the faust_jit_wrapper because it is templated. 

	The functions of this class implement:

	1. All the scriptnode processing callbacks using the instantiated faust_jit_wrapper object
	2. All the interface functions defined by the faust_jit_node_base base class
*/
template <int NV> struct faust_jit_node: public faust_jit_node_base
{
	faust_jit_node(DspNetwork* n, ValueTree v) :
		faust_jit_node_base(n, v),
		faust(new faust_jit_wrapper<NV>())
	{
		// we can't init yet, because we don't know the sample rate
		initialise(this);

        // connect the internal parameter::dynamic_list to the value tree to
        // dynamically update the number of parameters
        faust->ui.modParameters.initialise(this);
        
		// This needs to called here because it might trigger an asynchronous recompilation which would
		// cause pure virtual function calls if it's called from the base class constructor...
		setClass(classId.getValue());
	};

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		return new faust_jit_node<NV>(n, v);
	}

	// We need to template this function using the NV (voice amount) template parameter
	std::unique_ptr<faust_jit_wrapper<NV>> faust;

	// Overriding all virtual functions defined in faust_jit_node_base =============================

	bool setupFaust(const std::vector<std::string>& faustLibrary, std::string& error_msg) override
	{
		return faust->setup(getFaustLibraryPaths(), error_msg);
	}

	String getFaustClassId() const override
	{
		return faust->getClassId();
	}

    SimpleReadWriteLock& getFaustCompileLock() override
    {
        return faust->jitLock;
    };
    
	bool setFaustCode(const String& classId, const std::string& code) override
	{
		return faust->setCode(classId, code);
	}

	int getNumFaustParameters() const override
	{
		return (int)faust->ui.parameters.size();
	}

    int getNumFaustModulationOutputs() const override
    {
        return (int)faust->ui.modoutputs.size();
    }
    
    parameter::dynamic_list* getFaustModulationOutputs() override
    {
        return &faust->ui.modParameters;
    }
    
    String getFaustModulationOutputName(int index) const override
    {
        if(isPositiveAndBelow(index, getNumFaustModulationOutputs()))
            return faust->ui.modoutputs[index]->label;
        
        // shouldn't happen...
        return String(index);
    }
    
	bool hasFaustParameter(const String& id) const override
	{
		for (auto p : faust->ui.parameters)
		{
			if (p->label == id)
				return true;
		}

		return false;
	}

	void setupFaustParameters() override
	{
		int pIndex = 0;

		for (auto p : faust->ui.parameters)
		{
			auto pd = p->toParameterData();

			auto newDynamicParameter = new parameter::dynamic_base(pd.callback);

			// Try to use the existing value tree so that it can take the
			// customized parameter ranges
			auto parameterValueTree = getParameterTree().getChildWithProperty(PropertyIds::ID, p->label);

			if (!parameterValueTree.isValid())
				parameterValueTree = pd.createValueTree();

			newDynamicParameter->updateRange(parameterValueTree);


			if(parameterValueTree.hasProperty(PropertyIds::Value))
				newDynamicParameter->call((double)parameterValueTree[PropertyIds::Value]);

			auto h = dynamic_cast<parameter::dynamic_base_holder*>(parameterHolders[pIndex++].get());

			jassert(h != nullptr);

			h->setParameter(this, newDynamicParameter);

			addNewParameter(pd);
		}
	}

	bool isProcessingHiseEvent() const
	{
		return faust->ui.anyMidiZonesActive;
	}

	// Overriding all scriptnode processing callbacks using the faust object ============================

	void handleHiseEvent(HiseEvent& e)
	{
		if (isBypassed()) return;
		hise::SimpleReadWriteLock::ScopedReadLock sl(getFaustCompileLock());
		faust->handleHiseEvent(e);
	}

	void processFrame(FrameType& data)
	{
		if (isBypassed()) return;
		hise::SimpleReadWriteLock::ScopedReadLock sl(getFaustCompileLock());
		faust->faust_jit_wrapper<NV>::template processFrame<FrameType>(data);
	}

	void process(ProcessDataDyn& data)
	{
		if (isBypassed()) return;
        
        NodeProfiler np(this, data.getNumSamples());
        ProcessDataPeakChecker fd(this, data);
        
		hise::SimpleReadWriteLock::ScopedReadLock sl(getFaustCompileLock());
		faust->process(data);
	}

	void reset()
	{
		hise::SimpleReadWriteLock::ScopedReadLock sl(getFaustCompileLock());
		faust->reset();
	}

	void prepare(PrepareSpecs specs)
	{
		WrapperNode::prepare(specs);

		try
		{
			getRootNetwork()->getExceptionHandler().removeError(this, Error::ErrorCode::IllegalFaustChannelCount);

			lastSpecs = specs;
			hise::SimpleReadWriteLock::ScopedReadLock sl(getFaustCompileLock());
			faust->prepare(specs);
		}
		catch (scriptnode::Error& e)
		{
			getRootNetwork()->getExceptionHandler().addError(this, e);
		}
	}

	bool isUsingNormalisation() const override
	{
        return getNumFaustModulationOutputs() > 0;
	}
};


} // namespace faust
} // namespace scriptnode

#endif // __FAUST_JIT_NODE
