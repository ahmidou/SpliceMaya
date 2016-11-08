//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricDFGRigNode.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGProfiling.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>

MTypeId FabricDFGRigNode::id(0x0011AE49);
MObject FabricDFGRigNode::saveData;
MObject FabricDFGRigNode::evalID;
MObject FabricDFGRigNode::refFilePath;
MObject FabricDFGRigNode::matrixInputs;
MObject FabricDFGRigNode::scalarInputs;
MObject FabricDFGRigNode::matrixOutputs;
MObject FabricDFGRigNode::scalarOutputs;

FabricDFGRigNode::FabricDFGRigNode()
: FabricDFGBaseInterface()
{
}

void FabricDFGRigNode::postConstructor(){
  FabricDFGBaseInterface::constructBaseInterface();
  setExistWithoutInConnections(true);
  setExistWithoutOutConnections(true);
}

FabricDFGRigNode::~FabricDFGRigNode()
{
}

void* FabricDFGRigNode::creator(){
	return new FabricDFGRigNode();
}

MStatus FabricDFGRigNode::initialize(){
  FabricMayaProfilingEvent bracket("FabricDFGRigNode::initialize");

  MFnTypedAttribute typedAttr;
  MFnNumericAttribute nAttr;
  MFnMatrixAttribute mAttr;

  saveData = typedAttr.create("saveData", "svd", MFnData::kString);
  typedAttr.setHidden(true);
  typedAttr.setInternal(true);
  addAttribute(saveData);

  evalID = nAttr.create("evalID", "evalID", MFnNumericData::kInt);
  nAttr.setHidden(true);
  addAttribute(evalID);

  refFilePath = typedAttr.create("refFilePath", "rfp", MFnData::kString);
  typedAttr.setHidden(true);
  addAttribute(refFilePath);

  matrixInputs = mAttr.create("matrixInputs", "matrixIn");
  mAttr.setArray(true);
  mAttr.setUsesArrayDataBuilder(true); // todo? is this required
  mAttr.setReadable(false);
  mAttr.setWritable(true);
  mAttr.setKeyable(true);
  mAttr.setStorable(true);
  addAttribute(matrixInputs);

  scalarInputs = nAttr.create("scalarInputs", "scalarIn", MFnNumericData::kFloat);
  nAttr.setArray(true);
  nAttr.setUsesArrayDataBuilder(true); // todo? is this required
  nAttr.setReadable(false);
  nAttr.setWritable(true);
  nAttr.setKeyable(true);
  nAttr.setStorable(true);
  addAttribute(scalarInputs);

  matrixOutputs = mAttr.create("matrixOutputs", "matrixOut");
  mAttr.setArray(true);
  mAttr.setUsesArrayDataBuilder(true); // todo? is this required
  mAttr.setReadable(true);
  mAttr.setWritable(false);
  mAttr.setKeyable(false);
  mAttr.setCached(true);
  mAttr.setStorable(true);
  addAttribute(matrixOutputs);

  scalarOutputs = nAttr.create("scalarOutputs", "scalarOut", MFnNumericData::kFloat);
  nAttr.setArray(true);
  nAttr.setUsesArrayDataBuilder(true); // todo? is this required
  nAttr.setReadable(true);
  nAttr.setWritable(false);
  nAttr.setKeyable(false);
  nAttr.setCached(true);
  nAttr.setStorable(true);
  addAttribute(scalarOutputs);

  attributeAffects(evalID, matrixOutputs);
  attributeAffects(evalID, scalarOutputs);
  attributeAffects(matrixInputs, matrixOutputs);
  attributeAffects(matrixInputs, scalarOutputs);
  attributeAffects(scalarInputs, matrixOutputs);
  attributeAffects(scalarInputs, scalarOutputs);

  return MS::kSuccess;
}

