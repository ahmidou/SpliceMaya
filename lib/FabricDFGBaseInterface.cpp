//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricDFGWidget.h"
#include "FabricDFGProfiling.h"
#include "FabricSpliceHelpers.h"
#include "FabricSpliceMayaData.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricMayaException.h"
#include <Persistence/RTValToJSONEncoder.hpp>

#include <string>
#include <FTL/AutoSet.h>
#include <FTL/JSONValue.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MFileIO.h>
#include <maya/MAnimControl.h>
#include <maya/MFnStringData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnCompoundAttribute.h>

#if MAYA_API_VERSION >= 201600
# include <maya/MEvaluationNode.h>
# include <maya/MEvaluationManager.h>
#endif

#include <FabricUI/Commands/CommandRegistry.h>
#include <FabricUI/DFG/Commands/DFGPathValueResolver.h>
#include <FabricUI/Commands/PathValueResolverRegistry.h>

using namespace FabricUI;
using namespace DFG;
using namespace FabricCore;
using namespace FabricUI::Commands;

std::vector<FabricDFGBaseInterface*> FabricDFGBaseInterface::_instances;
#if MAYA_API_VERSION < 201300
  std::map<std::string, int> FabricDFGBaseInterface::_nodeCreatorCounts;
#endif
unsigned int FabricDFGBaseInterface::s_maxID = 1;
bool FabricDFGBaseInterface::s_use_evalContext = true; // [FE-6287]
MStringArray FabricDFGBaseInterface::s_queuedMelCommands;

FabricDFGBaseInterface::FabricDFGBaseInterface(
  CreateDFGBindingFunc createDFGBinding)
  : m_executeSharedDirty( true )
  , m_createDFGBinding( createDFGBinding )
{
  MStatus stat;
  MAYADFG_CATCH_BEGIN(&stat);

  if (getNumInstances() == 0)
  {
    MString version = "Fabric Engine version " + MString(GetVersionWithBuildInfoStr());
    mayaLogFunc(version);
  }

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

  m_evalContext = RTVal();
  m_binding = DFGBinding();

  QString resolverID = "DFGPathValueResolver_" + QString::number( m_id );
  PathValueResolverFactory<DFGPathValueResolver>::Unregister( resolverID );

  for(size_t i=0;i<_instances.size();i++){
    if(_instances[i] == this){
      std::vector<FabricDFGBaseInterface*>::iterator iter = _instances.begin() + i;
      _instances.erase(iter);
      break;
    }
  }
}

void FabricDFGBaseInterface::bindingChanged() 
{
  m_binding.setNotificationCallback( BindingNotificationCallback, this );
  MString idStr; idStr.set(m_id);
  m_binding.setMetadata("maya_id", idStr.asChar(), false);

  MFnDependencyNode thisNode(getThisMObject());
 
  m_binding.setMetadata("resolver_id", thisNode.name().asChar(), false);
  m_binding.setMetadata("host_app", "Maya", false);

  QString resolverID = "DFGPathValueResolver_" + QString::number(m_id);
  PathValueResolverFactory<DFGPathValueResolver>::Register(resolverID);
  DFGPathValueResolver *resolver = qobject_cast<DFGPathValueResolver *>(
    PathValueResolverRegistry::getRegistry()->getResolver(resolverID)
    );
  resolver->onBindingChanged(m_binding); 
}

void FabricDFGBaseInterface::constructBaseInterface()
{
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
  DFGHost dfgHost = m_client.getDFGHost();

  if(!MFileIO::isOpeningFile())
  {
    m_binding = m_createDFGBinding( dfgHost );
    bindingChanged();
  }

  m_evalContext = RTVal::Create(m_client, "EvalContext", 0, 0);
  m_evalContext = m_evalContext.callMethod("EvalContext", "getInstance", 0, 0);
  m_evalContext.setMember("host", RTVal::ConstructString(m_client, "Maya"));

  MAYADFG_CATCH_END(&stat);
}

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceByName(
  const std::string & name) 
{
  MSelectionList selList;
  MGlobal::getSelectionListByName(name.c_str(), selList);
  MObject spliceMayaNodeObj;
  selList.getDependNode(0, spliceMayaNodeObj);
  return getInstanceByMObject(spliceMayaNodeObj);
}

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceByMObject(const MObject & object)
{
  for(size_t i=0;i<_instances.size();i++)
  {
    if(_instances[i]->getThisMObject() == object)
    {
      return _instances[i];
    }
  }
  return NULL;
}

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceById(
  unsigned int id)
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

FabricDFGBaseInterface * FabricDFGBaseInterface::getInstanceByIndex(
  unsigned int index)
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

Client FabricDFGBaseInterface::getCoreClient()
{
  return m_client;
}

DFGHost FabricDFGBaseInterface::getDFGHost()
{
  if(m_binding.isValid())
    return m_binding.getHost();
  return DFGHost();
}

DFGBinding FabricDFGBaseInterface::getDFGBinding()
{
  return m_binding;
}

DFGExec FabricDFGBaseInterface::getDFGExec()
{
  if(m_binding.isValid())
    return m_binding.getExec();
  return DFGExec();
}

