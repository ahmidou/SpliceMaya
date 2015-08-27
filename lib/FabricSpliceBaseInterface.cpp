
#include "FabricMayaAttrs.h"
#include "FabricSpliceEditorWidget.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceMayaData.h"
#include "FabricSpliceHelpers.h"

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#if _SPLICE_MAYA_VERSION >= 2016
#include <maya/MEvaluationManager.h>
#endif
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

#if _SPLICE_MAYA_VERSION >= 2016
# include <maya/MEvaluationNode.h>
#endif

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
  _affectedPlugsDirty = true;
  _outputsDirtied = false;

  FabricSplice::setDCCOperatorSourceCodeCallback(&FabricSpliceEditorWidget::getSourceCodeForOperator);

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

  FabricSplice::Logging::AutoTimer globalTimer("Maya::FabricSpliceBaseInterface()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::FabricSpliceBaseInterface()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph = FabricSplice::DGGraph("MayaGraph");
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

bool FabricSpliceBaseInterface::transferInputValuesToSplice(
  MDataBlock& data
  )
{
  if(_isTransferingInputs)
    return false;

  managePortObjectValues(false); // recreate objects if not there yet

  FabricSplice::Logging::AutoTimer globalTimer("Maya::transferInputValuesToSplice()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::transferInputValuesToSplice()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _isTransferingInputs = true;

  MFnDependencyNode thisNode(getThisMObject());

  for(size_t i = 0; i < _dirtyPlugs.length(); ++i){
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
        {
          (*func)(plug, data, port);
        }
      }
    }
  }

  _dirtyPlugs.clear();
  _isTransferingInputs = false;
  
  return true;
}

void FabricSpliceBaseInterface::evaluate()
{
  MFnDependencyNode thisNode(getThisMObject());
  // printf( "evaluate %s\n", thisNode.name().asChar() );

  FabricSplice::Logging::AutoTimer globalTimer("Maya::evaluate()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::evaluate()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());
  managePortObjectValues(false); // recreate objects if not there yet

  if(_spliceGraph.usesEvalContext())
  {
    // setup the context
    FabricCore::RTVal context = _spliceGraph.getEvalContext();
    context.setMember("host", FabricSplice::constructStringRTVal("Maya"));
    context.setMember("graph", FabricSplice::constructStringRTVal(thisNode.name().asChar()));
    context.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));
    context.setMember("currentFilePath", FabricSplice::constructStringRTVal(mayaGetLastLoadedScene().asChar()));

    if(_evalContextPlugNames.length() > 0)
    {
      for(unsigned int i=0;i<_evalContextPlugNames.length();i++)
      {
        MString name = _evalContextPlugNames[i];
        MString portName = name;
        int periodPos = portName.index('.');
        if(periodPos > 0)
          portName = portName.substring(0, periodPos-1);
        FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.asChar());
        if(port.isValid()){
          if(port.getMode() != FabricSplice::Port_Mode_OUT)
          {
            std::vector<FabricCore::RTVal> args(1);
            args[0] = FabricSplice::constructStringRTVal(name.asChar());
            if(_evalContextPlugIds[i] >= 0)
              args.push_back(FabricSplice::constructSInt32RTVal(_evalContextPlugIds[i]));
            context.callMethod("", "_addDirtyInput", args.size(), &args[0]);
          }
        }
      }
    
      _evalContextPlugNames.clear();
      _evalContextPlugIds.clear();
    }
  }

  _spliceGraph.evaluate();
}

void FabricSpliceBaseInterface::transferOutputValuesToMaya(MDataBlock& data, bool isDeformer){
  if(_isTransferingInputs)
    return;

  managePortObjectValues(false); // recreate objects if not there yet

  FabricSplice::Logging::AutoTimer globalTimer("Maya::transferOutputValuesToMaya()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::transferOutputValuesToMaya()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());
  
  MFnDependencyNode thisNode(getThisMObject());

  for(size_t i = 0; i < _spliceGraph.getDGPortCount(); ++i){
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
            (*func)(port, plug, data);
            data.setClean(plug);
          }
        }
      }
    }
  }
}

