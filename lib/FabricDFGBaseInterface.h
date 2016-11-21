//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <DFG/DFGUI.h>

#include "FabricSpliceConversion.h"
#include "DFGUICmdHandler_Maya.h"
#include "FabricDFGConversion.h"

#include <vector>

#include <maya/MFnDependencyNode.h> 
#include <maya/MPlug.h> 
#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>
#include <maya/MFnCompoundAttribute.h>

#include <FabricSplice.h>
#include <Commands/CommandStack.h>

#include <FTL/OrderedStringMap.h>

using namespace FabricServices;
using namespace FabricUI;

#define MAYADFG_CATCH_BEGIN(statusPtr) \
  if(statusPtr) \
    *statusPtr=MS::kSuccess; \
  try {

#define MAYADFG_CATCH_END(statusPtr) } \
  catch (FabricCore::Exception e) { \
    if (e.getDescLength()) \
      mayaLogErrorFunc(e.getDesc_cstr()); \
    if (statusPtr) \
      *statusPtr=MS::kFailure; \
  }

class FabricDFGWidget;

class FabricDFGBaseInterface {

public:

  FabricDFGBaseInterface();
  virtual ~FabricDFGBaseInterface();
  void constructBaseInterface();

  static FabricDFGBaseInterface * getInstanceByName(const std::string & name);
  static FabricDFGBaseInterface * getInstanceById(unsigned int id);
  static FabricDFGBaseInterface * getInstanceByIndex(unsigned int index);
  static unsigned int getNumInstances();

  virtual MObject getThisMObject() = 0;
  virtual MPlug getSaveDataPlug() = 0;
  virtual MPlug getRefFilePathPlug() = 0;
  virtual MPlug getEnableEvalContextPlug() = 0;

  unsigned int getId() const;
  FabricCore::Client getCoreClient();
  ASTWrapper::KLASTManager * getASTManager();
  FabricCore::DFGHost getDFGHost();
  FabricCore::DFGBinding getDFGBinding();
  FabricCore::DFGExec getDFGExec();

  void storePersistenceData(MString file, MStatus *stat = 0);
  void restoreFromPersistenceData(MString file, MStatus *stat = 0);
  void restoreFromJSON(MString json, MStatus *stat = 0);
  void setReferencedFilePath(MString filePath);
  void reloadFromReferencedFilePath();

  // FabricSplice::DGGraph & getSpliceGraph() { return _spliceGraph; }
  void setDgDirtyEnabled(bool enabled) { _dgDirtyEnabled = enabled; }

  static void onNodeAdded(MObject &node, void *clientData);
  static void onNodeRemoved(MObject &node, void *clientData);
  static void onAnimCurveEdited(MObjectArray &editedCurves, void *clientData);

  void managePortObjectValues(bool destroy);

  static void allStorePersistenceData(MString file, MStatus *stat = 0);
  static void allRestoreFromPersistenceData(MString file, MStatus *stat = 0);
  static void allResetInternalData();
  static void setAllRestoredFromPersistenceData(bool value);

  virtual void invalidateNode();

  virtual void incrementEvalID();

  DFGUICmdHandler_Maya *getCmdHandler()
    { return &m_cmdHandler; }

  bool getExecuteShared();
  void setExecuteSharedDirty()
    { m_executeSharedDirty = true; }

  virtual MString getPlugName(const MString &portName);
  virtual MString getPortName(const MString &plugName);

protected:
  void invalidatePlug(MPlug & plug);
  virtual void setupMayaAttributeAffects(MString portName, FabricCore::DFGPortType portType, MObject newAttribute, MStatus *stat = 0);
  virtual bool useEvalContext();

  // private members and helper methods
  bool _restoredFromPersistenceData;

  // FabricSplice::DGGraph _spliceGraph;
  // MStringArray _dirtyPlugs;
  std::vector< unsigned int > _argIndexToAttributeIndex;
  std::vector< bool > _isAttributeIndexDirty;
  FTL::OrderedStringMap< unsigned int > _attributeNameToIndex;
  MPlugArray _attributePlugs;
  std::vector< DFGPlugToArgFunc > _plugToArgFuncs;
  std::vector< DFGArgToPlugFunc > _argToPlugFuncs;

  bool _isTransferingInputs;
  bool _portObjectsDestroyed;
  std::vector<std::string> mSpliceMayaDataOverride;

  virtual bool transferInputValuesToDFG(MDataBlock& data);
  void evaluate();
  virtual void transferOutputValuesToMaya(MDataBlock& data, bool isDeformer = false);
  virtual void collectDirtyPlug(MPlug const &inPlug);
  virtual void generateAttributeLookups();
  void affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs);
  void copyInternalData(MPxNode *node);
  bool getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx);
  bool setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx);
  void onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made);
  MStatus setDependentsDirty(MObject thisMObject, MPlug const &inPlug, MPlugArray &affectedPlugs);

