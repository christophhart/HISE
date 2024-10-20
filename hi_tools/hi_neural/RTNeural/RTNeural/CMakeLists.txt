add_library(RTNeural STATIC
    activation/activation.h
    activation/activation_eigen.h
    activation/activation_xsimd.h
    Model.h
    Layer.h
    conv1d/conv1d.h
    conv1d/conv1d.tpp
    conv1d_stateless/conv1d_stateless.h
    conv1d_stateless/conv1d_stateless.tpp
    conv1d_stateless/conv1d_stateless_eigen.h
    conv1d_stateless/conv1d_stateless_eigen.h
    conv2d/conv2d.h
    conv2d/conv2d.tpp
    conv2d/conv2d_eigen.h
    conv2d/conv2d_eigen.tpp
    dense/dense.h
    dense/dense_eigen.h
    dense/dense_xsimd.h
    gru/gru.h
    gru/gru.tpp
    gru/gru_eigen.h
    gru/gru_eigen.tpp
    gru/gru_xsimd.h
    gru/gru_xsimd.tpp
    lstm/lstm.h
    lstm/lstm.tpp
    lstm/lstm_eigen.h
    lstm/lstm_eigen.tpp
    lstm/lstm_xsimd.h
    lstm/lstm_xsimd.tpp
    batchnorm/batchnorm2d.h
    batchnorm/batchnorm2d.tpp
    batchnorm/batchnorm2d_eigen.h
    batchnorm/batchnorm2d_eigen.tpp
    model_loader.h
    RTNeural.h
    RTNeural.cpp
)

set_property(TARGET RTNeural PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(RTNeural PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(RTNeural
    PUBLIC
        ../modules/json
    INTERFACE
        ..
)
set(RTNEURAL_NAMESPACE "RTNeural" CACHE STRING "Namespace to use for RTNeural code")
target_compile_definitions(RTNeural
    PUBLIC
        RTNEURAL_NAMESPACE=${RTNEURAL_NAMESPACE}
)

if(RTNEURAL_ENABLE_RADSAN)
    rtneural_radsan_configure(RTNeural)
endif()
