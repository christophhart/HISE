#ifndef LAYER_H_INCLUDED
#define LAYER_H_INCLUDED

#include <cstddef>
#include <string>

namespace RTNEURAL_NAMESPACE
{

/** Virtual base class for a generic neural network layer. */
template <typename T>
class Layer
{
public:
    /** Constructs a layer with given input and output size. */
    Layer(int in_size, int out_size)
        : in_size(in_size)
        , out_size(out_size)
    {
    }

    virtual ~Layer() = default;

    /** Returns the name of this layer. */
    virtual std::string getName() const noexcept { return ""; }

    /** Resets the state of this layer. */
    virtual void reset() { }

    /** Implements the forward propagation step for this layer. */
    virtual void forward(const T* input, T* out) noexcept = 0;

    const int in_size;
    const int out_size;
};

} // namespace RTNEURAL_NAMESPACE

#endif // LAYER_H_INCLUDED
