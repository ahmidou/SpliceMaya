
#include "FabricSpliceEditorWidget.h"
#include "FabricSpliceMayaDeformer.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPointArray.h>
#include <maya/MItMeshVertex.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>

MTypeId FabricSpliceMayaDeformer::id(0x0011AE42);
MObject FabricSpliceMayaDeformer::saveData;
MObject FabricSpliceMayaDeformer::evalID;

FabricSpliceMayaDeformer::FabricSpliceMayaDeformer()
: FabricSpliceBaseInterface()
{
  mGeometryInitialized = 0;
}

FabricSpliceMayaDeformer::~FabricSpliceMayaDeformer()
{
}

void FabricSpliceMayaDeformer::postConstructor()
{
  FabricSpliceBaseInterface::constructBaseInterface();

  MObject object = thisMObject();

  m_attributeAddedOrRemovedCallbackID =
    MNodeMessage::addAttributeAddedOrRemovedCallback(
      object,
      &FabricSpliceBaseInterface::AttributeAddedOrRemoved,
      (FabricSpliceBaseInterface *)this
      );
      
  FabricSpliceEditorWidget::postUpdateAll();
}

void* FabricSpliceMayaDeformer::creator()
{
  return new FabricSpliceMayaDeformer();
}

MStatus FabricSpliceMayaDeformer::initialize(){
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

MStatus FabricSpliceMayaDeformer::deform(MDataBlock& block, MItGeometry& iter, const MMatrix&, unsigned int multiIndex){

  _outputsDirtied = false;
  
  MStatus stat;
  MAYASPLICE_CATCH_BEGIN(&stat);

  FabricSplice::Logging::AutoTimer timer("Maya::deform()");

  if(!_spliceGraph.checkErrors()){
    return MStatus::kFailure; // avoid evaluating on errors
  }

  if(mGeometryInitialized < 1){
    MPlug inputPlug(thisMObject(), input);
    mGeometryInitialized = initializePolygonMeshPorts(inputPlug, block);
    if(mGeometryInitialized < 0){
      return MStatus::kFailure;
    }
  }

  if(transferInputValuesToSplice(block))
  {

    FabricCore::RTVal rtValToSet;
    FabricCore::RTVal rtMesh;
    FabricSplice::DGPort port = _spliceGraph.getDGPort("meshes");

    if(port.isValid()){

      if(port.getMode() != FabricSplice::Port_Mode_IO)
        return MStatus::kSuccess;
      FabricCore::RTVal rtMeshes = port.getRTVal( FabricCore::LockType_Exclusive );
      if(!rtMeshes.isValid())
        return MStatus::kSuccess;
      if(!rtMeshes.isArray())
        return MStatus::kSuccess;
      rtValToSet = rtMeshes;

      rtMesh = rtMeshes.getArrayElement(multiIndex);

    } else {

      // backward compatibility
      MString portName;
      portName.set(multiIndex);
      portName = "mesh" + portName;

      port = _spliceGraph.getDGPort(portName.asChar());
      if(!port.isValid())
        return MStatus::kFailure;

      if(port.getMode() != FabricSplice::Port_Mode_IO)
        return MStatus::kSuccess;
      rtMesh = port.getRTVal( FabricCore::LockType_Exclusive );
      rtValToSet = rtMesh;
    }
    if(!rtMesh.isValid() || rtMesh.isNullObject())
      return MStatus::kSuccess;

    MPointArray mayaPoints;
    iter.allPositions(mayaPoints);

    // MPlug inputPlug(thisMObject(), input);
    // MPlug inputElementPlug = inputPlug.elementByPhysicalIndex(multiIndex);
    // MPlug meshPlug = inputElementPlug.child(inputGeom);
    // MDataHandle handle = block.outputValue(meshPlug);
    // MObject meshObj = handle.asMesh();
    // MFnMesh mesh(meshObj);

    // MFloatVectorArray mayaFnormals;
    // mesh.getNormals(mayaFnormals);
    // MIntArray mayaCounts, mayaIndices;
    // mesh.getNormalIds(mayaCounts, mayaIndices);

    // MVectorArray mayaNormals;
    // mayaNormals.setLength(mayaIndices.length());
    // for(unsigned int i=0;i<mayaIndices.length();i++)
    // {
    //   mayaNormals[i].x = mayaFnormals[mayaIndices[i]].x;
    //   mayaNormals[i].y = mayaFnormals[mayaIndices[i]].y;
    //   mayaNormals[i].z = mayaFnormals[mayaIndices[i]].z;
    // }

    try
    {
      std::vector<FabricCore::RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
      args[1] = FabricSplice::constructUInt32RTVal(4); // components
      rtMesh.callMethod("", "setPointsFromExternalArray_d", 2, &args[0]);

      // FabricCore::RTVal normalsVar = 
      //   FabricSplice::constructExternalArrayRTVal("Float64", mayaNormals.length() * 3, &mayaNormals[0]);
      // rtMesh.callMethod("", "setNormalsFromFloat64Array", 1, &normalsVar);
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(e.getDesc_cstr());
      return MStatus::kSuccess;
    }
    port.setRTVal_lockType( FabricCore::LockType_Exclusive, rtValToSet );

    evaluate();

    try
    {
      std::vector<FabricCore::RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
      args[1] = FabricSplice::constructUInt32RTVal(4); // components
      rtMesh.callMethod("", "getPointsAsExternalArray_d", 2, &args[0]);

      // FabricCore::RTVal normalsVar = 
      //     FabricSplice::constructExternalArrayRTVal("Float64", mayaNormals.length() * 3, &mayaNormals[0]);
      // rtMesh.callMethod("", "getNormalsAsFloat64Array", 1, &normalsVar);

      // for(unsigned int i=0;i<mayaIndices.length();i++)
      // {
      //   mayaFnormals[mayaIndices[i]].x = mayaNormals[i].x;
      //   mayaFnormals[mayaIndices[i]].y = mayaNormals[i].y;
      //   mayaFnormals[mayaIndices[i]].z = mayaNormals[i].z;
      // }
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(e.getDesc_cstr());
      return MStatus::kSuccess;
    }

    iter.setAllPositions(mayaPoints);
    // mesh.setNormals(mayaFnormals);
    transferOutputValuesToMaya(block, true);
  }

  MAYASPLICE_CATCH_END(&stat);
  
  return stat;
}

MStatus FabricSpliceMayaDeformer::setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs){
  MStatus stat = FabricSpliceBaseInterface::setDependentsDirty(thisMObject(), inPlug, affectedPlugs);

  MFnDependencyNode thisNode(thisMObject());
  MPlug output = thisNode.findPlug("outputGeometry");
  affectedPlugs.append(output);

  for(uint32_t i = 0; i < output.numElements(); ++i){
    affectedPlugs.append(output.elementByPhysicalIndex(i));
  }

  return stat;
}