#if MAYA_API_VERSION >= 201600
  MStatus doPreEvaluation(
    MObject thisMObject,
    const MDGContext& context,
    const MEvaluationNode& evaluationNode
    );
  MStatus doPostEvaluation(
    MObject thisMObject,
    const MDGContext& context,
    const MEvaluationNode& evaluationNode,
    MPxNode::PostEvaluationType evalType
    );
#endif

  MObject addMayaAttribute(MString portName, MString dataType, FabricCore::DFGPortType portType, MString arrayType = "", bool compoundChild = false, MStatus * stat = NULL);
  void removeMayaAttribute(MString portName, MStatus * stat = NULL);

  virtual FabricCore::LockType getLockType()
  {
    return getExecuteShared()?
      FabricCore::LockType_Shared:
      FabricCore::LockType_Exclusive;
  }

  // static MString sManipulationCommand;
  // MString _manipulationCommand;
  bool _dgDirtyEnabled;
  bool _affectedPlugsDirty;
  bool _outputsDirtied;
  bool _isReferenced;
  bool _isEvaluating;
  bool _dgDirtyQueued;
  unsigned int m_evalID;
  unsigned int m_evalIDAtLastEvaluate;
  MPlugArray _affectedPlugs;

#if MAYA_API_VERSION < 201300
  static std::map<std::string, int> _nodeCreatorCounts;
#endif
  static std::vector<FabricDFGBaseInterface*> _instances;

  FabricCore::Client m_client;
  FabricServices::ASTWrapper::KLASTManager * m_manager;
  FabricCore::DFGBinding m_binding;
  FabricCore::RTVal m_evalContext;
  std::map<std::string, std::string> _argTypes;
  DFGUICmdHandler_Maya m_cmdHandler;

private:

  void bindingNotificationCallback( FTL::CStrRef jsonStr );
  static void BindingNotificationCallback(
    void * userData, char const *jsonCString, uint32_t jsonLength
    )
  {
    FabricDFGBaseInterface * interf =
      static_cast<FabricDFGBaseInterface *>( userData );
    interf->bindingNotificationCallback(
      FTL::CStrRef( jsonCString, jsonLength )
      );
  }

protected:

  struct VisitCallbackUserData
  {
    VisitCallbackUserData(MObject inNode, MDataBlock & inData)
    : node(inNode)
    , data(inData)
    , returnValue(0)
    {
    }

    FabricDFGBaseInterface * interf;
    MFnDependencyNode node;
    bool isDeformer;
    MPlug meshPlug;
    MDataBlock & data;
    int returnValue;
  };

private:

  static void VisitInputArgsCallback(
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
    );  

  static void VisitOutputArgsCallback(
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
    );  

  bool plugInArray(const MPlug &plug, const MPlugArray &array);
  void renamePlug(const MPlug &plug, MString oldName, MString newName);
  static MString resolveEnvironmentVariables(const MString & filePath);

  unsigned int m_id;
  static unsigned int s_maxID;
  bool m_executeSharedDirty;
  bool m_executeShared;
  MString m_lastJson;
  bool m_isStoringJson;

// [FE-6287]
public:
  static bool s_use_evalContext;

public:

  // returns true if the binding's executable has a port called portName that matches the port type (input/output).
  // params:  in_portName     name of the port.
  //          testForInput    true: look for input port, else for output port.
  bool HasPort(const char *in_portName, const bool testForInput);

  // returns true if the binding's executable has an input port called portName.
  bool HasInputPort(const char *portName);
  bool HasInputPort(const std::string &portName);

  // gets the value of an argument (= a "port").
  // params:  binding     ref at binding.
  //          argName     name of the argument (= the "port").
  //          out         will contain the result.
  //          strict      true: the type must match perfectly, false: the type must 'kind of' match and will be converted if necessary (and if possible).
  // returns: 0 on success, -1 wrong port type, -2 invalid port, -3 unknown, -4 Fabric exception.
  static int GetArgValueBoolean(FabricCore::DFGBinding &binding, char const *argName, bool                 &out, bool strict = false);
  static int GetArgValueInteger(FabricCore::DFGBinding &binding, char const *argName, int                  &out, bool strict = false);
  static int GetArgValueFloat  (FabricCore::DFGBinding &binding, char const *argName, double               &out, bool strict = false);
  static int GetArgValueString (FabricCore::DFGBinding &binding, char const *argName, std::string          &out, bool strict = false);
  static int GetArgValueVec3   (FabricCore::DFGBinding &binding, char const *argName, std::vector <double> &out, bool strict = false);
  static int GetArgValueColor  (FabricCore::DFGBinding &binding, char const *argName, std::vector <double> &out, bool strict = false);
  static int GetArgValueMat44  (FabricCore::DFGBinding &binding, char const *argName, std::vector <double> &out, bool strict = false);
};
