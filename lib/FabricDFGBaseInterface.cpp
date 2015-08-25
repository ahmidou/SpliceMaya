
#include "FabricDFGBaseInterface.h"
#include "FabricDFGConversion.h"
#include "FabricDFGWidget.h"
#include "FabricMayaAttrs.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceHelpers.h"
#include "FabricSpliceMayaData.h"

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

#if _SPLICE_MAYA_VERSION >= 2016
# include <maya/MEvaluationNode.h>
#endif

std::vector<FabricDFGBaseInterface*> FabricDFGBaseInterface::_instances;
#if _SPLICE_MAYA_VERSION < 2013
  std::map<std::string, int> FabricDFGBaseInterface::_nodeCreatorCounts;
#endif
unsigned int FabricDFGBaseInterface::s_maxID = 1;

FabricDFGBaseInterface::FabricDFGBaseInterface(){

  MStatus stat;
  MAYADFG_CATCH_BEGIN(&stat);

  _restoredFromPersistenceData = false;
  _isTransferingInputs = false;
  _dgDirtyEnabled = true;
  _portObjectsDestroyed = false;
  _affectedPlugsDirty = true;
  _outputsDirtied = false;
  _isReferenced = false;
  _instances.push_back(this);
  m_addAttributeForNextAttribute = true;
  m_useNativeArrayForNextAttribute = false;
  m_useOpaqueForNextAttribute = false;

  m_id = s_maxID++;

  MAYADFG_CATCH_END(&stat);
}

FabricDFGBaseInterface::~FabricDFGBaseInterface(){
  // todo: eventually destroy the binding
  // m_binding = DFGWrapper::Binding();

  m_evalContext = FabricCore::RTVal();

  for(size_t i=0;i<_instances.size();i++){
    if(_instances[i] == this){
      std::vector<FabricDFGBaseInterface*>::iterator iter = _instances.begin() + i;
      _instances.erase(iter);
      break;
    }
  }
}

