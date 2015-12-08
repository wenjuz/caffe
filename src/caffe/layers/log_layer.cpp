#include <algorithm>
#include <vector>

#include "caffe/layer.hpp"
#include "caffe/neuron_layers.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
void LogLayer<Dtype,Mtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  NeuronLayer<Dtype,Mtype>::LayerSetUp(bottom, top);
  const Dtype base = this->layer_param_.log_param().base();
  if (base != -1) {
    CHECK_GT(base, 0.) << "base must be strictly positive.";
  }
  // If base == -1, interpret the base as e and set log_base = 1 exactly.
  // Otherwise, calculate its log explicitly.
  const Dtype log_base = (base == -1) ? typedConsts<Dtype>::one : log(base);
  CHECK(!isnan((double)log_base))
      << "NaN result: log(base) = log(" << base << ") = " << log_base;
  CHECK(!isinf((double)log_base))
      << "Inf result: log(base) = log(" << base << ") = " << log_base;
  base_scale_ = 1 / log_base;
  CHECK(!isnan((double)base_scale_))
      << "NaN result: 1/log(base) = 1/log(" << base << ") = " << base_scale_;
  CHECK(!isinf((double)base_scale_))
      << "Inf result: 1/log(base) = 1/log(" << base << ") = " << base_scale_;
  input_scale_ = this->layer_param_.log_param().scale();
  input_shift_ = this->layer_param_.log_param().shift();
  backward_num_scale_ = input_scale_ / log_base;
}

template <typename Dtype, typename Mtype>
void LogLayer<Dtype,Mtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
  const int count = bottom[0]->count();
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* top_data = top[0]->mutable_cpu_data();
  if (input_scale_ == 1 && input_shift_ == 0) {
    caffe_log<Dtype>(count, bottom_data, top_data);
  } else {
    caffe_copy(count, bottom_data, top_data);
    if (input_scale_ != 1) {
      caffe_scal<Dtype,Mtype>(count, input_scale_, top_data);
    }
    if (input_shift_ != 0) {
      caffe_add_scalar<Dtype,Mtype>(count, input_shift_, top_data);
    }
    caffe_log<Dtype>(count, top_data, top_data);
  }
  if (base_scale_ != 1) {
    caffe_scal<Dtype,Mtype>(count, base_scale_, top_data);
  }
}

template <typename Dtype, typename Mtype>
void LogLayer<Dtype,Mtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {
  if (!propagate_down[0]) { return; }
  const int count = bottom[0]->count();
  const Dtype* bottom_data = bottom[0]->cpu_data();
  const Dtype* top_diff = top[0]->cpu_diff();
  Dtype* bottom_diff = bottom[0]->mutable_cpu_diff();
  caffe_copy(count, bottom_data, bottom_diff);
  if (input_scale_ != 1) {
    caffe_scal<Dtype,Mtype>(count, input_scale_, bottom_diff);
  }
  if (input_shift_ != 0) {
    caffe_add_scalar<Dtype,Mtype>(count, input_shift_, bottom_diff);
  }
  caffe_powx<Dtype,Mtype>(count, bottom_diff, -1, bottom_diff);
  if (backward_num_scale_ != 1) {
    caffe_scal<Dtype,Mtype>(count, backward_num_scale_, bottom_diff);
  }
  caffe_mul(count, top_diff, bottom_diff, bottom_diff);
}

#ifdef CPU_ONLY
STUB_GPU(LogLayer);
#endif

INSTANTIATE_CLASS(LogLayer);
REGISTER_LAYER_CLASS(Log);

}  // namespace caffe
