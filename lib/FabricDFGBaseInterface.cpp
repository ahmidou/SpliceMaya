//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricDFGBaseInterface.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceMayaData.h"
#include "FabricDFGWidget.h"
#include "FabricSpliceHelpers.h"
#include "FabricMayaAttrs.h"
#include "FabricDFGProfiling.h"
#include <Persistence/RTValToJSONEncoder.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <FTL/AutoSet.h>

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
#include <maya/MQtUtil.h>

#if MAYA_API_VERSION >= 201600
# include <maya/MEvaluationNode.h>
# include <maya/MEvaluationManager.h>
#endif

std::vector<FabricDFGBaseInterface*> FabricDFGBaseInterface::_instances;
#if MAYA_API_VERSION < 201300
  std::map<std::string, int> FabricDFGBaseInterface::_nodeCreatorCounts;
#endif
unsigned int FabricDFGBaseInterface::s_maxID = 1;
bool FabricDFGBaseInterface::s_use_evalContext = true; // [FE-6287]

FabricDFGBaseInterface::FabricDFGBaseInterface()
  : m_executeSharedDirty( true )
{

  MStatus stat;
  MAYADFG_CATCH_BEGIN(&stat);

  _restoredFromPersistenceData = false;
  _isTransferingInputs = false;
  _dgDirtyEnabled = true;
  _portObjectsDestroyed = false;
  _affectedPlugsDirty = true;
  _outputsDirtied = false;
  _isReferenced = false;
  _isEvaluating = false;
  _dgDirtyQueued = false;
  m_evalID = 0;
  m_evalIDAtLastEvaluate = 0;
  m_isStoringJson = false;
  _instances.push_back(this);

  m_id = s_maxID++;

  MAYADFG_CATCH_END(&stat);
}

FabricDFGBaseInterface::~FabricDFGBaseInterface()
{
  if( m_binding )
  {
    m_binding.setNotificationCallback( NULL, NULL );

    // Release cached values and variables, for example InlineDrawingHandle
    m_binding.deallocValues();
  }

  m_evalContext = FabricCore::RTVal();
  m_binding = FabricCore::DFGBinding();

  for(size_t i=0;i<_instances.size();i++){
    if(_instances[i] == this){
      std::vector<FabricDFGBaseInterface*>::iterator iter = _instances.begin() + i;
      _instances.erase(iter);
      break;
    }
  }
}

void FabricDFGBaseInterface::constructBaseInterface(){

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::constructBaseInterface");

  MStatus stat;
  MAYADFG_CATCH_BEGIN(&stat);

  if(m_binding.isValid())
    return;

#if MAYA_API_VERSION < 201300
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

  m_client = FabricDFGWidget::GetCoreClient();
  FabricCore::DFGHost dfgHost = m_client.getDFGHost();

  m_binding = dfgHost.createBindingToNewGraph();
  m_binding.setNotificationCallback( BindingNotificationCallback, this );
  FabricCore::DFGExec exec = m_binding.getExec();

  MString idStr; idStr.set(m_id);
  m_binding.setMetadata("maya_id", idStr.asChar(), false);

  m_evalContext = FabricCore::RTVal::Create(m_client, "EvalContext", 0, 0);
  m_evalContext = m_evalContext.callMethod("EvalContext", "getInstance", 0, 0);
  m_evalContext.setMember("host", FabricCore::RTVal::ConstructString(m_client, "Maya"));

  MAYADFG_CATCH_END(&stat);
}

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceByName(const std::string & name) {

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

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceById(unsigned int id)
{
  for(size_t i=0;i<_instances.size();i++)
  {
    if(_instances[i]->getId() == id)
    {
      return _instances[i];
    }
  }
  return NULL;
}

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceByIndex(unsigned int index)
{
  if (index < _instances.size())
    return _instances[index];
  return NULL;
}

unsigned int FabricDFGBaseInterface::getNumInstances()
{
  return (unsigned int)_instances.size();
}

unsigned int FabricDFGBaseInterface::getId() const
{
  return m_id;
}

FabricCore::Client FabricDFGBaseInterface::getCoreClient()
{
  return m_client;
}

FabricCore::DFGHost FabricDFGBaseInterface::getDFGHost()
{
  return m_binding.getHost();
}

FabricCore::DFGBinding FabricDFGBaseInterface::getDFGBinding()
{
  return m_binding;
}

FabricCore::DFGExec FabricDFGBaseInterface::getDFGExec()
{
  return m_binding.getExec();
}

bool FabricDFGBaseInterface::transferInputValuesToDFG(MDataBlock& data){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::transferInputValuesToDFG");

  if(_isTransferingInputs)
    return false;

  managePortObjectValues(false); // recreate objects if not there yet

  FTL::AutoSet<bool> transfersInputs(_isTransferingInputs, true);

  VisitCallbackUserData ud(getThisMObject(), data);
  ud.interf = this;
  ud.isDeformer = false;

  getDFGBinding().visitArgs(getLockType(), &FabricDFGBaseInterface::VisitInputArgsCallback, &ud);
  return true;
}

void FabricDFGBaseInterface::evaluate(){

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::evaluate");

  FTL::AutoSet<bool> transfersInputs(_isEvaluating, true);
  _dgDirtyQueued = false;
  m_evalIDAtLastEvaluate = m_evalID;

  MFnDependencyNode thisNode(getThisMObject());

  managePortObjectValues(false); // recreate objects if not there yet

  if (useEvalContext())
  {
    FabricMayaProfilingEvent bracket("setting up eval context");

    if(m_evalContext.isValid())
    {
      try
      {
        m_evalContext.setMember("time", FabricCore::RTVal::ConstructFloat32(m_client, MAnimControl::currentTime().as(MTime::kSeconds)));
      }
      catch(FabricCore::Exception e)
      {
        mayaLogErrorFunc(e.getDesc_cstr());
      }
    }
    else
      mayaLogErrorFunc("EvalContext handle is invalid");
  }

  {
    FabricMayaProfilingEvent bracket("DFGBinding::execute");
    m_binding.execute_lockType( getLockType() );
  }
}

void FabricDFGBaseInterface::transferOutputValuesToMaya(MDataBlock& data, bool isDeformer){
  if(_isTransferingInputs)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::transferOutputValuesToMaya");

  managePortObjectValues(false); // recreate objects if not there yet

  VisitCallbackUserData ud(getThisMObject(), data);
  ud.interf = this;
  ud.isDeformer = isDeformer;

  getDFGBinding().visitArgs(getLockType(), &FabricDFGBaseInterface::VisitOutputArgsCallback, &ud);
}

void FabricDFGBaseInterface::collectDirtyPlug(MPlug const &inPlug){

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::collectDirtyPlug");

  // [hmathee 20161110] take a short cut - if we find the attribute index
  // based on the attribute name then we can exit
  {
    MString attrName = MFnAttribute(inPlug.attribute()).name();
    FTL::StrRef attrNameRef = attrName.asChar();
    FTL::OrderedStringMap< unsigned int >::const_iterator it = _attributeNameToIndex.find(attrNameRef);
    if(it != _attributeNameToIndex.end())
    {
      _isAttributeIndexDirty[it->second] = true;
      return;
    }
  }

  MStatus stat;
  MString plugName = inPlug.name();
  FTL::StrRef nameRef = plugName.asChar();
  nameRef = nameRef.rsplit('.').second;
  nameRef = nameRef.split('[').first;

  // filter out savedata
  if(nameRef == FTL_STR("saveData") || 
    nameRef == FTL_STR("refFilePath") || 
    nameRef == FTL_STR("enableEvalContext") ||
    nameRef == FTL_STR("nodeState") ||
    nameRef == FTL_STR("caching") ||
    nameRef == FTL_STR("frozen"))
    return;

  // if(_spliceGraph.usesEvalContext())
  // {
  //   _evalContextPlugNames.append(name);
  //   if(inPlug.isElement())
  //     _evalContextPlugIds.append(inPlug.logicalIndex());
  //   else
  //     _evalContextPlugIds.append(-1);
  // }

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

  // todo: get the attribute index
  FTL::OrderedStringMap< unsigned int >::const_iterator it = _attributeNameToIndex.find(nameRef);
  if(it != _attributeNameToIndex.end())
  {
    unsigned int index = it->second;
    while(index >= _isAttributeIndexDirty.size())
    {
      _isAttributeIndexDirty.push_back(true);
    }
    _isAttributeIndexDirty[index] = true;
  }
}

void FabricDFGBaseInterface::generateAttributeLookups()
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::generateAttributeLookups");

  _attributeNameToIndex.clear();
  _plugToArgFuncs.resize(0);
  _argToPlugFuncs.resize(0);

  MFnDependencyNode thisNode(getThisMObject());
  FabricCore::DFGExec exec = getDFGBinding().getExec();

  _attributePlugs.setLength(thisNode.attributeCount());
  for(unsigned int i = 0; i < thisNode.attributeCount(); ++i)
  {
    MFnAttribute attrib(thisNode.attribute(i));
    while(i >= _isAttributeIndexDirty.size())
    {
      _isAttributeIndexDirty.push_back(true); 
    }
    _attributePlugs[i] = MPlug(getThisMObject(), thisNode.attribute(i));

    MString attrName = attrib.name();
    FTL::StrRef attrNameRef = attrName.asChar();
    if(attrNameRef == FTL_STR("saveData") || 
      attrNameRef == FTL_STR("refFilePath") || 
      attrNameRef == FTL_STR("enableEvalContext") ||
      attrNameRef == FTL_STR("nodeState") ||
      attrNameRef == FTL_STR("caching") ||
      attrNameRef == FTL_STR("frozen"))
      continue;

    _attributeNameToIndex.insert(attrib.name().asChar(), i);
  }

  _argIndexToAttributeIndex.resize(exec.getExecPortCount());
  _plugToArgFuncs.resize(exec.getExecPortCount());
  _argToPlugFuncs.resize(exec.getExecPortCount());
  for(unsigned i = 0; i < exec.getExecPortCount(); ++i)
  {
    _argIndexToAttributeIndex[i] = UINT_MAX;
    _plugToArgFuncs[i] = NULL;
    _argToPlugFuncs[i] = NULL;
    MString argName = exec.getExecPortName(i);
    MString plugName = getPlugName(argName);
    for(unsigned j=0;j<thisNode.attributeCount();j++)
    {
      MFnAttribute attrib(thisNode.attribute(j));
      if(attrib.name() == plugName)
      {
        _argIndexToAttributeIndex[i] = j;
        break;
      }
    }
  }
}

