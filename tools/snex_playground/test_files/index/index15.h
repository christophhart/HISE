/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 0.5
  output: 32
  error: ""
  filename: "index/index15"
END_TEST_DATA
*/



index::lerp<index::normalised<float, index::wrapped<64, true>>> i;
span<float, 64> data = { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f, 17.f, 18.f, 19.f, 20.f, 21.f, 22.f, 23.f, 24.f, 25.f, 26.f, 27.f, 28.f, 29.f, 30.f, 31.f, 32.f, 33.f, 34.f, 35.f, 36.f, 37.f, 38.f, 39.f, 40.f, 41.f, 42.f, 43.f, 44.f, 45.f, 46.f, 47.f, 48.f, 49.f, 50.f, 51.f, 52.f, 53.f, 54.f, 55.f, 56.f, 57.f, 58.f, 59.f, 60.f, 61.f, 62.f, 63.f };
float main(float input)
{
	i = input;
	return data[i];
}