
#include "FabricSpliceEditorWidget.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceMayaData.h"
// #include "plugin.h"

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MCommandResult.h>
#include <maya/MPlugArray.h>
#include <maya/MFileObject.h>
#include <maya/MFnPluginData.h>
#include <maya/MAnimControl.h>

std::vector<FabricSpliceBaseInterface*> FabricSpliceBaseInterface::_instances;
#if _SPLICE_MAYA_VERSION < 2013
  std::map<std::string, int> FabricSpliceBaseInterface::_nodeCreatorCounts;
#endif

FabricSpliceBaseInterface::FabricSpliceBaseInterface(){

  MStatus stat;
  MAYASPLICE_CATCH_BEGIN(&stat);

_restoredFromPersistenceData = false;
  _dummyValue = 17;
  _spliceGraph = FabricSplice::DGGraph();
  _spliceGraph.setUserPointer(this);
  _isTransferingInputs = false;
  _instances.push_back(this);
  _dgDirtyEnabled = true;
  _portObjectsDestroyed = false;

  MAYASPLICE_CATCH_END(&stat);
}

FabricSpliceBaseInterface::~FabricSpliceBaseInterface(){
  for(size_t i=0;i<_instances.size();i++){
    if(_instances[i] == this){
      std::vector<FabricSpliceBaseInterface*>::iterator iter = _instances.begin() + i;
      _instances.erase(iter);
      break;
    }
  }
}

void FabricSpliceBaseInterface::constructBaseInterface(){

  MStatus stat;
  MAYASPLICE_CATCH_BEGIN(&stat);

  if(_spliceGraph.isValid())
    return;

#if _SPLICE_MAYA_VERSION < 2013
  // in earlier versions than 2013 maya would construct each node
  // once on startup. we avoid this by counting the numbers of nodes
  // constructor for each node type.
  MFnDependencyNode thisNode(getThisMObject());
  std::string nodeType = thisNode.typeName().asChar();
  if(_nodeCreatorCounts.find(nodeType) == _nodeCreatorCounts.end()){
    _nodeCreatorCounts.insert(std::pair<std::string,int>(nodeType, 1));
    return;
  }
#endif

  FabricSplice::Logging::AutoTimer timer("Maya::FabricSpliceBaseInterface()");

  _spliceGraph = FabricSplice::DGGraph("mayaGraph");
  _spliceGraph.constructDGNode("DGNode");

  MAYASPLICE_CATCH_END(&stat);

}

std::vector<FabricSpliceBaseInterface*> FabricSpliceBaseInterface::getInstances(){
  return _instances;
}

FabricSpliceBaseInterface * FabricSpliceBaseInterface::getInstanceByName(const std::string & name) {

  MSelectionList selList;
  MGlobal::getSelectionListByName(name.c_str(), selList);
  MObject spliceMayaNodeObj;
  selList.getDependNode(0, spliceMayaNodeObj);

  for(size_t i=0;i<_instances.size();i++)
  {
    if(_instances[i]->getThisMObject() == spliceMayaNodeObj)
    {
      return _instances[i];
    }
  }
  return NULL;
}

void FabricSpliceBaseInterface::transferInputValuesToSplice(MDataBlock& data){
  if(_isTransferingInputs)
    return;

  managePortObjectValues(false); // recreate objects if not there yet

  FabricSplice::Logging::AutoTimer timer("Maya::transferInputValuesToSplice()");

  _isTransferingInputs = true;

  MFnDependencyNode thisNode(getThisMObject());

  for(int i = 0; i < _dirtyPlugs.length(); ++i){
    MString plugName = _dirtyPlugs[i];
    MPlug plug = thisNode.findPlug(plugName);
    if(!plug.isNull()){
      FabricSplice::DGPort port = _spliceGraph.getDGPort(plugName.asChar());
      if(!port.isValid())
        continue;
      if(port.getMode() != FabricSplice::Port_Mode_OUT){

        std::string dataType = port.getDataType();
        for(size_t j=0;j<mSpliceMayaDataOverride.size();j++)
        {
          if(mSpliceMayaDataOverride[j] == plugName.asChar())
          {
            dataType = "SpliceMayaData";
            break;
          }
        }
        
        SplicePlugToPortFunc func = getSplicePlugToPortFunc(dataType, &port);
        if(func != NULL)
          (*func)(plug, data, port);
      }
    }
  }

  _dirtyPlugs.clear();
  _isTransferingInputs = false;
}

void FabricSpliceBaseInterface::evaluate(){
  MFnDependencyNode thisNode(getThisMObject());

  FabricSplice::Logging::AutoTimer timer("Maya::evaluate()");
  managePortObjectValues(false); // recreate objects if not there yet

  // setup the context
  FabricCore::RTVal context = _spliceGraph.getEvalContext();
  context.setMember("host", FabricSplice::constructStringRTVal("Maya"));
  context.setMember("graph", FabricSplice::constructStringRTVal(thisNode.name().asChar()));
  context.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));

  _spliceGraph.evaluate();
}

