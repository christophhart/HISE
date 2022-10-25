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
    DECLARE_NODE(X);

    template <int P> void setParameter(double v)
    {
      
    }

    void processFrame(span<float, 1>& data)
    {
        data[0] *= 0.5f;
    }
};

struct Y
{
    DECLARE_NODE(Y);

    template <int P> void setParameter(double v)
    {
      
    }

    void processFrame(span<float, 1>& data)
    {
        data[0] += 2.0f;
    }
};

container::chain<parameter::empty, wrap::fix<1, X>, Y, X> c;

span<float, 1> d = { 1.0f };

float main(float input)
{
	 c.processFrame(d);
    
    return d[0];
}