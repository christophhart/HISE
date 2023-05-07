/*
BEGIN_TEST_DATA
  f: {MyContainer}
  ret: ProcessData<1>
  args: ProcessData<1>
  input: zero.wav
  output: chain_test.wav
  error: ""
  filename: "variadic/chain_test"
END_TEST_DATA
*/

struct X
{
    DECLARE_NODE(X);

    int v = 0;

    template <int P> void setParameter(double v)
    {
      
    }
    
    void prepare(PrepareSpecs ps) {}
    
    void reset() {}
    
    void handleHiseEvent(HiseEvent& e) {}
    
    void processFrame(span<float, 1>& d) {}
    
    void process(ProcessData<1>& d)
    {
        for(auto& s: d[0])
        	s += 0.01f;
    }
};

using MyContainer = container::chain<parameter::empty, wrap::fix<1, X>, X, X, X, X, X, X, X, X, X, X, X, X>;