void FabricSpliceMayaDeformer::invalidateNode()
{
  FabricSpliceBaseInterface::invalidateNode();

  MFnDependencyNode thisNode(thisMObject());
  MPlug output = thisNode.findPlug("outputGeometry");
  invalidatePlug(output);

  for(uint32_t i = 0; i < output.numElements(); ++i){
    MPlug plug = output.elementByPhysicalIndex(i);
    invalidatePlug(plug);
  }

  mGeometryInitialized = false;
}

MStatus FabricSpliceMayaDeformer::shouldSave(const MPlug &plug, bool &isSaving){
  // guarantee dynamically added attributes are saved
  isSaving = true;
  return MS::kSuccess;
}

void FabricSpliceMayaDeformer::copyInternalData(MPxNode *node){
  FabricSpliceBaseInterface::copyInternalData(node);
}

int FabricSpliceMayaDeformer::initializePolygonMeshPorts(MPlug &meshPlug, MDataBlock &data){
  MFnDependencyNode thisNode(getThisMObject());

  MString portName = "meshes"; 
  FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.asChar());
  if(!port.isValid()){

    // backwards compatibility
    portName = "mesh0";
    port = _spliceGraph.getDGPort(portName.asChar());
    if(!port.isValid())
      return 0;
  }

  std::string dataType = port.getDataType();
  if(dataType != "PolygonMesh"){
    MGlobal::displayWarning("FabricSpliceMayaDeformer: Wrong port type");
    return -1;
  }

  getSplicePlugToPortFunc("PolygonMesh")(meshPlug, data, port);
  invalidatePlug(meshPlug);
  _spliceGraph.requireEvaluate();
  return 1;
}

MStatus FabricSpliceMayaDeformer::connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricSpliceBaseInterface::onConnection(plug, otherPlug, asSrc, true);
  return MS::kUnknownParameter;
}

MStatus FabricSpliceMayaDeformer::connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc)
{
  FabricSpliceBaseInterface::onConnection(plug, otherPlug, asSrc, false);
  return MS::kUnknownParameter;
}

#if _SPLICE_MAYA_VERSION >= 2016
MStatus FabricSpliceMayaDeformer::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
  return FabricSpliceBaseInterface::preEvaluation(thisMObject(), context, evaluationNode);
}
#endif

