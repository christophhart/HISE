/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 512
  error: ""
  filename: "variadic/prepare_specs_pass_by_value"
END_TEST_DATA
*/


void clearPrepareSpecs(PrepareSpecs ps)
{
    ps.blockSize = 9;
}

int main(int input)
{
	PrepareSpecs ps;
  ps.sampleRate = 44100.0;
  ps.blockSize = 512;
  ps.numChannels = 2;
	
	clearPrepareSpecs(ps);
	
	return ps.blockSize;
}