bool FabricDFGBaseInterface::transferInputValuesToDFG(
  MDataBlock& data)
{
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

void FabricDFGBaseInterface::evaluate()
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::evaluate");

  FTL::AutoSet<bool> transfersInputs(_isEvaluating, true);
  _dgDirtyQueued = false;

  MFnDependencyNode thisNode(getThisMObject());

  managePortObjectValues(false); // recreate objects if not there yet

  if (useEvalContext())
  {
    FabricMayaProfilingEvent bracket("setting up eval context");

    if(m_evalContext.isValid())
    {
      try
      {
        m_evalContext.setMember("time", RTVal::ConstructFloat32(m_client, MAnimControl::currentTime().as(MTime::kSeconds)));
        MString idStr; idStr.set(m_id);
        m_evalContext.setMember("evalContextID", RTVal::ConstructString(m_client, idStr.asChar()));
      }
      catch(Exception e)
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

void FabricDFGBaseInterface::transferOutputValuesToMaya(
  MDataBlock& data, 
  bool isDeformer)
{
  if(_isTransferingInputs)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::transferOutputValuesToMaya");

  managePortObjectValues(false); // recreate objects if not there yet

  VisitCallbackUserData ud(getThisMObject(), data);
  ud.interf = this;
  ud.isDeformer = isDeformer;

  getDFGBinding().visitArgs(getLockType(), &FabricDFGBaseInterface::VisitOutputArgsCallback, &ud);
}

void FabricDFGBaseInterface::collectDirtyPlug(
  MPlug const &inPlug)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::collectDirtyPlug");

  // [hmathee 20161110] take a short cut - if we find the attribute index
  // based on the attribute name then we can exit
  {
    MString attrName = MFnAttribute(inPlug.attribute()).name();
    FTL::StrRef attrNameRef = attrName.asChar();
    FTL::OrderedStringMap< unsigned int >::const_iterator it = _attributeNameToIndex.find(attrNameRef);
    if(it != _attributeNameToIndex.end())
    {
      _isAttributeIndexDirty[it->value()] = true;
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
    unsigned int index = it->value();
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
  DFGExec exec = getDFGBinding().getExec();

  // FE-7682 Reducing the length of the plugArray makes Maya crash
  // because it invokes the destruction of the plug  the plug has already been deleted.   
  // http://download.autodesk.com/us/maya/2011help/api/class_m_plug_array.html#97f9b95167d95e3512ab82b559263ba3   
  // _attributePlugs.setLength(thisNode.attributeCount());
  for(unsigned int i = 0; i < thisNode.attributeCount(); ++i)
  {
    MFnAttribute attrib(thisNode.attribute(i));

    while(i >= _isAttributeIndexDirty.size())
    {
      _isAttributeIndexDirty.push_back(true); 
    }

    if( i < _attributePlugs.length() )
      _attributePlugs.set(MPlug(getThisMObject(), thisNode.attribute(i)), i);
    else
      _attributePlugs.insert(MPlug(getThisMObject(), thisNode.attribute(i)), i);

    MString attrName = attrib.name();
    FTL::StrRef attrNameRef = attrName.asChar();
    if(attrNameRef == FTL_STR("saveData") || 
      attrNameRef == FTL_STR("refFilePath") || 
      attrNameRef == FTL_STR("enableEvalContext") ||
      attrNameRef == FTL_STR("nodeState") ||
      attrNameRef == FTL_STR("caching") ||
      attrNameRef == FTL_STR("frozen"))
      continue;

    // find the top level attribute
    MPlug plug(getThisMObject(), thisNode.attribute(i));
    while(plug.isChild()) {
      if(plug.parent().isElement())
        plug = plug.parent().array();
      else
        plug = plug.parent();
    }

    // for attributes which are not top level attributes
    // we need to store the index of the top level attribute
    // in the map. so say if plug positions[11].x is hit, we
    // need to store the index like so for example:
    // _attributeNameToIndex["positions"] = 3
    // _attributeNameToIndex["positions[11].x"] = 3
    // where 3 is the index of the positions attribute.
    MFnAttribute parentAttribute(plug.attribute());
    MString parentAttributeName = parentAttribute.name();
    FTL::StrRef parentAttributeNameRef = parentAttributeName.asChar();
    FTL::OrderedStringMap< unsigned int >::const_iterator it = _attributeNameToIndex.find(parentAttributeNameRef);
    if(it != _attributeNameToIndex.end())
      _attributeNameToIndex.insert(attrib.name().asChar(), it->value());
    else
      _attributeNameToIndex.insert(attrib.name().asChar(), i);
  }

  for(unsigned int i = thisNode.attributeCount(); i < _attributePlugs.length(); ++i)
    _attributePlugs.remove(i);


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

void FabricDFGBaseInterface::affectChildPlugs(
  MPlug &plug, 
  MPlugArray &affectedPlugs)
{
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

void FabricDFGBaseInterface::storePersistenceData(
  MString file, 
  MStatus *stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::storePersistenceData");

  MAYADFG_CATCH_BEGIN(stat);

  FTL::AutoSet<bool> storingJson(m_isStoringJson, true);

  std::string json = m_binding.exportJSON().getCString();
  MPlug saveDataPlug = getSaveDataPlug();
  saveDataPlug.setString(json.c_str());

  MAYADFG_CATCH_END(stat);
}

void FabricDFGBaseInterface::restoreFromPersistenceData(
  MString file, 
  MStatus *stat)
{
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

void FabricDFGBaseInterface::restoreFromJSON(
  MString json, 
  MStatus *stat)
{
  if(_restoredFromPersistenceData)
    return;

  // ensure to create the base interface here
  // this ensure to have a client + a binding objects
  constructBaseInterface();

  if(m_lastJson == json)
    return;

  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::restoreFromJSON");

  MAYADFG_CATCH_BEGIN(stat);

  if ( m_binding )
    m_binding.setNotificationCallback( NULL, NULL );

  DFGHost dfgHost = m_client.getDFGHost();
  m_binding = dfgHost.createBindingFromJSON(json.asChar());
  bindingChanged();

  // todo: update UI

  _restoredFromPersistenceData = true;

  invalidateNode();

  MFnDependencyNode thisNode(getThisMObject());
  FTL::StrRef execPath;
  DFGExec exec = m_binding.getExec();

  for(unsigned i = 0; i < exec.getExecPortCount(); ++i){
    std::string portName = exec.getExecPortName(i);
    MString plugName = getPlugName(portName.c_str());
    MPlug plug = thisNode.findPlug(plugName);
    if(!plug.isNull())
      continue;

    DFGPortType portType = exec.getExecPortType(i);
    if (!exec.getExecPortResolvedType(i)) continue; // [FE-5538]
    std::string dataType = exec.getExecPortResolvedType(i);

    FTL::StrRef opaque = exec.getExecPortMetadata(portName.c_str(), "opaque");
    if(opaque == "true")
      dataType = "SpliceMayaData";

    FabricServices::CodeCompletion::KLTypeDesc typeDesc(dataType);

    std::string arrayType = "Single Value";
    if( typeDesc.isArray() )
    {
      arrayType = "Array (Multi)";
      FTL::StrRef nativeArray = exec.getExecPortMetadata(portName.c_str(), "nativeArray");
      if(nativeArray == "true")
      {
        arrayType = "Array (Native)";
        exec.setExecPortMetadata(portName.c_str(), "nativeArray", "true", false /* canUndo */);
      }
    } else if( dataType == "Curves" )//We map single Curves to Maya curve arrays
      arrayType = "Array (Multi)";

    FTL::StrRef addAttribute = exec.getExecPortMetadata(portName.c_str(), "addAttribute");

    if(addAttribute != "false")
      addMayaAttribute(portName.c_str(), dataType.c_str(), portType, arrayType.c_str());
  }

  for(unsigned i = 0; i < exec.getExecPortCount(); ++i){
    std::string portName = exec.getExecPortName(i);
    MString plugName = getPlugName(portName.c_str());
    MPlug plug = thisNode.findPlug(plugName);
    if(plug.isNull())
      continue;

    // force an execution of the node    
    DFGPortType portType = exec.getExecPortType(i);
    if(portType != DFGPortType_Out)
    {
      MString command("dgeval ");
      // MGlobal::executeCommandOnIdle(command+thisNode.name()+"."+plugName);
      queueMelCommand(command+thisNode.name()+"."+plugName);
      break;
    }
  }

  m_lastJson = json;

  generateAttributeLookups();

  MAYADFG_CATCH_END(stat);
}

void FabricDFGBaseInterface::setReferencedFilePath(
  MString filePath)
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

void FabricDFGBaseInterface::setDgDirtyEnabled(
  bool enabled) 
{ 
  _dgDirtyEnabled = enabled; 
}

MString FabricDFGBaseInterface::getPlugName(
  const MString &portName)
{
  if      (portName == "message")            return "dfg_message";
  else if (portName == "saveData")           return "dfg_saveData";
  else if (portName == "refFilePath")        return "dfg_refFilePath";
  else if (portName == "enableEvalContext")  return "dfg_enableEvalContext";
  else                                       return portName;
}

MString FabricDFGBaseInterface::getPortName(
  const MString &plugName)
{
  if      (plugName == "dfg_message")           return "message";
  else if (plugName == "dfg_saveData")          return "saveData";
  else if (plugName == "dfg_refFilePath")       return "refFilePath";
  else if (plugName == "dfg_enableEvalContext") return "enableEvalContext";
  else                                          return plugName;
}

void FabricDFGBaseInterface::invalidatePlug(
  MPlug & plug)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::invalidatePlug");

  if(!_dgDirtyEnabled)
    return;
  if(plug.isNull())
    return;
  if(plug.attribute().isNull())
    return;
  if(!m_binding.isValid())
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
    // MGlobal::executeCommandOnIdle(command+plugName);
    queueMelCommand(command+plugName);
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
            // MGlobal::executeCommandOnIdle(cmds[i]);
            queueMelCommand(cmds[i]);
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

  unsigned int dirtiedInputs = 0;

  DFGExec exec = getDFGExec();
  if(exec.isValid())
  {
    // ensure we setup the mayaSpliceData overrides first
    mSpliceMayaDataOverride.resize(0);
    for(unsigned int i = 0; i < exec.getExecPortCount(); ++i){
      // if(ports[i]->isManipulatable())
      //   continue;
      std::string portName = exec.getExecPortName(i);
      MString plugName = getPlugName(portName.c_str());
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
    for(unsigned int i = 0; i < exec.getExecPortCount(); ++i){
      std::string portName = exec.getExecPortName(i);
      MString plugName = getPlugName(portName.c_str());
      MPlug plug = thisNode.findPlug(plugName);

      DFGPortType portType = exec.getExecPortType(i);
      if(!plug.isNull()){
        if(portType == DFGPortType_In)
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
  }

  // if there are no inputs on this 
  // node, let's rely on the eval id attribute
  if(dirtiedInputs == 0)
  {
    queueIncrementEvalID(true /* onIdle */);
  }

  _affectedPlugsDirty = true;
  _outputsDirtied = false;
}

void FabricDFGBaseInterface::queueIncrementEvalID(
  bool onIdle)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::queueIncrementEvalID");

  if(_dgDirtyQueued || onIdle)
  {
    MString command("FabricCanvasIncrementEvalID -index ");
    MString indexStr;
    indexStr.set((int)m_id);

    // MGlobal::executeCommandOnIdle(command+indexStr, false /*display*/);
    queueMelCommand(command+indexStr);
  }
  else
  {
    incrementEvalID();
  }
}

void FabricDFGBaseInterface::incrementEvalID()
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::incrementEvalID");

  MFnDependencyNode thisNode(getThisMObject());
  MPlug evalIDPlug = thisNode.findPlug("evalID");
  if(evalIDPlug.isNull())
    return;

  m_evalID++;

  MString plugName = evalIDPlug.name();
  MString command("setAttr ");
  MString evalIDStr;
  evalIDStr.set(m_evalID);

  MGlobal::executeCommand(command+plugName+" "+evalIDStr, false /*display*/, false /*undoable*/);
}

void FabricDFGBaseInterface::queueMelCommand(
  MString cmd)
{
  // todo: this needs to use a mutex~!
  s_queuedMelCommands.append(cmd);
  if(s_queuedMelCommands.length() == 1)
    MGlobal::executeCommandOnIdle("FabricCanvasProcessMelQueue;");
}

MStatus FabricDFGBaseInterface::processQueuedMelCommands()
{
  // todo: this needs to use a mutex~!
  MStatus result = MS::kSuccess;
  for(unsigned int i=0;i<s_queuedMelCommands.length();i++)
  {
    MStatus st = MGlobal::executeCommand(s_queuedMelCommands[i]);
    if(st != MS::kSuccess)
      result = st;
  }
  s_queuedMelCommands.clear();
  return result;
}

DFGUICmdHandler_Maya *FabricDFGBaseInterface::getCmdHandler() 
{ 
  return &m_cmdHandler; 
}

void FabricDFGBaseInterface::setExecuteSharedDirty() 
{ 
  m_executeSharedDirty = true; 
}

bool FabricDFGBaseInterface::plugInArray(
  const MPlug &plug, 
  const MPlugArray &array)
{
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

MStatus FabricDFGBaseInterface::setDependentsDirty(
  MObject thisMObject, 
  MPlug const &inPlug, 
  MPlugArray &affectedPlugs)
{
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
        DFGExec exec = getDFGExec();
        DFGPortType portType = exec.getExecPortType(attrib.name().asChar());
        if(portType == DFGPortType_In)
          continue;
      }
      catch(Exception e)
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

void FabricDFGBaseInterface::copyInternalData(
  MPxNode *node)
{
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

bool FabricDFGBaseInterface::getInternalValueInContext(
  const MPlug &plug, 
  MDataHandle &dataHandle, 
  MDGContext &ctx)
{
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

bool FabricDFGBaseInterface::setInternalValueInContext(
  const MPlug &plug, 
  const MDataHandle &dataHandle, 
  MDGContext &ctx)
{
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

void FabricDFGBaseInterface::onNodeAdded(
  MObject &node, 
  void *clientData)
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

    // // [FE-7498] lock the 'saveData' attribute to prevent problems
    // // when referencing .ma files that contain Canvas nodes.
    // MPlug plug = interf->getSaveDataPlug();
    // if (!plug.isNull())
    //   plug.setLocked(true);
  }
}

void FabricDFGBaseInterface::onNodeRemoved(
  MObject &node, 
  void *clientData)
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

void FabricDFGBaseInterface::onAnimCurveEdited(
  MObjectArray &editedCurves, 
  void *clientData)
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

void FabricDFGBaseInterface::onConnection(
  const MPlug &plug, 
  const MPlug &otherPlug, 
  bool asSrc, 
  bool made)
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

// ********************   ********************  //
inline bool AddSingleBaseTypeAttribute(
  DFGBinding &binding, 
  MString plugName, 
  MFnNumericData::Type numType, 
  MString portName,
  float uiSoftMin,
  float uiSoftMax,
  float uiHardMin,
  float uiHardMax, 
  bool &storable,
  MFnNumericAttribute &nAttr, 
  MFnTypedAttribute &tAttr,
  MObject &newAttribute) 
{
  bool res = false;
  try
  { 
    RTVal rtval = binding.getArgValue(portName.asChar());
    uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());

    if(binding.getExec().isExecPortResolvedType(portIndex, "String"))
    {
      MFnStringData stringData;
      MObject strObject = stringData.create(rtval.getStringCString());
      newAttribute = tAttr.create(plugName, plugName, MFnData::kString, strObject);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "Boolean"))
      newAttribute = nAttr.create(plugName, plugName, numType, rtval.getBoolean());

    else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt8"))
    {
      uint8_t val = rtval.getUInt8();
      if(uiHardMin < uiHardMax)
      {
        if(val < (uint8_t)uiHardMin) val = (uint8_t)uiHardMin;
        if(val > (uint8_t)uiHardMax) val = (uint8_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt16"))
    {
      uint16_t val = rtval.getUInt16();
      if(uiHardMin < uiHardMax)
      {
        if(val < (uint16_t)uiHardMin) val = (uint16_t)uiHardMin;
        if(val > (uint16_t)uiHardMax) val = (uint16_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt32"))
    {
      uint32_t val = rtval.getUInt32();
      if(uiHardMin < uiHardMax)
      {
        if(val < (uint32_t)uiHardMin) val = (uint32_t)uiHardMin;
        if(val > (uint32_t)uiHardMax) val = (uint32_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt64"))
    {
      uint64_t val = rtval.getUInt64();
      if(uiHardMin < uiHardMax)
      {
        if(val < (uint64_t)uiHardMin) val = (uint64_t)uiHardMin;
        if(val > (uint64_t)uiHardMax) val = (uint64_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt8"))
    {
      int8_t val = rtval.getSInt8();
      if(uiHardMin < uiHardMax)
      {
        if(val < (int8_t)uiHardMin) val = (int8_t)uiHardMin;
        if(val > (int8_t)uiHardMax) val = (int8_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt16"))
    {
      int16_t val = rtval.getSInt16();
      if(uiHardMin < uiHardMax)
      {
        if(val < (int16_t)uiHardMin) val = (int16_t)uiHardMin;
        if(val > (int16_t)uiHardMax) val = (int16_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt32"))
    {
      int32_t val = rtval.getSInt32();
      if(uiHardMin < uiHardMax)
      {
        if(val < (int32_t)uiHardMin) val = (int32_t)uiHardMin;
        if(val > (int32_t)uiHardMax) val = (int32_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt64"))
    {
      int64_t val = rtval.getSInt64();
      if(uiHardMin < uiHardMax)
      {
        if(val < (int64_t)uiHardMin) val = (int64_t)uiHardMin;
        if(val > (int64_t)uiHardMax) val = (int64_t)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "Float32"))
    {
      float val = rtval.getFloat32();
      if(uiHardMin < uiHardMax)
      {
        if(val < uiHardMin) val = uiHardMin;
        if(val > uiHardMax) val = uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    else if(binding.getExec().isExecPortResolvedType(portIndex, "Float64"))
    {
      double val = rtval.getFloat64();
      if(uiHardMin < uiHardMax)
      {
        if(val < (double)uiHardMin) val = (double)uiHardMin;
        if(val > (double)uiHardMax) val = (double)uiHardMax;
      }
      newAttribute = nAttr.create(plugName, plugName, numType, val);
    }

    if(!newAttribute.isNull())
    {
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
  }
  catch (Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
  return res;
}

inline bool AddBaseTypeAttributes( 
  DFGBinding &binding, 
  MString portName, 
  MString plugName, 
  MString dataType, 
  MString arrayType, 
  bool &storable,
  float uiSoftMin,
  float uiSoftMax,
  float uiHardMin,
  float uiHardMax,
  MFnNumericAttribute &nAttr, 
  MFnTypedAttribute &tAttr,
  MObject &newAttribute) 
{
  if(!binding.getExec().haveExecPort(portName.asChar()))
  {
    mayaLogErrorFunc("AddBaseTypeAttributes::AddMatrixAttributes : Creating maya attribute failed, No port found with name " + portName);
    return false;
  }

  MFnNumericData::Type numType = MFnNumericData::kInvalid;
  MFnData::Type strType = MFnData::kInvalid;

  uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());
  if(binding.getExec().isExecPortResolvedType(portIndex, "Boolean") ||
     binding.getExec().isExecPortResolvedType(portIndex, "Boolean[]"))     
    numType = MFnNumericData::kBoolean;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "String") || 
          binding.getExec().isExecPortResolvedType(portIndex, "String[]"))     
    strType = MFnNumericData::kString;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt8") ||
          binding.getExec().isExecPortResolvedType(portIndex, "UInt8[]"))     
    numType = MFnNumericData::kByte;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt8") ||
          binding.getExec().isExecPortResolvedType(portIndex, "SInt8[]"))     
    numType = MFnNumericData::kInt;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt16") ||
          binding.getExec().isExecPortResolvedType(portIndex, "UInt16[]"))     
    numType = MFnNumericData::kShort;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt16") ||
          binding.getExec().isExecPortResolvedType(portIndex, "SInt16[]"))     
    numType = MFnNumericData::kInt;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "UInt32") ||
          binding.getExec().isExecPortResolvedType(portIndex, "UInt32[]"))    
    numType = MFnNumericData::kInt;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "SInt32") ||
          binding.getExec().isExecPortResolvedType(portIndex, "SInt32[]"))     
    numType = MFnNumericData::kInt;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "Float32") ||
          binding.getExec().isExecPortResolvedType(portIndex, "Float32[]"))     
    numType = MFnNumericData::kFloat;
  
  else if(binding.getExec().isExecPortResolvedType(portIndex, "Float64") ||
          binding.getExec().isExecPortResolvedType(portIndex, "Float64[]"))     
    numType = MFnNumericData::kDouble;
  
    
  if(numType == MFnNumericData::kInvalid && strType == MFnData::kInvalid)
    return false;

  if(arrayType == "Array (Multi)")
  {
    if(numType != MFnNumericData::kInvalid)
    {
      newAttribute = nAttr.create(plugName, plugName, numType);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {          
      MFnStringData stringData;
      MObject defStrObject = stringData.create("");
      newAttribute = tAttr.create(plugName, plugName, MFnData::kString, defStrObject);
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
  }
  else if(arrayType == "Array (Native)")
  {
    if(numType == MFnNumericData::kDouble)
      newAttribute = tAttr.create(plugName, plugName, MFnData::kDoubleArray);
      
    else if(numType == MFnNumericData::kInt)
      newAttribute = tAttr.create(plugName, plugName, MFnData::kIntArray);

    else
    {
      mayaLogErrorFunc("FabricDFGBaseInterface::AddBaseTypeAttributes '"+ dataType + "' incompatible with ArrayType '"+arrayType+"'.");
      return false;
    }
  }
  else if(arrayType == "Single Value")
  {
    AddSingleBaseTypeAttribute(
      binding, 
      plugName,
      numType,
      portName, 
      uiSoftMin,
      uiSoftMax,
      uiHardMin,
      uiHardMax,
      storable,
      nAttr,
      tAttr,
      newAttribute);
  }

  return true;
}

inline bool AddSingleBaseStructAttribute(
  DFGBinding &binding, 
  MString plugName, 
  MFnNumericData::Type numType, 
  MFnUnitAttribute::Type unitType, 
  MString portName, 
  MString portType, 
  bool &storable,
  MFnUnitAttribute &uAttr, 
  MFnNumericAttribute &nAttr, 
  MObject &newAttribute) 
{
  bool sucess = false;
  try
  { 
    uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());
    if(binding.getExec().isExecPortResolvedType(portIndex, portType.asChar()))     
    {
      RTVal rtval = binding.getArgValue(portName.asChar());
      std::string str = rtval.getJSON().getStringCString();

      FTL::JSONStrWithLoc jsonSrcWithLoc( FTL::StrRef( str.c_str(), str.length() ) );
      FTL::OwnedPtr<FTL::JSONValue const> execValue( FTL::JSONValue::Decode( jsonSrcWithLoc ) );
      FTL::JSONObject const *execObject = execValue->cast<FTL::JSONObject>();

      MObject objs[3];
      if(numType != MFnNumericData::kInvalid)
      {        
        uint32_t count = 0;
        for ( FTL::JSONObject::const_iterator it = execObject->begin(); it != execObject->end(); ++it )
        {
          FTL::CStrRef key = it->key();
          MString mayaPortName = plugName;
          MString subPortName = key.data();
          mayaPortName += subPortName.toUpperCase();

          FTL::JSONValue const *value = it->value();
          if(value->isBoolean())
          {
            if(count <= 2)
            {
              objs[count] = nAttr.create(mayaPortName, mayaPortName, numType, value->getBooleanValue());
              nAttr.setStorable(storable);
              nAttr.setKeyable(storable);
              count ++;
            }
          }
          else if(value->isSInt32())
          {
            if(count <=  2)
            {
              objs[count] = nAttr.create(mayaPortName, mayaPortName, numType, value->getSInt32Value());
              nAttr.setStorable(storable);
              nAttr.setKeyable(storable);
              count ++;
            }
          }
          else if(value->isFloat64())
          {
            if(count <= 2)
            {
              objs[count] = nAttr.create(mayaPortName, mayaPortName, numType, value->getFloat64Value());
              nAttr.setStorable(storable);
              nAttr.setKeyable(storable);
              count ++;
            }
          }
        }
      }
      
      else if(unitType == MFnUnitAttribute::kAngle)
      {
        objs[0] = uAttr.create(plugName+"X", plugName+"X", unitType);
        uAttr.setStorable(storable);
        uAttr.setKeyable(storable);

        objs[1] = uAttr.create(plugName+"Y", plugName+"Y", unitType);
        uAttr.setStorable(storable);
        uAttr.setKeyable(storable);

        objs[2] = uAttr.create(plugName+"Z", plugName+"Z", unitType);
        uAttr.setStorable(storable);
        uAttr.setKeyable(storable);
      }

      newAttribute = nAttr.create(plugName, plugName, objs[0], objs[1], objs[2]);  
      sucess =  true;
    }
  }
  catch (Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
  return sucess;
}

inline bool AddBaseStructAttributes( 
  DFGBinding &binding, 
  MString portName, 
  MString plugName, 
  MString dataType, 
  MString arrayType, 
  bool &storable,
  float uiSoftMin,
  float uiSoftMax,
  float uiHardMin,
  float uiHardMax,
  MFnUnitAttribute &uAttr, 
  MFnNumericAttribute &nAttr, 
  MFnTypedAttribute &tAttr,
  MObject &newAttribute) 
{
  if(!binding.getExec().haveExecPort(portName.asChar()))
  {
    mayaLogErrorFunc("AddBaseStructAttributes::AddMatrixAttributes : Creating maya attribute failed, No port found with name " + portName);
    return false;
  }

  MFnNumericData::Type numType = MFnNumericData::kInvalid;
  MFnUnitAttribute::Type unitType = MFnUnitAttribute::kInvalid;

  bool isColor = false;
  uint32_t nbParameter = 3;
  uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());
  if( binding.getExec().isExecPortResolvedType(portIndex, "Vec3") || 
      binding.getExec().isExecPortResolvedType(portIndex, "Vec3[]") ) 
    numType = MFnNumericData::kFloat; 

  else if(binding.getExec().isExecPortResolvedType(portIndex, "Euler")  || 
      binding.getExec().isExecPortResolvedType(portIndex, "Euler[]") || 
      binding.getExec().isExecPortResolvedType(portIndex, "Euler_d") || 
      binding.getExec().isExecPortResolvedType(portIndex, "Euler_d[]") )
    unitType = MFnUnitAttribute::kAngle;  
 
  else if(binding.getExec().isExecPortResolvedType(portIndex, "Vec3_d") || 
          binding.getExec().isExecPortResolvedType(portIndex, "Vec3_d[]")  )
    numType = MFnNumericData::kDouble;

  else if(binding.getExec().isExecPortResolvedType(portIndex, "Vec3_i")  || 
          binding.getExec().isExecPortResolvedType(portIndex, "Vec3_i[]") )
    numType = MFnNumericData::kInt;

  else if(binding.getExec().isExecPortResolvedType(portIndex, "Color") ||
          binding.getExec().isExecPortResolvedType(portIndex, "Color[]")  )
  {
    isColor = true;
    numType = MFnNumericData::kFloat;
  }

  else
  {
    nbParameter = 2;
    if(binding.getExec().isExecPortResolvedType(portIndex, "Vec2")||
       binding.getExec().isExecPortResolvedType(portIndex, "Vec2[]") )
      numType = MFnNumericData::kFloat;  

    else if(binding.getExec().isExecPortResolvedType(portIndex, "Vec2_d") ||
            binding.getExec().isExecPortResolvedType(portIndex, "Vec2_d[]") )     
      numType = MFnNumericData::kDouble;
    
    else if(binding.getExec().isExecPortResolvedType(portIndex, "Vec2_i") ||
            binding.getExec().isExecPortResolvedType(portIndex, "Vec2_i[]") )     
      numType = MFnNumericData::kInt;
  }

  
  if(numType == MFnNumericData::kInvalid && unitType == MFnUnitAttribute::kInvalid)
    return false;

  if(arrayType == "Array (Multi)")
  {
    
    if(numType != MFnNumericData::kInvalid)
    {        
      MString mayaPortName = plugName;
      mayaPortName += (!isColor) ? "X" : "R";    
      MObject x = nAttr.create(mayaPortName, mayaPortName, numType);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);

      mayaPortName = plugName;
      mayaPortName += (!isColor) ? "Y" : "G";
      MObject y = nAttr.create(mayaPortName, mayaPortName, numType);
      nAttr.setStorable(storable);
      nAttr.setKeyable(storable);

      if(nbParameter == 2)
          newAttribute = nAttr.create(plugName, plugName, x, y);      
      else
      {
        mayaPortName = plugName;
        mayaPortName += (!isColor) ? "Z" : "B";
        MObject z = nAttr.create(mayaPortName, mayaPortName, numType);
        nAttr.setStorable(storable);
        nAttr.setKeyable(storable);

        newAttribute = nAttr.create(plugName, plugName, x, y, z);      
      }

      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else if(unitType != MFnUnitAttribute::kInvalid)
    {
      MObject x = uAttr.create(plugName+"X", plugName+"X", unitType);
      uAttr.setStorable(storable);
      MObject y = uAttr.create(plugName+"Y", plugName+"Y", unitType);
      uAttr.setStorable(storable);
      MObject z = uAttr.create(plugName+"Z", plugName+"Z", unitType);
      uAttr.setStorable(storable);

      newAttribute = nAttr.create(plugName, plugName, x, y, z);   
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
  }
  else if(arrayType == "Array (Native)")
  {
    if( binding.getExec().isExecPortResolvedType(portIndex, "Vec3") || 
      binding.getExec().isExecPortResolvedType(portIndex, "Vec3[]") ) 
    newAttribute = tAttr.create(plugName, plugName, MFnData::kVectorArray);

    else
    {
      mayaLogErrorFunc("FabricDFGBaseInterface::AddBaseStructAttributes '"+ dataType + "' incompatible with ArrayType '"+arrayType+"'.");
      return false;
    }
  }
  else if(arrayType == "Single Value")
  {
    AddSingleBaseStructAttribute(
      binding, 
      plugName,
      numType,
      unitType, 
      portName, 
      dataType, 
      storable,
      uAttr,
      nAttr,
      newAttribute);
  }

  return true;
}

inline bool AddSingleMat44Attributes(
  DFGBinding &binding, 
  MString plugName, 
  MString portName, 
  MFnMatrixAttribute &mAttr, 
  MObject &newAttribute)
{
  uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());
  RTVal rtval = binding.getArgValue(portName.asChar());

  if( binding.getExec().isExecPortResolvedType(portIndex, "Mat44") ||
      binding.getExec().isExecPortResolvedType(portIndex, "Xfo") )
  {     
    if(binding.getExec().isExecPortResolvedType(portIndex, "Xfo"))
      rtval = rtval.callMethod("Mat44", "toMat44", 0, NULL);

    RTVal rtvalData = rtval.callMethod("Data", "data", 0, 0);  
    uint64_t dataSize = rtval.callMethod("UInt64", "dataSize", 0, 0).getUInt64();   
    newAttribute = mAttr.create(plugName, plugName, MFnMatrixAttribute::kFloat);
    float vals[4][4];
    memcpy(vals, rtvalData.getData(), dataSize);
    mAttr.setDefault(MFloatMatrix(vals));
    return true;
  }

  else if(binding.getExec().isExecPortResolvedType(portIndex, "Mat44_d"))
  {     
    RTVal rtvalData = rtval.callMethod("Data", "data", 0, 0);  
    uint64_t dataSize = rtval.callMethod("UInt64", "dataSize", 0, 0).getUInt64();   
    newAttribute = mAttr.create(plugName, plugName, MFnMatrixAttribute::kDouble);
    double vals[4][4];
    memcpy(vals, rtvalData.getData(), dataSize);
    mAttr.setDefault(MMatrix(vals));
    return true;
  }

  else
    return false;
}

inline bool AddMatrixAttributes( 
  DFGBinding &binding, 
  MString portName, 
  MString plugName, 
  MString dataType, 
  MString arrayType, 
  bool &storable,
  float uiSoftMin,
  float uiSoftMax,
  float uiHardMin,
  float uiHardMax,
  MFnMatrixAttribute &mAttr, 
  MObject &newAttribute) 
{
  if(!binding.getExec().haveExecPort(portName.asChar()))
  {
    mayaLogErrorFunc("FabricDFGBaseInterface::AddMatrixAttributes : Creating maya attribute failed, No port found with name " + portName);
    return false;
  }
 
  MFnMatrixAttribute::Type matType = MFnMatrixAttribute::kFloat;
  uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());
  if( binding.getExec().isExecPortResolvedType(portIndex, "Mat44") ||
      binding.getExec().isExecPortResolvedType(portIndex, "Mat44[]") ||
      binding.getExec().isExecPortResolvedType(portIndex, "Xfo") ||
      binding.getExec().isExecPortResolvedType(portIndex, "Xfo[]") )
    matType = MFnMatrixAttribute::kFloat;  

  else if( binding.getExec().isExecPortResolvedType(portIndex, "Mat44_d") ||
      binding.getExec().isExecPortResolvedType(portIndex, "Mat44_d[]") )
    matType = MFnMatrixAttribute::kDouble;  

  else
    return false;

  if(arrayType == "Single Value")
  {
    AddSingleMat44Attributes(
      binding, 
      plugName,
      portName, 
      mAttr, 
      newAttribute);
  }
  else if(arrayType == "Array (Multi)")
  {
    newAttribute = mAttr.create(plugName, plugName, matType);
    mAttr.setArray(true);
    mAttr.setUsesArrayDataBuilder(true);
  }

  return true;
}

inline bool AddGeometryAttributes( 
  DFGBinding &binding, 
  MString portName, 
  MString plugName, 
  MString dataType, 
  MString arrayType, 
  bool &storable,
  float uiSoftMin,
  float uiSoftMax,
  float uiHardMin,
  float uiHardMax,
  MFnTypedAttribute &tAttr,
  MObject &newAttribute) 
{
  if(!binding.getExec().haveExecPort(portName.asChar()))
  {
    mayaLogErrorFunc("FabricDFGBaseInterface::AddGeometryAttributes : Creating maya attribute failed, No port found with name " + portName);
    return false;
  }

  uint32_t portIndex = binding.getExec().getExecPortIndex(portName.asChar());
 
  if(binding.getExec().isExecPortResolvedType(portIndex, "PolygonMesh") ||
      binding.getExec().isExecPortResolvedType(portIndex, "PolygonMesh[]") )
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

    return true;
  }

  else if(binding.getExec().isExecPortResolvedType(portIndex, "Lines") ||
      binding.getExec().isExecPortResolvedType(portIndex, "Lines[]") ||
      binding.getExec().isExecPortResolvedType( portIndex, "Curves" ) ||
      binding.getExec().isExecPortResolvedType( portIndex, "Curve" ) )
  {
    if(arrayType == "Single Value" )
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
    return true;
  }

  return false;
}

inline bool AddOtherAttributes(
  DFGBinding &binding, 
  MString portName, 
  MString plugName, 
  MString dataType, 
  MString arrayType, 
  bool &storable,
  std::vector<std::string> &spliceMayaDataOverride,
  MFnMessageAttribute &pAttr,
  MFnTypedAttribute &tAttr,
  MObject &newAttribute) 
{
  if(!binding.getExec().haveExecPort(portName.asChar()))
  {
    mayaLogErrorFunc("FabricDFGBaseInterface::AddOtherAttributes : Creating maya attribute failed, No port found with name " + portName);
    return false;
  }
 
  if(strcmp(dataType.asChar(), "AnimX::AnimCurve") == 0)
  {
    if(arrayType == "Single Value")
    {
      newAttribute = pAttr.create(plugName, plugName);
      pAttr.setStorable(storable);
      pAttr.setKeyable(storable);
      pAttr.setCached(false);
    }
    else
    {
      newAttribute = pAttr.create(plugName, plugName);
      pAttr.setStorable(storable);
      pAttr.setKeyable(storable);
      pAttr.setArray(true);
      pAttr.setCached(false);
    }

    return true;
  }

  else if(strcmp(dataType.asChar(), "SpliceMayaData") == 0)
  {  
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(plugName, plugName, FabricSpliceMayaData::id);
      spliceMayaDataOverride.push_back(portName.asChar());
      storable = false;

      // disable input conversion by default
      // only enable it again if there is a connection to the port
      binding.getExec().setExecPortMetadata(portName.asChar(), "disableSpliceMayaDataConversion", "true", false); // canUndo );
    } 
    else
    {
      newAttribute = tAttr.create(plugName, plugName, FabricSpliceMayaData::id);
      spliceMayaDataOverride.push_back(portName.asChar());
      storable = false;
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);

      // disable input conversion by default
      // only enable it again if there is a connection to the port
      binding.getExec().setExecPortMetadata(portName.asChar(), "disableSpliceMayaDataConversion", "true", false); // canUndo );
    }

    return true;
  }

  return false;
}

// To-do, was commented
inline bool AddCompoundAttributes(
  DFGBinding &binding, 
  MString portName, 
  MString plugName, 
  MString dataType, 
  MString arrayType, 
  bool &storable,
  std::vector<std::string> &spliceMayaDataOverride,
  MFnMessageAttribute &pAttr,
  MFnTypedAttribute &tAttr,
  MObject &newAttribute) 
{
  return false;
  /*
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
        for(Variant::DictIter childIter(compoundStructure); !childIter.isDone(); childIter.next())
        {
          MString childNameStr = childIter.getKey()->getStringData();
          const Variant * value = childIter.getValue();
          if(!value)
            continue;
          if(value->isNull())
            continue;
          if(value->isDict()) {
            const Variant * childDataType = value->getDictValue("dataType");
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

                MObject child = addMayaAttribute(childNameStr, childDataTypeStr, childArrayTypeStr, portType, true, *value, stat);
                if(child.isNull())
                  return newAttribute;
                
                children.append(child);
                continue;
              }
            }

            // we assume it's a nested compound param
            MObject child = addMayaAttribute(childNameStr, "CompoundParam", "Single Value", portType, true, *value, stat);
            if(child.isNull())
              return newAttribute;
            children.append(child);
          }
        }

        /// now create the compound attribute
        newAttribute = cAttr.create(plugName, plugName);
        if(arrayType == "Array (Multi)")
        {
          cAttr.setArray(true);
          cAttr.setUsesArrayDataBuilder(true);
        }
        for(unsigned int i=0;i<children.length();i++)
        {
          cAttr.addChild(children[i]);
        }

        // initialize the compound param
        _dirtyPlugs.append(portName);
      }
      else
      {
        mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
        return newAttribute;
      }
    }
  */
}
// ********************   ********************  //


MObject FabricDFGBaseInterface::addMayaAttribute(
  MString portName, 
  MString dataType, 
  DFGPortType portType, 
  MString arrayType, 
  bool compoundChild, 
  MStatus * stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::addMayaAttribute ");

  MAYADFG_CATCH_BEGIN(stat);

  MObject newAttribute;

  DFGExec exec = m_binding.getExec();

  // skip if disabled by used in the EditPortDialog
  FTL::StrRef addAttributeMD = exec.getExecPortMetadata(portName.asChar(), "addAttribute");
  if(addAttributeMD == "false")
    return newAttribute;

  MString dataTypeOverride = dataType;
  FTL::StrRef opaqueMD = exec.getExecPortMetadata(portName.asChar(), "opaque");
  bool isOpaqueData = false;
 
  if(opaqueMD == "true")
  {
    dataTypeOverride = "SpliceMayaData";
    isOpaqueData = true;
  }

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
  if( dataTypeOverride == "Curves" ) // Curves is implicitly mapped to an array
    arrayType = "Array (Multi)";

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

  bool isAccepted = false;
  bool storable = portType != DFGPortType_Out;

  if(!isAccepted && !isOpaqueData)
    isAccepted = AddBaseTypeAttributes( 
    m_binding, portName, plugName, 
    dataTypeOverride, arrayType, storable, 
    uiSoftMin, uiSoftMax, uiHardMin, uiHardMax,
    nAttr, tAttr, newAttribute);

  if(!isAccepted && !isOpaqueData)
    isAccepted = AddBaseStructAttributes( 
      m_binding, portName, plugName, 
      dataTypeOverride, arrayType, storable,
      uiSoftMin, uiSoftMax, uiHardMin, uiHardMax,
      uAttr, nAttr, tAttr, newAttribute);

  if(!isAccepted && !isOpaqueData)
    isAccepted = AddMatrixAttributes( 
      m_binding, portName, plugName, 
      dataTypeOverride, arrayType, storable,
      uiSoftMin, uiSoftMax, uiHardMin, uiHardMax,
      mAttr, newAttribute);
    
  if(!isAccepted && !isOpaqueData)
    isAccepted = AddGeometryAttributes( 
      m_binding, portName, plugName, 
      dataTypeOverride, arrayType, storable,
      uiSoftMin, uiSoftMax, uiHardMin, uiHardMax,
      tAttr, newAttribute);

  if(!isAccepted) 
    isAccepted = AddOtherAttributes( 
      m_binding, portName, plugName, 
      dataTypeOverride, arrayType, storable,
      mSpliceMayaDataOverride,
      pAttr, tAttr, newAttribute);

  if(!isAccepted)
  {
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
    nAttr.setReadable(portType != DFGPortType_In);
    tAttr.setReadable(portType != DFGPortType_In);
    mAttr.setReadable(portType != DFGPortType_In);
    uAttr.setReadable(portType != DFGPortType_In);
    cAttr.setReadable(portType != DFGPortType_In);
    pAttr.setReadable(portType != DFGPortType_In);

    nAttr.setWritable(portType != DFGPortType_Out);
    tAttr.setWritable(portType != DFGPortType_Out);
    mAttr.setWritable(portType != DFGPortType_Out);
    uAttr.setWritable(portType != DFGPortType_Out);
    cAttr.setWritable(portType != DFGPortType_Out);
    pAttr.setWritable(portType != DFGPortType_Out);

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

  // FE-7923: if this is an in or io plug ensure to 
  // invalidate it.
  if(portType != DFGPortType_Out)
  {
    MPlug newAttributePlug(getThisMObject(), newAttribute);
    invalidatePlug(newAttributePlug);
  }

  _affectedPlugsDirty = true;
  return newAttribute;

  MAYADFG_CATCH_END(stat);

  return MObject();
}

void FabricDFGBaseInterface::removeMayaAttribute(
  MString portName, 
  MStatus * stat)
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

FabricCore::LockType FabricDFGBaseInterface::getLockType()
{
  return getExecuteShared()?
    FabricCore::LockType_Shared:
    FabricCore::LockType_Exclusive;
}

void FabricDFGBaseInterface::setupMayaAttributeAffects(
  MString portName, 
  DFGPortType portType, 
  MObject newAttribute, 
  MStatus *stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setupMayaAttributeAffects");

  MAYASPLICE_CATCH_BEGIN(stat);

  MFnDependencyNode thisNode(getThisMObject());
  MPxNode * userNode = thisNode.userNode();

  if(userNode != NULL)
  {
    DFGExec exec = getDFGExec();
  
    if(portType != DFGPortType_In)
    {
      for(unsigned i = 0; i < exec.getExecPortCount(); ++i) {
        std::string otherPortName = exec.getExecPortName(i);
        if(otherPortName == portName.asChar() && portType != DFGPortType_IO)
          continue;
        if(exec.getExecPortType(i) != DFGPortType_In)
          continue;
        MString otherPlugName = getPlugName(otherPortName.c_str());
        MPlug plug = thisNode.findPlug(otherPlugName);
        if(plug.isNull())
          continue;
        userNode->attributeAffects(plug.attribute(), newAttribute);
      }
    }

    if(portType != DFGPortType_Out)
    {
      for(unsigned i = 0; i < exec.getExecPortCount(); ++i) {
        std::string otherPortName = exec.getExecPortName(i);
        if(otherPortName == portName.asChar() && portType != DFGPortType_IO)
          continue;
        if(exec.getExecPortType(i) == DFGPortType_In)
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

void FabricDFGBaseInterface::managePortObjectValues(
  bool destroy)
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
      RTVal value  = getDFGBinding().getArgValue(i);
       if(!value.isValid())
         continue;
       if(!value.isObject())
         continue;
       if(value.isNullObject())
         continue;

       RTVal objectRtVal = FabricSplice::constructRTVal("Object", 1, &value);
       if(!objectRtVal.isValid())
         continue;

       RTVal detachable = FabricSplice::constructInterfaceRTVal("Detachable", objectRtVal);
       if(detachable.isNullObject())
         continue;

       if(destroy)
         detachable.callMethod("", "detach", 0, 0);
       else
         detachable.callMethod("", "attach", 0, 0);
     }
     catch(Exception e)
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

void FabricDFGBaseInterface::allStorePersistenceData(
  MString file, 
  MStatus *stat)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::allStorePersistenceData");

  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->storePersistenceData(file, stat);
}

void FabricDFGBaseInterface::allRestoreFromPersistenceData(
  MString file, 
  MStatus *stat)
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

void FabricDFGBaseInterface::setAllRestoredFromPersistenceData(
  bool value)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::setAllRestoredFromPersistenceData");

  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->_restoredFromPersistenceData = value;
}

void FabricDFGBaseInterface::bindingNotificationCallback(
  FTL::CStrRef jsonStr)
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
  DFGNotifBracket notifBracket( getDFGHost() );

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
      queueIncrementEvalID(false /* onIdle */);
  }
  else if( descStr == FTL_STR("argTypeChanged") )
  {
    std::string nameStr = jsonObject->getString( FTL_STR("name") );
    MString plugName = getPlugName(nameStr.c_str());
    std::string newTypeStr = jsonObject->getString( FTL_STR("newType") );

    MFnDependencyNode thisNode(getThisMObject());

    // remove existing attributes if types don't match
    MPlug plug = thisNode.findPlug(plugName);
    if( !plug.isNull() ) {
      // FE-7722 -> Only work for maya > 2017 update 4
      if( MGlobal::apiVersion() >= 201760 )
        removeMayaAttribute( nameStr.c_str() );
      else
        return;
    }
 
    DFGExec exec = getDFGExec();
    DFGPortType portType = exec.getExecPortType(nameStr.c_str());
    addMayaAttribute(nameStr.c_str(), newTypeStr.c_str(), portType);
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
    // FE-7722 -> Only work for maya > 2017 update 4
    if( MGlobal::apiVersion() >= 201760 ) {
      FABRIC_MAYA_CATCH_BEGIN();

      if( jsonObject->has( FTL_STR( "name" ) ) && jsonObject->has( FTL_STR( "type" ) ) ) {
        // In some cases (eg: unsupported types in maya), the type has no String JSON value (Type_Null)     
        const FTL::JSONString* nameStrValue = jsonObject->get( FTL_STR( "name" ) )->maybeCast<FTL::JSONString>();
        const FTL::JSONString* typeStrValue = jsonObject->get( FTL_STR( "type" ) )->maybeCast<FTL::JSONString>();

        if( nameStrValue && typeStrValue ) {
          FTL::CStrRef nameStr = nameStrValue->getValue();
          FTL::CStrRef typeStr = nameStrValue->getValue();

          MString plugName = getPlugName( nameStr.data() );

          FabricCore::DFGExec exec = getDFGExec();
          FabricCore::DFGPortType portType = exec.getExecPortType( nameStr.data() );
          addMayaAttribute( nameStr.data(), typeStr.data(), portType );
        }
      }
      FABRIC_MAYA_CATCH_END( "FabricDFGBaseInterface::bindingNotificationCallback.argInserted" );
    }
    generateAttributeLookups();
  }
  else if(   descStr == FTL_STR("varInserted")
          || descStr == FTL_STR("varRemoved") )
  {
    if ( FabricDFGWidget::Instance( false /* createIfNull */ ) )
    {
      if(  FabricDFGWidget::Instance()->getDfgWidget()
        && FabricDFGWidget::Instance()->getDfgWidget()->getUIController())
      FabricDFGWidget::Instance()->getDfgWidget()->getUIController()->emitVarsChanged();
    }
  }
  // else
  // {
  //   mayaLogFunc(jsonStr.c_str());
  // }
}

void FabricDFGBaseInterface::BindingNotificationCallback(
  void * userData, 
  char const *jsonCString, 
  uint32_t jsonLength)
{
  FabricDFGBaseInterface * interf =
    static_cast<FabricDFGBaseInterface *>( userData );
  interf->bindingNotificationCallback(
    FTL::CStrRef( jsonCString, jsonLength )
    );
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
  void *getSetUD)
{
  if(argOutsidePortType == DFGPortType_Out)
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

    FTL::StrRef portDataType = argTypeCanonicalName; 
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
      argTypeCanonicalName,
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
  void *getSetUD)
{
  if(argOutsidePortType == DFGPortType_In)
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

    FTL::StrRef portDataType = argTypeCanonicalName;
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

    if( ud->isDeformer && ( portDataType == FTL_STR( "PolygonMesh" ) || portDataType == FTL_STR( "Lines" ) || portDataType == FTL_STR( "Curves" ) || portDataType == FTL_STR( "Curve" ) ) )
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
      argTypeCanonicalName,
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

void FabricDFGBaseInterface::renamePlug(
  const MPlug &plug, 
  MString oldName, 
  MString newName)
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

MString FabricDFGBaseInterface::resolveEnvironmentVariables(
  const MString & filePath)
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
  const MEvaluationNode& evaluationNode)
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
  MPxNode::PostEvaluationType evalType)
{
  return MStatus::kSuccess;
}
#endif

bool FabricDFGBaseInterface::HasPort(
  const char *in_portName, 
  const bool testForInput)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::HasPort");

  try
  {
    // check/init.
    if (!in_portName || in_portName[0] == '\0')  return false;
    const DFGPortType portType = (testForInput ? DFGPortType_In : DFGPortType_Out);

    // get the graph.
    DFGExec graph = m_binding.getExec();
    if (!graph.isValid())
    {
      std::string s = "BaseInterface::HasPort(): failed to get graph!";
      mayaLogErrorFunc(s.c_str(), s.length());
      return false;
    }

    // return result.
    return (graph.haveExecPort(in_portName) && graph.getExecPortType(in_portName) == portType);
  }
  catch (Exception e)
  {
    return false;
  }
}

bool FabricDFGBaseInterface::HasInputPort(
  const char *portName)
{
  FabricMayaProfilingEvent bracket("FabricDFGBaseInterface::HasInputPort");
  return HasPort(portName, true);
}
