#include <vector>

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/filler.hpp"
#include "caffe/layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
void InnerProductLayer<Dtype,Mtype>::LayerSetUp(const vector<BlobBase*>& bottom,
      const vector<BlobBase*>& top) {
  const int num_output = this->layer_param_.inner_product_param().num_output();
  bias_term_ = this->layer_param_.inner_product_param().bias_term();
  N_ = num_output;
  const int axis = bottom[0]->CanonicalAxisIndex(
      this->layer_param_.inner_product_param().axis());
  // Dimensions starting from "axis" are "flattened" into a single
  // length K_ vector. For example, if bottom[0]'s shape is (N, C, H, W),
  // and axis == 1, N inner products with dimension CHW are performed.
  K_ = bottom[0]->count(axis);
  // Check if we need to set up the weights
  if (this->blobs_.size() > 0) {
    LOG(INFO) << "Skipping parameter initialization";
  } else {
    if (bias_term_) {
      this->blobs_.resize(2);
    } else {
      this->blobs_.resize(1);
    }
    // Intialize the weight
    vector<int> weight_shape(2);
    weight_shape[0] = N_;
    weight_shape[1] = K_;
    this->blobs_[0].reset(new Blob<Dtype>(weight_shape));
    // fill the weights
    shared_ptr<Filler<Dtype,Mtype> > weight_filler(GetFiller<Dtype,Mtype>(
        this->layer_param_.inner_product_param().weight_filler()));
    weight_filler->Fill(this->blobs_[0].get());
    // If necessary, intiialize and fill the bias term
    if (bias_term_) {
      vector<int> bias_shape(1, N_);
      this->blobs_[1].reset(new Blob<Dtype>(bias_shape));
      shared_ptr<Filler<Dtype,Mtype> > bias_filler(GetFiller<Dtype,Mtype>(
          this->layer_param_.inner_product_param().bias_filler()));
      bias_filler->Fill(this->blobs_[1].get());
    }
  }  // parameter initialization
  this->param_propagate_down_.resize(this->blobs_.size(), true);
}

template <typename Dtype, typename Mtype>
void InnerProductLayer<Dtype,Mtype>::Reshape(const vector<BlobBase*>& bottom,
      const vector<BlobBase*>& top) {
  // Figure out the dimensions
  const int axis = bottom[0]->CanonicalAxisIndex(
      this->layer_param_.inner_product_param().axis());
  const int new_K = bottom[0]->count(axis);
  CHECK_EQ(K_, new_K)
      << "Input size incompatible with inner product parameters.";
  // The first "axis" dimensions are independent inner products; the total
  // number of these is M_, the product over these dimensions.
  M_ = bottom[0]->count(0, axis);
  // The top shape will be the bottom shape with the flattened axes dropped,
  // and replaced by a single axis with dimension num_output (N_).
  vector<int> top_shape = bottom[0]->shape();
  top_shape.resize(axis + 1);
  top_shape[axis] = N_;
  top[0]->Reshape(top_shape);
  // Set up the bias multiplier
  if (bias_term_) {
    vector<int> bias_shape(1, M_);
    bias_multiplier_.Reshape(bias_shape);
    caffe_set(M_, typedConsts<Dtype>::one, bias_multiplier_.mutable_cpu_data());
  }
}

template <typename Dtype, typename Mtype>
void InnerProductLayer<Dtype,Mtype>::Forward_cpu(const vector<BlobBase*>& bottom,
    const vector<BlobBase*>& top) {
  const Dtype* bottom_data = bottom[0]->cpu_data_base<Dtype>();
  Dtype* top_data = top[0]->mutable_cpu_data_base<Dtype>();
  const Dtype* weight = this->blobs_[0]->template cpu_data_base<Dtype>();
  caffe_cpu_gemm<Dtype,Mtype>(CblasNoTrans, CblasTrans, M_, N_, K_, (Mtype)1.,
      bottom_data, weight, (Mtype)0., top_data);
  if (bias_term_) {
    caffe_cpu_gemm<Dtype,Mtype>(CblasNoTrans, CblasNoTrans, M_, N_, 1, (Mtype)1.,
        bias_multiplier_.cpu_data(),
        this->blobs_[1]->template cpu_data_base<Dtype>(), (Mtype)1., top_data);
  }
}

template <typename Dtype, typename Mtype>
void InnerProductLayer<Dtype,Mtype>::Backward_cpu(const vector<BlobBase*>& top,
    const vector<bool>& propagate_down,
    const vector<BlobBase*>& bottom) {
  if (this->param_propagate_down_[0]) {
    const Dtype* top_diff = top[0]->cpu_diff_base<Dtype>();
    const Dtype* bottom_data = bottom[0]->cpu_data_base<Dtype>();
    // Gradient with respect to weight
    caffe_cpu_gemm<Dtype,Mtype>(CblasTrans, CblasNoTrans, N_, K_, M_, (Mtype)1.,
        top_diff, bottom_data, (Mtype)1., this->blobs_[0]->template mutable_cpu_diff_base<Dtype>());
  }
  if (bias_term_ && this->param_propagate_down_[1]) {
    const Dtype* top_diff = top[0]->cpu_diff_base<Dtype>();
    // Gradient with respect to bias
    caffe_cpu_gemv<Dtype,Mtype>(CblasTrans, M_, N_, (Mtype)1., top_diff,
        bias_multiplier_.cpu_data(), (Mtype)1.,
        this->blobs_[1]->template mutable_cpu_diff_base<Dtype>());
  }
  if (propagate_down[0]) {
    const Dtype* top_diff = top[0]->cpu_diff_base<Dtype>();
    // Gradient with respect to bottom data
    caffe_cpu_gemm<Dtype,Mtype>(CblasNoTrans, CblasNoTrans, M_, K_, N_, (Mtype)1.,
        top_diff, this->blobs_[0]->template cpu_data_base<Dtype>(), (Mtype)0.,
        bottom[0]->mutable_cpu_diff_base<Dtype>());
  }
}

#ifdef CPU_ONLY
STUB_GPU(InnerProductLayer);
#endif

INSTANTIATE_CLASS(InnerProductLayer);
REGISTER_LAYER_CLASS(InnerProduct);

}  // namespace caffe
