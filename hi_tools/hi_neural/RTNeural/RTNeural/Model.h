#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <vector>

#include "Layer.h"
#include "activation/activation.h"
#include "batchnorm/batchnorm.h"
#include "batchnorm/batchnorm.tpp"
#include "batchnorm/batchnorm2d.h"
#include "batchnorm/batchnorm2d.tpp"
#include "config.h"
#include "conv1d/conv1d.h"
#include "conv1d/conv1d.tpp"
#include "conv2d/conv2d.h"
#include "conv2d/conv2d.tpp"
#include "dense/dense.h"
#include "gru/gru.h"
#include "gru/gru.tpp"
#include "lstm/lstm.h"
#include "lstm/lstm.tpp"

namespace RTNEURAL_NAMESPACE
{

/**
 *  A dynamic sequential neural network model.
 *
 *  Instances of this class should typically be created
 *  `json_parser::parseJson`.
 */
template <typename T>
class Model
{
public:
    /** Constructs a sequential model for a given input size. */
    explicit Model(int in_size)
        : in_size(in_size)
    {
    }

    /** Destructor. */
    ~Model()
    {
        for(auto l : layers)
            delete l;
        layers.clear();

        outs.clear();
    }

    /** Returns the model's input size */
    int getInSize() const { return layers.front()->in_size; }

    /** Returns the model's output size */
    int getOutSize() const { return layers.back()->out_size; }

    /** Returns the required input size for the next layer being added to the network. */
    int getNextInSize() const
    {
        if(layers.empty())
            return in_size;

        return layers.back()->out_size;
    }

    /** Adds a new layer to the sequential model. */
    void addLayer(Layer<T>* layer)
    {
        layers.push_back(layer);
        outs.push_back(vec_type(layer->out_size, (T)0));
    }

    /** Resets the state of the network layers. */
    RTNEURAL_REALTIME void reset()
    {
        for(auto* l : layers)
            l->reset();
    }

    /** Performs forward propagation for this model. */
    RTNEURAL_REALTIME inline T forward(const T* input)
    {
        layers[0]->forward(input, outs[0].data());

        for(int i = 1; i < (int)layers.size(); ++i)
        {
            layers[i]->forward(outs[i - 1].data(), outs[i].data());
        }

        return outs.back()[0];
    }

    /** Returns a pointer to the output of the final layer in the network. */
    RTNEURAL_REALTIME inline const T* getOutputs() const noexcept
    {
        return outs.back().data();
    }

    /** A vector storing the network layers in sequential order. */
    std::vector<Layer<T>*> layers;

private:
#if RTNEURAL_USE_XSIMD
    using vec_type = std::vector<T, xsimd::aligned_allocator<T>>;
#elif RTNEURAL_USE_EIGEN
    using vec_type = std::vector<T, Eigen::aligned_allocator<T>>;
#else
    using vec_type = std::vector<T>;
#endif

    const int in_size;
    std::vector<vec_type> outs;
};

} // namespace RTNEURAL_NAMESPACE

#endif // MODEL_H_INCLUDED
