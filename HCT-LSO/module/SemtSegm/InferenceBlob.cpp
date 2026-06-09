#include "pch.h"
#include "SemtSegm2.h"
#include "InferenceBlob.h"

using namespace semt_segm;
using namespace cv;
using namespace InferenceEngine;

struct InferenceBlob::InferenceBlobImpl
{
	const OcularImage* image = nullptr;

	InferenceBlobImpl()
	{
	}
};

InferenceBlob::InferenceBlob(const OcularImage* image) :
	d_ptr(make_unique<InferenceBlobImpl>())
{
	getImpl().image = image;
}

semt_segm::InferenceBlob::~InferenceBlob() = default;
semt_segm::InferenceBlob::InferenceBlob(InferenceBlob && rhs) = default;
InferenceBlob & semt_segm::InferenceBlob::operator=(InferenceBlob && rhs) = default;

InferenceEngine::Blob::Ptr semt_segm::InferenceBlob::wrapImageToBlobFp32() const
{
	if (!getImpl().image) {
		return nullptr;
	}

	cv::Mat mat = getImpl().image->getCvMatConst().clone();

	size_t channels = mat.channels();
	size_t height = mat.size().height;
	size_t width = mat.size().width;

	size_t strideH = mat.step.buf[0];
	size_t strideW = mat.step.buf[1];
	size_t elemSize = mat.elemSize();
	auto data = reinterpret_cast<float*>(mat.data);

	bool is_dense =
		strideW == channels * elemSize &&
		strideH == channels * width * elemSize;

	if (!is_dense) THROW_IE_EXCEPTION
		<< "Doesn't support conversion from not dense cv::Mat";

	InferenceEngine::TensorDesc tDesc(InferenceEngine::Precision::FP32,
		{ 1, channels, height, width },
		InferenceEngine::Layout::NCHW);

	/** Fill input tensor with planes. First b channel, then g and r channels **/
	// assuming input precision was asked to be U8 in prev step
	auto input = InferenceEngine::make_shared_blob<float>(tDesc, data);
	// input->allocate();

	return input;
}

InferenceBlob::InferenceBlobImpl & semt_segm::InferenceBlob::getImpl(void) const
{
	return *d_ptr;
}


