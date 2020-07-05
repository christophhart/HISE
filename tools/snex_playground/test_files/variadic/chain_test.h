/*
BEGIN_TEST_DATA
  f: {MyContainer}
  ret: ProcessData<1>
  args: ProcessData<1>
  input: zero.wav
  output: zero.wav
  error: ""
  filename: "variadic/chain_test"
END_TEST_DATA
*/

struct X
{
    int v = 0;
    
    void prepare(PrepareSpecs ps) {}
    
    void reset() {}
    
    void handleEvent(HiseEvent& e) {}
    
    void processFrame(span<float, 1>& d) {}
    
    void process(ProcessData<2>& d)
    {
        for(auto& s: d[0])
        	s += 0.01f;
    }
};

using MyContainer = container::chain<parameter::empty, X, X, X, X, X, X, X, X, X, X, X, X, X>;

