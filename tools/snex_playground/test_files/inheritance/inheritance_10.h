/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 1.4
  error: ""
  filename: "inheritance/inheritance_10"
END_TEST_DATA
*/

namespace impl
{

struct chain_t: public container::chain<parameter::empty, wrap::fix<2, math::add>>
{
	struct metadata
	{
		SNEX_METADATA_ID(chain_t);
		SNEX_METADATA_NUM_CHANNELS(2);
		SNEX_METADATA_PARAMETERS(0, "");
	};
	
	chain_t()
	{
		auto& math = this->get<0>();
		
		math.setParameter<0>(0.7);
	}
};
	
}

using instance = wrap::node<impl::chain_t>;


span<float, 2> data;
instance obj;

float main(int input)
{
	obj.processFrame(data);

	return data[0] + data[1];
}

