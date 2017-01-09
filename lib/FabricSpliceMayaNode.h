//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricSpliceBaseInterface.h"

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>

class FabricSpliceMayaNode: public MPxNode, public FabricSpliceBaseInterface{
  //temporarely disabled
  // friend  void onAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* userData);

public:
  static void* creator();
  static MStatus initialize();

  FabricSpliceMayaNode();
  void postConstructor();
  ~FabricSpliceMayaNode();

  // implement pure virtual functions
  virtual MObject getThisMObject() { return thisMObject(); }
  virtual MPlug getSaveDataPlug() { return MPlug(thisMObject(), saveData); }

  MStatus compute(const MPlug& plug, MDataBlock& data);
  MStatus setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs);
  MStatus shouldSave(const MPlug &plug, bool &isSaving);
  void copyInternalData(MPxNode *node);

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

private:

  MCallbackId m_attributeAddedOrRemovedCallbackID;
};
