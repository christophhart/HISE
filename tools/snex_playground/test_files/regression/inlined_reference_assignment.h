// Run with --args:4 --O:Inlining --backend:interpreter.
// Expected result: 4. The inlined getter must return a writable reference.
struct Box
{
	int value = 0;
	int& get() { return value; }
};

Box box;

int test(int input)
{
	box.get() = input;
	return box.value;
}
