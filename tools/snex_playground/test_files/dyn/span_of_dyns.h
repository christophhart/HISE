/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 48
  error: ""
  filename: "dyn/span_of_dyns"
END_TEST_DATA
*/

span<int, 8> data = { 2 };

span<int, 8> data2 = { 4 };

using StereoChannels = span<dyn<int>, 2>;

StereoChannels d;

int main(int input)
{
    d[0].referTo(data, data.size());
    d[1].referTo(data2, data.size());
	
    for(auto& s: d)
    {
        for(auto& v: s)
            input += v;
    }
    
    return input;
}