void FabricSpliceBaseInterface::collectDirtyPlug(MPlug const &inPlug)
{
  FabricSplice::Logging::AutoTimer globalTimer("Maya::collectDirtyPlug()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::collectDirtyPlug()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  MStatus stat;
  MString name;

  name = inPlug.name();
  int periodPos = name.rindex('.');
  if(periodPos > -1)
    name = name.substring(periodPos+1, name.length()-1);
  int bracketPos = name.index('[');
  if(bracketPos > -1)
    name = name.substring(0, bracketPos-1);

  if(_spliceGraph.usesEvalContext())
  {
    _evalContextPlugNames.append(name);
    if(inPlug.isElement())
      _evalContextPlugIds.append(inPlug.logicalIndex());
    else
      _evalContextPlugIds.append(-1);
  }

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

  for(size_t i = 0; i < _dirtyPlugs.length(); ++i){
    if(_dirtyPlugs[i] == name)
      return;
  }

  _dirtyPlugs.append(name);
}

void FabricSpliceBaseInterface::affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs){
  if(plug.isNull()){
    return;
  }

  for(size_t i = 0; i < plug.numChildren(); ++i){
    MPlug childPlug = plug.child(i);
    if(!childPlug.isNull()){
      affectedPlugs.append(childPlug);
      affectChildPlugs(childPlug, affectedPlugs);
    }
  }

  for(size_t i = 0; i < plug.numElements(); ++i){
    MPlug elementPlug = plug.elementByPhysicalIndex(i);
    if(!elementPlug.isNull()){
      affectedPlugs.append(elementPlug);
      affectChildPlugs(elementPlug, affectedPlugs);
    }
  }
}

