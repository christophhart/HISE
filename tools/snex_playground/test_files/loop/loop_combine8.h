/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<2>
  input: "zero2.wav"
  output: "half2.wav"
  error: ""
  loop_count: 6
  compile_flags: Inlining LoopOptimisation
  filename: "loop/loop_combine8"
END_TEST_DATA
*/


struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double d)
	{
		
	}

	void reset() {}
	void prepare(PrepareSpecs ps) {}
	void processFrame(span<float, 2>& data) {}
	void process(ProcessData<2>& data)
	{
		for(auto& c: data)
		{
			for(auto& s: data.toChannelData(c))
			{
				s += 0.25f;
			}
		}
	}
	
	void handleEvent(HiseEvent& e) {}
};


struct Y
{
	DECLARE_NODE(Y);

	template <int P> void setParameter(double d)
	{
		
	}

	void reset() {}
	void prepare(PrepareSpecs ps) {}
	void processFrame(span<float, 2>& data) {}
	void process(ProcessData<2>& data)
	{
		for(auto& c: data)
		{
			for(auto& s: data.toChannelData(c))
			{
				s *= 2.0f;
			}
		}
	}
	
	void handleEvent(HiseEvent& e) {}
};

container::chain<parameter::empty, wrap::fix<2, X>, Y> c;

int main(ProcessData<2>& data)
{
	c.process(data);
	
	
	return 0;
}

