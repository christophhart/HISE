struct Box
{
    int value = 0;
    void set(int v) { value = v; }
    void relay(int v) { set(v); }
};
Box box;
int test(int input)
{
    box.relay(input);
    return box.value;
}
