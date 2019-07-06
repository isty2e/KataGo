/* Data descriptors shared between the backends. Supports I/O to simple text
   format generated by the python training. */

#ifndef DESC_H
#define DESC_H

#include <istream>
#include <string>
#include <vector>

#include "../game/rules.h"

struct ConvLayerDesc {
  std::string name;
  int convYSize;
  int convXSize;
  int inChannels;
  int outChannels;
  int dilationY;
  int dilationX;
  std::vector<float> weights;

  ConvLayerDesc();
  ConvLayerDesc(std::istream& in);
  ConvLayerDesc(ConvLayerDesc&& other);

  ConvLayerDesc(const ConvLayerDesc&) = delete;
  ConvLayerDesc& operator=(const ConvLayerDesc&) = delete;

  ConvLayerDesc& operator=(ConvLayerDesc&& other);
};

struct BatchNormLayerDesc {
  std::string name;
  int numChannels;
  float epsilon;
  bool hasScale;
  bool hasBias;
  std::vector<float> mean;
  std::vector<float> variance;
  std::vector<float> scale;
  std::vector<float> bias;

  BatchNormLayerDesc();
  BatchNormLayerDesc(std::istream& in);
  BatchNormLayerDesc(BatchNormLayerDesc&& other);

  BatchNormLayerDesc(const BatchNormLayerDesc&) = delete;
  BatchNormLayerDesc& operator=(const BatchNormLayerDesc&) = delete;

  BatchNormLayerDesc& operator=(BatchNormLayerDesc&& other);
};

struct ActivationLayerDesc {
  std::string name;

  ActivationLayerDesc();
  ActivationLayerDesc(std::istream& in);
  ActivationLayerDesc(ActivationLayerDesc&& other);

  ActivationLayerDesc(const ActivationLayerDesc&) = delete;
  ActivationLayerDesc& operator=(const ActivationLayerDesc&) = delete;

  ActivationLayerDesc& operator=(ActivationLayerDesc&& other);
};

struct MatMulLayerDesc {
  std::string name;
  int inChannels;
  int outChannels;
  std::vector<float> weights;

  MatMulLayerDesc();
  MatMulLayerDesc(std::istream& in);
  MatMulLayerDesc(MatMulLayerDesc&& other);

  MatMulLayerDesc(const MatMulLayerDesc&) = delete;
  MatMulLayerDesc& operator=(const MatMulLayerDesc&) = delete;

  MatMulLayerDesc& operator=(MatMulLayerDesc&& other);
};

struct MatBiasLayerDesc {
  std::string name;
  int numChannels;
  std::vector<float> weights;

  MatBiasLayerDesc();
  MatBiasLayerDesc(std::istream& in);
  MatBiasLayerDesc(MatBiasLayerDesc&& other);

  MatBiasLayerDesc(const MatBiasLayerDesc&) = delete;
  MatBiasLayerDesc& operator=(const MatBiasLayerDesc&) = delete;

  MatBiasLayerDesc& operator=(MatBiasLayerDesc&& other);
};

struct ResidualBlockDesc {
  std::string name;
  BatchNormLayerDesc preBN;
  ActivationLayerDesc preActivation;
  ConvLayerDesc regularConv;
  BatchNormLayerDesc midBN;
  ActivationLayerDesc midActivation;
  ConvLayerDesc finalConv;

  ResidualBlockDesc();
  ResidualBlockDesc(std::istream& in);
  ResidualBlockDesc(ResidualBlockDesc&& other);

  ResidualBlockDesc(const ResidualBlockDesc&) = delete;
  ResidualBlockDesc& operator=(const ResidualBlockDesc&) = delete;

  ResidualBlockDesc& operator=(ResidualBlockDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
};

struct DilatedResidualBlockDesc {
  std::string name;
  BatchNormLayerDesc preBN;
  ActivationLayerDesc preActivation;
  ConvLayerDesc regularConv;
  ConvLayerDesc dilatedConv;
  BatchNormLayerDesc midBN;
  ActivationLayerDesc midActivation;
  ConvLayerDesc finalConv;

  DilatedResidualBlockDesc();
  DilatedResidualBlockDesc(std::istream& in);
  DilatedResidualBlockDesc(DilatedResidualBlockDesc&& other);

  DilatedResidualBlockDesc(const DilatedResidualBlockDesc&) = delete;
  DilatedResidualBlockDesc& operator=(const DilatedResidualBlockDesc&) = delete;

  DilatedResidualBlockDesc& operator=(DilatedResidualBlockDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
};

struct GlobalPoolingResidualBlockDesc {
  std::string name;
  int version;
  BatchNormLayerDesc preBN;
  ActivationLayerDesc preActivation;
  ConvLayerDesc regularConv;
  ConvLayerDesc gpoolConv;
  BatchNormLayerDesc gpoolBN;
  ActivationLayerDesc gpoolActivation;
  MatMulLayerDesc gpoolToBiasMul;
  BatchNormLayerDesc midBN;
  ActivationLayerDesc midActivation;
  ConvLayerDesc finalConv;

