//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricDFGCanvasDeformer.h"
#include "FabricDFGConversion.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGProfiling.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>

MTypeId FabricDFGMayaDeformer::id(0x0011AE48);
MObject FabricDFGMayaDeformer::saveData;
MObject FabricDFGMayaDeformer::evalID;
MObject FabricDFGMayaDeformer::refFilePath;
MObject FabricDFGMayaDeformer::enableEvalContext;

FabricDFGMayaDeformer::FabricDFGMayaDeformer()
: FabricDFGBaseInterface()
{
  mGeometryInitialized = 0;
}

void FabricDFGMayaDeformer::postConstructor(){
  FabricDFGBaseInterface::constructBaseInterface();
  setExistWithoutInConnections(true);
  setExistWithoutOutConnections(true);
}

FabricDFGMayaDeformer::~FabricDFGMayaDeformer()
{
}

void* FabricDFGMayaDeformer::creator()
{
	return new FabricDFGMayaDeformer();
}

MStatus FabricDFGMayaDeformer::initialize(){
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

MStatus FabricDFGMayaDeformer::deform(MDataBlock& block, MItGeometry& iter, const MMatrix&, unsigned int multiIndex)
{
  FabricMayaProfilingEvent bracket("FabricDFGMayaDeformer::deform");

  _outputsDirtied = false;
  
  MStatus stat;

  // get the node's state data handle.
  MDataHandle stateData = block.outputValue(state, &stat);
  if (stat != MS::kSuccess)
    return stat;

  if (stateData.asShort() == 0)       // 0: Normal.
  {
    MAYADFG_CATCH_BEGIN(&stat);

    FabricCore::DFGBinding binding = getDFGBinding();
    FabricCore::DFGExec    exec    = getDFGExec();
    if (!binding.isValid() || !exec.isValid())
      return MStatus::kFailure;

    if(mGeometryInitialized < 1){
      MPlug inputPlug(thisMObject(), input);
      mGeometryInitialized = initializePolygonMeshPorts(inputPlug, block);
      if(mGeometryInitialized < 0){
        return MStatus::kFailure;
      }
    }

    if(transferInputValuesToDFG(block))
    {
      FabricCore::RTVal rtMesh, rtMeshes;

      MString portName = "meshes";
      if (!exec.haveExecPort(portName.asChar()))
        return MStatus::kSuccess;
      if (exec.getExecPortType(portName.asChar()) != FabricCore::DFGPortType_IO)
      { mayaLogFunc("FabricDFGMayaDeformer: port \"meshes\" is not an IO port");
        return MStatus::kSuccess; }
      if (exec.getExecPortResolvedType(portName.asChar()) != std::string("PolygonMesh[]"))
      { mayaLogFunc("FabricDFGMayaDeformer: port \"meshes\" has the wrong resolved data type");
        return MStatus::kSuccess; }

      rtMeshes = binding.getArgValue(portName.asChar());
      if(!rtMeshes.isValid()) return MStatus::kSuccess;
      if(!rtMeshes.isArray()) return MStatus::kSuccess;
      rtMesh = rtMeshes.getArrayElement(multiIndex);
      if(!rtMesh.isValid() || rtMesh.isNullObject())
        return MStatus::kSuccess;

      MPointArray mayaPoints;
      iter.allPositions(mayaPoints);

      try
      {
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
        args[1] = FabricSplice::constructUInt32RTVal(4); // components
        rtMesh.callMethod("", "setPointsFromExternalArray_d", 2, &args[0]);
      }
      catch(FabricCore::Exception e)
      {
        mayaLogErrorFunc(e.getDesc_cstr());
        return MStatus::kSuccess;
      }
      binding.setArgValue_lockType(getLockType(), portName.asChar(), rtMeshes, false);

      evaluate();

      // [FE-7528] the Canvas graph could be using a different
      // mesh for the output as for the input, so we get the
      // argument value and mesh RTVal again to ensure we are
      // using the right one.
      rtMeshes = binding.getArgValue(portName.asChar());
      if(!rtMeshes.isValid()) return MStatus::kSuccess;
      if(!rtMeshes.isArray()) return MStatus::kSuccess;
      rtMesh = rtMeshes.getArrayElement(multiIndex);
      if(!rtMesh.isValid() || rtMesh.isNullObject())
        return MStatus::kSuccess;

      try
      {
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
        args[1] = FabricSplice::constructUInt32RTVal(4); // components
        rtMesh.callMethod("", "getPointsAsExternalArray_d", 2, &args[0]);
      }
      catch(FabricCore::Exception e)
      {
        mayaLogErrorFunc(e.getDesc_cstr());
        return MStatus::kSuccess;
      }

      iter.setAllPositions(mayaPoints);
      transferOutputValuesToMaya(block, true);
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

MStatus FabricDFGMayaDeformer::setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs){
  MStatus stat = FabricDFGBaseInterface::setDependentsDirty(thisMObject(), inPlug, affectedPlugs);

  MFnDependencyNode thisNode(thisMObject());
  MPlug output = thisNode.findPlug("outputGeometry");
  affectedPlugs.append(output);

  for(uint32_t i = 0; i < output.numElements(); ++i){
    affectedPlugs.append(output.elementByPhysicalIndex(i));
  }

  return stat;
}

void FabricDFGMayaDeformer::invalidateNode()
{
  FabricDFGBaseInterface::invalidateNode();

  MFnDependencyNode thisNode(thisMObject());
  MPlug output = thisNode.findPlug("outputGeometry");
  invalidatePlug(output);

  for(uint32_t i = 0; i < output.numElements(); ++i){
    MPlug plug = output.elementByPhysicalIndex(i);
    invalidatePlug(plug);
  }

  mGeometryInitialized = false;
}

MStatus FabricDFGMayaDeformer::shouldSave(const MPlug &plug, bool &isSaving){
  // guarantee dynamically added attributes are saved
  isSaving = true;
  return MS::kSuccess;
}

void FabricDFGMayaDeformer::copyInternalData(MPxNode *node){
  FabricDFGBaseInterface::copyInternalData(node);
}

int FabricDFGMayaDeformer::initializePolygonMeshPorts(MPlug &meshPlug, MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("FabricDFGMayaDeformer::initializePolygonMeshPorts");

  MFnDependencyNode thisNode(getThisMObject());

  FabricCore::DFGExec exec = getDFGExec();
  if (!exec.isValid())
    return 0;

  // [FE-6492]
  if (exec.getExecPortCount() <= 1)
    return 0;

  MString portName = "meshes";
  if (!exec.haveExecPort(portName.asChar())){
    MGlobal::displayWarning("FabricDFGMayaDeformer: missing port: " + portName);
    return 0;
  }  

  VisitCallbackUserData ud(getThisMObject(), data);
  ud.interf = this;
  ud.isDeformer = false;
  ud.returnValue = 1;
  ud.meshPlug = meshPlug;
  getDFGBinding().visitArgs(getLockType(), &FabricDFGMayaDeformer::VisitMeshCallback, &ud);
  if(ud.returnValue != 1)
    return ud.returnValue;

  invalidatePlug(meshPlug);
  return 1;
}

bool FabricDFGMayaDeformer::getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx){
  return FabricDFGBaseInterface::getInternalValueInContext(plug, dataHandle, ctx);
}

bool FabricDFGMayaDeformer::setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx){
  return FabricDFGBaseInterface::setInternalValueInContext(plug, dataHandle, ctx);
}

MStatus FabricDFGMayaDeformer::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricDFGBaseInterface::onConnection(plug, otherPlug, asSrc, true);
  return MS::kUnknownParameter;
}

