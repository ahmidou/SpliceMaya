
#include "FabricSpliceEditorWidget.h"
#include "FabricSpliceMayaNode.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>

MTypeId FabricSpliceMayaNode::id(0x0011AE41);
MObject FabricSpliceMayaNode::saveData;
MObject FabricSpliceMayaNode::evalID;

FabricSpliceMayaNode::FabricSpliceMayaNode()
: FabricSpliceBaseInterface()
{
}

void FabricSpliceMayaNode::postConstructor(){
  FabricSpliceBaseInterface::constructBaseInterface();
  FabricSpliceEditorWidget::postUpdateAll();
}

FabricSpliceMayaNode::~FabricSpliceMayaNode()
{
}

void* FabricSpliceMayaNode::creator(){
	return new FabricSpliceMayaNode();
}

MStatus FabricSpliceMayaNode::initialize(){
  MFnTypedAttribute typedAttr;
  MFnNumericAttribute numericAttr;

  saveData = typedAttr.create("saveData", "svd", MFnData::kString);
  typedAttr.setHidden(true);
  addAttribute(saveData);

  evalID = numericAttr.create("evalID", "evalID", MFnNumericData::kInt, 0);
  numericAttr.setKeyable(true);
  numericAttr.setHidden(true);
  numericAttr.setReadable(true);
  numericAttr.setWritable(true);
  numericAttr.setStorable(false);
  numericAttr.setCached(false);
  addAttribute(evalID);

  return MS::kSuccess;
}

MStatus FabricSpliceMayaNode::compute(const MPlug& plug, MDataBlock& data){

  _outputsDirtied = false;
  
  MStatus stat;
  
  MAYASPLICE_CATCH_BEGIN(&stat);

  FabricSplice::Logging::AutoTimer timer("Maya::compute()");

  if(!_spliceGraph.checkErrors()){
    return MStatus::kFailure; // avoid evaluating on errors
  }

  transferInputValuesToSplice(data);
  evaluate();
  transferOutputValuesToMaya(data);

  MAYASPLICE_CATCH_END(&stat);

  return stat;
}

MStatus FabricSpliceMayaNode::setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs){
  MStatus stat;
  MAYASPLICE_CATCH_BEGIN(&stat);
  
  FabricSpliceBaseInterface::setDependentsDirty(thisMObject(), inPlug, affectedPlugs);

  MAYASPLICE_CATCH_END(&stat);

  return stat;
}

MStatus FabricSpliceMayaNode::shouldSave(const MPlug &plug, bool &isSaving){
  // guarantee dynamically added attributes are saved
  isSaving = true;
  return MS::kSuccess;
}

void FabricSpliceMayaNode::copyInternalData(MPxNode *node){
  FabricSpliceBaseInterface::copyInternalData(node);
}

MStatus FabricSpliceMayaNode::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricSpliceBaseInterface::onConnection(plug, otherPlug, asSrc, true);
  return MS::kUnknownParameter;
}

MStatus FabricSpliceMayaNode::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricSpliceBaseInterface::onConnection(plug, otherPlug, asSrc, false);
  return MS::kUnknownParameter;
}