#include <vector>

#include "caffe/layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
void SplitLayer<Dtype,Mtype>::Reshape(const vector<BlobBase*>& bottom,
      const vector<BlobBase*>& top) {
  count_ = bottom[0]->count();
  for (int i = 0; i < top.size(); ++i) {
    // Do not allow in-place computation in the SplitLayer.  Instead, share data
    // by reference in the forward pass, and keep separate diff allocations in
    // the backward pass.  (Technically, it should be possible to share the diff
    // blob of the first split output with the input, but this seems to cause
    // some strange effects in practice...)
    CHECK_NE(top[i], bottom[0]) << this->type() << " Layer does not "
        "allow in-place computation.";
    top[i]->ReshapeLike(*bottom[0]);
    CHECK_EQ(count_, top[i]->count());
  }
}

template <typename Dtype, typename Mtype>
void SplitLayer<Dtype,Mtype>::Forward_cpu(const vector<BlobBase*>& bottom,
      const vector<BlobBase*>& top) {
  for (int i = 0; i < top.size(); ++i) {
    top[i]->ShareData(*bottom[0]);
  }
}

template <typename Dtype, typename Mtype>
void SplitLayer<Dtype,Mtype>::Backward_cpu(const vector<BlobBase*>& top,
      const vector<bool>& propagate_down, const vector<BlobBase*>& bottom) {
  if (!propagate_down[0]) { return; }
  if (top.size() == 1) {
    caffe_copy(count_, top[0]->cpu_diff<Dtype>(),
        bottom[0]->mutable_cpu_diff<Dtype>());
    return;
  }
  caffe_add(count_, top[0]->cpu_diff<Dtype>(), top[1]->cpu_diff<Dtype>(),
            bottom[0]->mutable_cpu_diff<Dtype>());
  // Add remaining top blob diffs.
  for (int i = 2; i < top.size(); ++i) {
    const Dtype* top_diff = top[i]->cpu_diff<Dtype>();
    Dtype* bottom_diff = bottom[0]->mutable_cpu_diff<Dtype>();
    caffe_axpy<Dtype,Mtype>(count_, Mtype(1.), top_diff, bottom_diff);
  }
}


#ifdef CPU_ONLY
STUB_GPU(SplitLayer);
#endif

INSTANTIATE_CLASS(SplitLayer);
REGISTER_LAYER_CLASS(Split);

}  // namespace caffe
