/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "node/chain_num_channels"
END_TEST_DATA
*/

struct dc
{
	DECLARE_NODE(dc);

	static const int NumChannels = 1;

	template <int P> void setParameter(double v)
	{
		
	}

	void processFrame(span<float, NumChannels>& data)
	{
		data[0] = 1.0f;
	}
	
	
};

container::chain<parameter::empty, dc, dc> c;

int main(int input)
{
	span<float, 1> data = {0.0f};
	
	c.processFrame(data);
	
	return (int)data[0];
	
}