MStatus FabricDFGRigNode::compute(const MPlug& plug, MDataBlock& data){

  FabricMayaProfilingEvent bracket("FabricDFGRigNode::compute");

  _outputsDirtied = false;
  
  MStatus stat;

  // get the node's state data handle.
  MDataHandle stateData = data.outputValue(state, &stat);
  if (stat != MS::kSuccess)
    return stat;

  if (stateData.asShort() == 0)       // 0: Normal.
  {
    MAYADFG_CATCH_BEGIN(&stat);

    // if(!_spliceGraph.checkErrors()){
    //   return MStatus::kFailure; // avoid evaluating on errors
    // }

    if(transferInputValuesToDFG(data))
    {
      evaluate();
      transferOutputValuesToMaya(data);
    }

    MAYADFG_CATCH_END(&stat);
  }
  else if (stateData.asShort() == 1)  // 1: HasNoEffect.
  {
    stat = MS::kNotImplemented;
  }
  else                                // not supported by Canvas node.
  {
    stat = MS::kNotImplemented;
  }

  return stat;
}

MStatus FabricDFGRigNode::setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs){
  return FabricDFGBaseInterface::setDependentsDirty(thisMObject(), inPlug, affectedPlugs);
}

MStatus FabricDFGRigNode::shouldSave(const MPlug &plug, bool &isSaving){
  // guarantee dynamically added attributes are saved
  isSaving = true;
  return MS::kSuccess;
}

void FabricDFGRigNode::copyInternalData(MPxNode *node){
  FabricDFGBaseInterface::copyInternalData(node);
}

bool FabricDFGRigNode::getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx){
  return FabricDFGBaseInterface::getInternalValueInContext(plug, dataHandle, ctx);
}

bool FabricDFGRigNode::setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx){
  return FabricDFGBaseInterface::setInternalValueInContext(plug, dataHandle, ctx);
}

MString FabricDFGRigNode::getPlugName(const MString &portName)
{
  if      (portName == "matrixInputs")  return "matrixInputs";
  else if (portName == "scalarInputs")  return "scalarInputs";
  else if (portName == "matrixOutputs") return "matrixOutputs";
  else if (portName == "scalarOutputs") return "scalarOutputs";

  return FabricDFGBaseInterface::getPlugName(portName);
}

MString FabricDFGRigNode::getPortName(const MString &plugName)
{
  if      (plugName == "matrixInputs")  return "matrixInputs";
  else if (plugName == "scalarInputs")  return "scalarInputs";
  else if (plugName == "matrixOutputs") return "matrixOutputs";
  else if (plugName == "scalarOutputs") return "scalarOutputs";

  return FabricDFGBaseInterface::getPortName(plugName);
}

bool FabricDFGRigNode::transferInputValuesToDFG(MDataBlock& data)
{
  // todo: out custom node conversion - we also want to shrink the _dirtyPlugs
  return FabricDFGBaseInterface::transferInputValuesToDFG(data);
}

void FabricDFGRigNode::setupMayaAttributeAffects(MString portName, FabricCore::DFGPortType portType, MObject newAttribute, MStatus *stat)
{
  FabricDFGBaseInterface::setupMayaAttributeAffects(portName, portType, newAttribute, stat);

  MFnDependencyNode thisNode(getThisMObject());
  MPxNode * userNode = thisNode.userNode();

  if(portType == FabricCore::DFGPortType_In)
  {
    userNode->attributeAffects(newAttribute, matrixOutputs);
    userNode->attributeAffects(newAttribute, scalarOutputs);
  }
  else
  {
    userNode->attributeAffects(matrixInputs, newAttribute);
    userNode->attributeAffects(scalarInputs, newAttribute);
  }
}

MStatus FabricDFGRigNode::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricDFGBaseInterface::onConnection(plug, otherPlug, asSrc, true);
  return MS::kUnknownParameter;
}

MStatus FabricDFGRigNode::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricDFGBaseInterface::onConnection(plug, otherPlug, asSrc, false);
  return MS::kUnknownParameter;
}

#if MAYA_API_VERSION >= 201600
MStatus FabricDFGRigNode::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
  return FabricDFGBaseInterface::preEvaluation(thisMObject(), context, evaluationNode);
}
#endif
