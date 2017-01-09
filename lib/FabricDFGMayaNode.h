//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricDFGBaseInterface.h"

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>

#include <FTL/Config.h>

class FabricDFGMayaNode: public MPxNode, public FabricDFGBaseInterface{

public:
  
  static MStatus initialize();

  FabricDFGMayaNode(
    FabricDFGBaseInterface::CreateDFGBindingFunc createDFGBinding
    );
  void postConstructor();
  ~FabricDFGMayaNode();

  // implement pure virtual functions
  virtual MObject getThisMObject() { return thisMObject(); }
  virtual MPlug getSaveDataPlug() { return MPlug(thisMObject(), saveData); }
  virtual MPlug getRefFilePathPlug() { return MPlug(thisMObject(), refFilePath); }
  virtual MPlug getEnableEvalContextPlug() { return MPlug(thisMObject(), enableEvalContext); }

  MStatus compute(const MPlug& plug, MDataBlock& data);
  MStatus setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs);
  MStatus shouldSave(const MPlug &plug, bool &isSaving);
  void copyInternalData(MPxNode *node);
  bool getInternalValueInContext(const MPlug &plug, MDataHandle &dataHandle, MDGContext &ctx);
  bool setInternalValueInContext(const MPlug &plug, const MDataHandle &dataHandle, MDGContext &ctx);

  virtual MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
  virtual MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

#if MAYA_API_VERSION >= 201600
  SchedulingType schedulingType() const
    { return kParallel; }
  virtual MStatus preEvaluation(
    const MDGContext& context,
    const MEvaluationNode& evaluationNode
    ) FTL_OVERRIDE;
  virtual MStatus postEvaluation(
    const MDGContext& context,
    const MEvaluationNode& evaluationNode,
    PostEvaluationType evalType
    ) FTL_OVERRIDE;
#endif

  // node attributes
  static MObject saveData;
  static MObject evalID;
  static MObject refFilePath;
  static MObject enableEvalContext;
};
