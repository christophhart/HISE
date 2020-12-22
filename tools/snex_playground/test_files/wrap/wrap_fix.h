/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "wrap/wrap_fix"
END_TEST_DATA
*/

struct dc1
{
	DECLARE_NODE(dc1);

	static const int NumChannels = 1;

	template <int P> void setParameter(double d)
	{
		
	}

	void process(ProcessData<1>& d)
	{
		
	}

	template <int C> void processFrame(span<float, C>& data)
	{
		for(auto& s: data)
		    s += 1.0f;
	}
};

struct dc2
{
	DECLARE_NODE(dc2);
	
	static const int NumChannels = 1;

	template <int P> void setParameter(double d)
	{
		
	}

	void process(ProcessData<1>& d)
	{
		
	}

	template <typename D> void processFrame(D& data)
	{
		for(auto& s: data)
			s += 2.0f;
	}
};


wrap::fix<1, dc1> wdc1_obj;

wrap::fix<1, dc2> wdc2_obj;

container::chain<parameter::empty, wrap::fix<1, dc1>> cwdc1_obj;

container::chain<parameter::empty, dc1> cdc1_obj;
container::chain<parameter::empty, dc2, dc1> cdc2_obj;

int main(int input)
{
	span<float, 1> data = {0.0f};
	
	wdc1_obj.processFrame(data);
	wdc2_obj.processFrame(data);
	
	cwdc1_obj.processFrame(data);
	cdc1_obj.processFrame(data);
	cdc2_obj.processFrame(data);
	
	return (int)data[0];
	
}

