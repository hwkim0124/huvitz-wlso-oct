#include "pch.h"
#include "BscanSegmentator.h"
#include "LayeredBscan.h"
#include "Sampling.h"
#include "Bordering.h"
#include "Measuring.h"
#include "SemtSegm2.h"

using namespace semt_segm;
using namespace std;


struct BscanSegmentator::BscanSegmentatorImpl
{
	const LayeredBscan* bscan = nullptr;

	unique_ptr<Sampling> sampling;
	unique_ptr<Bordering> bordering;
	unique_ptr<Classifier> classifier;
	unique_ptr<Measuring> measuring;

	unique_ptr<LayerInner> layerInn;
	unique_ptr<LayerOuter> layerOut;

	unique_ptr<LayerILM> layerILM;
	unique_ptr<LayerNFL> layerNFL;
	unique_ptr<LayerIPL> layerIPL;
	unique_ptr<LayerOPL> layerOPL;
	unique_ptr<LayerONL> layerONL;
	unique_ptr<LayerIOS> layerIOS;
	unique_ptr<LayerRPE> layerRPE;
	unique_ptr<LayerBRM> layerBRM;
	unique_ptr<LayerOPR> layerOPR;

	BscanSegmentatorImpl()
	{
	}
};


BscanSegmentator::BscanSegmentator(LayeredBscan* bscan) :
	d_ptr(make_unique<BscanSegmentatorImpl>())
{
	getImpl().bscan = bscan;
}


semt_segm::BscanSegmentator::~BscanSegmentator() = default;
semt_segm::BscanSegmentator::BscanSegmentator(BscanSegmentator && rhs) = default;
BscanSegmentator & semt_segm::BscanSegmentator::operator=(BscanSegmentator && rhs) = default;


const LayeredBscan * semt_segm::BscanSegmentator::getLayeredBscan() const
{
	return getImpl().bscan;
}

const OctScanImageDescript& semt_segm::BscanSegmentator::getImageSource() const 
{
	return getImpl().bscan->getImageSource();
}

Sampling * semt_segm::BscanSegmentator::getSampling() const
{
	return getImpl().sampling.get();
}

Bordering * semt_segm::BscanSegmentator::getBodering() const
{
	return getImpl().bordering.get();
}

Classifier * semt_segm::BscanSegmentator::getClassifier() const
{
	return getImpl().classifier.get();
}

Measuring * semt_segm::BscanSegmentator::getMeasuring() const
{
	return getImpl().measuring.get();
}

LayerInner * semt_segm::BscanSegmentator::getLayerInner() const
{
	return getImpl().layerInn.get();
}

LayerOuter * semt_segm::BscanSegmentator::getLayerOuter() const
{
	return getImpl().layerOut.get();
}

LayerILM * semt_segm::BscanSegmentator::getLayerILM() const
{
	return getImpl().layerILM.get();
}

LayerNFL * semt_segm::BscanSegmentator::getLayerNFL() const
{
	return getImpl().layerNFL.get();
}

LayerIPL * semt_segm::BscanSegmentator::getLayerIPL() const
{
	return  getImpl().layerIPL.get();
}

LayerOPL * semt_segm::BscanSegmentator::getLayerOPL() const
{
	return getImpl().layerOPL.get();
}

LayerONL * semt_segm::BscanSegmentator::getLayerONL() const
{
	return getImpl().layerONL.get();
}

LayerIOS * semt_segm::BscanSegmentator::getLayerIOS() const
{
	return getImpl().layerIOS.get();
}

LayerRPE * semt_segm::BscanSegmentator::getLayerRPE() const
{
	return getImpl().layerRPE.get();
}

LayerBRM * semt_segm::BscanSegmentator::getLayerBRM() const
{
	return getImpl().layerBRM.get();
}

LayerOPR * semt_segm::BscanSegmentator::getLayerOPR() const
{
	return getImpl().layerOPR.get();
}

int semt_segm::BscanSegmentator::getImageIndex() const
{
	return getImpl().bscan->getImageIndex();
}

float semt_segm::BscanSegmentator::getImageRangeX() const
{
	return getImpl().bscan->getImageRangeX();
}

void semt_segm::BscanSegmentator::resetAlgorithms()
{
	getImpl().sampling = move(unique_ptr<Sampling>(createSampling()));
	getImpl().bordering = move(unique_ptr<Bordering>(createBordering()));
	getImpl().classifier = move(unique_ptr<Classifier>(createClassifier()));
	getImpl().measuring = move(unique_ptr<Measuring>(createMeasuring()));

	getImpl().layerInn = move(unique_ptr<LayerInner>(createLayerInner()));
	getImpl().layerOut = move(unique_ptr<LayerOuter>(createLayerOuter()));

	getImpl().layerILM = move(unique_ptr<LayerILM>(createLayerILM()));
	getImpl().layerNFL = move(unique_ptr<LayerNFL>(createLayerNFL()));
	getImpl().layerIPL = move(unique_ptr<LayerIPL>(createLayerIPL()));
	getImpl().layerOPL = move(unique_ptr<LayerOPL>(createLayerOPL()));
	getImpl().layerONL = move(unique_ptr<LayerONL>(createLayerONL()));
	getImpl().layerIOS = move(unique_ptr<LayerIOS>(createLayerIOS()));
	getImpl().layerRPE = move(unique_ptr<LayerRPE>(createLayerRPE()));
	getImpl().layerBRM = move(unique_ptr<LayerBRM>(createLayerBRM()));
	getImpl().layerOPR = move(unique_ptr<LayerOPR>(createLayerOPR()));
}

Sampling * semt_segm::BscanSegmentator::createSampling() 
{
	return new Sampling();
}

Bordering * semt_segm::BscanSegmentator::createBordering() 
{
	return new Bordering(this);
}

Classifier * semt_segm::BscanSegmentator::createClassifier()
{
	return new Classifier(this);
}

Measuring * semt_segm::BscanSegmentator::createMeasuring()
{
	return new Measuring(this);
}

LayerInner * semt_segm::BscanSegmentator::createLayerInner() 
{
	return new LayerInner(this);
}

LayerOuter * semt_segm::BscanSegmentator::createLayerOuter() 
{
	return new LayerOuter(this);
}

LayerILM * semt_segm::BscanSegmentator::createLayerILM()
{
	return new LayerILM(this);
}

LayerNFL * semt_segm::BscanSegmentator::createLayerNFL()
{
	return new LayerNFL(this);
}

LayerIPL * semt_segm::BscanSegmentator::createLayerIPL()
{
	return new LayerIPL(this);
}

LayerOPL * semt_segm::BscanSegmentator::createLayerOPL()
{
	return new LayerOPL(this);
}

LayerONL * semt_segm::BscanSegmentator::createLayerONL()
{
	return new LayerONL(this);
}

LayerIOS * semt_segm::BscanSegmentator::createLayerIOS()
{
	return new LayerIOS(this);
}

LayerRPE * semt_segm::BscanSegmentator::createLayerRPE()
{
	return new LayerRPE(this);
}

LayerBRM * semt_segm::BscanSegmentator::createLayerBRM()
{
	return new LayerBRM(this);
}

LayerOPR * semt_segm::BscanSegmentator::createLayerOPR()
{
	return new LayerOPR(this);
}


BscanSegmentator::BscanSegmentatorImpl & semt_segm::BscanSegmentator::getImpl(void) const
{
	return *d_ptr;
}


