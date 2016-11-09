//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricDFGCanvasNode.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGProfiling.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>

MTypeId FabricDFGMayaNode::id(0x0011AE47);
MObject FabricDFGMayaNode::saveData;
MObject FabricDFGMayaNode::evalID;
MObject FabricDFGMayaNode::refFilePath;
MObject FabricDFGMayaNode::enableEvalContext;

FabricDFGMayaNode::FabricDFGMayaNode()
: FabricDFGBaseInterface()
{
}

void FabricDFGMayaNode::postConstructor(){
  FabricDFGBaseInterface::constructBaseInterface();
  setExistWithoutInConnections(true);
  setExistWithoutOutConnections(true);
}

FabricDFGMayaNode::~FabricDFGMayaNode()
{
}

void* FabricDFGMayaNode::creator(){
	return new FabricDFGMayaNode();
}

MStatus FabricDFGMayaNode::initialize(){
  FabricMayaProfilingEvent bracket("FabricDFGMayaNode::initialize");

  MFnTypedAttribute typedAttr;
  MFnNumericAttribute nAttr;

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

  enableEvalContext = nAttr.create("enableEvalContext", "ctx", MFnNumericData::kBoolean, 1.0);
  nAttr.setHidden(true);
  nAttr.setConnectable(false);
  addAttribute(enableEvalContext);

  return MS::kSuccess;
}

MStatus FabricDFGMayaNode::compute(const MPlug& plug, MDataBlock& data){

  FabricMayaProfilingEvent bracket("FabricDFGMayaNode::compute");

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

MStatus FabricDFGMayaNode::setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs){
  return FabricDFGBaseInterface::setDependentsDirty(thisMObject(), inPlug, affectedPlugs);
}

MStatus FabricDFGMayaNode::shouldSave(const MPlug &plug, bool &isSaving){
  // guarantee dynamically added attributes are saved
  isSaving = true;
  return MS::kSuccess;
}

void FabricDFGMayaNode::copyInternalData(MPxNode *node){
  FabricDFGBaseInterface::copyInternalData(node);
}

bool FabricDFGMayaNode::getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx){
  return FabricDFGBaseInterface::getInternalValueInContext(plug, dataHandle, ctx);
}

bool FabricDFGMayaNode::setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx){
  return FabricDFGBaseInterface::setInternalValueInContext(plug, dataHandle, ctx);
}

MStatus FabricDFGMayaNode::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricDFGBaseInterface::onConnection(plug, otherPlug, asSrc, true);
  return MS::kUnknownParameter;
}

MStatus FabricDFGMayaNode::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricDFGBaseInterface::onConnection(plug, otherPlug, asSrc, false);
  return MS::kUnknownParameter;
}

#if MAYA_API_VERSION >= 201600
MStatus FabricDFGMayaNode::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
  return FabricDFGBaseInterface::preEvaluation(thisMObject(), context, evaluationNode);
}
#endif
