#ifndef ACTIVATIONXSIMD_H_INCLUDED
#define ACTIVATIONXSIMD_H_INCLUDED

#include "../common.h"
#include "../config.h"
#include "../maths/maths_xsimd.h"

namespace RTNEURAL_NAMESPACE
{

/** Dynamic implementation of a tanh activation layer. */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class TanhActivation : public Activation<T>
{
public:
    /** Constructs a tanh activation layer for a given size. */
    explicit TanhActivation(int size)
        : Activation<T>(size, {}, "tanh")
    {
    }

    TanhActivation(std::initializer_list<int> sizes)
        : TanhActivation(*sizes.begin())
    {
    }

    /** Performs forward propagation for tanh activation. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        tanh<T, MathsProvider>(input, out, Layer<T>::in_size);
    }
};

/** Static implementation of a tanh activation layer. */
template <typename T, int size, typename MathsProvider = DefaultMathsProvider>
class TanhActivationT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_io_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    TanhActivationT()
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = v_type((T)0);
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "tanh"; }

    /** Returns true since this layer is an activation layer. */
    constexpr bool isActivation() const noexcept { return true; }

    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for tanh activation. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = MathsProvider::tanh(ins[i]);
    }

    v_type outs[v_io_size];
};

/** Dynamic implementation of a ReLU activation layer. */
template <typename T>
class ReLuActivation : public Activation<T>
{
public:
    /** Constructs a ReLU activation layer for a given size. */
    explicit ReLuActivation(int size)
        : Activation<T>(size, {}, "relu")
    {
        zeros.resize(size, (T)0);
    }

    ReLuActivation(std::initializer_list<int> sizes)
        : ReLuActivation(*sizes.begin())
    {
    }

    /** Performs forward propagation for ReLU activation. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        xsimd::transform(
            input, &input[Layer<T>::in_size], zeros.begin(), out,
            [](auto const& a, auto const& b)
            { return xsimd::max(a, b); });
    }

    std::vector<T, xsimd::aligned_allocator<T>> zeros;
};

/** Static implementation of a ReLU activation layer. */
template <typename T, int size>
class ReLuActivationT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_io_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    ReLuActivationT()
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = v_type((T)0);
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "relu"; }

    /** Returns true since this layer is an activation layer. */
    constexpr bool isActivation() const noexcept { return true; }

    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for ReLU activation. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = xsimd::max(ins[i], v_type((T)0));
    }

    v_type outs[v_io_size];
};

/** Dynamic implementation of a sigmoid activation layer. */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class SigmoidActivation : public Activation<T>
{
public:
    /** Constructs a sigmoid activation layer for a given size. */
    explicit SigmoidActivation(int size)
        : Activation<T>(size, {}, "sigmoid")
    {
    }

    SigmoidActivation(std::initializer_list<int> sizes)
        : SigmoidActivation(*sizes.begin())
    {
    }

    /** Performs forward propagation for sigmoid activation. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        sigmoid<T, MathsProvider>(input, out, Layer<T>::in_size);
    }
};

/** Static implementation of a sigmoid activation layer. */
template <typename T, int size, typename MathsProvider = DefaultMathsProvider>
class SigmoidActivationT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_io_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    SigmoidActivationT()
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = v_type((T)0);
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "sigmoid"; }

    /** Returns true since this layer is an activation layer. */
    constexpr bool isActivation() const noexcept { return true; }

    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for sigmoid activation. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = MathsProvider::sigmoid(ins[i]);
    }

    v_type outs[v_io_size];
};

/** Dynamic implementation of a softmax activation layer. */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class SoftmaxActivation : public Activation<T>
{
public:
    /** Constructs a softmax activation layer for a given size. */
    explicit SoftmaxActivation(int size)
        : Activation<T>(size, {}, "softmax")
    {
    }

    SoftmaxActivation(std::initializer_list<int> sizes)
        : SoftmaxActivation(*sizes.begin())
    {
    }

    /** Performs forward propagation for softmax activation. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        softmax<T, MathsProvider>(input, out, Layer<T>::in_size);
    }
};

/** Static implementation of a softmax activation layer. */
template <typename T, int size, typename MathsProvider = DefaultMathsProvider>
class SoftmaxActivationT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_io_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    SoftmaxActivationT()
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = v_type((T)0);
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "softmax"; }

    /** Returns true since this layer is an activation layer. */
    constexpr bool isActivation() const noexcept { return true; }

    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for softmax activation. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_io_size]) noexcept
    {
        v_type exp_sum {};
        for(int i = 0; i < v_io_size; ++i)
        {
            outs[i] = MathsProvider::exp(ins[i]);
            exp_sum += outs[i];
        }

        const auto exp_sum_recip = v_type((T)1 / xsimd::reduce_add(exp_sum));
        for(int i = 0; i < v_io_size; ++i)
            outs[i] *= exp_sum_recip;
    }

    v_type outs[v_io_size];
};

/** Dynamic implementation of a elu activation layer. */
template <typename T, typename MathsProvider = DefaultMathsProvider>
class ELuActivation final : public Activation<T>
{
public:
    /** Constructs a elu activation layer for a given size. */
    explicit ELuActivation(int size)
        : Activation<T>(size, {}, "elu")
    {
    }