float getScalarOption(const char * key, FabricCore::Variant options, float value) {
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

std::string getStringOption(const char * key, FabricCore::Variant options, std::string value) {
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

MObject FabricSpliceBaseInterface::addMayaAttribute(
  const MString &portName,
  const MString &dataType,
  const MString &arrayType,
  const FabricSplice::Port_Mode &portMode,
  bool compoundChild,
  FabricCore::Variant compoundStructure,
  MStatus *stat
  )
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::addMayaAttribute()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::addMayaAttribute()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  MFnDependencyNode thisNode( getThisMObject() );
  MPlug existingPlug = thisNode.findPlug( portName );
  if ( !existingPlug.isNull() )
  {
    std::string error;
    error += "Attribute '";
    error += portName.asChar();
    error += "' already exists on node '";
    error += thisNode.name().asChar();
    error += "'";
    throw FabricCore::Exception( error.c_str() );
  }

  MObject obj =
    FabricMaya::CreateMayaAttribute(
      portName.asChar(),
      FabricMaya::ParseDataType( dataType.asChar() ),
      dataType.asChar(),
      FabricMaya::ParseArrayType( arrayType.asChar() ),
      arrayType.asChar(),
      portMode != FabricSplice::Port_Mode_OUT, // isInput
      portMode != FabricSplice::Port_Mode_IN, // isOutput
      compoundStructure
      );

  thisNode.addAttribute(obj);

  setupMayaAttributeAffects(portName, portMode, obj);

  _affectedPlugsDirty = true;
  return obj;

  MAYASPLICE_CATCH_END(stat);

  return MObject();
}

void FabricSpliceBaseInterface::setupMayaAttributeAffects(MString portName, FabricSplice::Port_Mode portMode, MObject newAttribute, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::setupMayaAttributeAffects()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::setupMayaAttributeAffects()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  // MFnDependencyNode thisNode(getThisMObject());
  // MPxNode * userNode = thisNode.userNode();
  // if(userNode != NULL)
  // {
  //   if(portMode != FabricSplice::Port_Mode_IN)
  //   {
  //     for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i) {
  //       std::string otherPortName = _spliceGraph.getDGPortName(i);
  //       if(otherPortName == portName.asChar() && portMode != FabricSplice::Port_Mode_IO)
  //         continue;
  //       FabricSplice::DGPort otherPort = _spliceGraph.getDGPort(otherPortName.c_str());
  //       if(!otherPort.isValid())
  //         continue;
  //       if(otherPort.getMode() != FabricSplice::Port_Mode_IN)
  //         continue;
  //       MPlug plug = thisNode.findPlug(otherPortName.c_str());
  //       if(plug.isNull())
  //         continue;
  //       userNode->attributeAffects(plug.attribute(), newAttribute);
  //     }

  //     MPlug evalIDPlug = thisNode.findPlug("evalID");
  //     if(!evalIDPlug.isNull())
  //       userNode->attributeAffects(evalIDPlug.attribute(), newAttribute);
  //   }
  //   else
  //   {
  //     for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i) {
  //       std::string otherPortName = _spliceGraph.getDGPortName(i);
  //       if(otherPortName == portName.asChar() && portMode != FabricSplice::Port_Mode_IO)
  //         continue;
  //       FabricSplice::DGPort otherPort = _spliceGraph.getDGPort(otherPortName.c_str());
  //       if(!otherPort.isValid())
  //         continue;
  //       if(otherPort.getMode() == FabricSplice::Port_Mode_IN)
  //         continue;
  //       MPlug plug = thisNode.findPlug(otherPortName.c_str());
  //       if(plug.isNull())
  //         continue;
  //       userNode->attributeAffects(newAttribute, plug.attribute());
  //     }
  //   }
  // }
  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::addPort(const MString &portName, const MString &dataType, const FabricSplice::Port_Mode &portMode, const MString & dgNode, bool autoInitObjects, const MString & extension, const FabricCore::Variant & defaultValue, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::addPort()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::addPort()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph.addDGNodeMember(portName.asChar(), dataType.asChar(), defaultValue, dgNode.asChar(), extension.asChar());
  _spliceGraph.addDGPort(portName.asChar(), portName.asChar(), portMode, dgNode.asChar(), autoInitObjects);
  _affectedPlugsDirty = true;

  // initialize the compound param
  if(dataType == "CompoundParam")
  {
    MFnDependencyNode thisNode(getThisMObject());
    MPlug plug = thisNode.findPlug(portName);
    if(!plug.isNull())
      _dirtyPlugs.append(portName);
  }

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::removeMayaAttribute(const MString &portName, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  MFnDependencyNode thisNode(getThisMObject());
  MPlug plug = thisNode.findPlug(portName);
  if(!plug.isNull())
  {
    MString command = "deleteAttr "+thisNode.name()+"."+portName;
    MGlobal::executeCommandOnIdle(command); 
    // in Maya 2015 this is causing a crash in Qt due to a bug in Maya.
    // thisNode.removeAttribute(plug.attribute());
    _affectedPlugsDirty = true;
  }

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::removePort(const MString &portName, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.asChar());
  _spliceGraph.removeDGNodeMember(portName.asChar(), port.getDGNodeName());
  _affectedPlugsDirty = true;

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::addKLOperator(const MString &operatorName, const MString &operatorCode, const MString &operatorEntry, const MString &dgNode, const FabricCore::Variant & portMap, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::addKLOperator()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::addKLOperator()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph.constructKLOperator(operatorName.asChar(), operatorCode.asChar(), operatorEntry.asChar(), dgNode.asChar(), portMap);
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::setKLOperatorEntry(const MString &operatorName, const MString &operatorEntry, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::setKLOperatorEntry()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::setKLOperatorEntry()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph.setKLOperatorEntry(operatorName.asChar(), operatorEntry.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::setKLOperatorIndex(const MString &operatorName, unsigned int operatorIndex, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::setKLOperatorIndex()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::setKLOperatorIndex()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph.setKLOperatorIndex(operatorName.asChar(), operatorIndex);
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::setKLOperatorCode(const MString &operatorName, const MString &operatorCode, const MString &operatorEntry, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::setKLOperatorCode()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::setKLOperatorCode()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

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

  FabricSplice::Logging::AutoTimer globalTimer("Maya::setKLOperatorFile()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::setKLOperatorFile()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph.setKLOperatorFilePath(operatorName.asChar(), filename.asChar(), entry.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::removeKLOperator(const MString &operatorName, const MString & dgNode, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::removeKLOperator()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::removeKLOperator()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  _spliceGraph.removeKLOperator(operatorName.asChar(), dgNode.asChar());
  invalidateNode();

  MAYASPLICE_CATCH_END(stat);
}

void FabricSpliceBaseInterface::storePersistenceData(MString file, MStatus *stat){
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::storePersistenceData()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::storePersistenceData()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

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

  FabricSplice::Logging::AutoTimer globalTimer("Maya::restoreFromPersistenceData()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::restoreFromPersistenceData()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(file.asChar());

  MPlug saveDataPlug = getSaveDataPlug();

  std::string dictString = saveDataPlug.asString().asChar();
  if(dictString == "" || dictString == "null")
  {
    MFnDependencyNode thisNode(getThisMObject());
    MString message = "The persistance data for Splice on '";
    message += thisNode.name();
    message += "' is corrupt.";
    mayaLogErrorFunc(message);
    return;
  }
  FabricCore::Variant dictData = FabricCore::Variant::CreateFromJSON(dictString);
  bool dataRestored = _spliceGraph.setFromPersistenceDataDict(dictData, &info, file.asChar());

  if(dataRestored){
    // const FabricCore::Variant * manipulationCommandVar = dictData.getDictValue("manipulationCommand");
    // if(manipulationCommandVar){
    //   std::string manipCmd = manipulationCommandVar->getStringData();
    //   _manipulationCommand = manipCmd.c_str();
    // }
  }

  _restoredFromPersistenceData = true;

  // update all attributes, and eventually add new ones!
  if(_spliceGraph.isReferenced())
  {
    MFnDependencyNode thisNode(getThisMObject());
    for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i){
      std::string portName = _spliceGraph.getDGPortName(i);
      FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.c_str());

      MPlug plug = thisNode.findPlug(portName.c_str());
      if(plug.isNull())
      {
        createAttributeForPort(port);
      }
    }
  }

  invalidateNode();

  MFnDependencyNode thisNode(getThisMObject());
  for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i){
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

  FabricSplice::Logging::AutoTimer globalTimer("Maya::resetInternalData()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::resetInternalData()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

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

  if(plugName.index('.') > -1)
  {
    plugName = plugName.substring(plugName.index('.')+1, plugName.length());
  	if(plugName.index('[') > -1)
	   plugName = plugName.substring(0, plugName.index('[')-1);

    FabricSplice::DGPort port = getPort(plugName.asChar());
    if(port.isValid())
    {
      std::string dataType = port.getDataType();
      if(dataType.substr(0, 8) == "Compound")
      {
        // ensure to set the attribute values one more time
        // to guarantee that the values are reflected within KL
        MStringArray cmds;
        plug.getSetAttrCmds(cmds);
        for(unsigned int i=0;i<cmds.length();i++)
        {
          // strip
          while(cmds[i].asChar()[0] == ' ' || cmds[i].asChar()[0] == '\t')
            cmds[i] = cmds[i].substring(1, cmds[i].length());

          // ensure to only use direct setAttr's
          if(cmds[i].substring(0, 9) == "setAttr \".")
          {
            MFnDependencyNode node(plug.node());
            MString cmdPlugName = cmds[i].substring(9, cmds[i].length());
            cmdPlugName = cmdPlugName.substring(0, cmdPlugName.index('"')-1);
            MString condition = "if(size(`listConnections -d no \"" + node.name() + cmdPlugName +"\"`) == 0)";
            cmds[i] = condition + "{ setAttr \"" + node.name() + cmds[i].substring(9, cmds[i].length()) + " }";
            MGlobal::executeCommandOnIdle(cmds[i]);
          }
        }
      }
    }
  }
}

void FabricSpliceBaseInterface::invalidateNode()
{
  if(!_dgDirtyEnabled)
    return;
  FabricSplice::Logging::AutoTimer globalTimer("Maya::invalidateNode()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::invalidateNode()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

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
  for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i){
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
        for(uint32_t j=0;j<plugs.length();j++)
          invalidatePlug(plugs[j]);
      }
      else
      {
        invalidatePlug(plug);
        if(mode == FabricSplice::Port_Mode_IO)
          collectDirtyPlug(plug);

        MPlugArray plugs;
        affectChildPlugs(plug, plugs);
        for(uint32_t j=0;j<plugs.length();j++)
          invalidatePlug(plugs[j]);
      }
    }
  }
  _affectedPlugsDirty = true;
  _outputsDirtied = false;
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
  FabricSplice::Logging::AutoTimer globalTimer("Maya::saveToFile()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::saveToFile()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(fileName.asChar());

  _spliceGraph.saveToFile(fileName.asChar(), &info);
}

MStatus FabricSpliceBaseInterface::loadFromFile(MString fileName, bool asReferenced)
{
  MStatus loadStatus;
  MAYASPLICE_CATCH_BEGIN(&loadStatus);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::loadFromFile()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::loadFromFile()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(fileName.asChar());

  _spliceGraph.loadFromFile(fileName.asChar(), &info, asReferenced);

  // create all relevant maya attributes
  for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    std::string portName = _spliceGraph.getDGPortName(i);
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.c_str());
    if(!port.isValid())
      continue;
    createAttributeForPort(port);
  }

  invalidateNode();

  MAYASPLICE_CATCH_END(&loadStatus);
  return loadStatus;
}

MStatus FabricSpliceBaseInterface::reloadFromFile()
{
  MStatus reloadStatus;
  MAYASPLICE_CATCH_BEGIN(&reloadStatus);

  FabricSplice::Logging::AutoTimer globalTimer("Maya::loadFromFile()");
  std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::loadFromFile()").c_str();
  FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

  FabricSplice::PersistenceInfo info;
  info.hostAppName = FabricCore::Variant::CreateString("Maya");
  info.hostAppVersion = FabricCore::Variant::CreateString(MGlobal::mayaVersion().asChar());
  info.filePath = FabricCore::Variant::CreateString(_spliceGraph.getReferencedFilePath());

  _spliceGraph.reloadFromFile(&info);

  // create all relevant maya attributes
  for(uint32_t i = 0; i < _spliceGraph.getDGPortCount(); ++i){
    std::string portName = _spliceGraph.getDGPortName(i);
    FabricSplice::DGPort port = _spliceGraph.getDGPort(portName.c_str());
    if(!port.isValid())
      continue;
    createAttributeForPort(port);
  }

  invalidateNode();

  MAYASPLICE_CATCH_END(&reloadStatus);
  return reloadStatus;
}

float createAttributeForPort_getFloatFromVariant(const FabricCore::Variant * variant)
{
  float value = 0.0;
  if(variant->isSInt8())
    value = (float)variant->getSInt8();
  else if(variant->isSInt16())
    value = (float)variant->getSInt16();
  else if(variant->isSInt32())
    value = (float)variant->getSInt32();
  else if(variant->isSInt64())
    value = (float)variant->getSInt64();
  else if(variant->isUInt8())
    value = (float)variant->getUInt8();
  else if(variant->isUInt16())
    value = (float)variant->getUInt16();
  else if(variant->isUInt32())
    value = (float)variant->getUInt32();
  else if(variant->isUInt64())
    value = (float)variant->getUInt64();
  else if(variant->isFloat32())
    value = (float)variant->getFloat32();
  else if(variant->isFloat64())
    value = (float)variant->getFloat64();
  return value;  
}

void createAttributeForPort_setFloatOnPlug(MPlug & plug, float value)
{
  MDataHandle handle = plug.asMDataHandle();
  if(handle.numericType() == MFnNumericData::kFloat)
    plug.setFloat(value);
  else if(handle.numericType() == MFnNumericData::kDouble)
    plug.setDouble(value);
  else if(handle.numericType() == MFnNumericData::kInt)
    plug.setInt((int)value);
}

MStatus FabricSpliceBaseInterface::createAttributeForPort(FabricSplice::DGPort port)
{
  MStatus portStatus;
  MAYASPLICE_CATCH_BEGIN(&portStatus);

  std::string portName = port.getName();

  MString dataType = port.getDataType();
  if(port.hasOption("opaque")) {
    if(port.getOption("opaque").getBoolean())
      dataType = "SpliceMayaData";
  }

  bool isArray = port.isArray();
  FabricSplice::Port_Mode portMode = port.getMode();

  MString arrayType = "Single Value";
  if(isArray)
    arrayType = "Array (Multi)";

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
    MFnDependencyNode thisNode(getThisMObject());
    MPlug plug = thisNode.findPlug(portName.c_str());
    if(!plug.isNull())
      return portStatus;

    MStatus attributeStatus;
    addMayaAttribute(portName.c_str(), dataType, arrayType, portMode, false, compoundStructure, &attributeStatus);
    if(attributeStatus != MS::kSuccess)
      return attributeStatus;

    if(portMode != FabricSplice::Port_Mode_OUT && !isArray)
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
          return MStatus::kSuccess;
        else if(variant.isArray())
          return MStatus::kSuccess;
        else if(variant.isDict())
        {
          if(dataType == "Vec3")
          {
            MPlug x = plug.child(0);
            MPlug y = plug.child(1);
            MPlug z = plug.child(2);
            if(!x.isNull() && !x.isNull() && !z.isNull())
            {
              const FabricCore::Variant * xVar = variant.getDictValue("x");
              const FabricCore::Variant * yVar = variant.getDictValue("y");
              const FabricCore::Variant * zVar = variant.getDictValue("z");
              if(xVar && yVar && zVar)
              {
                createAttributeForPort_setFloatOnPlug(x, createAttributeForPort_getFloatFromVariant(xVar));
                createAttributeForPort_setFloatOnPlug(y, createAttributeForPort_getFloatFromVariant(yVar));
                createAttributeForPort_setFloatOnPlug(z, createAttributeForPort_getFloatFromVariant(zVar));
              }
            }
          }
          else if(dataType == "Euler")
          {
            MPlug x = plug.child(0);
            MPlug y = plug.child(1);
            MPlug z = plug.child(2);
            if(!x.isNull() && !x.isNull() && !z.isNull())
            {
              const FabricCore::Variant * xVar = variant.getDictValue("x");
              const FabricCore::Variant * yVar = variant.getDictValue("y");
              const FabricCore::Variant * zVar = variant.getDictValue("z");
              if(xVar && yVar && zVar)
              {
                MAngle xangle(createAttributeForPort_getFloatFromVariant(xVar), MAngle::kRadians);
                x.setMAngle(xangle);
                MAngle yangle(createAttributeForPort_getFloatFromVariant(yVar), MAngle::kRadians);
                y.setMAngle(yangle);
                MAngle zangle(createAttributeForPort_getFloatFromVariant(zVar), MAngle::kRadians);
                z.setMAngle(zangle);
              }
            }
          }
          else if(dataType == "Color")
          {
            const FabricCore::Variant * rVar = variant.getDictValue("r");
            const FabricCore::Variant * gVar = variant.getDictValue("g");
            const FabricCore::Variant * bVar = variant.getDictValue("b");
            if(rVar && gVar && bVar)
            {
              MDataHandle handle = plug.asMDataHandle();
              if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
                handle.setMFloatVector(MFloatVector(
                  createAttributeForPort_getFloatFromVariant(rVar),
                  createAttributeForPort_getFloatFromVariant(gVar),
                  createAttributeForPort_getFloatFromVariant(bVar)
                ));
              }else{
                handle.setMVector(MVector(
                  createAttributeForPort_getFloatFromVariant(rVar),
                  createAttributeForPort_getFloatFromVariant(gVar),
                  createAttributeForPort_getFloatFromVariant(bVar)
                ));
              }
            }
          }
        }
        else
        {
          float value = createAttributeForPort_getFloatFromVariant(&variant);
          createAttributeForPort_setFloatOnPlug(plug, value);
        }
      }
    }
  }

  MAYASPLICE_CATCH_END(&portStatus);  

  return portStatus;
}

bool FabricSpliceBaseInterface::plugInArray(const MPlug &plug, const MPlugArray &array){
  bool found = false;
  for(uint32_t i = 0; i < array.length(); ++i){
    if(array[i] == plug){
      found = true;
      break;
    }
  }

  return found;
}

MStatus FabricSpliceBaseInterface::setDependentsDirty(
  MObject thisObject,
  MPlug const &inPlug,
  MPlugArray &outPlugs
  )
{
#if _SPLICE_MAYA_VERSION >= 2016
    bool constructingEvaluationGraph =
      MEvaluationManager::graphConstructionActive();
#else
    static const bool constructingEvaluationGraph = false;
#endif

  // printf(
  //   "setDependentsDirty %s graphConstructionActive=%s\n = ",
  //   inPlug.name().asChar(),
  //   unsigned(constructingEvaluationGraph)
  //   );

  MFnAttribute inAttr( inPlug.attribute() );
  if ( !inAttr.isHidden()
    && inAttr.isDynamic()
    && inAttr.isWritable() )
  {
    MFnDependencyNode thisNode( thisObject );

    FabricSplice::Logging::AutoTimer globalTimer("Maya::setDependentsDirty()");
    std::string localTimerName = (std::string("Maya::")+_spliceGraph.getName()+"::setDependentsDirty()").c_str();
    FabricSplice::Logging::AutoTimer localTimer(localTimerName.c_str());

    if ( !constructingEvaluationGraph )
    {
      // we can't ask for the plug value here, so we fill an array for the compute to only transfer newly dirtied values
      collectDirtyPlug(inPlug);

      if ( _outputsDirtied )
      {
        // printf( "_outputsDirtied\n");
        return MS::kSuccess;
      }
    }

    if ( _affectedPlugsDirty )
    {
      _affectedPlugs.clear();

      for ( unsigned i = 0; i < thisNode.attributeCount(); ++i )
      {
        MFnAttribute attr( thisNode.attribute( i ) );

        // FTL::CStrRef attrNameCStr = attr.name().asChar();
        // printf(
        //   "%u %s isReadable=%s isWritable=%s\n",
        //   i, attrNameCStr.c_str(),
        //   attr.isReadable()? "true": "false",
        //   attr.isWritable()? "true": "false"
        //   );
        if ( attr.isHidden() )
          continue;
        if ( !attr.isDynamic() )
          continue;
        if ( !attr.isReadable() )
          continue;
        if ( !attr.parent().isNull() )
          continue;

        MPlug outPlug =
          thisNode.findPlug(
            attr.object(),
            false // wantNetworkedPlug
            );
        assert( !outPlug.isNull() );
        if ( !plugInArray( outPlug, _affectedPlugs ) )
        {
          _affectedPlugs.append( outPlug );
          affectChildPlugs( outPlug, _affectedPlugs );
        }
      }

      _affectedPlugsDirty = false;
    }

    outPlugs = _affectedPlugs;

    if ( !constructingEvaluationGraph )
      _outputsDirtied = true;
  }

  // printf( "[" );
  // for ( unsigned i = 0; i < outPlugs.length(); ++i )
  // {
  //   MPlug outPlug = outPlugs[i];
  //   printf( "%s%s", (i==0? "": ", "), outPlug.name().asChar() );
  // }
  // printf( "]\n" );
  // fflush( stdout );

  return MS::kSuccess;
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

void FabricSpliceBaseInterface::onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made)
{
  _affectedPlugsDirty = true;

  if(!asSrc)
  {
    MString plugName = plug.name();

    if(plugName.index('.') > -1)
    {
      plugName = plugName.substring(plugName.index('.')+1, plugName.length());
      if(plugName.index('[') > -1)
       plugName = plugName.substring(0, plugName.index('[')-1);
    }

    for(size_t j=0;j<mSpliceMayaDataOverride.size();j++)
    {
      if(mSpliceMayaDataOverride[j] == plugName.asChar())
      {
        FabricSplice::DGPort port = getPort(plugName.asChar());
        if(port.isValid())
        {
          // if there are no connections, 
          // ensure to disable the conversion
          port.setOption("disableSpliceMayaDataConversion", FabricCore::Variant::CreateBoolean(!made));
        }
        break;
      }
    }

  }
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

      FabricCore::RTVal objectRtVal = FabricSplice::constructRTVal("Object", 1, &value);
      if(!objectRtVal.isValid())
        continue;

      FabricCore::RTVal detachable = FabricSplice::constructInterfaceRTVal("Detachable", objectRtVal);
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
    catch(FabricSplice::Exception e)
    {
      // ignore errors, probably an object which does not implement deattach and attach
    }
  }

  _portObjectsDestroyed = destroy;
}

#if _SPLICE_MAYA_VERSION >= 2016
MStatus FabricSpliceBaseInterface::preEvaluation(MObject thisMObject, const MDGContext& context, const MEvaluationNode& evaluationNode)
{
  MStatus status;
  if(!context.isNormal()) 
    return MStatus::kFailure;

  // [andrew 20150616] in 2016 this needs to also happen here because
  // setDependentsDirty isn't called in Serial or Parallel eval mode
  for (MEvaluationNodeIterator dirtyIt = evaluationNode.iterator();
      !dirtyIt.isDone(); dirtyIt.next())
  {
    collectDirtyPlug(dirtyIt.plug());
  }
  return MS::kSuccess;
}
#endif

void FabricSpliceBaseInterface::attributeAddedOrRemoved(
  MNodeMessage::AttributeMessage msg,
  MPlug &plug
  )
{
  _affectedPlugsDirty = true;
  _outputsDirtied = false;
  _affectedPlugs.clear();
}