  GlobalPoolingResidualBlockDesc();
  GlobalPoolingResidualBlockDesc(std::istream& in, int vrsn);
  GlobalPoolingResidualBlockDesc(GlobalPoolingResidualBlockDesc&& other);

  GlobalPoolingResidualBlockDesc(const GlobalPoolingResidualBlockDesc&) = delete;
  GlobalPoolingResidualBlockDesc& operator=(const GlobalPoolingResidualBlockDesc&) = delete;

  GlobalPoolingResidualBlockDesc& operator=(GlobalPoolingResidualBlockDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
};

constexpr int ORDINARY_BLOCK_KIND = 0;
constexpr int DILATED_BLOCK_KIND = 1;
constexpr int GLOBAL_POOLING_BLOCK_KIND = 2;

struct TrunkDesc {
  std::string name;
  int version;
  int numBlocks;
  int trunkNumChannels;
  int midNumChannels;      // Currently every plain residual block must have the same number of mid conv channels
  int regularNumChannels;  // Currently every dilated or gpool residual block must have the same number of regular conv hannels
  int dilatedNumChannels;  // Currently every dilated residual block must have the same number of dilated conv channels
  int gpoolNumChannels;    // Currently every gpooling residual block must have the same number of gpooling conv channels
  ConvLayerDesc initialConv;
  MatMulLayerDesc initialMatMul;
  std::vector<std::pair<int, void*>> blocks;
  BatchNormLayerDesc trunkTipBN;
  ActivationLayerDesc trunkTipActivation;

  TrunkDesc();
  ~TrunkDesc();
  TrunkDesc(std::istream& in, int vrsn);
  TrunkDesc(TrunkDesc&& other);

  TrunkDesc(const TrunkDesc&) = delete;
  TrunkDesc& operator=(const TrunkDesc&) = delete;

  TrunkDesc& operator=(TrunkDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
};

struct PolicyHeadDesc {
  std::string name;
  int version;
  ConvLayerDesc p1Conv;
  ConvLayerDesc g1Conv;
  BatchNormLayerDesc g1BN;
  ActivationLayerDesc g1Activation;
  MatMulLayerDesc gpoolToBiasMul;
  BatchNormLayerDesc p1BN;
  ActivationLayerDesc p1Activation;
  ConvLayerDesc p2Conv;
  MatMulLayerDesc gpoolToPassMul;

  PolicyHeadDesc();
  ~PolicyHeadDesc();
  PolicyHeadDesc(std::istream& in, int vrsn);
  PolicyHeadDesc(PolicyHeadDesc&& other);

  PolicyHeadDesc(const PolicyHeadDesc&) = delete;
  PolicyHeadDesc& operator=(const PolicyHeadDesc&) = delete;

  PolicyHeadDesc& operator=(PolicyHeadDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
};

struct ValueHeadDesc {
  std::string name;
  int version;
  ConvLayerDesc v1Conv;
  BatchNormLayerDesc v1BN;
  ActivationLayerDesc v1Activation;
  MatMulLayerDesc v2Mul;
  MatBiasLayerDesc v2Bias;
  ActivationLayerDesc v2Activation;
  MatMulLayerDesc v3Mul;
  MatBiasLayerDesc v3Bias;
  MatMulLayerDesc sv3Mul;
  MatBiasLayerDesc sv3Bias;
  ConvLayerDesc vOwnershipConv;

  ValueHeadDesc();
  ~ValueHeadDesc();
  ValueHeadDesc(std::istream& in, int vrsn);
  ValueHeadDesc(ValueHeadDesc&& other);

  ValueHeadDesc(const ValueHeadDesc&) = delete;
  ValueHeadDesc& operator=(const ValueHeadDesc&) = delete;

  ValueHeadDesc& operator=(ValueHeadDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
};

struct ModelDesc {
  std::string name;
  int version;
  int xSizePreV3;
  int ySizePreV3;
  int numInputChannels;
  int numInputGlobalChannels;
  int numValueChannels;
  int numScoreValueChannels;
  int numOwnershipChannels;

  TrunkDesc trunk;
  PolicyHeadDesc policyHead;
  ValueHeadDesc valueHead;

  ModelDesc();
  ~ModelDesc();
  ModelDesc(std::istream& in);
  ModelDesc(ModelDesc&& other);

  ModelDesc(const ModelDesc&) = delete;
  ModelDesc& operator=(const ModelDesc&) = delete;

  ModelDesc& operator=(ModelDesc&& other);

  void iterConvLayers(std::function<void(const ConvLayerDesc& dest)> f) const;
  int maxConvChannels(int convXSize, int convYSize) const;

  //Loads a model from a file that may or may not be gzipped, storing it in descBuf
  static void loadFromFileMaybeGZipped(const std::string& fileName, ModelDesc& descBuf);

  //Return the "nearest" supported ruleset to desiredRules by this model.
  //Fills supported with true if desiredRules itself was exactly supported, false if some modifications had to be made.
  Rules getSupportedRules(const Rules& desiredRules, bool& supported) const;
};

#endif  // #ifndef DESC_H
