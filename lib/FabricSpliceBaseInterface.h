//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

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

#define MAYASPLICE_CATCH_BEGIN(statusPtr) \
  if(statusPtr) \
    *statusPtr=MS::kSuccess; \
  try {

#define MAYASPLICE_CATCH_END(statusPtr) } \
  catch (FabricSplice::Exception e) { \
    mayaLogErrorFunc(e.what()); \
    if (statusPtr) \
      *statusPtr=MS::kFailure; \
  } \
  catch (FabricCore::Exception e) { \
    mayaLogErrorFunc(e.getDesc_cstr()); \
    if (statusPtr) \
      *statusPtr=MS::kFailure; \
  }

class FabricSpliceBaseInterface {

public:

  FabricSpliceBaseInterface();
  virtual ~FabricSpliceBaseInterface();
  void constructBaseInterface();

  virtual MObject getThisMObject() = 0;
  virtual MPlug getSaveDataPlug() = 0;

  static std::vector<FabricSpliceBaseInterface*> getInstances();
  static FabricSpliceBaseInterface * getInstanceByName(const std::string & name);

  MObject addMayaAttribute(const MString &portName, const MString &dataType, const MString &arrayType, const FabricSplice::Port_Mode &portMode, bool compoundChild = false, FabricCore::Variant compoundStructure = FabricCore::Variant(), MStatus *stat = 0);
  void addPort(const MString &portName, const MString &dataType, const FabricSplice::Port_Mode &portMode, const MString & dgNode, bool autoInitObjects, const MString & extension, const FabricCore::Variant & defaultValue, MStatus *stat = 0);
  void removeMayaAttribute(const MString &portName, MStatus *stat = 0);
  void removePort(const MString &portName, MStatus *stat = 0);
  void addKLOperator(const MString &operatorName, const MString &operatorCode, const MString &operatorEntry, const MString & dgNode, const FabricCore::Variant & portMap, MStatus *stat = 0);
  void setKLOperatorEntry(const MString &operatorName, const MString &operatorEntry, MStatus *stat = 0);
  void setKLOperatorIndex(const MString &operatorName, unsigned int operatorIndex, MStatus *stat = 0);
  void setKLOperatorCode(const MString &operatorName, const MString &operatorCode, const MString &operatorEntry, MStatus *stat = 0);
  std::string getKLOperatorCode(const MString &operatorName, MStatus *stat = 0);
  void setKLOperatorFile(const MString &operatorName, const MString &filename, const MString &entry, MStatus *stat = 0);
  void removeKLOperator(const MString &operatorName, const MString & dgNode, MStatus *stat = 0);
  void storePersistenceData(MString file, MStatus *stat = 0);
  void restoreFromPersistenceData(MString file, MStatus *stat = 0);
  void resetInternalData(MStatus *stat = 0);
  MStringArray getKLOperatorNames();
  MStringArray getPortNames();
  FabricSplice::DGPort getPort(MString name);
  void saveToFile(MString fileName);
  MStatus loadFromFile(MString fileName, bool asReferenced = false);
  MStatus reloadFromFile();
  MStatus createAttributeForPort(FabricSplice::DGPort port);
  void setPortPersistence(const MString &portName, bool persistence);
  FabricSplice::DGGraph & getSpliceGraph() { return _spliceGraph; }
  void setDgDirtyEnabled(bool enabled) { _dgDirtyEnabled = enabled; }
  void setEvaluateShared(bool evauateShared);

  static void onNodeAdded(MObject &node, void *clientData);
  static void onNodeRemoved(MObject &node, void *clientData);

  void managePortObjectValues(bool destroy);

protected:
  void invalidatePlug(MPlug & plug);
  virtual void invalidateNode();
  void incEvalID();
  
  void setupMayaAttributeAffects(
    MString portName,
    FabricSplice::Port_Mode portMode,
    MObject newAttribute
    );

  // private members and helper methods
  static std::vector<FabricSpliceBaseInterface*> _instances;
  bool _restoredFromPersistenceData;
  unsigned int _dummyValue;

  FabricSplice::DGGraph _spliceGraph;
  MStringArray _dirtyPlugs;
  MStringArray _evalContextPlugNames;
  MIntArray _evalContextPlugIds;
  std::vector<std::string> mSpliceMayaDataOverride;
  bool _isTransferingInputs;
  bool _portObjectsDestroyed;

  bool transferInputValuesToSplice(MDataBlock& data);
  void evaluate();
  void transferOutputValuesToMaya(MDataBlock& data, bool isDeformer = false);
  void collectDirtyPlug(MPlug const &inPlug);
  void affectChildPlugs(MPlug &plug, MPlugArray &affectedPlugs);
  void copyInternalData(MPxNode *node);
  void onConnection(const MPlug &plug, const MPlug &otherPlug, bool asSrc, bool made);
  MStatus setDependentsDirty(MObject thisMObject, MPlug const &inPlug, MPlugArray &affectedPlugs);

#if MAYA_API_VERSION >= 201600
  MStatus preEvaluation(MObject thisMObject, const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  // static MString sManipulationCommand;
  // MString _manipulationCommand;
  bool _dgDirtyEnabled;
  bool _affectedPlugsDirty;
  bool _outputsDirtied;
  MPlugArray _affectedPlugs;

#if MAYA_API_VERSION < 201400
  static std::map<std::string, int> _nodeCreatorCounts;
#endif

protected:

  void attributeAddedOrRemoved(
    MNodeMessage::AttributeMessage msg,
    MPlug &plug
    );

  static void AttributeAddedOrRemoved(
    MNodeMessage::AttributeMessage msg,
    MPlug &plug,
    void *clientData
    )
  {
    static_cast<FabricSpliceBaseInterface *>( clientData )
      ->attributeAddedOrRemoved( msg, plug );
  }

private:
  bool plugInArray(const MPlug &plug, const MPlugArray &array);
};

float getScalarOption(const char * key, FabricCore::Variant options, float value = 0.0);
std::string getStringOption(const char * key, FabricCore::Variant options, std::string value = "");