void FabricDFGBaseInterface::affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::affectChildPlugs");
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

void FabricDFGBaseInterface::storePersistenceData(MString file, MStatus *stat){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::storePersistenceData");

  MAYADFG_CATCH_BEGIN(stat);

  FTL::AutoSet<bool> storingJson(m_isStoringJson, true);

  std::string json = m_binding.exportJSON().getCString();
  MPlug saveDataPlug = getSaveDataPlug();
  saveDataPlug.setString(json.c_str());

  MAYADFG_CATCH_END(stat);
}

void FabricDFGBaseInterface::restoreFromPersistenceData(MString file, MStatus *stat){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::restoreFromPersistenceData");
  if(_restoredFromPersistenceData)
    return;

  MPlug saveDataPlug = getSaveDataPlug();
  MString json = saveDataPlug.asString();
  MPlug refFilePathPlug = getRefFilePathPlug();

  _isReferenced = false;

  MString refFilePath = refFilePathPlug.asString();
  if(refFilePath.length() > 0)
  {
    MString resolvedRefFilePath = resolveEnvironmentVariables(refFilePath);
    if(resolvedRefFilePath != refFilePath)
      mayaLogFunc("Referenced file path '"+refFilePath+"' resolved to '"+resolvedRefFilePath+"'.");

    FILE * file = fopen(resolvedRefFilePath.asChar(), "rb");
    if(!file)
    {
      mayaLogErrorFunc("Referenced file path '"+refFilePath+"' cannot be opened, falling back to locally saved json.");
    }
    else
    {
      fseek( file, 0, SEEK_END );
      long fileSize = ftell( file );
      rewind( file );

      char * buffer = (char*) malloc(fileSize + 1);
      buffer[fileSize] = '\0';

      size_t readBytes = fread(buffer, 1, fileSize, file);
      assert(readBytes == size_t(fileSize));
      (void)readBytes;

      fclose(file);

      json = buffer;
      free(buffer);

      _isReferenced = true;
    }
  }

  restoreFromJSON(json, stat);
}

void FabricDFGBaseInterface::restoreFromJSON(MString json, MStatus *stat){
  if(_restoredFromPersistenceData)
    return;
  if(m_lastJson == json)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::restoreFromJSON");

  MAYADFG_CATCH_BEGIN(stat);

  if ( m_binding )
    m_binding.setNotificationCallback( NULL, NULL );

  FabricCore::DFGHost dfgHost = m_client.getDFGHost();
  m_binding = dfgHost.createBindingFromJSON(json.asChar());
  m_binding.setNotificationCallback( BindingNotificationCallback, this );

  FTL::StrRef execPath;
  FabricCore::DFGExec exec = m_binding.getExec();

  MString idStr; idStr.set(m_id);
  m_binding.setMetadata("maya_id", idStr.asChar(), false);

  // todo: update UI

  _restoredFromPersistenceData = true;

  invalidateNode();

  MFnDependencyNode thisNode(getThisMObject());

  for(unsigned i = 0; i < exec.getExecPortCount(); ++i){
    std::string portName = exec.getExecPortName(i);
    MString plugName = getPlugName(portName.c_str());
    MPlug plug = thisNode.findPlug(plugName);
    if(!plug.isNull())
      continue;

    FabricCore::DFGPortType portType = exec.getExecPortType(i);
    if (!exec.getExecPortResolvedType(i)) continue; // [FE-5538]
    std::string dataType = exec.getExecPortResolvedType(i);

    FTL::StrRef opaque = exec.getExecPortMetadata(portName.c_str(), "opaque");
    if(opaque == "true")
      dataType = "SpliceMayaData";

    FabricServices::CodeCompletion::KLTypeDesc typeDesc(dataType);

    std::string arrayType = "Single Value";
    if(typeDesc.isArray())
    {
      arrayType = "Array (Multi)";
      FTL::StrRef nativeArray = exec.getExecPortMetadata(portName.c_str(), "nativeArray");
      if(nativeArray == "true")
      {
        arrayType = "Array (Native)";
        exec.setExecPortMetadata(portName.c_str(), "nativeArray", "true", false /* canUndo */);
      }
    }

    FTL::StrRef addAttribute = exec.getExecPortMetadata(portName.c_str(), "addAttribute");
    if(addAttribute != "false")
      addMayaAttribute(portName.c_str(), dataType.c_str(), portType, arrayType.c_str());

    if(portType != FabricCore::DFGPortType_Out)
    {
      MFnDependencyNode thisNode(getThisMObject());
      MPlug plug = thisNode.findPlug(plugName);
      if(!plug.isNull())
      {
        FabricCore::RTVal value = m_binding.getArgValue(portName.c_str());
        std::string typeName = value.getTypeName().getStringCString();
        if(typeName == "String")
          plug.setString(value.getStringCString());
        else if(typeName == "Boolean")
          plug.setBool(value.getBoolean());
        else if(!value.isValid())
          continue;
        else if(value.isArray())
          continue;
        else if(value.isDict())
          continue;
        else
        {
          float fvalue = 0.0;
          if(typeName == "SInt8")
            fvalue = (float)value.getSInt8();
          else if(typeName == "SInt16")
            fvalue = (float)value.getSInt16();
          else if(typeName == "SInt32")
            fvalue = (float)value.getSInt32();
          else if(typeName == "SInt64")
            fvalue = (float)value.getSInt64();
          else if(typeName == "UInt8")
            fvalue = (float)value.getUInt8();
          else if(typeName == "UInt16")
            fvalue = (float)value.getUInt16();
          else if(typeName == "UInt32")
            fvalue = (float)value.getUInt32();
          else if(typeName == "UInt64")
            fvalue = (float)value.getUInt64();
          else if(typeName == "Float32")
            fvalue = (float)value.getFloat32();
          else if(typeName == "Float64")
            fvalue = (float)value.getFloat64();

          MDataHandle handle = plug.asMDataHandle();
          if(handle.numericType() == MFnNumericData::kFloat)
            plug.setFloat(fvalue);
          else if(handle.numericType() == MFnNumericData::kDouble)
            plug.setDouble(fvalue);
          else if(handle.numericType() == MFnNumericData::kInt)
            plug.setInt((int)fvalue);
        }
      }
    }
  }

  for(unsigned i = 0; i < exec.getExecPortCount(); ++i){
    std::string portName = exec.getExecPortName(i);
    MString plugName = getPlugName(portName.c_str());
    MPlug plug = thisNode.findPlug(plugName);
    if(plug.isNull())
      continue;

    // force an execution of the node    
    FabricCore::DFGPortType portType = exec.getExecPortType(i);
    if(portType != FabricCore::DFGPortType_Out)
    {
      MString command("dgeval ");
      MGlobal::executeCommandOnIdle(command+thisNode.name()+"."+plugName);
      break;
    }
  }

  m_lastJson = json;
  // todo... set values?
  generateAttributeLookups();

  MAYADFG_CATCH_END(stat);
}

void FabricDFGBaseInterface::setReferencedFilePath(MString filePath)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setReferencedFilePath");

  MPlug plug = getRefFilePathPlug();
  plug.setString(filePath);
  if(m_binding.isValid())
  {
    m_binding.setMetadata("editable", "false", false);
    _isReferenced = true;
  }
}

void FabricDFGBaseInterface::reloadFromReferencedFilePath()
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::reloadFromReferencedFilePath");

  MPlug plug = getRefFilePathPlug();
  MString filePath = plug.asString();
  if(filePath.length() == 0)
    return;

  _restoredFromPersistenceData = false;
  MStatus status;
  restoreFromPersistenceData(mayaGetLastLoadedScene(), &status);
}

MString FabricDFGBaseInterface::getPlugName(const MString &portName)
{
  if      (portName == "message")            return "dfg_message";
  else if (portName == "saveData")           return "dfg_saveData";
  else if (portName == "refFilePath")        return "dfg_refFilePath";
  else if (portName == "enableEvalContext")  return "dfg_enableEvalContext";
  else                                       return portName;
}

MString FabricDFGBaseInterface::getPortName(const MString &plugName)
{
  if      (plugName == "dfg_message")           return "message";
  else if (plugName == "dfg_saveData")          return "saveData";
  else if (plugName == "dfg_refFilePath")       return "refFilePath";
  else if (plugName == "dfg_enableEvalContext") return "enableEvalContext";
  else                                          return plugName;
}