MStatus FabricDFGMayaDeformer::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricDFGBaseInterface::onConnection(plug, otherPlug, asSrc, false);
  return MS::kUnknownParameter;
}

#if MAYA_API_VERSION >= 201600
MStatus FabricDFGMayaDeformer::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
  return FabricDFGBaseInterface::preEvaluation(thisMObject(), context, evaluationNode);
}

MStatus FabricDFGMayaDeformer::postEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode, PostEvaluationType evalType)
{
  return FabricDFGBaseInterface::postEvaluation(thisMObject(), context, evaluationNode, evalType);
}
#endif

void FabricDFGMayaDeformer::VisitMeshCallback(
  void *userdata,
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  char const *argTypeCanonicalName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD
  ) {

  if(argOutsidePortType != FabricCore::DFGPortType_IO)
    return;

  VisitCallbackUserData * ud = (VisitCallbackUserData *)userdata;
  FTL::StrRef argNameRef = argName;
  if(argNameRef != FTL_STR("meshes"))
    return;

  FTL::StrRef argTypeNameRef = argTypeName;
  if(argTypeNameRef != FTL_STR("PolygonMesh[]"))
  {
    MGlobal::displayWarning("FabricDFGMayaDeformer: Wrong port data type: " + MString(argTypeNameRef.data()));
    ud->returnValue = -1;
    return;
  }

  DFGPlugToArgFunc func = getDFGPlugToArgFunc(FTL_STR("PolygonMesh"));
  if(func == NULL)
    return;

  (*func)(
    argIndex,
    argName,
    argTypeName,
    argOutsidePortType,
    argRawDataSize,
    getCB,
    getRawCB,
    setCB,
    setRawCB,
    getSetUD,
    ud->meshPlug,
    ud->data
    );
}
