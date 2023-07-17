/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 11
  output: 10.0f
  error: ""
  filename: "00 Examples/07_templates2"
END_TEST_DATA
*/


/** Now let's look at a barebone node definition in SNEX and how templates are used there.

	It's not complete as they are a few callbacks missing, but it includes
	 every callback that contains a template parameter.)
*/
struct ExampleNode
{
	SNEX_NODE(ExampleNode);
	

	/*  This method will be called to do block based processing.
		It is templated because the argument type can be different
		according to whether the processing is using a fix amount of
		channels (in this case, ProcessDataType will be `ProcessData<NumChannels>`)
		or a dynamic one (in this case ProcessDataType will be `ProcessDataDyn`)
	*/
	template <typename ProcessDataType> void process(ProcessDataType& data)
	{
		// We'll leave this empty for now.
	
	}
	
	/** This method will be called to do frame based processing. A frame
	    is a consecutive float array with one sample per channel, so you
	    can do interleaved processing.
	    
	    Because this function will be called very often, it needs to be
	    able to make compile time assumptions about the channel amount,
	    so the FrameDataType will always be a `span<float, NumChannels>`.
	*/
	template <typename FrameDataType> void processFrame(FrameDataType& data) 
	{
		// This loop will most likely be unrolled because the compiler
		// knows exactly how many elements `data` has
		for(auto& s: data)
		    s = value;
	}
	
	/** This method will be called when a parameter is changed. The function signature
	    needs to be always
	    
	    void function(double value)
	  
	  	and it will be used by the parameter classes to create compile time 
	  	connections for changing parameters.
	*/
	template <int P> void setParameter(double newValue)
	{
		// Since P is a compile time constant, this branching will be removed
		// at template instantiation
		if(P == 0)
			value = (float)newValue * 2.0f;
	}
	
	float value = 1.0f;
};


span<float, 1> monoFrame = { 0.0f };
span<float, 2> stereoFrame = { 0.0f };

ExampleNode testNode;


float main(int input)
{
	/*  Take a look at the assembly of the float main(int input) method:
	
		mov rcx, 2274697052612    ; load the address of the stereo frame
		movss xmm0, dword [L2]    ; load the constant 0.8 into the xmm0 register
		movss [rcx], xmm0         ; write the register xmm0 to stereoFrame[0]
		movss [rcx+4], xmm0       ; write the register xmm0 to stereoFrame[1]
		
	    As you can see, the compiler has fully inlined the function call and
	    unrolled the loop
	*/
	testNode.processFrame(stereoFrame);
	

	/** Now we call the parameter method that changes the internal value.
	
		The assembly of that function call looks like this:
		
		
		mov rax, 2274916337836	 ; load the address of testNode::value
		movss xmm0, dword [L2+8] ; load the float constant 8.0f into xmm0
		movss dword [rax], xmm0  ; write the xmm0 register to testnode::value
		
		As you can see, it has condensed the function call to a single memory
		write operation. It has even removed the multiplication because it
		could evaluate the literal argument at compile time (the original function 
		template can't make that assumption, so if you look at the assembly of 
		
		function void ExampleNode::setParameter<0>
		
		you will see that there is an actual multiplication (and type cast) happening.
	*/
	testNode.setParameter<0>(4.0);
	

	/*  And now the assembly for the mono call
	
		mov rax, 2274697052608	  ; load the address of the mono frame data
		movss [rax], xmm0         ; write the register xmm0 to monoFrame[0]
		
		It could reuse the 0.8f constant in the xmm0 register because and
		unrolled the loop to a single operation.
		
		You can play around with the different optimisations (The most 
		interesting ones are Inlining and LoopOptimisation to see how it
		affects the code generation.)
	*/	
	testNode.processFrame(monoFrame);
	
	// Sum all values to check that the test is passed...
	return monoFrame[0] + stereoFrame[0] + stereoFrame[1];
}