void FabricDFGBaseInterface::constructBaseInterface(){

  MStatus stat;
  MAYADFG_CATCH_BEGIN(&stat);

  if(m_binding.isValid())
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

  FabricSplice::Logging::AutoTimer timer("Maya::FabricDFGBaseInterface()");

  m_client = FabricDFGWidget::Instance()->getCoreClient();
  FabricCore::DFGHost &dfgHost = FabricDFGWidget::Instance()->getDFGHost();

  m_binding = dfgHost.createBindingToNewGraph();
  m_binding.setNotificationCallback( BindingNotificationCallback, this );
  FabricCore::DFGExec exec = m_binding.getExec();

  MString idStr; idStr.set(m_id);
  m_binding.setMetadata("maya_id", idStr.asChar(), false);
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

FabricCore::DFGBinding FabricDFGBaseInterface::getDFGBinding()
{
  return m_binding;
}

FabricCore::DFGExec FabricDFGBaseInterface::getDFGExec()
{
  return m_binding.getExec();
}

bool FabricDFGBaseInterface::transferInputValuesToDFG(MDataBlock& data){
  if(_isTransferingInputs)
    return false;

  managePortObjectValues(false); // recreate objects if not there yet

  FabricSplice::Logging::AutoTimer timer("Maya::transferInputValuesToDFG()");

  _isTransferingInputs = true;

  MFnDependencyNode thisNode(getThisMObject());

  FabricCore::DFGExec exec = getDFGExec();
  for(size_t i = 0; i < _dirtyPlugs.length(); ++i){
    MString plugName = _dirtyPlugs[i];
    if(plugName == "evalID" || plugName == "saveData" || plugName == "refFilePath")
      continue;

    MString portName = getPortName(plugName);
    MPlug plug = thisNode.findPlug(plugName);
    if(!plug.isNull()){

      if(!exec.haveExecPort(portName.asChar()))
        continue;

      if(exec.getExecPortType(portName.asChar()) != FabricCore::DFGPortType_Out){

        std::string portDataType = exec.getExecPortResolvedType(portName.asChar());

        if(portDataType.substr(portDataType.length()-2, 2) == "[]")
          portDataType = portDataType.substr(0, portDataType.length()-2);

        for(size_t j=0;j<mSpliceMayaDataOverride.size();j++)
        {
          if(mSpliceMayaDataOverride[j] == portName.asChar())
          {
            portDataType = "SpliceMayaData";
            break;
          }
        }
        
        DFGPlugToArgFunc func = getDFGPlugToArgFunc(portDataType);
        if(func != NULL)
          (*func)(plug, data, m_binding, portName.asChar());
      }
    }
  }

  _dirtyPlugs.clear();
  _isTransferingInputs = false;
  return true;
}

void FabricDFGBaseInterface::evaluate(){
  MFnDependencyNode thisNode(getThisMObject());

  FabricSplice::Logging::AutoTimer timer("Maya::evaluate()");
  managePortObjectValues(false); // recreate objects if not there yet

  if(!m_evalContext.isValid())
  {
    try
    {
      m_evalContext = FabricCore::RTVal::Create(m_client, "EvalContext", 0, 0);
      m_evalContext = m_evalContext.callMethod("EvalContext", "getInstance", 0, 0);
      m_evalContext.setMember("host", FabricCore::RTVal::ConstructString(m_client, "Maya"));
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(e.getDesc_cstr());
    }
  }  
  if(m_evalContext.isValid())
  {
    try
    {
      m_evalContext.setMember("graph", FabricCore::RTVal::ConstructString(m_client, thisNode.name().asChar()));
      m_evalContext.setMember("time", FabricCore::RTVal::ConstructFloat32(m_client, MAnimControl::currentTime().as(MTime::kSeconds)));
      m_evalContext.setMember("currentFilePath", FabricCore::RTVal::ConstructString(m_client, mayaGetLastLoadedScene().asChar()));
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(e.getDesc_cstr());
    }
  }

  m_binding.execute();
}

void FabricDFGBaseInterface::transferOutputValuesToMaya(MDataBlock& data, bool isDeformer){
  if(_isTransferingInputs)
    return;

  managePortObjectValues(false); // recreate objects if not there yet

  FabricSplice::Logging::AutoTimer timer("Maya::transferOutputValuesToMaya()");
  
  MFnDependencyNode thisNode(getThisMObject());

  FabricCore::DFGExec exec = getDFGExec();

  for(unsigned i = 0; i < exec.getExecPortCount(); ++i){

    FabricCore::DFGPortType portType = exec.getExecPortType(i);
    if(portType != FabricCore::DFGPortType_In){
      
      std::string portName = exec.getExecPortName(i);
      std::string plugName = getPlugName(portName.c_str()).asChar();
      std::string portDataType = exec.getExecPortResolvedType(i);

      if(portDataType.substr(portDataType.length()-2, 2) == "[]")
        portDataType = portDataType.substr(0, portDataType.length()-2);

      MPlug plug = thisNode.findPlug(plugName.c_str());
      if(!plug.isNull()){
        for(size_t j=0;j<mSpliceMayaDataOverride.size();j++)
        {
          if(mSpliceMayaDataOverride[j] == portName)
          {
            portDataType = "SpliceMayaData";
            break;
          }
        }

        if(isDeformer && portDataType == "PolygonMesh") {
          data.setClean(plug);
        } else {
          DFGArgToPlugFunc func = getDFGArgToPlugFunc(portDataType);
          if(func != NULL) {
            FabricSplice::Logging::AutoTimer timer("Maya::transferOutputValuesToMaya::conversionFunc()");
            (*func)(m_binding, portName.c_str(), plug, data);
            data.setClean(plug);
          }
        }
      }
    }
  }
}

void FabricDFGBaseInterface::collectDirtyPlug(MPlug const &inPlug){

  FabricSplice::Logging::AutoTimer timer("Maya::collectDirtyPlug()");

  MStatus stat;
  MString name;

  name = inPlug.name();
  int periodPos = name.rindex('.');
  if(periodPos > -1)
    name = name.substring(periodPos+1, name.length()-1);
  int bracketPos = name.index('[');
  if(bracketPos > -1)
    name = name.substring(0, bracketPos-1);

  // filter out savedata
  if(name == "saveData" || name == "refFilePath")
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

  for(size_t i = 0; i < _dirtyPlugs.length(); ++i){
    if(_dirtyPlugs[i] == name)
      return;
  }

  _dirtyPlugs.append(name);
}

void FabricDFGBaseInterface::affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs){
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
  MAYADFG_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::storePersistenceData()");

  std::string json = m_binding.exportJSON().getCString();
  MPlug saveDataPlug = getSaveDataPlug();
  saveDataPlug.setString(json.c_str());

  MAYADFG_CATCH_END(stat);
}

