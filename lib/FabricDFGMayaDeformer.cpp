
#include "FabricDFGMayaDeformer.h"
#include "FabricDFGConversion.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>

MTypeId FabricDFGMayaDeformer::id(0x0011AE48);
MObject FabricDFGMayaDeformer::saveData;
MObject FabricDFGMayaDeformer::evalID;
MObject FabricDFGMayaDeformer::refFilePath;

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

  return MS::kSuccess;
}

MStatus FabricDFGMayaDeformer::deform(MDataBlock& block, MItGeometry& iter, const MMatrix&, unsigned int multiIndex)
{
  _outputsDirtied = false;
  
  MStatus stat;

  // get the node's state data handle.
  MDataHandle stateData = block.outputValue(state, &stat);
  if (stat != MS::kSuccess)
    return stat;

  if (stateData.asShort() == 0)       // 0: Normal.
  {
    MAYADFG_CATCH_BEGIN(&stat);

    FabricSplice::Logging::AutoTimer timer("Maya::deform()");

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
      FabricCore::RTVal rtValToSet;
      FabricCore::RTVal rtMesh;

      MString portName = "meshes";
      if (!exec.haveExecPort(portName.asChar()))
        return MStatus::kSuccess;
      if (exec.getExecPortType(portName.asChar()) != FabricCore::DFGPortType_IO)
      { mayaLogFunc("FabricDFGMayaDeformer: port \"meshes\" is not an IO port");
        return MStatus::kSuccess; }
      if (exec.getExecPortResolvedType(portName.asChar()) != std::string("PolygonMesh[]"))
      { mayaLogFunc("FabricDFGMayaDeformer: port \"meshes\" has the wrong resolved data type");
        return MStatus::kSuccess; }

      FabricCore::RTVal rtMeshes = binding.getArgValue(portName.asChar());
      //FabricCore::LockType_Exclusive   ???

      if(!rtMeshes.isValid()) return MStatus::kSuccess;
      if(!rtMeshes.isArray()) return MStatus::kSuccess;
      rtValToSet = rtMeshes;
      rtMesh     = rtMeshes.getArrayElement(multiIndex);

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
      binding.setArgValue(portName.asChar(), rtValToSet, false);

      evaluate();

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
  MFnDependencyNode thisNode(getThisMObject());

  FabricCore::DFGExec exec = getDFGExec();
  if (!exec.isValid())
    return 0;

  MString portName = "meshes";
  if (!exec.haveExecPort(portName.asChar())){
    MGlobal::displayWarning("FabricDFGMayaDeformer: missing port: " + portName);
    return 0;
  }

  std::string dataType = exec.getExecPortResolvedType(portName.asChar());
  if(dataType != "PolygonMesh[]"){
    MGlobal::displayWarning("FabricDFGMayaDeformer: Wrong port data type: " + MString(dataType.c_str()));
    return -1;
  }

  DFGConversionTimers timers;
  getDFGPlugToArgFunc("PolygonMesh")(meshPlug, data, m_binding, getLockType(), portName.asChar(), &timers);
  invalidatePlug(meshPlug);
  //_spliceGraph.requireEvaluate();
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

#if _SPLICE_MAYA_VERSION >= 2016
MStatus FabricDFGMayaDeformer::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
  return FabricDFGBaseInterface::preEvaluation(thisMObject(), context, evaluationNode);
}
#endif

