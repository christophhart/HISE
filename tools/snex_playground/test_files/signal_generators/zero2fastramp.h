/* Tests the block iterator and setting values to zero.

BEGIN_TEST_DATA
  f: main
  ret: int
  args: ProcessData<1>
  input: "zero.wav"
  output: "fastramp.wav"
  error: ""
  filename: "zero2fastramp"
END_TEST_DATA
*/

int main(ProcessData<1>& data)
{
    float v = 0.0f;
    float delta = 16.0f / 1024.0f;

    for(auto& s: data[0])
    {
        s = Math.fmod(v, 1.0f);
        v += delta;
    }
    
    return 0;
}

