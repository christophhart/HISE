/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 0.125f
  error: ""
  filename: "variadic/process_single_test"
END_TEST_DATA
*/

struct X
{
    void processSingle(span<float, 2>& data)
    {
        data[0] *= 0.5f;
    }
};

container::chain<X, X, X> c;

span<float, 2> d = { 1.0f, 1.0f };

float main(float input)
{
	 c.processSingle(d);
    
    return d[0];
}