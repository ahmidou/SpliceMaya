//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricDFGBaseInterface.h"

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>

class FabricDFGRigNode: public MPxNode, public FabricDFGBaseInterface{

public:
  static void* creator();
  static MStatus initialize();

  FabricDFGRigNode();
  void postConstructor();
  ~FabricDFGRigNode();

  // implement pure virtual functions
  virtual MObject getThisMObject() { return thisMObject(); }
  virtual MPlug getSaveDataPlug() { return MPlug(thisMObject(), saveData); }
  virtual MPlug getRefFilePathPlug() { return MPlug(thisMObject(), refFilePath); }

  MStatus compute(const MPlug& plug, MDataBlock& data);
  MStatus setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs);
  MStatus shouldSave(const MPlug &plug, bool &isSaving);
  void copyInternalData(MPxNode *node);
  bool getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx);
  bool setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx);

  // overrides from DFGBaseInterface
  virtual MString getPlugName(const MString &portName);
  virtual MString getPortName(const MString &plugName);
  virtual bool transferInputValuesToDFG(MDataBlock& data);
  virtual void setupMayaAttributeAffects(MString portName, FabricCore::DFGPortType portType, MObject newAttribute, MStatus *stat = 0);

  virtual MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
  virtual MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

#if MAYA_API_VERSION >= 201600
  SchedulingType schedulingType() const
    { return kParallel; }
  virtual MStatus preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  // node attributes
  static MTypeId id;
  static MObject saveData;
  static MObject evalID;
  static MObject refFilePath;

  static MObject matrixInputs;
  static MObject vectorInputs;
  static MObject scalarInputs;
  static MObject matrixOutputs;
  static MObject vectorOutputs;
  static MObject scalarOutputs;
};