void FabricDFGBaseInterface::restoreFromPersistenceData(MString file, MStatus *stat){
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

  MAYADFG_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::restoreFromPersistenceData()");

  FabricCore::DFGHost &dfgHost = FabricDFGWidget::Instance()->getDFGHost();
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

    createAttributeForPort(portName.c_str());
    FabricCore::DFGPortType portType = exec.getExecPortType(portName.c_str());

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

  // todo... set values?

  MAYADFG_CATCH_END(stat);
}

void FabricDFGBaseInterface::setReferencedFilePath(MString filePath)
{
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
  MPlug plug = getRefFilePathPlug();
  MString filePath = plug.asString();
  if(filePath.length() == 0)
    return;

  _restoredFromPersistenceData = false;
  MStatus status;
  restoreFromPersistenceData(mayaGetLastLoadedScene(), &status);
}

MString FabricDFGBaseInterface::getPlugName(MString portName)
{
  if(portName == "message")
    return "dfg_message";
  else if(portName == "saveData")
    return "dfg_saveData";
  else if(portName == "refFilePath")
    return "dfg_refFilePath";
  return portName;
}

MString FabricDFGBaseInterface::getPortName(MString plugName)
{
  if(plugName == "dfg_message")
    return "message";
  else if(plugName == "dfg_saveData")
    return "saveData";
  else if(plugName == "dfg_refFilePath")
    return "refFilePath";
  return plugName;
}

void FabricDFGBaseInterface::invalidatePlug(MPlug & plug)
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

void FabricDFGBaseInterface::invalidateNode()
{
  if(!_dgDirtyEnabled)
    return;
  FabricSplice::Logging::AutoTimer timer("Maya::invalidateNode()");

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
  _affectedPlugsDirty = true;
  _outputsDirtied = false;
}

void FabricDFGBaseInterface::incrementEvalID()
{
  MFnDependencyNode thisNode(getThisMObject());
  MPlug evalIDPlug = thisNode.findPlug("evalID");
  if(evalIDPlug.isNull())
    return;

  int id = evalIDPlug.asInt();
  evalIDPlug.setValue(id+1);

  MString idStr;
  idStr.set(id);
}

bool FabricDFGBaseInterface::plugInArray(const MPlug &plug, const MPlugArray &array){
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

  MFnDependencyNode thisNode(thisMObject);

  FabricSplice::Logging::AutoTimer timer("Maya::setDependentsDirty()");

  // we can't ask for the plug value here, so we fill an array for the compute to only transfer newly dirtied values
  collectDirtyPlug(inPlug);

  if(_outputsDirtied)
    return MS::kSuccess;

  if(_affectedPlugsDirty)
  {
    FabricSplice::Logging::AutoTimer timer("Maya::setDependentsDirty() _affectedPlugsDirty");
    
    _affectedPlugs.clear();

    // todo: performance considerations
    for(unsigned int i = 0; i < thisNode.attributeCount(); ++i){
      MFnAttribute attrib(thisNode.attribute(i));
      if(attrib.isHidden())
        continue;
      if(!attrib.isDynamic())
        continue;
      if(!attrib.isReadable())
        continue;

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
    FabricSplice::Logging::AutoTimer timer("Maya::setDependentsDirty() copying _affectedPlugs");
    affectedPlugs = _affectedPlugs;
  }

  _outputsDirtied = true;

  return MS::kSuccess;
}

void FabricDFGBaseInterface::copyInternalData(MPxNode *node){
  // FabricDFGBaseInterface *otherSpliceInterface = getInstanceByName(node->name().asChar());

  // std::string jsonData = otherSpliceInterface->_spliceGraph.getPersistenceDataJSON();
  // _spliceGraph.setFromPersistenceDataJSON(jsonData.c_str());
}

void FabricDFGBaseInterface::onNodeAdded(MObject &node, void *clientData)
{
  MFnDependencyNode thisNode(node);
  FabricDFGBaseInterface * interf = getInstanceByName(thisNode.name().asChar());
  if(interf)
    interf->managePortObjectValues(false); // reattach
}

void FabricDFGBaseInterface::onNodeRemoved(MObject &node, void *clientData)
{
  MFnDependencyNode thisNode(node);
  FabricDFGBaseInterface * interf = getInstanceByName(thisNode.name().asChar());
  if(interf)
    interf->managePortObjectValues(true); // detach
}

void FabricDFGBaseInterface::onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made)
{
  _affectedPlugsDirty = true;
}

