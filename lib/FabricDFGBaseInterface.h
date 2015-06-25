#ifndef _FABRICDFGBASEINTERFACE_H_
#define _FABRICDFGBASEINTERFACE_H_

#include <DFG/DFGUI.h>

#include "FabricSpliceConversion.h"

#include <vector>

#include <maya/MFnDependencyNode.h> 
#include <maya/MPlug.h> 
#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>
#include <maya/MFnCompoundAttribute.h>

#include <FabricSplice.h>
#include <DFG/DFGValueEditor.h>
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
  FabricCore::DFGHost getDFGHost();
  FabricCore::DFGBinding getDFGBinding();
  FabricCore::DFGExec getDFGGraph();
  DFG::DFGNotificationRouter * getDFGRouter();
  DFG::DFGController * getDFGController();

  void storePersistenceData(MString file, MStatus *stat = 0);
  void restoreFromPersistenceData(MString file, MStatus *stat = 0);
  void restoreFromJSON(MString json, MStatus *stat = 0);
  void setReferencedFilePath(MString filePath);
  void reloadFromReferencedFilePath();

  // FabricSplice::DGGraph & getSpliceGraph() { return _spliceGraph; }
  void setDgDirtyEnabled(bool enabled) { _dgDirtyEnabled = enabled; }

  static void onNodeAdded(MObject &node, void *clientData);
  static void onNodeRemoved(MObject &node, void *clientData);
  void managePortObjectValues(bool destroy);

  static void allStorePersistenceData(MString file, MStatus *stat = 0);
  static void allRestoreFromPersistenceData(MString file, MStatus *stat = 0);
  static void allResetInternalData();

  virtual void invalidateNode();

  virtual void incrementEvalID();

  void setUseNativeArrayForNextAttribute(bool state) { m_useNativeArrayForNextAttribute = state; }
  void setUseOpaqueForNextAttribute(bool state) { m_useOpaqueForNextAttribute = state; }

protected:
  MString getPlugName(MString portName);
  MString getPortName(MString plugName);
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
  void onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made);
  MStatus setDependentsDirty(MObject thisMObject, MPlug const &inPlug, MPlugArray &affectedPlugs);

#if _SPLICE_MAYA_VERSION >= 2016
  MStatus preEvaluation(MObject thisMObject, const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  MObject addMayaAttribute(MString portName, MString dataType, FabricCore::DFGPortType portType, MString arrayType = "", bool compoundChild = false, MStatus * stat = NULL);
  void removeMayaAttribute(MString portName, MStatus * stat = NULL);

  // static MString sManipulationCommand;
  // MString _manipulationCommand;
  bool _dgDirtyEnabled;
  bool _affectedPlugsDirty;
  bool _outputsDirtied;
  bool _isReferenced;
  MPlugArray _affectedPlugs;

#if _SPLICE_MAYA_VERSION < 2013
  static std::map<std::string, int> _nodeCreatorCounts;
#endif
  static std::vector<FabricDFGBaseInterface*> _instances;

  FabricCore::Client m_client;
  FabricServices::ASTWrapper::KLASTManager * m_manager;
  FabricCore::DFGHost m_host;
  FabricCore::DFGBinding m_binding;
  FabricCore::RTVal m_evalContext;
  DFG::DFGNotificationRouter * m_router;
  FabricUI::DFG::DFGController * m_ctrl;
  std::map<std::string, std::string> _argTypes;

private:
  static void bindingNotificationCallback(void * userData, char const *jsonCString, uint32_t jsonLength);
  bool plugInArray(const MPlug &plug, const MPlugArray &array);
  void renamePlug(const MPlug &plug, MString oldName, MString newName);
  static MString resolveEnvironmentVariables(const MString & filePath);

  unsigned int m_id;
  static unsigned int s_maxID;
  bool m_useNativeArrayForNextAttribute;
  bool m_useOpaqueForNextAttribute;
};

#endif
