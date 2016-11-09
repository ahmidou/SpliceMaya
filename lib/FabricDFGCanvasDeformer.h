//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricDFGBaseInterface.h"

#include <maya/MPxDeformerNode.h>
#include <maya/MTypeId.h>
#include <maya/MItGeometry.h>
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>

class FabricDFGMayaDeformer: public MPxDeformerNode, public FabricDFGBaseInterface{

public:
  static void* creator();
  static MStatus initialize();

  FabricDFGMayaDeformer();
  void postConstructor();
  ~FabricDFGMayaDeformer();

  // implement pure virtual functions
  virtual MObject getThisMObject() { return thisMObject(); }
  virtual MPlug getSaveDataPlug() { return MPlug(thisMObject(), saveData); }
  virtual MPlug getRefFilePathPlug() { return MPlug(thisMObject(), refFilePath); }
  virtual MPlug getEnableEvalContextPlug() { return MPlug(thisMObject(), enableEvalContext); }

  MStatus deform(MDataBlock& block, MItGeometry& iter, const MMatrix&, unsigned int multiIndex);
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
  virtual MStatus preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  static void VisitMeshCallback(
    void *userdata,
    unsigned argIndex,
    char const *argName,
    char const *argTypeName,
    FEC_DFGPortType argOutsidePortType,
    uint64_t argRawDataSize,
    FEC_DFGBindingVisitArgs_GetCB getCB,
    FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
    FEC_DFGBindingVisitArgs_SetCB setCB,
    FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
    void *getSetUD
    );    

  // node attributes
  static MTypeId id;
  static MObject saveData;
  static MObject evalID;
  static MObject refFilePath;
  static MObject enableEvalContext;

protected:
  virtual void invalidateNode();

private:

  int initializePolygonMeshPorts(MPlug &meshPlug, MDataBlock &data);
  // void initializeGeometry(MObject &meshObj);
  int mGeometryInitialized;
};