MObject FabricDFGBaseInterface::addMayaAttribute(
  MString portName,
  MString dataTypeMString, 
  FabricCore::DFGPortType portType,
  MString arrayTypeMString,
  MStatus * stat
  )
{
  MAYADFG_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::addMayaAttribute()");

  MFnDependencyNode thisNode( getThisMObject() );
  MString plugName = getPlugName( portName );
  MPlug plug = thisNode.findPlug( plugName );
  if ( !plug.isNull() )
  {
    if ( _isReferenced )
      return plug.attribute();

    std::string error;
    error += "Attribute '";
    error += portName.asChar();
    error += "' already exists on node '";
    error += thisNode.name().asChar();
    error += "'";
    throw FabricCore::Exception( error.c_str() );
  }

  FTL::CStrRef dataTypeCStr = dataTypeMString.asChar();

  // skip if disabled by used in the EditPortDialog
  if ( !m_addAttributeForNextAttribute )
  {
    m_addAttributeForNextAttribute = true;
    FabricCore::DFGExec exec = m_binding.getExec();
    exec.setExecPortMetadata(
      portName.asChar(), "addAttribute", "false", false
      );
    return MObject();
  }

  if ( m_useOpaqueForNextAttribute )
  {
    dataTypeCStr = FTL_STR("SpliceMayaData");
    FabricCore::DFGExec exec = m_binding.getExec();
    exec.setExecPortMetadata(portName.asChar(), "opaque", "true", false);
    m_useOpaqueForNextAttribute = false;
  }

  FabricMaya::DataType dataType = FabricMaya::ParseDataType( dataTypeCStr );

  FTL::CStrRef arrayTypeCStr = arrayTypeMString.asChar();
  if ( m_useNativeArrayForNextAttribute )
  {
    m_useNativeArrayForNextAttribute = false;
    arrayTypeCStr = FTL_STR("Array (Native)");
    FabricCore::DFGExec exec = m_binding.getExec();
    exec.setExecPortMetadata(portName.asChar(), "nativeArray", "true", false);
  }
  else if ( arrayTypeCStr.empty() )
    arrayTypeCStr = FTL_STR("Single Value");
  FabricMaya::ArrayType arrayType =
    FabricMaya::ParseArrayType( arrayTypeCStr );

  FabricCore::Variant compoundStructure;
  MObject obj =
    FabricMaya::CreateMayaAttribute(
      portName.asChar(),
      dataType,
      dataTypeCStr,
      arrayType,
      arrayTypeCStr,
      portType != FabricCore::DFGPortType_Out,
      portType != FabricCore::DFGPortType_In,
      compoundStructure
      );

  thisNode.addAttribute( obj );

  setupMayaAttributeAffects( portName, portType, obj );

  _affectedPlugsDirty = true;
  return obj;

  MAYADFG_CATCH_END(stat);

  return MObject();
}

void FabricDFGBaseInterface::removeMayaAttribute(MString portName, MStatus * stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  MFnDependencyNode thisNode(getThisMObject());
  MString plugName = getPlugName(portName);
  MPlug plug = thisNode.findPlug(plugName);
  if(!plug.isNull())
  {
    thisNode.removeAttribute(plug.attribute());
    _affectedPlugsDirty = true;
  }

  MAYASPLICE_CATCH_END(stat);
}

