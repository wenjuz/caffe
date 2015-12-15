#ifdef USE_CUDNN
#include <algorithm>
#include <cfloat>
#include <vector>

#include "thrust/device_vector.h"

#include "caffe/layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
void CuDNNSoftmaxLayer<Dtype,Mtype>::Forward_gpu(const vector<BlobBase*>& bottom,
    const vector<BlobBase*>& top) {
    SoftmaxLayer<Dtype,Mtype>::Forward_gpu(bottom, top);
}

template <typename Dtype, typename Mtype>
void CuDNNSoftmaxLayer<Dtype,Mtype>::Backward_gpu(const vector<BlobBase*>& top,
    const vector<bool>& propagate_down, const vector<BlobBase*>& bottom) {
    SoftmaxLayer<Dtype,Mtype>::Backward_gpu(top, propagate_down, bottom);
}

INSTANTIATE_LAYER_GPU_FUNCS(CuDNNSoftmaxLayer);

}  // namespace caffe
#endif
