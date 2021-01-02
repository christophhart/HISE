/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 0.5
  output: 0.5
  error: ""
  filename: "node_library/math_add_in_container"
END_TEST_DATA
*/


struct instance: public container::chain<parameter::empty,
                                         math::add>
{
	instance()
	{
		get<0>().setParameter<0>(0.5);
	}
};

instance obj;
span<float, 1> data = {0.0f};

float main(int input)
{
	obj.processFrame(data);
	
	return data[0];
}

