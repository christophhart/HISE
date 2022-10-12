#ifndef __FAUST_JIT_NODE
#define __FAUST_JIT_NODE

namespace scriptnode {
namespace faust {

struct faust_jit_wrapper;

struct faust_jit_node: public scriptnode::WrapperNode
{
	SN_NODE_ID("faust");
	JUCE_DECLARE_WEAK_REFERENCEABLE(faust_jit_node);
	virtual Identifier getTypeId() const { RETURN_STATIC_IDENTIFIER("faust"); }

	faust_jit_node(DspNetwork* n, ValueTree v);
	virtual void prepare(PrepareSpecs specs) override;
	virtual void reset() override;
	virtual void process(ProcessDataDyn& data) override;
	virtual void processFrame(FrameType& data) override;
	virtual void handleHiseEvent(HiseEvent& e) override;

	/** This method will check whether the node header should draw the MIDI icon or not. */
	virtual bool isProcessingHiseEvent() const override;

	static NodeBase* createNode(DspNetwork* n, ValueTree v)
	{
		return new faust_jit_node(n, v);
	}
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

protected:
	std::unique_ptr<faust_jit_wrapper> faust;
	void setupParameters();
	void resetParameters();

private:
	void initialise(NodeBase* n);
	void loadSource();
	NodePropertyT<String> classId;
	void updateClassId(Identifier, var newValue);
};
} // namespace faust
} // namespace scriptnode

#endif // __FAUST_JIT_NODE