MObject FabricDFGBaseInterface::createAttributeForPort(MString portName)
{
  MStatus stat;
  MAYASPLICE_CATCH_BEGIN(&stat);

  MFnDependencyNode thisNode(getThisMObject());
  MString plugName = getPlugName(portName);
  MPlug plug = thisNode.findPlug(plugName);
  if(plug.isNull())
  {
    FabricCore::DFGExec exec = getDFGExec();

    FabricCore::DFGPortType portType = exec.getExecPortType(portName.asChar());
    std::string dataType = exec.getExecPortResolvedType(portName.asChar());

    FTL::StrRef opaque = exec.getExecPortMetadata(portName.asChar(), "opaque");
    if(opaque == "true")
      dataType = "SpliceMayaData";

    FabricServices::CodeCompletion::KLTypeDesc typeDesc(dataType);

    std::string arrayType = "Single Value";
    if(typeDesc.isArray())
    {
      arrayType = "Array (Multi)";
      FTL::StrRef nativeArray = exec.getExecPortMetadata(portName.asChar(), "nativeArray");
      if(nativeArray == "true")
      {
        arrayType = "Array (Native)";
        exec.setExecPortMetadata(portName.asChar(), "nativeArray", "true", false);
      }
    }

    FTL::StrRef addAttribute = exec.getExecPortMetadata(portName.asChar(), "addAttribute");
    if(addAttribute != "false")
    {
      _affectedPlugsDirty = true;
      return addMayaAttribute(portName.asChar(), dataType.c_str(), portType, arrayType.c_str());
    }

  }

  MAYASPLICE_CATCH_END(&stat);
  return MObject();
}

void FabricDFGBaseInterface::setupMayaAttributeAffects(MString portName, FabricCore::DFGPortType portType, MObject newAttribute, MStatus *stat)
{
  MAYASPLICE_CATCH_BEGIN(stat);

  FabricSplice::Logging::AutoTimer timer("Maya::setupMayaAttributeAffects()");

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
    else
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

void FabricDFGBaseInterface::managePortObjectValues(bool destroy)
{
  if(_portObjectsDestroyed == destroy)
    return;

  // for(unsigned int i = 0; i < _spliceGraph.getDGPortCount(); ++i) {
  //   FabricSplice::DGPort port = _spliceGraph.getDGPort(i);
  //   if(!port->isValid())
  //     continue;
  //   if(!port->isObject())
  //     continue;

  //   try
  //   {
  //     FabricCore::RTVal value = port->getRTVal();
  //     if(!value.isValid())
  //       continue;
  //     if(value.isNullObject())
  //       continue;

  //     FabricCore::RTVal objectRtVal = FabricSplice::constructRTVal("Object", 1, &value);
  //     if(!objectRtVal.isValid())
  //       continue;

  //     FabricCore::RTVal detachable = FabricSplice::constructInterfaceRTVal("Detachable", objectRtVal);
  //     if(detachable.isNullObject())
  //       continue;

  //     if(destroy)
  //       detachable.callMethod("", "detach", 0, 0);
  //     else
  //       detachable.callMethod("", "attach", 0, 0);
  //   }
  //   catch(FabricCore::Exception e)
  //   {
  //     // ignore errors, probably an object which does not implement deattach and attach
  //   }
  //   catch(FabricSplice::Exception e)
  //   {
  //     // ignore errors, probably an object which does not implement deattach and attach
  //   }
  // }

  _portObjectsDestroyed = destroy;
}

void FabricDFGBaseInterface::allStorePersistenceData(MString file, MStatus *stat)
{
  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->storePersistenceData(file, stat);
}

void FabricDFGBaseInterface::allRestoreFromPersistenceData(MString file, MStatus *stat)
{
  for(size_t i=0;i<_instances.size();i++)
    _instances[i]->restoreFromPersistenceData(file, stat);
}

void FabricDFGBaseInterface::allResetInternalData()
{
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

void FabricDFGBaseInterface::bindingNotificationCallback(
  FTL::CStrRef jsonStr
  )
{
  // MGlobal::displayInfo(jsonCString);

  FTL::JSONStrWithLoc jsonStrWithLoc( jsonStr );
  FTL::OwnedPtr<FTL::JSONObject const> jsonObject(
    FTL::JSONValue::Decode( jsonStrWithLoc )->cast<FTL::JSONObject>()
    );

  FTL::CStrRef descStr = jsonObject->getString( FTL_STR("desc") );

  if ( descStr == FTL_STR("dirty") )
  {
    _outputsDirtied = false;
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
  }
}

void FabricDFGBaseInterface::renamePlug(const MPlug &plug, MString oldName, MString newName)
{
  if(plug.isNull())
    return;

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

#if _SPLICE_MAYA_VERSION >= 2016
MStatus FabricDFGBaseInterface::preEvaluation(MObject thisMObject, const MDGContext& context, const MEvaluationNode& evaluationNode)
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

