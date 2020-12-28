/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 89
  error: ""
  filename: "wrap/wrap_mod"
END_TEST_DATA
*/

struct ModSource
{
	DECLARE_NODE(ModSource);
	
	static const int NumChannels = 1;
	
	template <int P> void setParameter(double d)
	{
		
	}

	void processFrame(span<float, 1>& data)
	{

	}

	void reset()
	{

	}

	bool handleModulation(double& v)
	{
		v = 90.0f;
		return true;
	}

	int x = 90;
};

struct ModTarget
{
	DECLARE_NODE(ModTarget);
	
	template <int P> void setParameter(double d)
	{
		value = (float)d;
	}

	void processFrame(span<float, 1>& data)
	{
		data[0] = value;
	}

	void reset()
	{

	}
	
	float value = 0.0f;
};

using ParameterType = parameter::plain<ModTarget, 0>;
using ModType = wrap::mod<ParameterType, ModSource>;

using ChainType = container::chain<parameter::empty, ModType, ModTarget>;

struct ChainType_Init
{
	ChainType_Init(ChainType& c)
	{
		auto& source = c.get<0>();
		auto& target = c.get<1>();
		
		source.getParameter().connect<0>(target);
		
		double v = 0.0;

		source.checkModValue();

#if 0
		if(source.getWrappedObject().handleModulation(v))
		{
			source.getParameter().call(v);
		}
#endif
	}
};

using Processor = wrap::init<ChainType, ChainType_Init>;

Processor p;
span<float, 1> x = { 0.0f };

int main(int input)
{
	p.reset();
	p.processFrame(x);
	return (int)x[0];
}