void FabricDFGBaseInterface::invalidatePlug(MPlug & plug)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::invalidatePlug");

  if(!_dgDirtyEnabled)
    return;
  if(plug.isNull())
    return;
  if(plug.attribute().isNull())
    return;

  // filter plugs containing [-1]
  MString plugName = plug.name();
  MStringArray plugNameParts;
  plugName.split('[', plugNameParts);
  if(plugNameParts.length() > 1)
  {
    if(plugNameParts[1].substring(0, 2) == "-1]")
      return;
  }

  if(!_dgDirtyQueued)
  {
    _dgDirtyQueued = true;
    MString command("dgdirty ");
    MGlobal::executeCommandOnIdle(command+plugName);
  }

  if(plugName.index('.') > -1)
  {
    plugName = plugName.substring(plugName.index('.')+1, plugName.length());
    if(plugName.index('[') > -1)
      plugName = plugName.substring(0, plugName.index('[')-1);

    if (getDFGBinding().getExec().haveExecPort(plugName.asChar()))
    {
      char const *dataTypeCStr = getDFGBinding().getExec().getExecPortResolvedType(plugName.asChar());
      std::string dataType( dataTypeCStr? dataTypeCStr: "");
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

void FabricDFGBaseInterface::invalidateNode()
{
  if(!_dgDirtyEnabled)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::invalidateNode");

  MFnDependencyNode thisNode(getThisMObject());

  FabricCore::DFGExec exec = getDFGExec();

  // ensure we setup the mayaSpliceData overrides first
  mSpliceMayaDataOverride.resize(0);
  for(unsigned int i = 0; i < exec.getExecPortCount(); ++i){
    // if(ports[i]->isManipulatable())
    //   continue;
    std::string portName = exec.getExecPortName(i);
    MString plugName = getPortName(portName.c_str());
    MPlug plug = thisNode.findPlug(plugName);
    MObject attrObj = plug.attribute();
    if(attrObj.apiType() == MFn::kTypedAttribute)
    {
      MFnTypedAttribute attr(attrObj);
      MFnData::Type type = attr.attrType();
      if(type == MFnData::kPlugin || type == MFnData::kInvalid)
        mSpliceMayaDataOverride.push_back(portName.c_str());
    }
  }

  // ensure that the node is invalidated
  unsigned int dirtiedInputs = 0;
  for(unsigned int i = 0; i < exec.getExecPortCount(); ++i){
    std::string portName = exec.getExecPortName(i);
    MString plugName = getPortName(portName.c_str());
    MPlug plug = thisNode.findPlug(plugName);

    FabricCore::DFGPortType portType = exec.getExecPortType(i);
    if(!plug.isNull()){
      if(portType == FabricCore::DFGPortType_In)
      {
        collectDirtyPlug(plug);
        MPlugArray plugs;
        plug.connectedTo(plugs,true,false);
        for(size_t j=0;j<plugs.length();j++)
          invalidatePlug(plugs[j]);
        dirtiedInputs++;
      }
      else
      {
        invalidatePlug(plug);

        MPlugArray plugs;
        affectChildPlugs(plug, plugs);
        for(size_t j=0;j<plugs.length();j++)
          invalidatePlug(plugs[j]);
      }
    }
  }

  // if there are no inputs on this 
  // node, let's rely on the eval id attribute
  if(dirtiedInputs == 0)
  {
    incrementEvalID();
  }

  _affectedPlugsDirty = true;
  _outputsDirtied = false;
}

void FabricDFGBaseInterface::incrementEvalID()
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::incrementEvalID");
  if( m_evalID == m_evalIDAtLastEvaluate )
  {
    MFnDependencyNode thisNode(getThisMObject());
    MPlug evalIDPlug = thisNode.findPlug("evalID");
    if(evalIDPlug.isNull())
      return;

    evalIDPlug.setValue( (int)++m_evalID );
  } 
}

bool FabricDFGBaseInterface::plugInArray(const MPlug &plug, const MPlugArray &array){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::plugInArray");

  bool found = false;
  for(size_t i = 0; i < array.length(); ++i){
    if(array[i] == plug){
      found = true;
      break;
    }
  }
  return found;
}

MStatus FabricDFGBaseInterface::setDependentsDirty(MObject thisMObject, MPlug const &inPlug, MPlugArray &affectedPlugs){

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setDependentsDirty");

  MFnDependencyNode thisNode(thisMObject);

  // we can't ask for the plug value here, so we fill an array for the compute to only transfer newly dirtied values
  collectDirtyPlug(inPlug);

#if MAYA_API_VERSION >= 201600
  bool emConstructionActive = MEvaluationManager::graphConstructionActive();
#else
  bool emConstructionActive = false;
#endif

  if(_outputsDirtied && !emConstructionActive)
    return MS::kSuccess;

  if(_affectedPlugsDirty || emConstructionActive)
  {
    FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setDependentsDirty _affectedPlugsDirty");
    
    _affectedPlugs.clear();

    // todo: performance considerations
    for(unsigned int i = 0; i < thisNode.attributeCount(); ++i){
      MFnAttribute attrib(thisNode.attribute(i));

      // [hmathe 20161108] some nodes might have static attributes that need to be dirtied
      //if(!attrib.isDynamic())
      MString attrName = attrib.name();
      FTL::StrRef attrNameRef = attrName.asChar();
      if(attrNameRef == FTL_STR("saveData") || 
        attrNameRef == FTL_STR("refFilePath") || 
        attrNameRef == FTL_STR("enableEvalContext") ||
        attrNameRef == FTL_STR("nodeState") ||
        attrNameRef == FTL_STR("caching") ||
        attrNameRef == FTL_STR("frozen"))
        continue;

      // check if the attribute is an output
      // otherwise continue
      try
      {
        FabricCore::DFGExec exec = getDFGExec();
        FabricCore::DFGPortType portType = exec.getExecPortType(attrib.name().asChar());
        if(portType == FabricCore::DFGPortType_In)
          continue;
      }
      catch(FabricCore::Exception e)
      {
        continue;
      }

      MPlug outPlug = thisNode.findPlug(attrib.name());
      if(!outPlug.isNull()){
        if(!plugInArray(outPlug, _affectedPlugs)){
          _affectedPlugs.append(outPlug);
          affectChildPlugs(outPlug, _affectedPlugs);
        }
      }
    }

    _affectedPlugsDirty = false;
  }

  {
    FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setDependentsDirty copy _affectedPlugs");
    affectedPlugs = _affectedPlugs;
  }

  _outputsDirtied = true;

  return MS::kSuccess;
}

void FabricDFGBaseInterface::copyInternalData(MPxNode *node){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::copyInternalData");

  if (node)
  {
    FabricDFGBaseInterface *otherInterface = getInstanceByName(node->name().asChar());
    if (otherInterface)
    {
      MStatus stat = MS::kSuccess;
      MAYADFG_CATCH_BEGIN(&stat);
      restoreFromJSON(otherInterface->getDFGBinding().exportJSON().getCString(), &stat);
      MAYADFG_CATCH_END(&stat); 
    }
  }
}

bool FabricDFGBaseInterface::getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::getInternalValueInContext");

  if(plug.partialName() == "saveData" || plug.partialName() == "svd"){
    // somebody is pulling on the save data, let's persist it either way
    MStatus stat = MS::kSuccess;
    MAYADFG_CATCH_BEGIN(&stat);
    dataHandle.setString(m_binding.exportJSON().getCString());
    MAYADFG_CATCH_END(&stat); 
    return stat == MS::kSuccess;
  }
  return false;
}

bool FabricDFGBaseInterface::setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx){
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setInternalValueInContext");

  if(plug.partialName() == "saveData" || plug.partialName() == "svd"){
    if(!m_isStoringJson)
    {
      MString json = dataHandle.asString();
      if(json.length() > 0)
      {
        if(m_lastJson != json)
        {
          MStatus st;
          restoreFromJSON(json, &st);
          _restoredFromPersistenceData = false;
        }
      }
    }
    return true;
  }
  return false;
}

void FabricDFGBaseInterface::onNodeAdded(MObject &node, void *clientData)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::onNodeAdded");

  MFnDependencyNode thisNode(node);
  FabricDFGBaseInterface * interf = getInstanceByName(thisNode.name().asChar());
  if( interf )
  {
    // reattach
    interf->managePortObjectValues( false );

    // In case it is from the delete of an undo, 
    // needs to be re-evaluated since values were flushed
    interf->invalidateNode();

    // [FE-7498] lock the 'saveData' attribute to prevent problems
    // when referencing .ma files that contain Canvas nodes.
    MGlobal::executeCommandOnIdle("setAttr -lock true " + thisNode.name() + ".saveData");
  }
}

void FabricDFGBaseInterface::onNodeRemoved(MObject &node, void *clientData)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::onNodeRemoved");

  MFnDependencyNode thisNode(node);
  FabricDFGBaseInterface *interf = getInstanceByName(thisNode.name().asChar());

  if (interf)
  {
    // detach
    interf->managePortObjectValues(true);

    // [FE-7513] check if the removed node is the one currently being
    // displayed in Canvas. If that is the case then close Canvas.
    if (MGlobal::mayaState() == MGlobal::kInteractive)
    {
      if (interf == FabricDFGWidget::getBaseInterface())
      {
        QWidget *w = MQtUtil().findWindow("FabricDFGUIPane");
        if (!w)  w = MQtUtil().findLayout("FabricDFGUIPane", MQtUtil().mainWindow());
        if (!w)  return;
        if (!w->close())
          MGlobal::displayInfo("[Fabric] failed to close the Canvas window.");
      }
    }
  }
}

void FabricDFGBaseInterface::onAnimCurveEdited(MObjectArray &editedCurves, void *clientData)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::onAnimCurveEdited");

  for (unsigned int i=0;i<editedCurves.length();i++)
  {
    // get the curve and its connected plugs.
    MFnAnimCurve curve(editedCurves[i]);
    MPlugArray curvePlugs;
    if (curve.getConnections(curvePlugs) != MS::kSuccess)
      continue;

    // go through the connected plugs and check
    // if they are connected to a Canvas node.
    for (unsigned int j=0;j<curvePlugs.length();j++)
    {
      // get the plug and its connections.
      MPlug &curvePlug = curvePlugs[j];
      MPlugArray destPlugs;
      if (!curvePlug.connectedTo(destPlugs, false /* asDst */, true /* asSrc */) || !destPlugs.length())
        continue;

      // go through the connected plugs and
      // check if they belong to a Canvas node.
      for (unsigned int k=0;k<destPlugs.length();k++)
      {
        MPlug &destPlug = destPlugs[k];

        std::string n = destPlug.name().asChar();
        size_t t = n.find_last_of('.');
        if (t == std::string::npos)
          continue;

        FabricDFGBaseInterface *b = getInstanceByName(n.substr(0, t).c_str());
        if (b)  b->invalidatePlug(destPlug);
      }
    }
  }
}