    ELuActivation(std::initializer_list<int> sizes)
        : ELuActivation(*sizes.begin())
    {
    }

    /** Performs forward propagation for softmax activation. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        elu<T, MathsProvider>(input, out, Layer<T>::in_size, alpha);
    }

    /** Sets a custom value for the layer's "alpha" parameter. */
    RTNEURAL_REALTIME void set_alpha(T newAlpha) { alpha = newAlpha; }

private:
    T alpha = (T)1;
};

/** Static implementation of a elu activation layer. */
template <typename T, int size, int AlphaNumerator = 1, int AlphaDenominator = 1, typename MathsProvider = DefaultMathsProvider>
class ELuActivationT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_io_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    ELuActivationT() = default;

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "elu"; }

    /** Returns true since this layer is an activation layer. */
    constexpr bool isActivation() const noexcept { return true; }

    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for elu activation. */
    template <int A_N = AlphaNumerator, int A_D = AlphaDenominator>
    RTNEURAL_REALTIME inline typename std::enable_if<A_N == 1 && A_D == 1, void>::type
    forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = xsimd::select(ins[i] > (T)0, ins[i], MathsProvider::exp(ins[i]) - (T)1);
    }

    /** Performs forward propagation for elu activation (with custom alpha parameter). */
    template <int A_N = AlphaNumerator, int A_D = AlphaDenominator>
    RTNEURAL_REALTIME inline typename std::enable_if<A_N != 1 || A_D != 1, void>::type
    forward(const v_type (&ins)[v_io_size]) noexcept
    {
        static constexpr T alpha = (T)AlphaNumerator / (T)AlphaDenominator;
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = xsimd::select(ins[i] > (T)0, ins[i], alpha * (MathsProvider::exp(ins[i]) - (T)1));
    }

    v_type outs[v_io_size];
};

/** Dynamic implementation of a PReLU activation layer. */
template <typename T>
class PReLUActivation final : public Activation<T>
{
public:
    explicit PReLUActivation(int size)
        : Activation<T>(size, {}, "prelu")
        , alpha(size, {})
    {
    }

    /** Performs forward propagation for prelu activation. */
    RTNEURAL_REALTIME inline void forward(const T* input, T* out) noexcept override
    {
        using b_type = xsimd::simd_type<T>;
        constexpr auto inc = (int)b_type::size;

        // size for which the vectorization is possible
        auto vec_size = Layer<T>::in_size - Layer<T>::in_size % inc;
        for(int i = 0; i < vec_size; i += inc)
        {
            b_type x_vec = xsimd::load_aligned(&input[i]);
            b_type a_vec = xsimd::load_aligned(&alpha[i]);
            b_type y_vec = xsimd::select(x_vec >= (T)0, x_vec, x_vec * a_vec);
            xsimd::store_aligned(&out[i], y_vec);
        }

        // Remaining part that cannot be vectorized
        for(auto i = vec_size; i < Layer<T>::in_size; ++i)
            out[i] = input[i] >= (T)0 ? input[i] : (input[i] * alpha[i]);
    }

    RTNEURAL_REALTIME void setAlphaVals(const std::vector<T>& alphaVals)
    {
        if(alphaVals.size() == 1)
        {
            std::fill(alpha.begin(), alpha.end(), alphaVals[0]);
        }
        else
        {
            std::copy(alphaVals.begin(), alphaVals.end(), alpha.begin());
        }
    }

    std::vector<T, xsimd::aligned_allocator<T>> alpha;
};

/** Static implementation of a PReLU activation layer. */
template <typename T, int size>
class PReLUActivationT
{
    using v_type = xsimd::simd_type<T>;
    static constexpr auto v_size = (int)v_type::size;
    static constexpr auto v_io_size = ceil_div(size, v_size);

public:
    static constexpr auto in_size = size;
    static constexpr auto out_size = size;

    PReLUActivationT()
    {
        for(int i = 0; i < v_io_size; ++i)
        {
            outs[i] = v_type((T)0);
            alpha[i] = v_type((T)0);
        }
    }

    /** Returns the name of this layer. */
    std::string getName() const noexcept { return "prelu"; }

    /** Returns false since this layer has weights even though it is an activation layer. */
    constexpr bool isActivation() const noexcept { return false; }

    RTNEURAL_REALTIME void reset() { }

    /** Performs forward propagation for prelu activation. */
    RTNEURAL_REALTIME inline void forward(const v_type (&ins)[v_io_size]) noexcept
    {
        for(int i = 0; i < v_io_size; ++i)
            outs[i] = xsimd::select(ins[i] >= (T)0, ins[i], ins[i] * alpha[i]);
    }

    RTNEURAL_REALTIME void setAlphaVals(const std::vector<T>& alphaVals)
    {
        if(alphaVals.size() == 1)
        {
            for(int i = 0; i < v_io_size; ++i)
                alpha[i] = alphaVals[0];
        }
        else
        {
            for(int i = 0; i < out_size; ++i)
                alpha[i / v_size] = set_value(alpha[i / v_size], i % v_size, alphaVals[i]);
        }
    }

    v_type outs[v_io_size];
    v_type alpha[v_io_size];
};
} // namespace RTNEURAL_NAMESPACE

#endif // ACTIVATIONXSIMD_H_INCLUDED