void FabricSpliceBaseInterface::transferOutputValuesToMaya(MDataBlock& data, bool isDeformer){
  if(_isTransferingInputs)
    return;

  managePortObjectValues(false); // recreate objects if not there yet

  FabricSplice::Logging::AutoTimer timer("Maya::transferOutputValuesToMaya()");
  
  MFnDependencyNode thisNode(getThisMObject());

  for(int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    FabricSplice::DGPort port = _spliceGraph.getDGPort((unsigned int)i);
    if(!port.isValid())
      continue;
    int portMode = (int)port.getMode();
    if(portMode != (int)FabricSplice::Port_Mode_IN){
      
      std::string portName = port.getName();
      std::string portDataType = port.getDataType();

      MPlug plug = thisNode.findPlug(portName.c_str());
      if(!plug.isNull()){
        for(size_t i=0;i<mSpliceMayaDataOverride.size();i++)
        {
          if(mSpliceMayaDataOverride[i] == portName)
          {
            portDataType = "SpliceMayaData";
            break;
          }
        }

        if(isDeformer && portDataType == "PolygonMesh") {
          data.setClean(plug);
        } else {
          SplicePortToPlugFunc func = getSplicePortToPlugFunc(portDataType, &port);
          if(func != NULL) {
            FabricSplice::Logging::AutoTimer timer("Maya::transferOutputValuesToMaya::conversionFunc()");
            (*func)(port, plug, data);
            data.setClean(plug);
          }
        }
      }
    }
  }
}

void FabricSpliceBaseInterface::collectDirtyPlug(MPlug const &inPlug){

  FabricSplice::Logging::AutoTimer timer("Maya::collectDirtyPlug()");

  MStatus stat;
  MString name;

  MAYASPLICE_CATCH_BEGIN(&stat);

  // notify the context about this
  FabricCore::RTVal context = _spliceGraph.getEvalContext();

  if(inPlug.isElement()){
    name = inPlug.array().partialName(false, false, false, false, false, true);
  }
  else{
    name = inPlug.partialName(false, false, false, false, false, true);
  }

  std::vector<FabricCore::RTVal> args(1);
  args[0] = FabricSplice::constructStringRTVal(name.asChar());
  if(inPlug.isElement()){
    args.push_back(FabricSplice::constructSInt32RTVal(inPlug.logicalIndex()));
  }
  MStringArray splitBuffer;
  name.split('.', splitBuffer);

  MString portName = splitBuffer[0];
  FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.asChar());
  if(port.isValid()){
    if(port.getMode() != FabricSplice::Port_Mode_OUT)
      context.callMethod("", "_addDirtyInput", args.size(), &args[0]);
  }

  MAYASPLICE_CATCH_END(&stat);

  if(inPlug.isChild()){
    // if plug belongs to translation or rotation we collect the parent to transfer all x,y,z values
    if(inPlug.parent().isElement()){
      collectDirtyPlug(inPlug.parent().array());
      return;
    }
    else{
      collectDirtyPlug(inPlug.parent());
      return;
    }
  }

  for(int i = 0; i < _dirtyPlugs.length(); ++i){
    if(_dirtyPlugs[i] == name)
      return;
  }

  _dirtyPlugs.append(name);
}

void FabricSpliceBaseInterface::affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs){
  if(plug.isNull()){
    return;
  }

  for(int i = 0; i < plug.numChildren(); ++i){
    const MPlug &childPlug = plug.child(i);
    if(!childPlug.isNull()){
      affectedPlugs.append(childPlug);
    }
  }

  for(int i = 0; i < plug.numElements(); ++i){
    const MPlug &elementPlug = plug.elementByPhysicalIndex(i);
    if(!elementPlug.isNull()){
      affectedPlugs.append(elementPlug);
      for(int j = 0; j < elementPlug.numChildren(); ++j){
        const MPlug &childPlug = elementPlug.child(j);
        if(!childPlug.isNull()){
          affectedPlugs.append(childPlug);
        }
      }
    }
  }
}

float getScalarOption(const char * key, FabricCore::Variant options, float value = 0.0) {
  if(options.isNull())
    return value;
  if(!options.isDict())
    return value;
  const FabricCore::Variant * option = options.getDictValue(key);
  if(!option)
    return value;
  if(option->isSInt8())
    return (float)option->getSInt8();
  if(option->isSInt16())
    return (float)option->getSInt8();
  if(option->isSInt32())
    return (float)option->getSInt8();
  if(option->isSInt64())
    return (float)option->getSInt8();
  if(option->isUInt8())
    return (float)option->getUInt8();
  if(option->isUInt16())
    return (float)option->getUInt8();
  if(option->isUInt32())
    return (float)option->getUInt8();
  if(option->isUInt64())
    return (float)option->getUInt8();
  if(option->isFloat32())
    return (float)option->getFloat32();
  if(option->isFloat64())
    return (float)option->getFloat64();
  return value;
} 

std::string getStringOption(const char * key, FabricCore::Variant options, std::string value = "") {
  if(options.isNull())
    return value;
  if(!options.isDict())
    return value;
  const FabricCore::Variant * option = options.getDictValue(key);
  if(!option)
    return value;
  if(option->isString())
    return option->getStringData();
  return value;
} 

