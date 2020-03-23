/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "dsp/delay_funk"
END_TEST_DATA
*/

using DelayBuffer = span<float, 32768>;
using WrappedIndex = DelayBuffer::wrapped;
static const int NumChannels = 1;

span<DelayBuffer, NumChannels> buffer;
WrappedIndex readBuffer;
WrappedIndex writeBuffer;

int main(int input)
{
    
    
	auto& l = buffer[0];
	
	l[1] = 5.0f;
	
	return buffer[0][1];
}