void FabricDFGBaseInterface::onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::onConnection");

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
        if(getDFGExec().haveExecPort(plugName.asChar()))
        {
          // if there are no connections,
          // ensure to disable the conversion
          getDFGExec().setExecPortMetadata(plugName.asChar(), "disableSpliceMayaDataConversion", made ? "false" : "true", false /* canUndo */);
        }
        break;
      }
    }
  }

  generateAttributeLookups();
}

MObject FabricDFGBaseInterface::addMayaAttribute(MString portName, MString dataType, FabricCore::DFGPortType portType, MString arrayType, bool compoundChild, MStatus * stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::addMayaAttribute");

  MAYADFG_CATCH_BEGIN(stat);

  MObject newAttribute;

  FabricCore::DFGExec exec = m_binding.getExec();

  // skip if disabled by used in the EditPortDialog
  FTL::StrRef addAttributeMD = exec.getExecPortMetadata(portName.asChar(), "addAttribute");
  if(addAttributeMD == "false")
    return newAttribute;

  MString dataTypeOverride = dataType;
  FTL::StrRef opaqueMD = exec.getExecPortMetadata(portName.asChar(), "opaque");
  if(opaqueMD == "true")
    dataTypeOverride = "SpliceMayaData";

  // remove []
  MStringArray splitBuffer;
  dataTypeOverride.split('[', splitBuffer);
  if(splitBuffer.length()){
    dataTypeOverride = splitBuffer[0];
    if(splitBuffer.length() > 1 && arrayType.length() == 0)
    {
      FTL::StrRef nativeArrayMD = exec.getExecPortMetadata(portName.asChar(), "nativeArray");
      if(nativeArrayMD == "true")
        arrayType = "Array (Native)";
      else
        arrayType = "Array (Multi)";
    }
  }

  if(arrayType.length() == 0)
    arrayType = "Single Value";

  MFnDependencyNode thisNode(getThisMObject());
  MString plugName = getPlugName(portName);
  MPlug plug = thisNode.findPlug(plugName);
  if(!plug.isNull()){
    if(_isReferenced)
      return plug.attribute();

    mayaLogFunc("Attribute '"+portName+"' already exists on node '"+thisNode.name()+"'.");
    return newAttribute;
  }

  // extract the ui range info
  float uiSoftMin = 0.0f;
  float uiSoftMax = 0.0f;
  float uiHardMin = 0.0f;
  float uiHardMax = 0.0f;
  FTL::CStrRef uiSoftRangeRef = exec.getExecPortMetadata(portName.asChar(), "uiRange");
  FTL::CStrRef uiHardRangeRef = exec.getExecPortMetadata(portName.asChar(), "uiHardRange");
  if (uiSoftRangeRef.size() > 2)
  {
    MString uiRangeStr = uiSoftRangeRef.c_str();
    if (uiRangeStr.asChar()[0] == '(')
      uiRangeStr = uiRangeStr.substring(1, uiRangeStr.length()-1);
    if (uiRangeStr.asChar()[uiRangeStr.length()-1] == ')')
      uiRangeStr = uiRangeStr.substring(0, uiRangeStr.length()-2);
    MStringArray parts;
    uiRangeStr.split(',', parts);
    if (parts.length() == 2)
    {
      uiSoftMin = parts[0].asFloat();
      uiSoftMax = parts[1].asFloat();
    }
  }
  if (uiHardRangeRef.size() > 2)
  {
    MString uiRangeStr = uiHardRangeRef.c_str();
    if (uiRangeStr.asChar()[0] == '(')
      uiRangeStr = uiRangeStr.substring(1, uiRangeStr.length()-1);
    if (uiRangeStr.asChar()[uiRangeStr.length()-1] == ')')
      uiRangeStr = uiRangeStr.substring(0, uiRangeStr.length()-2);
    MStringArray parts;
    uiRangeStr.split(',', parts);
    if (parts.length() == 2)
    {
      uiHardMin = parts[0].asFloat();
      uiHardMax = parts[1].asFloat();
    }
  }

  // correct soft range, if necessary.
  if (uiSoftMin < uiSoftMax && uiHardMin < uiHardMax)
  {
    if (uiSoftMin < uiHardMin)  uiSoftMin = uiHardMin;
    if (uiSoftMax > uiHardMax)  uiSoftMax = uiHardMax;
  }

  MFnNumericAttribute nAttr;
  MFnTypedAttribute tAttr;
  MFnUnitAttribute uAttr;
  MFnMatrixAttribute mAttr;
  MFnMessageAttribute pAttr;
  MFnCompoundAttribute cAttr;

  bool storable = portType != FabricCore::DFGPortType_Out;

  // if(dataTypeOverride == "CompoundParam")
  // {
  //   if(compoundStructure.isNull())
  //   {
  //     mayaLogErrorFunc("CompoundParam used for a maya attribute but no compound structure provided.");
  //     return newAttribute;
  //   }

  //   if(!compoundStructure.isDict())
  //   {
  //     mayaLogErrorFunc("CompoundParam used for a maya attribute but compound structure does not contain a dictionary.");
  //     return newAttribute;
  //   }

  //   if(arrayType == "Single Value" || arrayType == "Array (Multi)")
  //   {
  //     MObjectArray children;
  //     for(FabricCore::Variant::DictIter childIter(compoundStructure); !childIter.isDone(); childIter.next())
  //     {
  //       MString childNameStr = childIter.getKey()->getStringData();
  //       const FabricCore::Variant * value = childIter.getValue();
  //       if(!value)
  //         continue;
  //       if(value->isNull())
  //         continue;
  //       if(value->isDict()) {
  //         const FabricCore::Variant * childDataType = value->getDictValue("dataType");
  //         if(childDataType)
  //         {
  //           if(childDataType->isString())
  //           {
  //             MString childArrayTypeStr = "Single Value";
  //             MString childDataTypeStr = childDataType->getStringData();
  //             MStringArray childDataTypeStrParts;
  //             childDataTypeStr.split('[', childDataTypeStrParts);
  //             if(childDataTypeStrParts.length() > 1)
  //               childArrayTypeStr = "Array (Multi)";

  //             MObject child = addMayaAttribute(childNameStr, childDataTypeStr, childArrayTypeStr, portType, true, *value, stat);
  //             if(child.isNull())
  //               return newAttribute;
              
  //             children.append(child);
  //             continue;
  //           }
  //         }

  //         // we assume it's a nested compound param
  //         MObject child = addMayaAttribute(childNameStr, "CompoundParam", "Single Value", portType, true, *value, stat);
  //         if(child.isNull())
  //           return newAttribute;
  //         children.append(child);
  //       }
  //     }

  //     /// now create the compound attribute
  //     newAttribute = cAttr.create(plugName, plugName);
  //     if(arrayType == "Array (Multi)")
  //     {
  //       cAttr.setArray(true);
  //       cAttr.setUsesArrayDataBuilder(true);
  //     }
  //     for(unsigned int i=0;i<children.length();i++)
  //     {
  //       cAttr.addChild(children[i]);
  //     }

  //     // initialize the compound param
  //     _dirtyPlugs.append(portName);
  //   }
  //   else
  //   {
  //     mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
  //     return newAttribute;
  //   }
  // }
  // else if(dataTypeOverride == "Boolean")
  if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Boolean)
  {
    if(arrayType == "Single Value")
    {
      bool defValue = 0;
      GetArgValueBoolean(m_binding, portName.asChar(), defValue);
      newAttribute = nAttr.create(plugName, plugName, MFnNumericData::kBoolean, (double)defValue);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.create(plugName, plugName, MFnNumericData::kBoolean);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Integer)
  {
    if(arrayType == "Single Value")
    {
      int defValue = 0;
      GetArgValueInteger(m_binding, portName.asChar(), defValue);
      if(uiHardMin < uiHardMax)
      {
        if(defValue < (int)uiHardMin)   defValue = (int)uiHardMin;
        if(defValue > (int)uiHardMax)   defValue = (int)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, MFnNumericData::kInt, defValue);
      if(uiSoftMin < uiSoftMax)
      {
        nAttr.setSoftMin(uiSoftMin);
        nAttr.setSoftMax(uiSoftMax);
      }
      if(uiHardMin < uiHardMax)
      {
        nAttr.setMin(uiHardMin);
        nAttr.setMax(uiHardMax);
      }
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.create(plugName, plugName, MFnNumericData::kInt);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else if(arrayType == "Array (Native)")
    {
      newAttribute = tAttr.create(plugName, plugName, MFnData::kIntArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }

    // float uiMin = getScalarOption("uiMin", compoundStructure);
    // float uiMax = getScalarOption("uiMax", compoundStructure);
    // if(uiMin < uiMax) 
    // {
    //   nAttr.setMin(uiMin);
    //   nAttr.setMax(uiMax);
    //   float uiSoftMin = getScalarOption("uiSoftMin", compoundStructure);
    //   float uiSoftMax = getScalarOption("uiSoftMax", compoundStructure);
    //   if(uiSoftMin < uiSoftMax) 
    //   {
    //     nAttr.setSoftMin(uiSoftMin);
    //     nAttr.setSoftMax(uiSoftMax);
    //   }
    //   else
    //   {
    //     nAttr.setSoftMin(uiMin);
    //     nAttr.setSoftMax(uiMax);
    //   }
    // }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Scalar)
  {
    bool isUnitAttr = true;
    // std::string scalarUnit = getStringOption("scalarUnit", compoundStructure);
    std::string scalarUnit = "";
    if(arrayType == "Single Value" || arrayType == "Array (Multi)")
    {
      double defValue = 0;
      GetArgValueFloat(m_binding, portName.asChar(), defValue);
      if(uiHardMin < uiHardMax)
      {
        if(defValue < uiHardMin)   defValue = uiHardMin;
        if(defValue > uiHardMax)   defValue = uiHardMax;
      }

      if(scalarUnit == "time")
        newAttribute = uAttr.create(plugName, plugName, MFnUnitAttribute::kTime, defValue);
      else if(scalarUnit == "angle")
        newAttribute = uAttr.create(plugName, plugName, MFnUnitAttribute::kAngle, defValue);
      else if(scalarUnit == "distance")
        newAttribute = uAttr.create(plugName, plugName, MFnUnitAttribute::kDistance, defValue);
      else
      {
        newAttribute = nAttr.create(plugName, plugName, MFnNumericData::kDouble, defValue);
        isUnitAttr = false;
      }

      if(uiSoftMin < uiSoftMax)
      {
        nAttr.setSoftMin(uiSoftMin);
        nAttr.setSoftMax(uiSoftMax);
      }
      if(uiHardMin < uiHardMax)
      {
        nAttr.setMin(uiHardMin);
        nAttr.setMax(uiHardMax);
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
      newAttribute = tAttr.create(plugName, plugName, MFnData::kDoubleArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }

    // float uiMin = getScalarOption("uiMin", compoundStructure);
    // float uiMax = getScalarOption("uiMax", compoundStructure);
    // if(uiMin < uiMax) 
    // {
    //   if(isUnitAttr)
    //   {
    //     uAttr.setMin(uiMin);
    //     uAttr.setMax(uiMax);
    //   }
    //   else
    //   {
    //     nAttr.setMin(uiMin);
    //     nAttr.setMax(uiMax);
    //   }
    //   float uiSoftMin = getScalarOption("uiSoftMin", compoundStructure);
    //   float uiSoftMax = getScalarOption("uiSoftMax", compoundStructure);
    //   if(isUnitAttr)
    //   {
    //     if(uiSoftMin < uiSoftMax) 
    //     {
    //       uAttr.setSoftMin(uiSoftMin);
    //       uAttr.setSoftMax(uiSoftMax);
    //     }
    //     else
    //     {
    //       uAttr.setSoftMin(uiMin);
    //       uAttr.setSoftMax(uiMax);
    //     }
    //   }
    //   else
    //   {
    //     if(uiSoftMin < uiSoftMax) 
    //     {
    //       nAttr.setSoftMin(uiSoftMin);
    //       nAttr.setSoftMax(uiSoftMax);
    //     }
    //     else
    //     {
    //       nAttr.setSoftMin(uiMin);
    //       nAttr.setSoftMax(uiMax);
    //     }
    //   }
    // }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_String)
  {
    MFnStringData stringData;
    if(arrayType == "Single Value")
    {
      std::string defValue;
      GetArgValueString(m_binding, portName.asChar(), defValue);
      MObject defValueObject = stringData.create(defValue.c_str());
      newAttribute = tAttr.create(plugName, plugName, MFnData::kString, defValueObject);
    }
    else if(arrayType == "Array (Multi)"){
      MObject defValueObject = stringData.create("");
      newAttribute = tAttr.create(plugName, plugName, MFnData::kString, defValueObject);
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Color)
  {
    if(arrayType == "Single Value")
    {
      newAttribute = nAttr.createColor(plugName, plugName);
      std::vector <double> defValue;
      GetArgValueColor(m_binding, portName.asChar(), defValue);
      if (defValue.size() == 4)
        nAttr.setDefault(defValue[0], defValue[1], defValue[2]);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.createColor(plugName, plugName);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Vec3)
  {
    if(arrayType == "Single Value")
    {
      std::vector <double> defValue;
      GetArgValueVec3(m_binding, portName.asChar(), defValue);
      double defValueX = 0.0;
      double defValueY = 0.0;
      double defValueZ = 0.0;
      if (defValue.size() == 3)
      {
        defValueX = defValue[0];
        defValueY = defValue[1];
        defValueZ = defValue[2];
      }
      if(uiHardMin < uiHardMax)
      {
        if(defValueX < uiHardMin)   defValueX = uiHardMin;
        if(defValueX > uiHardMax)   defValueX = uiHardMax;
        if(defValueY < uiHardMin)   defValueY = uiHardMin;
        if(defValueY > uiHardMax)   defValueY = uiHardMax;
        if(defValueZ < uiHardMin)   defValueZ = uiHardMin;
        if(defValueZ > uiHardMax)   defValueZ = uiHardMax;
      }

      MObject x = nAttr.create(plugName+"X", plugName+"X", MFnNumericData::kDouble, defValueX);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);
      MObject y = nAttr.create(plugName+"Y", plugName+"Y", MFnNumericData::kDouble, defValueY);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);
      MObject z = nAttr.create(plugName+"Z", plugName+"Z", MFnNumericData::kDouble, defValueZ);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);

      newAttribute = nAttr.create(plugName, plugName, x, y, z);      
    }
    else if(arrayType == "Array (Multi)")
    {
      MObject x = nAttr.create(plugName+"X", plugName+"X", MFnNumericData::kDouble);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);
      MObject y = nAttr.create(plugName+"Y", plugName+"Y", MFnNumericData::kDouble);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);
      MObject z = nAttr.create(plugName+"Z", plugName+"Z", MFnNumericData::kDouble);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);

      newAttribute = nAttr.create(plugName, plugName, x, y, z);      
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else if(arrayType == "Array (Native)")
    {
      newAttribute = tAttr.create(plugName, plugName, MFnData::kVectorArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Euler)
  {
    if(arrayType == "Single Value")
    {
      MObject x = uAttr.create(plugName+"X", plugName+"X", MFnUnitAttribute::kAngle);
      uAttr.setStorable(storable);
      uAttr.setKeyable(storable);
      MObject y = uAttr.create(plugName+"Y", plugName+"Y", MFnUnitAttribute::kAngle);
      uAttr.setStorable(storable);
      uAttr.setKeyable(storable);
      MObject z = uAttr.create(plugName+"Z", plugName+"Z", MFnUnitAttribute::kAngle);
      uAttr.setStorable(storable);
      uAttr.setKeyable(storable);

      newAttribute = nAttr.create(plugName, plugName, x, y, z);      
    }
    else if(arrayType == "Array (Multi)")
    {
      MObject x = uAttr.create(plugName+"X", plugName+"X", MFnUnitAttribute::kAngle);
      uAttr.setStorable(storable);
      MObject y = uAttr.create(plugName+"Y", plugName+"Y", MFnUnitAttribute::kAngle);
      uAttr.setStorable(storable);
      MObject z = uAttr.create(plugName+"Z", plugName+"Z", MFnUnitAttribute::kAngle);
      uAttr.setStorable(storable);

      newAttribute = nAttr.create(plugName, plugName, x, y, z);      
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Mat44)
  {
    if(arrayType == "Single Value")
    {
      std::vector <double> defValue;
      GetArgValueMat44(m_binding, portName.asChar(), defValue);
      newAttribute = mAttr.create(plugName, plugName);
      if (defValue.size() == 16)
      {
        double vals[4][4] = {
          { defValue[0], defValue[4], defValue[8],  defValue[12] },
          { defValue[1], defValue[5], defValue[9],  defValue[13] },
          { defValue[2], defValue[6], defValue[10], defValue[14] },
          { defValue[3], defValue[7], defValue[11], defValue[15] }
        };
        mAttr.setDefault(MMatrix(vals));
      }
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = mAttr.create(plugName, plugName);
      mAttr.setArray(true);
      mAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_PolygonMesh)
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(plugName, plugName, MFnData::kMesh);
      storable = false;
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = tAttr.create(plugName, plugName, MFnData::kMesh);
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
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_Lines)
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(plugName, plugName, MFnData::kNurbsCurve);
      storable = false;
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = tAttr.create(plugName, plugName, MFnData::kNurbsCurve);
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
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_KeyframeTrack){
    
    if(arrayType == "Single Value")
    {
      if(getDFGExec().haveExecPort(portName.asChar())) {
        newAttribute = pAttr.create(plugName, plugName);
        pAttr.setStorable(storable);
        pAttr.setKeyable(storable);
        pAttr.setCached(false);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
    else
    {
      if(getDFGExec().haveExecPort(portName.asChar())) {
        newAttribute = pAttr.create(plugName, plugName);
        pAttr.setStorable(storable);
        pAttr.setKeyable(storable);
        pAttr.setArray(true);
        pAttr.setCached(false);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
  }
  else if(FabricMaya::ParseDataType(dataTypeOverride.asChar()) == FabricMaya::DT_SpliceMayaData){
    
    if(arrayType == "Single Value")
    {
      if(getDFGExec().haveExecPort(portName.asChar())) {
        newAttribute = tAttr.create(plugName, plugName, FabricSpliceMayaData::id);
        mSpliceMayaDataOverride.push_back(portName.asChar());
        storable = false;

        // disable input conversion by default
        // only enable it again if there is a connection to the port
        getDFGExec().setExecPortMetadata(portName.asChar(), "disableSpliceMayaDataConversion", "true", false /* canUndo */);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
    else
    {
      if(getDFGExec().haveExecPort(portName.asChar())) {
        newAttribute = tAttr.create(plugName, plugName, FabricSpliceMayaData::id);
        mSpliceMayaDataOverride.push_back(portName.asChar());
        storable = false;
        tAttr.setArray(true);
        tAttr.setUsesArrayDataBuilder(true);

        // disable input conversion by default
        // only enable it again if there is a connection to the port
        getDFGExec().setExecPortMetadata(portName.asChar(), "disableSpliceMayaDataConversion", "true", false /* canUndo */);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + portName);
        return newAttribute;
      }
    }
  }
  else
  {
    // mayaLogErrorFunc("DataType '"+dataType+"' not supported.");
    return newAttribute;
  }

  if( storable ) {
    // Set ports added with a "storable type" as persistable so their values are 
    // exported if saving the graph
    // TODO: handle this in a "clean" way; here we are not in the context of an undo-able command.
    //       We would need that the DFG knows which binding types are "stored" as attributes on the
    //       DCC side and set these as persistable in the source "addPort" command.
    exec.setExecPortMetadata( portName.asChar(), DFG_METADATA_UIPERSISTVALUE, "true", false /* canUndo [FE-6169] */ );
  }

  // set the mode
  if(!newAttribute.isNull())
  {
    nAttr.setReadable(portType != FabricCore::DFGPortType_In);
    tAttr.setReadable(portType != FabricCore::DFGPortType_In);
    mAttr.setReadable(portType != FabricCore::DFGPortType_In);
    uAttr.setReadable(portType != FabricCore::DFGPortType_In);
    cAttr.setReadable(portType != FabricCore::DFGPortType_In);
    pAttr.setReadable(portType != FabricCore::DFGPortType_In);

    nAttr.setWritable(portType != FabricCore::DFGPortType_Out);
    tAttr.setWritable(portType != FabricCore::DFGPortType_Out);
    mAttr.setWritable(portType != FabricCore::DFGPortType_Out);
    uAttr.setWritable(portType != FabricCore::DFGPortType_Out);
    cAttr.setWritable(portType != FabricCore::DFGPortType_Out);
    pAttr.setWritable(portType != FabricCore::DFGPortType_Out);

    nAttr.setKeyable(storable);
    tAttr.setKeyable(storable);
    mAttr.setKeyable(storable);
    uAttr.setKeyable(storable);
    cAttr.setKeyable(storable);
    pAttr.setKeyable(storable);

    nAttr.setStorable(storable);
    tAttr.setStorable(storable);
    mAttr.setStorable(storable);
    uAttr.setStorable(storable);
    cAttr.setStorable(storable);
    pAttr.setStorable(storable);

    if(!compoundChild)
      thisNode.addAttribute(newAttribute);
  }

  if(!compoundChild)
    setupMayaAttributeAffects(portName, portType, newAttribute);

  generateAttributeLookups();

  _affectedPlugsDirty = true;
  return newAttribute;

  MAYADFG_CATCH_END(stat);

  return MObject();
}

void FabricDFGBaseInterface::removeMayaAttribute(MString portName, MStatus * stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::removeMayaAttribute");

  MAYASPLICE_CATCH_BEGIN(stat);

  MFnDependencyNode thisNode(getThisMObject());
  MString plugName = getPlugName(portName);
  MPlug plug = thisNode.findPlug(plugName);
  if(!plug.isNull())
  {
    thisNode.removeAttribute(plug.attribute());
    generateAttributeLookups();
    _affectedPlugsDirty = true;
  }

  MAYASPLICE_CATCH_END(stat);
}

void FabricDFGBaseInterface::setupMayaAttributeAffects(MString portName, FabricCore::DFGPortType portType, MObject newAttribute, MStatus *stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setupMayaAttributeAffects");

  MAYASPLICE_CATCH_BEGIN(stat);

  MFnDependencyNode thisNode(getThisMObject());
  MPxNode * userNode = thisNode.userNode();

  if(userNode != NULL)
  {
    FabricCore::DFGExec exec = getDFGExec();
  
    if(portType != FabricCore::DFGPortType_In)
    {
      for(unsigned i = 0; i < exec.getExecPortCount(); ++i) {
        std::string otherPortName = exec.getExecPortName(i);
        if(otherPortName == portName.asChar() && portType != FabricCore::DFGPortType_IO)
          continue;
        if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
          continue;
        MString otherPlugName = getPlugName(otherPortName.c_str());
        MPlug plug = thisNode.findPlug(otherPlugName);
        if(plug.isNull())
          continue;
        userNode->attributeAffects(plug.attribute(), newAttribute);
      }
    }

    if(portType != FabricCore::DFGPortType_Out)
    {
      for(unsigned i = 0; i < exec.getExecPortCount(); ++i) {
        std::string otherPortName = exec.getExecPortName(i);
        if(otherPortName == portName.asChar() && portType != FabricCore::DFGPortType_IO)
          continue;
        if(exec.getExecPortType(i) == FabricCore::DFGPortType_In)
          continue;
        MString otherPlugName = getPlugName(otherPortName.c_str());
        MPlug plug = thisNode.findPlug(otherPlugName);
        if(plug.isNull())
          continue;
        userNode->attributeAffects(newAttribute, plug.attribute());
      }
    }
  }

  MAYASPLICE_CATCH_END(stat);
}

bool FabricDFGBaseInterface::useEvalContext()
{
  MPlug enableEvalContextPlug = getEnableEvalContextPlug();
  if(!enableEvalContextPlug.asBool())
    return false;
  return s_use_evalContext;
}

void FabricDFGBaseInterface::managePortObjectValues(bool destroy)
{
  if(_portObjectsDestroyed == destroy)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::managePortObjectValues");

  // check if we have a valid client
  if(FTL::StrRef(FabricSplice::GetClientContextID()).empty())
    return;

   for(unsigned int i = 0; i < getDFGExec().getExecPortCount(); ++i) {
     try
     {
      FabricCore::RTVal value  = getDFGBinding().getArgValue(i);
       if(!value.isValid())
         continue;
       if(!value.isObject())
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

  if( destroy && m_binding ) {
    // Release cached values and variables, for example InlineDrawingHandle
    m_binding.deallocValues();
  }

  _portObjectsDestroyed = destroy;
}

void FabricDFGBaseInterface::allStorePersistenceData(MString file, MStatus *stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::allStorePersistenceData");

  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->storePersistenceData(file, stat);
}

void FabricDFGBaseInterface::allRestoreFromPersistenceData(MString file, MStatus *stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::allRestoreFromPersistenceData");

  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->restoreFromPersistenceData(file, stat);
}

void FabricDFGBaseInterface::allResetInternalData()
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::allResetInternalData");

  for(size_t i=0;i<_instances.size();i++)
  {
    _instances[i]->_restoredFromPersistenceData = false;
    _instances[i]->_isTransferingInputs = false;
    _instances[i]->_dgDirtyEnabled = true;
    _instances[i]->_portObjectsDestroyed = false;
    _instances[i]->_affectedPlugsDirty = true;
    _instances[i]->_outputsDirtied = false;
    // todo: eventually destroy the binding
    // m_binding = DFGWrapper::Binding();
  }
}

void FabricDFGBaseInterface::setAllRestoredFromPersistenceData(bool value)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setAllRestoredFromPersistenceData");

  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->_restoredFromPersistenceData = value;
}

void FabricDFGBaseInterface::bindingNotificationCallback(
  FTL::CStrRef jsonStr
  )
{
  // [hm 20161108] None of the cases below apply during playback 
  // so we should avoid the json decoding alltogether.
  if(_isEvaluating || _isTransferingInputs)
  {
    return;
  }

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::bindingNotificationCallback");

  // [pz 20160818] We need a bracket here because some the options below
  // can cause further notifications to be fired
  FabricCore::DFGNotifBracket notifBracket( getDFGHost() );

  // MGlobal::displayInfo(jsonStr.data());

  FTL::JSONStrWithLoc jsonStrWithLoc( jsonStr );
  FTL::OwnedPtr<FTL::JSONObject const> jsonObject(
    FTL::JSONValue::Decode( jsonStrWithLoc )->cast<FTL::JSONObject>()
    );

  FTL::CStrRef descStr = jsonObject->getString( FTL_STR("desc") );

  if ( descStr == FTL_STR("dirty") )
  {
    // when we receive this notification we need to 
    // ensure that the DCC reevaluates the node
    if(!_isEvaluating && !_isTransferingInputs)
      incrementEvalID();
  }
  else if( descStr == FTL_STR("argTypeChanged") )
  {
    std::string nameStr = jsonObject->getString( FTL_STR("name") );
    MString plugName = getPlugName(nameStr.c_str());
    std::string newTypeStr = jsonObject->getString( FTL_STR("newType") );

    MFnDependencyNode thisNode(getThisMObject());

    // remove existing attributes if types don't match
    MPlug plug = thisNode.findPlug(plugName);
    if(!plug.isNull())
    {
      return;
      // std::map<std::string, std::string>::iterator it = _argTypes.find(nameStr);
      // if(it != _argTypes.end())
      // {
      //   if(it->second == newTypeStr)
      //     continue;
      // }
      // removeMayaAttribute(nameStr.c_str());
      // _argTypes.erase(it);
    }

    FabricCore::DFGExec exec = getDFGExec();
    FabricCore::DFGPortType portType = exec.getExecPortType(nameStr.c_str());
    addMayaAttribute(nameStr.c_str(), newTypeStr.c_str(), portType);
    _argTypes.insert(std::pair<std::string, std::string>(nameStr, newTypeStr));
  }
  else if( descStr == FTL_STR("argRemoved") )
  {
    std::string nameStr = jsonObject->getString( FTL_STR("name") );
    MString plugName = getPlugName(nameStr.c_str());

    MFnDependencyNode thisNode(getThisMObject());

    // remove existing attributes if types match
    MPlug plug = thisNode.findPlug(plugName);
    if(!plug.isNull())
      removeMayaAttribute(nameStr.c_str());

    generateAttributeLookups();
  }
  else if( descStr == FTL_STR("argRenamed") )
  {
    std::string oldNameStr = jsonObject->getString( FTL_STR("oldName") );
    MString oldPlugName = getPlugName(oldNameStr.c_str());
    std::string newNameStr = jsonObject->getString( FTL_STR("newName") );
    MString newPlugName = getPlugName(newNameStr.c_str());

    MFnDependencyNode thisNode(getThisMObject());

    // remove existing attributes if types match
    MPlug plug = thisNode.findPlug(oldPlugName);
    renamePlug(plug, oldPlugName, newPlugName);

    generateAttributeLookups();
  }
  else if( descStr == FTL_STR("argInserted") )
  {
    generateAttributeLookups();
  }
  else if(   descStr == FTL_STR("varInserted")
          || descStr == FTL_STR("varRemoved") )
  {
    if (   FabricDFGWidget::Instance()
        && FabricDFGWidget::Instance()->getDfgWidget()
        && FabricDFGWidget::Instance()->getDfgWidget()->getUIController())
    FabricDFGWidget::Instance()->getDfgWidget()->getUIController()->emitVarsChanged();
  }
  // else
  // {
  //   mayaLogFunc(jsonStr.c_str());
  // }
}

void FabricDFGBaseInterface::VisitInputArgsCallback(
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
)
{
  if(argOutsidePortType == FabricCore::DFGPortType_Out)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::VisitInputArgsCallback");

  VisitCallbackUserData * ud = (VisitCallbackUserData *)userdata;

  assert(argIndex < ud->interf->_argIndexToAttributeIndex.size());
  unsigned int attributeIndex = ud->interf->_argIndexToAttributeIndex[argIndex];
  if(attributeIndex == UINT_MAX)
    return;

  assert(attributeIndex < ud->interf->_isAttributeIndexDirty.size());
  if(!ud->interf->_isAttributeIndexDirty[attributeIndex])
    return;

  MPlug plug = ud->interf->_attributePlugs[attributeIndex];

  DFGPlugToArgFunc func = ud->interf->_plugToArgFuncs[argIndex];
  if(func == NULL)
  {
    FabricMayaProfilingEvent bracket("finding conversion func");

    FTL::StrRef portDataType = argTypeName;
    if(portDataType.substr(portDataType.size()-2, 2) == FTL_STR("[]"))
      portDataType = portDataType.substr(0, portDataType.size()-2);

    // todo: the mSpliceMayaDataOverride should also be a uint32 lookup
    for(size_t j=0;j<ud->interf->mSpliceMayaDataOverride.size();j++)
    {
      if(ud->interf->mSpliceMayaDataOverride[j] == argName)
      {
        portDataType = FTL_STR("SpliceMayaData");
        break;
      }
    }

    func = getDFGPlugToArgFunc(portDataType);
    if(func == NULL)
      return;
    ud->interf->_plugToArgFuncs[argIndex] = func;
  }

  {
    FabricMayaProfilingEvent bracket("conversion");
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
      plug,
      ud->data
      );

    ud->interf->_isAttributeIndexDirty[attributeIndex] = false;
  }
}

void FabricDFGBaseInterface::VisitOutputArgsCallback(
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
)
{
  if(argOutsidePortType == FabricCore::DFGPortType_In)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::VisitOutputArgsCallback");

  VisitCallbackUserData * ud = (VisitCallbackUserData *)userdata;

  assert(argIndex < ud->interf->_argIndexToAttributeIndex.size());
  unsigned int attributeIndex = ud->interf->_argIndexToAttributeIndex[argIndex];
  if(attributeIndex == UINT_MAX)
    return;
  MPlug plug = ud->interf->_attributePlugs[attributeIndex];

  DFGArgToPlugFunc func = ud->interf->_argToPlugFuncs[argIndex];
  if(func == NULL)
  {
    FabricMayaProfilingEvent bracket("getting conversion func");

    FTL::StrRef portDataType = argTypeName;
    if(portDataType.substr(portDataType.size()-2, 2) == FTL_STR("[]"))
      portDataType = portDataType.substr(0, portDataType.size()-2);

    for(size_t j=0;j<ud->interf->mSpliceMayaDataOverride.size();j++)
    {
      if(ud->interf->mSpliceMayaDataOverride[j] == argName)
      {
        portDataType = FTL_STR("SpliceMayaData");
        break;
      }
    }

    if(ud->isDeformer && portDataType == FTL_STR("PolygonMesh"))
      //data.setClean(plug);  // [FE-6087]
                              // 'setClean()' need not be called for MPxDeformerNode.
                              // (see comments of FE-6087 for more detailed information)
      return;

    func = getDFGArgToPlugFunc(portDataType);
    if(func == NULL)
      return;
    ud->interf->_argToPlugFuncs[argIndex] = func;
  }

  {
    FabricMayaProfilingEvent bracket("conversion");
    (*func)(
      argIndex,
      argName,
      argTypeName,
      argOutsidePortType,
      argRawDataSize,
      getCB,
      getRawCB,
      getSetUD,
      plug,
      ud->data
      );
    ud->data.setClean(plug);
  }
}

void FabricDFGBaseInterface::renamePlug(const MPlug &plug, MString oldName, MString newName)
{
  if(plug.isNull())
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::renamePlug");

  MFnDependencyNode thisNode(getThisMObject());

  MString newPlugName = newName + plug.partialName().substring(oldName.length(), plug.partialName().length());
  MString cmdStr = "renameAttr \"";
  cmdStr += thisNode.name() + "." + plug.partialName();
  cmdStr += "\" \"" + newPlugName + "\";";
  MGlobal::executeCommand( cmdStr, false, false );

  for(unsigned int i=0;i<plug.numChildren();i++)
    renamePlug( plug.child(i), oldName, newName );
}

MString FabricDFGBaseInterface::resolveEnvironmentVariables(const MString & filePath)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::resolveEnvironmentVariables");
  std::string text = filePath.asChar();
  std::string output;
  for(unsigned int i=0;i<text.length()-1;i++)
  {
    if(text[i] == '$' && text[i+1] == '{')
    {
      size_t closePos = text.find('}', i);
      if(closePos != std::string::npos)
      {
        std::string envVarName = text.substr(i+2, closePos - i - 2);
        const char * envVarValue = getenv(envVarName.c_str());
        if(envVarValue != NULL)
        {
          output += envVarValue;
          i = (unsigned int)closePos;
          continue;
        }
      }
    }
    output += text[i];
  }

  if(text.length() > 0)
  {
    if(text[text.length()-1] != '}')
      output += text[text.length()-1];
  }
  return output.c_str();
}

bool FabricDFGBaseInterface::getExecuteShared()
{
  if ( m_executeSharedDirty )
  {
    FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::getExecuteShared");

    FTL::CStrRef executeSharedMetadataCStr =
      m_binding.getMetadata( "executeShared" );
    if ( executeSharedMetadataCStr == FTL_STR("true") )
      m_executeShared = true;
    else if ( executeSharedMetadataCStr == FTL_STR("false") )
      m_executeShared = false;
    else
    {
      static bool haveDefaultExecuteShared = false;
      static bool defaultExecuteShared;
      if ( !haveDefaultExecuteShared )
      {
        char const *envvar = ::getenv( "FABRIC_CANVAS_PARALLEL_DEFAULT" );
        defaultExecuteShared = envvar && atoi( envvar ) > 0;
        haveDefaultExecuteShared = true;
      }
      m_executeShared = defaultExecuteShared;
    }
    m_executeSharedDirty = false;
  }
  return m_executeShared;
}

#if MAYA_API_VERSION >= 201600
MStatus FabricDFGBaseInterface::doPreEvaluation(
  MObject thisMObject,
  const MDGContext& context,
  const MEvaluationNode& evaluationNode
  )
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::doPreEvaluation");

  MStatus status;
  if(!context.isNormal()) 
    return MStatus::kFailure;

  // [andrew 20150616] in 2016 this needs to also happen here because
  // setDependentsDirty isn't called in Serial or Parallel eval mode
  for (MEvaluationNodeIterator dirtyIt = evaluationNode.iterator();
      !dirtyIt.isDone(); dirtyIt.next())
  {
    _outputsDirtied = true;
    collectDirtyPlug(dirtyIt.plug());
  }
  return MS::kSuccess;
}

MStatus FabricDFGBaseInterface::doPostEvaluation(
  MObject thisMObject,
  const MDGContext& context,
  const MEvaluationNode& evaluationNode,
  MPxNode::PostEvaluationType evalType
  )
{
  return MStatus::kSuccess;
}
#endif

bool FabricDFGBaseInterface::HasPort(const char *in_portName, const bool testForInput)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::HasPort");

  try
  {
    // check/init.
    if (!in_portName || in_portName[0] == '\0')  return false;
    const FabricCore::DFGPortType portType = (testForInput ? FabricCore::DFGPortType_In : FabricCore::DFGPortType_Out);

    // get the graph.
    FabricCore::DFGExec graph = m_binding.getExec();
    if (!graph.isValid())
    {
      std::string s = "BaseInterface::HasPort(): failed to get graph!";
      mayaLogErrorFunc(s.c_str(), s.length());
      return false;
    }

    // return result.
    return (graph.haveExecPort(in_portName) && graph.getExecPortType(in_portName) == portType);
  }
  catch (FabricCore::Exception e)
  {
    return false;
  }
}

bool FabricDFGBaseInterface::HasInputPort(const char *portName)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::HasInputPort");
  return HasPort(portName, true);
}

int FabricDFGBaseInterface::GetArgValueBoolean(FabricCore::DFGBinding &binding, char const * argName, bool &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueBoolean");

  // init output.
  out = false;

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "Boolean")     out = rtval.getBoolean();

    else if (!strict)
    {
      if      (resolvedType == "Scalar")    out = (0 != rtval.getFloat32());
      else if (resolvedType == "Float32")   out = (0 != rtval.getFloat32());
      else if (resolvedType == "Float64")   out = (0 != rtval.getFloat64());

      else if (resolvedType == "Integer")   out = (0 != rtval.getSInt32());
      else if (resolvedType == "SInt8")     out = (0 != rtval.getSInt8());
      else if (resolvedType == "SInt16")    out = (0 != rtval.getSInt16());
      else if (resolvedType == "SInt32")    out = (0 != rtval.getSInt32());
      else if (resolvedType == "SInt64")    out = (0 != rtval.getSInt64());

      else if (resolvedType == "Byte")      out = (0 != rtval.getUInt8());
      else if (resolvedType == "UInt8")     out = (0 != rtval.getUInt8());
      else if (resolvedType == "UInt16")    out = (0 != rtval.getUInt16());
      else if (resolvedType == "Count")     out = (0 != rtval.getUInt32());
      else if (resolvedType == "Index")     out = (0 != rtval.getUInt32());
      else if (resolvedType == "Size")      out = (0 != rtval.getUInt32());
      else if (resolvedType == "UInt32")    out = (0 != rtval.getUInt32());
      else if (resolvedType == "DataSize")  out = (0 != rtval.getUInt64());
      else if (resolvedType == "UInt64")    out = (0 != rtval.getUInt64());

      else return -1;
    }
    else return -1;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}

int FabricDFGBaseInterface::GetArgValueInteger(FabricCore::DFGBinding &binding, char const * argName, int &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueInteger");

  // init output.
  out = 0;

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "Integer")     out = (int)rtval.getSInt32();
    else if (resolvedType == "SInt8")       out = (int)rtval.getSInt8();
    else if (resolvedType == "SInt16")      out = (int)rtval.getSInt16();
    else if (resolvedType == "SInt32")      out = (int)rtval.getSInt32();
    else if (resolvedType == "SInt64")      out = (int)rtval.getSInt64();

    else if (resolvedType == "Byte")        out = (int)rtval.getUInt8();
    else if (resolvedType == "UInt8")       out = (int)rtval.getUInt8();
    else if (resolvedType == "UInt16")      out = (int)rtval.getUInt16();
    else if (resolvedType == "Count")       out = (int)rtval.getUInt32();
    else if (resolvedType == "Index")       out = (int)rtval.getUInt32();
    else if (resolvedType == "Size")        out = (int)rtval.getUInt32();
    else if (resolvedType == "UInt32")      out = (int)rtval.getUInt32();
    else if (resolvedType == "DataSize")    out = (int)rtval.getUInt64();
    else if (resolvedType == "UInt64")      out = (int)rtval.getUInt64();

    else if (!strict)
    {
      if      (resolvedType == "Boolean")   out = (int)rtval.getBoolean();

      else if (resolvedType == "Scalar")    out = (int)rtval.getFloat32();
      else if (resolvedType == "Float32")   out = (int)rtval.getFloat32();
      else if (resolvedType == "Float64")   out = (int)rtval.getFloat64();

      else return -1;
    }
    else return -1;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}

int FabricDFGBaseInterface::GetArgValueFloat(FabricCore::DFGBinding &binding, char const * argName, double &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueFloat");

  // init output.
  out = 0;

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "Scalar")      out = (double)rtval.getFloat32();
    else if (resolvedType == "Float32")     out = (double)rtval.getFloat32();
    else if (resolvedType == "Float64")     out = (double)rtval.getFloat64();

    else if (!strict)
    {
      if      (resolvedType == "Boolean")   out = (double)rtval.getBoolean();

      else if (resolvedType == "Integer")   out = (double)rtval.getSInt32();
      else if (resolvedType == "SInt8")     out = (double)rtval.getSInt8();
      else if (resolvedType == "SInt16")    out = (double)rtval.getSInt16();
      else if (resolvedType == "SInt32")    out = (double)rtval.getSInt32();
      else if (resolvedType == "SInt64")    out = (double)rtval.getSInt64();

      else if (resolvedType == "Byte")      out = (double)rtval.getUInt8();
      else if (resolvedType == "UInt8")     out = (double)rtval.getUInt8();
      else if (resolvedType == "UInt16")    out = (double)rtval.getUInt16();
      else if (resolvedType == "Count")     out = (double)rtval.getUInt32();
      else if (resolvedType == "Index")     out = (double)rtval.getUInt32();
      else if (resolvedType == "Size")      out = (double)rtval.getUInt32();
      else if (resolvedType == "UInt32")    out = (double)rtval.getUInt32();
      else if (resolvedType == "DataSize")  out = (double)rtval.getUInt64();
      else if (resolvedType == "UInt64")    out = (double)rtval.getUInt64();

      else return -1;
    }
    else return -1;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}

int FabricDFGBaseInterface::GetArgValueString(FabricCore::DFGBinding &binding, char const * argName, std::string &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueString");

  // init output.
  out = "";

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)    return -1;

    else if (resolvedType == "String")      out = rtval.getStringCString();

    else if (!strict)
    {
      char    s[64];
      bool    b;
      int     i;
      double  f;

      if (GetArgValueBoolean(binding, argName, b, true) == 0)
      {
        out = (b ? "true" : "false");
        return 0;
      }

      if (GetArgValueInteger(binding, argName, i, true) == 0)
      {
        #ifdef _WIN32
          sprintf_s(s, sizeof(s), "%ld", i);
        #else
          snprintf(s, sizeof(s), "%d", i);
        #endif
        out = s;
        return 0;
      }

      if (GetArgValueFloat(binding, argName, f, true) == 0)
      {
        #ifdef _WIN32
          sprintf_s(s, sizeof(s), "%f", f);
        #else
          snprintf(s, sizeof(s), "%f", f);
        #endif
        out = s;
        return 0;
      }

      return -1;
    }
    else
      return -1;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}

int FabricDFGBaseInterface::GetArgValueVec3(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueVec3");

  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Vec3")        {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                            }
    else if (!strict)
    {
        if      (resolvedType == "Color")   {
                                              out.push_back(rtval.maybeGetMember("r").getFloat32());
                                              out.push_back(rtval.maybeGetMember("g").getFloat32());
                                              out.push_back(rtval.maybeGetMember("b").getFloat32());
                                            }
        else if (resolvedType == "Vec4")    {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                            }
        else
          return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}

int FabricDFGBaseInterface::GetArgValueColor(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueColor");

  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Color")      {
                                              out.push_back(rtval.maybeGetMember("r").getFloat32());
                                              out.push_back(rtval.maybeGetMember("g").getFloat32());
                                              out.push_back(rtval.maybeGetMember("b").getFloat32());
                                              out.push_back(rtval.maybeGetMember("a").getFloat32());
                                           }
    else if (!strict)
    {
        if      (resolvedType == "Vec4")   {
                                              out.push_back(rtval.maybeGetMember("x").getFloat32());
                                              out.push_back(rtval.maybeGetMember("y").getFloat32());
                                              out.push_back(rtval.maybeGetMember("z").getFloat32());
                                              out.push_back(rtval.maybeGetMember("t").getFloat32());
                                            }
        else if (resolvedType == "RGB")     {
                                              out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                              out.push_back(1);
                                            }
        else if (resolvedType == "RGBA")    {
                                              out.push_back(rtval.maybeGetMember("r").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("g").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("b").getUInt8() / 255.0);
                                              out.push_back(rtval.maybeGetMember("a").getUInt8() / 255.0);
                                            }
        else
          return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}

int FabricDFGBaseInterface::GetArgValueMat44(FabricCore::DFGBinding &binding, char const * argName, std::vector <double> &out, bool strict)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::GetArgValueMat44");

  // init output.
  out.clear();

  // set out from port value.
  try
  {
    // invalid port?
    if (!binding.getExec().haveExecPort(argName))
      return -2;

    std::string resolvedType = binding.getExec().getExecPortResolvedType(argName);
    FabricCore::RTVal rtval  = binding.getArgValue(argName);

    if      (resolvedType.length() == 0)      return -1;

    else if (resolvedType == "Mat44")       {
                                              char member[32];
                                              FabricCore::RTVal rtRow;
                                              for (int i = 0; i < 4; i++)
                                              {
                                                #ifdef _WIN32
                                                  sprintf_s(member, sizeof(member), "row%ld", i);
                                                #else
                                                  snprintf(member, sizeof(member), "row%d", i);
                                                #endif
                                                rtRow = rtval.maybeGetMember(member);
                                                out.push_back(rtRow.maybeGetMember("x").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("y").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("z").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("t").getFloat32());
                                              }
                                            }
    else if (!strict)
    {
        if      (resolvedType == "Xfo")     {
                                              FabricCore::RTVal rtmat44 = rtval.callMethod("Mat44", "toMat44", 0, NULL);
                                              char member[32];
                                              FabricCore::RTVal rtRow;
                                              for (int i = 0; i < 4; i++)
                                              {
                                                #ifdef _WIN32
                                                  sprintf_s(member, sizeof(member), "row%ld", i);
                                                #else
                                                  snprintf(member, sizeof(member), "row%d", i);
                                                #endif
                                                rtRow = rtmat44.maybeGetMember(member);
                                                out.push_back(rtRow.maybeGetMember("x").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("y").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("z").getFloat32());
                                                out.push_back(rtRow.maybeGetMember("t").getFloat32());
                                              }
                                            }
        else
          return -1;
    }
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return -4;
  }

  // done.
  return 0;
}
