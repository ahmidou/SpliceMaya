
#include "FabricDFGMayaNode.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>

MTypeId FabricDFGMayaNode::id(0x0011AE46);
MObject FabricDFGMayaNode::saveData;
MObject FabricDFGMayaNode::evalID;
MObject FabricDFGMayaNode::refFilePath;

FabricDFGMayaNode::FabricDFGMayaNode()
: FabricDFGBaseInterface()
{
}

void FabricDFGMayaNode::postConstructor(){
  FabricDFGBaseInterface::constructBaseInterface();
}

FabricDFGMayaNode::~FabricDFGMayaNode()
{
}

void* FabricDFGMayaNode::creator(){
	return new FabricDFGMayaNode();
}

MStatus FabricDFGMayaNode::initialize(){
  MFnTypedAttribute typedAttr;
  MFnNumericAttribute nAttr;

  saveData = typedAttr.create("saveData", "svd", MFnData::kString);
  typedAttr.setHidden(true);
  addAttribute(saveData);

  evalID = nAttr.create("evalID", "evalID", MFnNumericData::kInt);
  nAttr.setHidden(true);
  addAttribute(evalID);

  refFilePath = typedAttr.create("refFilePath", "rfp", MFnData::kString);
  typedAttr.setHidden(true);
  addAttribute(refFilePath);

  return MS::kSuccess;
}

MStatus FabricDFGMayaNode::compute(const MPlug& plug, MDataBlock& data){

  _outputsDirtied = false;
  
  MStatus stat;
  
  MAYADFG_CATCH_BEGIN(&stat);

  FabricSplice::Logging::AutoTimer timer("Maya::compute()");

  // if(!_spliceGraph.checkErrors()){
  //   return MStatus::kFailure; // avoid evaluating on errors
  // }

  if(transferInputValuesToDFG(data))
  {
    evaluate();
    transferOutputValuesToMaya(data);
  }

  MAYADFG_CATCH_END(&stat);

  return stat;
}

MStatus FabricDFGMayaNode::setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs){
  MStatus stat;
  MAYADFG_CATCH_BEGIN(&stat);
  
  FabricDFGBaseInterface::setDependentsDirty(thisMObject(), inPlug, affectedPlugs);

  MAYADFG_CATCH_END(&stat);

  return stat;
}

MStatus FabricDFGMayaNode::shouldSave(const MPlug &plug, bool &isSaving){
  // guarantee dynamically added attributes are saved
  isSaving = true;
  return MS::kSuccess;
}

void FabricDFGMayaNode::copyInternalData(MPxNode *node){
  FabricDFGBaseInterface::copyInternalData(node);
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
