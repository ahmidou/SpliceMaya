//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <DFG/DFGUI.h>

#include "FabricSpliceConversion.h"
#include "DFGUICmdHandler_Maya.h"

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

using namespace FabricServices;
using namespace FabricUI;

#define MAYADFG_CATCH_BEGIN(statusPtr) \
  if(statusPtr) \
    *statusPtr=MS::kSuccess; \
  try {

#define MAYADFG_CATCH_END(statusPtr) } \
  catch (FabricCore::Exception e) { \
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
  static unsigned int getNumInstances();

  virtual MObject getThisMObject() = 0;
  virtual MPlug getSaveDataPlug() = 0;
  virtual MPlug getRefFilePathPlug() = 0;

  unsigned int getId() const;
  FabricCore::Client getCoreClient();
  ASTWrapper::KLASTManager * getASTManager();
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

  virtual void invalidateNode();

  virtual void incrementEvalID();

  DFGUICmdHandler_Maya *getCmdHandler()
    { return &m_cmdHandler; }

  bool getExecuteShared();
  void setExecuteSharedDirty()
    { m_executeSharedDirty = true; }

protected:
  inline MString getPlugName(const MString &portName);
  inline MString getPortName(const MString &plugName);
  void invalidatePlug(MPlug & plug);
  void setupMayaAttributeAffects(MString portName, FabricCore::DFGPortType portType, MObject newAttribute, MStatus *stat = 0);

  // private members and helper methods
  bool _restoredFromPersistenceData;

  // FabricSplice::DGGraph _spliceGraph;
  MStringArray _dirtyPlugs;
  MStringArray _evalContextPlugNames;
  MIntArray _evalContextPlugIds;
  bool _isTransferingInputs;
  bool _portObjectsDestroyed;
  std::vector<std::string> mSpliceMayaDataOverride;

  bool transferInputValuesToDFG(MDataBlock& data);
  void evaluate();
  void transferOutputValuesToMaya(MDataBlock& data, bool isDeformer = false);
  void collectDirtyPlug(MPlug const &inPlug);
  void affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs);
  void copyInternalData(MPxNode *node);
  bool getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx);
  bool setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx);
  void onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made);
  MStatus setDependentsDirty(MObject thisMObject, MPlug const &inPlug, MPlugArray &affectedPlugs);

#if _SPLICE_MAYA_VERSION >= 2016
  MStatus preEvaluation(MObject thisMObject, const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  MObject addMayaAttribute(MString portName, MString dataType, FabricCore::DFGPortType portType, MString arrayType = "", bool compoundChild = false, MStatus * stat = NULL);
  void removeMayaAttribute(MString portName, MStatus * stat = NULL);

  FabricCore::LockType getLockType()
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

#if _SPLICE_MAYA_VERSION < 2013
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
};
