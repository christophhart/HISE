/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.0f
  output: 1.25f
  error: ""
  filename: "variadic/process_single_test"
END_TEST_DATA
*/

struct X
{
    void processFrame(span<float, 2>& data)
    {
        data[0] *= 0.5f;
    }
};

struct Y
{
    void processFrame(span<float, 2>& data)
    {
        data[0] += 2.0f;
    }
};

container::chain<parameter::empty, X, Y, X> c;

span<float, 2> d = { 1.0f, 1.0f };

float main(float input)
{
	 c.processFrame(d);
    
    return d[0];
}