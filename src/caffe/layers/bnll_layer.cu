#include <algorithm>
#include <vector>

#include "caffe/layer.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
__global__ void BNLLForward(const int n, const Dtype* in, Dtype* out) {
  CUDA_KERNEL_LOOP(index, n) {
    Mtype in_index = in[index];
    out[index] = in_index > 0 ?
        in_index + log(1. + exp(-in_index)) :
        log(1. + exp(in_index)) ;
  }
}

template <typename Dtype, typename Mtype>
void BNLLLayer<Dtype,Mtype>::Forward_gpu(const vector<BlobBase*>& bottom,
    const vector<BlobBase*>& top) {
  const Dtype* bottom_data = bottom[0]->gpu_data_base<Dtype>();
  Dtype* top_data = top[0]->mutable_gpu_data_base<Dtype>();
  const int count = bottom[0]->count();
  // NOLINT_NEXT_LINE(whitespace/operators)
  BNLLForward<Dtype,Mtype><<<CAFFE_GET_BLOCKS(count), CAFFE_CUDA_NUM_THREADS>>>(
      count, bottom_data, top_data);
  CUDA_POST_KERNEL_CHECK;
}

template <typename Dtype, typename Mtype>
__global__ void BNLLBackward(const int n, const Dtype* in_diff,
    const Dtype* in_data, Dtype* out_diff) {
  CUDA_KERNEL_LOOP(index, n) {
    Mtype expval(exp(min(in_data[index], Mtype( 50. ))));
    out_diff[index] = in_diff[index] * expval / (expval + 1.) ;
  }
}

template <typename Dtype, typename Mtype>
void BNLLLayer<Dtype,Mtype>::Backward_gpu(const vector<BlobBase*>& top,
    const vector<bool>& propagate_down,
    const vector<BlobBase*>& bottom) {
  if (propagate_down[0]) {
    const Dtype* bottom_data = bottom[0]->gpu_data_base<Dtype>();
    const Dtype* top_diff = top[0]->gpu_diff_base<Dtype>();
    Dtype* bottom_diff = bottom[0]->mutable_gpu_diff_base<Dtype>();
    const int count = bottom[0]->count();
    // NOLINT_NEXT_LINE(whitespace/operators)
    BNLLBackward<Dtype,Mtype><<<CAFFE_GET_BLOCKS(count), CAFFE_CUDA_NUM_THREADS>>>(
        count, top_diff, bottom_data, bottom_diff);
    CUDA_POST_KERNEL_CHECK;
  }
}

INSTANTIATE_LAYER_GPU_FUNCS(BNLLLayer);


}  // namespace caffe