MObject FabricSpliceBaseInterface::addMayaAttribute(const MString &portName, const MString &dataType, const MString &arrayType, const FabricSplice::Port_Mode &portMode, bool compoundChild, FabricCore::Variant compoundStructure, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::addMayaAttribute()");
  MObject newAttribute;

  MString dataTypeOverride = dataType;

  // remove []
  MStringArray splitBuffer;
  dataTypeOverride.split('[', splitBuffer);
  if(splitBuffer.length()){
    dataTypeOverride = splitBuffer[0];
  }

  MFnDependencyNode thisNode(getThisMObject());
  MPlug plug = thisNode.findPlug(portName);
  if(!plug.isNull()){
    mayaLogFunc("Attribute '"+portName+"' already exists on node '"+thisNode.name()+"'.");
    return newAttribute;
  }

  MFnNumericAttribute nAttr;
  MFnTypedAttribute tAttr;
  MFnUnitAttribute uAttr;
  MFnMatrixAttribute mAttr;
  MFnMessageAttribute pAttr;
  MFnCompoundAttribute cAttr;
  MFnStringData emptyStringData;
  MObject emptyStringObject = emptyStringData.create("");

  bool storable = true;


  if(dataTypeOverride == "CompoundParam")
  {
    if(compoundStructure.isNull())
    {
      mayaLogErrorFunc("CompoundParam used for a maya attribute but no compound structure provided.");
      return newAttribute;
    }

    if(!compoundStructure.isDict())
    {
      mayaLogErrorFunc("CompoundParam used for a maya attribute but compound structure does not contain a dictionary.");
      return newAttribute;
    }

    if(arrayType == "Single Value" || arrayType == "Array (Multi)")
    {
      MObjectArray children;
      for(FabricCore::Variant::DictIter childIter(compoundStructure); !childIter.isDone(); childIter.next())
      {
        MString childNameStr = childIter.getKey()->getStringData();
        const FabricCore::Variant * value = childIter.getValue();
        if(!value)
          continue;
        if(value->isNull())
          continue;
        if(value->isDict()) {
          const FabricCore::Variant * childDataType = value->getDictValue("dataType");
          if(childDataType)
          {
            if(childDataType->isString())
            {
              MString childArrayTypeStr = "Single Value";
              MString childDataTypeStr = childDataType->getStringData();
              MStringArray childDataTypeStrParts;
              childDataTypeStr.split('[', childDataTypeStrParts);
              if(childDataTypeStrParts.length() > 1)
                childArrayTypeStr = "Array (Multi)";

              MObject child = addMayaAttribute(childNameStr, childDataTypeStr, childArrayTypeStr, portMode, true, *value, stat);
              if(child.isNull())
                return newAttribute;
              
              children.append(child);
              continue;
            }
          }

          // we assume it's a nested compound param
          MObject child = addMayaAttribute(childNameStr, "CompoundParam", "Single Value", portMode, true, *value, stat);
          if(child.isNull())
            return newAttribute;
          children.append(child);
        }
      }

      /// now create the compound attribute
      newAttribute = cAttr.create(portName, portName);
      if(arrayType == "Array (Multi)")
      {
        cAttr.setArray(true);
        cAttr.setUsesArrayDataBuilder(true);
      }
      for(unsigned int i=0;i<children.length();i++)
      {
        cAttr.addChild(children[i]);
      }
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Boolean")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = nAttr.create(portName, portName, MFnNumericData::kBoolean);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.create(portName, portName, MFnNumericData::kBoolean);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Integer")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = nAttr.create(portName, portName, MFnNumericData::kInt);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.create(portName, portName, MFnNumericData::kInt);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else if(arrayType == "Array (Native)")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kIntArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }

    float uiMin = getScalarOption("uiMin", compoundStructure);
    float uiMax = getScalarOption("uiMax", compoundStructure);
    if(uiMin < uiMax) 
    {
      nAttr.setMin(uiMin);
      nAttr.setMax(uiMax);
      float uiSoftMin = getScalarOption("uiSoftMin", compoundStructure);
      float uiSoftMax = getScalarOption("uiSoftMax", compoundStructure);
      if(uiSoftMin < uiSoftMax) 
      {
        nAttr.setSoftMin(uiSoftMin);
        nAttr.setSoftMax(uiSoftMax);
      }
      else
      {
        nAttr.setSoftMin(uiMin);
        nAttr.setSoftMax(uiMax);
      }
    }
  }
  else if(dataTypeOverride == "Scalar")
  {
    bool isUnitAttr = true;
    std::string scalarUnit = getStringOption("scalarUnit", compoundStructure);
    if(arrayType == "Single Value" || arrayType == "Array (Multi)")
    {
      if(scalarUnit == "time")
        newAttribute = uAttr.create(portName, portName, MFnUnitAttribute::kTime);
      else if(scalarUnit == "angle")
        newAttribute = uAttr.create(portName, portName, MFnUnitAttribute::kAngle);
      else if(scalarUnit == "distance")
        newAttribute = uAttr.create(portName, portName, MFnUnitAttribute::kDistance);
      else
      {
        newAttribute = nAttr.create(portName, portName, MFnNumericData::kDouble);
        isUnitAttr = false;
      }

      if(arrayType == "Array (Multi)") 
      {
        if(isUnitAttr)
        {
          uAttr.setArray(true);
          uAttr.setUsesArrayDataBuilder(true);
        }
        else
        {
          nAttr.setArray(true);
          nAttr.setUsesArrayDataBuilder(true);
        }
      }
    }
    else if(arrayType == "Array (Native)")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kDoubleArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }

    float uiMin = getScalarOption("uiMin", compoundStructure);
    float uiMax = getScalarOption("uiMax", compoundStructure);
    if(uiMin < uiMax) 
    {
      if(isUnitAttr)
      {
        uAttr.setMin(uiMin);
        uAttr.setMax(uiMax);
      }
      else
      {
        nAttr.setMin(uiMin);
        nAttr.setMax(uiMax);
      }
      float uiSoftMin = getScalarOption("uiSoftMin", compoundStructure);
      float uiSoftMax = getScalarOption("uiSoftMax", compoundStructure);
      if(isUnitAttr)
      {
        if(uiSoftMin < uiSoftMax) 
        {
          uAttr.setSoftMin(uiSoftMin);
          uAttr.setSoftMax(uiSoftMax);
        }
        else
        {
          uAttr.setSoftMin(uiMin);
          uAttr.setSoftMax(uiMax);
        }
      }
      else
      {
        if(uiSoftMin < uiSoftMax) 
        {
          nAttr.setSoftMin(uiSoftMin);
          nAttr.setSoftMax(uiSoftMax);
        }
        else
        {
          nAttr.setSoftMin(uiMin);
          nAttr.setSoftMax(uiMax);
        }
      }
    }
  }
  else if(dataTypeOverride == "String")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kString, emptyStringObject);
    }
    else if(arrayType == "Array (Multi)"){
      newAttribute = tAttr.create(portName, portName, MFnData::kString, emptyStringObject);
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Color")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = nAttr.createColor(portName, portName);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.createColor(portName, portName);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Vec3")
  {
    if(arrayType == "Single Value")
    {
      MObject x = nAttr.create(portName+"_x", portName+"_x", MFnNumericData::kDouble);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      MObject y = nAttr.create(portName+"_y", portName+"_y", MFnNumericData::kDouble);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      MObject z = nAttr.create(portName+"_z", portName+"_z", MFnNumericData::kDouble);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      newAttribute = cAttr.create(portName, portName);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
    }
    else if(arrayType == "Array (Multi)")
    {
      MObject x = nAttr.create(portName+"_x", portName+"_x", MFnNumericData::kDouble);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      MObject y = nAttr.create(portName+"_y", portName+"_y", MFnNumericData::kDouble);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      MObject z = nAttr.create(portName+"_z", portName+"_z", MFnNumericData::kDouble);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      newAttribute = cAttr.create(portName, portName);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
      cAttr.setArray(true);
      cAttr.setUsesArrayDataBuilder(true);
    }
    else if(arrayType == "Array (Native)")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kVectorArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Euler")
  {
    if(arrayType == "Single Value")
    {
      MObject x = uAttr.create(portName+"_x", portName+"_x", MFnUnitAttribute::kAngle);
      uAttr.setStorable(true);
      uAttr.setKeyable(true);
      MObject y = uAttr.create(portName+"_y", portName+"_y", MFnUnitAttribute::kAngle);
      uAttr.setStorable(true);
      uAttr.setKeyable(true);
      MObject z = uAttr.create(portName+"_z", portName+"_z", MFnUnitAttribute::kAngle);
      uAttr.setStorable(true);
      uAttr.setKeyable(true);
      newAttribute = cAttr.create(portName, portName);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
    }
    else if(arrayType == "Array (Multi)")
    {
      MObject x = uAttr.create(portName+"_x", portName+"_x", MFnUnitAttribute::kAngle);
      uAttr.setStorable(true);
      MObject y = uAttr.create(portName+"_y", portName+"_y", MFnUnitAttribute::kAngle);
      uAttr.setStorable(true);
      MObject z = uAttr.create(portName+"_z", portName+"_z", MFnUnitAttribute::kAngle);
      uAttr.setStorable(true);
      newAttribute = cAttr.create(portName, portName);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
      cAttr.setArray(true);
      cAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Mat44")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = mAttr.create(portName, portName);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = mAttr.create(portName, portName);
      mAttr.setArray(true);
      mAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "PolygonMesh")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kMesh);
      storable = false;
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kMesh);
      storable = false;
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Lines")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kNurbsCurve);
      storable = false;
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = tAttr.create(portName, portName, MFnData::kNurbsCurve);
      storable = false;
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "KeyframeTrack"){
    
    if(arrayType == "Single Value")
    {
      if(_spliceGraph.getDGPort(portName.asChar()).isValid()){
        newAttribute = pAttr.create(portName, portName);
        pAttr.setStorable(true);
        pAttr.setKeyable(true);
        pAttr.setCached(false);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
    else
    {
      if(_spliceGraph.getDGPort(portName.asChar()).isValid()){
        newAttribute = pAttr.create(portName, portName);
        pAttr.setStorable(true);
        pAttr.setKeyable(true);
        pAttr.setArray(true);
        pAttr.setCached(false);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
  }
  else if(dataTypeOverride == "SpliceMayaData"){
    
    if(arrayType == "Single Value")
    {
      if(_spliceGraph.getDGPort(portName.asChar()).isValid()){
        newAttribute = tAttr.create(portName, portName, FabricSpliceMayaData::id);
        mSpliceMayaDataOverride.push_back(portName.asChar());
        storable = false;
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
    else
    {
      if(_spliceGraph.getDGPort(portName.asChar()).isValid()){
        newAttribute = tAttr.create(portName, portName, FabricSpliceMayaData::id);
        mSpliceMayaDataOverride.push_back(portName.asChar());
        storable = false;
        tAttr.setArray(true);
        tAttr.setUsesArrayDataBuilder(true);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
  }
  else
  {
    mayaLogErrorFunc("DataType '"+dataType+"' not supported.");
    return newAttribute;
  }

  // set the mode
  if(!newAttribute.isNull())
  {
    if(portMode != FabricSplice::Port_Mode_IN)
    {
      nAttr.setReadable(true);
      tAttr.setReadable(true);
      mAttr.setReadable(true);
      uAttr.setReadable(true);
      cAttr.setReadable(true);
      pAttr.setReadable(true);
    }
    if(portMode != FabricSplice::Port_Mode_OUT)
    {
      nAttr.setWritable(true);
      tAttr.setWritable(true);
      mAttr.setWritable(true);
      uAttr.setWritable(true);
      cAttr.setWritable(true);
      pAttr.setWritable(true);
    }
    if(portMode == FabricSplice::Port_Mode_IN && storable)
    {
      nAttr.setKeyable(true);
      tAttr.setKeyable(true);
      mAttr.setKeyable(true);
      uAttr.setKeyable(true);
      cAttr.setKeyable(true);
      pAttr.setKeyable(true);

      nAttr.setStorable(true);
      tAttr.setStorable(true);
      mAttr.setStorable(true);
      uAttr.setStorable(true);
      cAttr.setStorable(true);
      pAttr.setStorable(true);
    }

    if(!compoundChild)
      thisNode.addAttribute(newAttribute);
  }

  if(!compoundChild)
    setupMayaAttributeAffects(portName, portMode, newAttribute);

  return newAttribute;

  MAYASPLICE_CATCH_END(stat);

  return MObject();
}

void FabricSpliceBaseInterface::setupMayaAttributeAffects(MString portName, FabricSplice::Port_Mode portMode, MObject newAttribute, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::setupMayaAttributeAffects()");

  MFnDependencyNode thisNode(getThisMObject());
  MPxNode * userNode = thisNode.userNode();
  if(userNode != NULL)
  {
    if(portMode != FabricSplice::Port_Mode_IN)
    {
      for(int i = 0; i < _spliceGraph.getDGPortCount(); ++i) {
        std::string otherPortName = _spliceGraph.getDGPortName(i);
        if(otherPortName == portName.asChar() && portMode != FabricSplice::Port_Mode_IO)
          continue;
        FabricSplice::DGPort otherPort = _spliceGraph.getDGPort(otherPortName.c_str());
        if(!otherPort.isValid())
          continue;
        if(otherPort.getMode() != FabricSplice::Port_Mode_IN)
          continue;
        MPlug plug = thisNode.findPlug(otherPortName.c_str());
        if(plug.isNull())
          continue;
        userNode->attributeAffects(plug.attribute(), newAttribute);
      }

      MPlug evalIDPlug = thisNode.findPlug("evalID");
      if(!evalIDPlug.isNull())
        userNode->attributeAffects(evalIDPlug.attribute(), newAttribute);
    }
    else
    {
      for(int i = 0; i < _spliceGraph.getDGPortCount(); ++i) {
        std::string otherPortName = _spliceGraph.getDGPortName(i);
        if(otherPortName == portName.asChar() && portMode != FabricSplice::Port_Mode_IO)
          continue;
        FabricSplice::DGPort otherPort = _spliceGraph.getDGPort(otherPortName.c_str());
        if(!otherPort.isValid())
          continue;
        if(otherPort.getMode() == FabricSplice::Port_Mode_IN)
          continue;
        MPlug plug = thisNode.findPlug(otherPortName.c_str());
        if(plug.isNull())
          continue;
        userNode->attributeAffects(newAttribute, plug.attribute());
      }
    }
  }
  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::addPort(const MString &portName, const MString &dataType, const FabricSplice::Port_Mode &portMode, const MString & dgNode, bool autoInitObjects, const MString & extension, const FabricCore::Variant & defaultValue, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::addPort()");

  _spliceGraph.addDGNodeMember(portName.asChar(), dataType.asChar(), defaultValue, dgNode.asChar(), extension.asChar());
  _spliceGraph.addDGPort(portName.asChar(), portName.asChar(), portMode, dgNode.asChar(), autoInitObjects);

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::removeMayaAttribute(const MString &portName, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  MFnDependencyNode thisNode(getThisMObject());
  MPlug plug = thisNode.findPlug(portName);
  if(!plug.isNull())
    thisNode.removeAttribute(plug.attribute());

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::removePort(const MString &portName, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.asChar());
  _spliceGraph.removeDGNodeMember(portName.asChar(), port.getDGNodeName());

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::addKLOperator(const MString &operatorName, const MString &operatorCode, const MString &operatorEntry, const MString &dgNode, const FabricCore::Variant & portMap, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::addKLOperator()");

  _spliceGraph.constructKLOperator(operatorName.asChar(), operatorCode.asChar(), operatorEntry.asChar(), dgNode.asChar(), portMap);
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::setKLOperatorEntry(const MString &operatorName, const MString &operatorEntry, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::setKLOperatorEntry()");

  _spliceGraph.setKLOperatorEntry(operatorName.asChar(), operatorEntry.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::setKLOperatorIndex(const MString &operatorName, unsigned int operatorIndex, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::setKLOperatorIndex()");

  _spliceGraph.setKLOperatorIndex(operatorName.asChar(), operatorIndex);
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::setKLOperatorCode(const MString &operatorName, const MString &operatorCode, const MString &operatorEntry, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::setKLOperatorCode()");

  _spliceGraph.setKLOperatorSourceCode(operatorName.asChar(), operatorCode.asChar(), operatorEntry.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

std::string FabricSpliceBaseInterface::getKLOperatorCode(const MString &operatorName, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  return _spliceGraph.getKLOperatorSourceCode(operatorName.asChar());

  MAYASPLICE_CATCH_END(stat);
  return "";
}

void FabricSpliceBaseInterface::setKLOperatorFile(const MString &operatorName, const MString &filename, const MString &entry, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::setKLOperatorFile()");

  _spliceGraph.setKLOperatorFilePath(operatorName.asChar(), filename.asChar(), entry.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::removeKLOperator(const MString &operatorName, const MString & dgNode, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::removeKLOperator()");

  _spliceGraph.removeKLOperator(operatorName.asChar(), dgNode.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::storePersistenceData(MString file, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::storePersistenceData()");

  MPlug saveDataPlug = getSaveDataPlug();

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(file.asChar());

  FabricCore::Variant dictData = _spliceGraph.getPersistenceDataDict(&info);
  
  saveDataPlug.setString(dictData.getJSONEncoding().getStringData());

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::restoreFromPersistenceData(MString file, MStatus *stat){
  if(_restoredFromPersistenceData)
    return;

  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::restoreFromPersistenceData()");

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(file.asChar());

  MPlug saveDataPlug = getSaveDataPlug();

  FabricCore::Variant dictData = FabricCore::Variant::CreateFromJSON(saveDataPlug.asString().asChar());
  bool dataRestored = _spliceGraph.setFromPersistenceDataDict(dictData, &info);

  if(dataRestored){
    // const FabricCore::Variant * manipulationCommandVar = dictData.getDictValue("manipulationCommand");
    // if(manipulationCommandVar){
    //   std::string manipCmd = manipulationCommandVar->getStringData();
    //   _manipulationCommand = manipCmd.c_str();
    // }
  }

  _restoredFromPersistenceData = true;

  invalidateNode();

  MFnDependencyNode thisNode(getThisMObject());
  for(int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    std::string portName = _spliceGraph.getDGPortName(i);
    
    MPlug plug = thisNode.findPlug(portName.c_str());
    if(plug.isNull())
      continue;

    FabricSplice::DGPort port= _spliceGraph.getDGPort(portName.c_str());
    if(!port.isValid())
      continue;
    
    FabricSplice::Port_Mode mode = port.getMode();

    // force an execution of the node    
    if(mode != FabricSplice::Port_Mode_OUT)
    {
      MString command("dgeval ");
      MGlobal::executeCommandOnIdle(command+thisNode.name()+"."+portName.c_str());
      break;
    }
  }

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::resetInternalData(MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::resetInternalData()");

  _spliceGraph.clear();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::invalidatePlug(MPlug & plug)
{
  if(!_dgDirtyEnabled)
    return;
  if(plug.isNull())
    return;
  if(plug.attribute().isNull())
    return;

  MString command("dgdirty ");

  // filter plugs containing [-1]
  MString plugName = plug.name();
  MStringArray plugNameParts;
  plugName.split('[', plugNameParts);
  if(plugNameParts.length() > 1)
  {
    if(plugNameParts[1].substring(0, 2) == "-1]")
      return;
  }

  MGlobal::executeCommandOnIdle(command+plugName);
}

void FabricSpliceBaseInterface::invalidateNode()
{
  if(!_dgDirtyEnabled)
    return;
  FabricSplice::Logging::AutoTimer timer("Maya::invalidateNode()");

  MFnDependencyNode thisNode(getThisMObject());

  // ensure we setup the mayaSpliceData overrides first
  mSpliceMayaDataOverride.resize(0);
  for(unsigned int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    FabricSplice::DGPort port = _spliceGraph.getDGPort(i);
    if(!port.isValid())
      continue;
    // if(port.isManipulatable())
    //   continue;
    std::string portName = _spliceGraph.getDGPortName(i);
    MPlug plug = thisNode.findPlug(portName.c_str());
    MObject attrObj = plug.attribute();
    if(attrObj.apiType() == MFn::kTypedAttribute)
    {
      MFnTypedAttribute attr(attrObj);
      MFnData::Type type = attr.attrType();
      if(type == MFnData::kPlugin || type == MFnData::kInvalid)
        mSpliceMayaDataOverride.push_back(port.getName());
    }
  }

  // ensure that the node is invalidated
  for(int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    std::string portName = _spliceGraph.getDGPortName(i);
    
    MPlug plug = thisNode.findPlug(portName.c_str());

    FabricSplice::DGPort port= _spliceGraph.getDGPort(portName.c_str());
    if(!port.isValid())
      continue;
    
    FabricSplice::Port_Mode mode = port.getMode();
    
    if(!plug.isNull()){
      if(mode == FabricSplice::Port_Mode_IN)
      {
        collectDirtyPlug(plug);
        MPlugArray plugs;
        plug.connectedTo(plugs,true,false);
        for(int j=0;j<plugs.length();j++)
          invalidatePlug(plugs[j]);
      }
      else
      {
        invalidatePlug(plug);

        MPlugArray plugs;
        affectChildPlugs(plug, plugs);
        for(int j=0;j<plugs.length();j++)
          invalidatePlug(plugs[j]);
      }
    }
  }
}

void FabricSpliceBaseInterface::incEvalID(){
  MFnDependencyNode thisNode(getThisMObject());
  MPlug evalIDPlug = thisNode.findPlug("evalID");
  if(!evalIDPlug.isNull())
    evalIDPlug.setInt((evalIDPlug.asInt() + 1) % 1000);
}

MStringArray FabricSpliceBaseInterface::getKLOperatorNames(){
  MStringArray names;
  for(unsigned int i=0;i<_spliceGraph.getDGNodeCount();i++)
  {
    MString dgNode = _spliceGraph.getDGNodeName(i);
    for(unsigned int j = 0; j < _spliceGraph.getKLOperatorCount(dgNode.asChar()); ++j)
    {
      MString opName = _spliceGraph.getKLOperatorName(j, dgNode.asChar());
      names.append(dgNode + " - " + opName);
    }
  }

  return names;
}

MStringArray FabricSpliceBaseInterface::getPortNames(){
  MStringArray ports;
  for(unsigned int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    ports.append(MString(_spliceGraph.getDGPortName(i)));
  }

  return ports;
}

FabricSplice::DGPort FabricSpliceBaseInterface::getPort(MString name)
{
  return _spliceGraph.getDGPort(name.asChar());
}

void FabricSpliceBaseInterface::saveToFile(MString fileName)
{
  FabricSplice::Logging::AutoTimer timer("Maya::saveToFile()");

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(fileName.asChar());

  _spliceGraph.saveToFile(fileName.asChar(), &info);
}

MStatus FabricSpliceBaseInterface::loadFromFile(MString fileName)
{
  MStatus loadStatus;
  MAYASPLICE_CATCH_BEGIN(&loadStatus);

  FabricSplice::Logging::AutoTimer timer("Maya::loadFromFile()");

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(fileName.asChar());

  _spliceGraph.loadFromFile(fileName.asChar());

  // create all relevant maya attributes
  for(int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    std::string portName = _spliceGraph.getDGPortName(i);
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.c_str());
    if(!port.isValid())
      continue;

    MString dataType = port.getDataType();
    if(getSplicePlugToPortFunc(dataType.asChar(), &port) == NULL &&
       getSplicePortToPlugFunc(dataType.asChar(), &port) == NULL)
      continue;

    bool isArray = port.isArray();
    FabricSplice::Port_Mode portMode = port.getMode();

    MString arrayType = "Single Value";
    if(isArray)
      arrayType = "Array (Multi)";

    if(port.hasOption("opaque")) {
      if(port.getOption("opaque").getBoolean())
        dataType = "SpliceMayaData";
    }

    bool addMayaAttr = true;
    if(port.hasOption("internal")) {
      if(port.getOption("internal").getBoolean())
        addMayaAttr = false;
    }

    if(port.hasOption("nativeArray")) {
      if(port.getOption("nativeArray").getBoolean())
        arrayType = "Array (Native)";
    }

    FabricCore::Variant compoundStructure;
    if(port.hasOption("compoundStructure")) {
      compoundStructure = port.getOption("compoundStructure");
    }

    if(addMayaAttr)
    {
      MStatus portStatus;
      addMayaAttribute(portName.c_str(), dataType, arrayType, portMode, false, compoundStructure, &portStatus);
      if(portStatus != MS::kSuccess)
        return portStatus;

      if(portMode != FabricSplice::Port_Mode_OUT)
      {
        MFnDependencyNode thisNode(getThisMObject());
        MPlug plug = thisNode.findPlug(portName.c_str());
        if(!plug.isNull())
        {
          FabricCore::Variant variant = port.getDefault();
          if(variant.isString())
            plug.setString(variant.getStringData());
          else if(variant.isBoolean())
            plug.setBool(variant.getBoolean());
          else if(variant.isNull())
            continue;
          else if(variant.isArray())
            continue;
          else if(variant.isDict())
            continue;
          else
          {
            float value = 0.0;
            if(variant.isSInt8())
              value = (float)variant.getSInt8();
            else if(variant.isSInt16())
              value = (float)variant.getSInt16();
            else if(variant.isSInt32())
              value = (float)variant.getSInt32();
            else if(variant.isSInt64())
              value = (float)variant.getSInt64();
            else if(variant.isUInt8())
              value = (float)variant.getUInt8();
            else if(variant.isUInt16())
              value = (float)variant.getUInt16();
            else if(variant.isUInt32())
              value = (float)variant.getUInt32();
            else if(variant.isUInt64())
              value = (float)variant.getUInt64();
            else if(variant.isFloat32())
              value = (float)variant.getFloat32();
            else if(variant.isFloat64())
              value = (float)variant.getFloat64();
            MDataHandle handle = plug.asMDataHandle();
            if(handle.numericType() == MFnNumericData::kFloat)
              plug.setFloat(value);
            else if(handle.numericType() == MFnNumericData::kDouble)
              plug.setDouble(value);
            else if(handle.numericType() == MFnNumericData::kInt)
              plug.setInt((int)value);
          }
        }
      }
    }
  }

  invalidateNode();

  MAYASPLICE_CATCH_END(&loadStatus);
  return loadStatus;
}

bool FabricSpliceBaseInterface::plugInArray(const MPlug &plug, const MPlugArray &array){
  bool found = false;
  for(int i = 0; i < array.length(); ++i){
    if(array[i] == plug){
      found = true;
      break;
    }
  }

  return found;
}

void FabricSpliceBaseInterface::setDependentsDirty(MObject thisMObject, MPlug const &inPlug, MPlugArray &affectedPlugs){

  MFnDependencyNode thisNode(thisMObject);

  // we can't ask for the plug value here, so we fill an array for the compute to only transfer newly dirtied values
  collectDirtyPlug(inPlug);

  for(unsigned int i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    FabricSplice::DGPort port = _spliceGraph.getDGPort(i);
    if(!port.isValid())
      continue;
    int portMode = (int)port.getMode();
    
    if(port.getMode() != FabricSplice::Port_Mode_IN){
      MPlug outPlug = thisNode.findPlug(port.getName());
      if(!outPlug.isNull()){
        if(!plugInArray(outPlug, affectedPlugs)){
          affectedPlugs.append(outPlug);
          affectChildPlugs(outPlug, affectedPlugs);
        }
      }
    }
  }
}

void FabricSpliceBaseInterface::copyInternalData(MPxNode *node){
  FabricSpliceBaseInterface *otherSpliceInterface = getInstanceByName(node->name().asChar());

  std::string jsonData = otherSpliceInterface->_spliceGraph.getPersistenceDataJSON();
  _spliceGraph.setFromPersistenceDataJSON(jsonData.c_str());
}

void FabricSpliceBaseInterface::setPortPersistence(const MString &portName, bool persistence){
  _spliceGraph.setMemberPersistence(portName.asChar(), persistence);
}

void FabricSpliceBaseInterface::onNodeAdded(MObject &node, void *clientData)
{
  MFnDependencyNode thisNode(node);
  FabricSpliceBaseInterface * interf = getInstanceByName(thisNode.name().asChar());
  if(interf)
    interf->managePortObjectValues(false); // reattach
}

void FabricSpliceBaseInterface::onNodeRemoved(MObject &node, void *clientData)
{
  MFnDependencyNode thisNode(node);
  FabricSpliceBaseInterface * interf = getInstanceByName(thisNode.name().asChar());
  if(interf)
    interf->managePortObjectValues(true); // detach
}

void FabricSpliceBaseInterface::managePortObjectValues(bool destroy)
{
  if(_portObjectsDestroyed == destroy)
    return;

  for(unsigned int i = 0; i < _spliceGraph.getDGPortCount(); ++i) {
    FabricSplice::DGPort port = _spliceGraph.getDGPort(i);
    if(!port.isValid())
      continue;
    if(!port.isObject())
      continue;

    try
    {
      FabricCore::RTVal value = port.getRTVal();
      if(!value.isValid())
        continue;
      if(value.isNullObject())
        continue;

      FabricCore::RTVal detachable = FabricSplice::constructObjectRTVal("Detachable", 1, &value);
      if(detachable.isNullObject())
        continue;

      if(destroy)
        detachable.callMethod("", "detach", 0, 0);
      else
        detachable.callMethod("", "attach", 0, 0);
    }
    catch(FabricCore::Exception e)
    {
      // ignore errors, probably an object which does not implement deattach and attach
    }
  }

  _portObjectsDestroyed = destroy;
}
