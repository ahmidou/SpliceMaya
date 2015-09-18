
#ifndef _FABRICDFGMAYANODE_H_
#define _FABRICDFGMAYANODE_H_

#include "FabricDFGBaseInterface.h"

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MNodeMessage.h>
#include <maya/MStringArray.h>

class FabricDFGMayaNode: public MPxNode, public FabricDFGBaseInterface{

public:
  static void* creator();
  static MStatus initialize();

  FabricDFGMayaNode();
  void postConstructor();
  ~FabricDFGMayaNode();

  // implement pure virtual functions
  virtual MObject getThisMObject() { return thisMObject(); }
  virtual MPlug getSaveDataPlug() { return MPlug(thisMObject(), saveData); }
  virtual MPlug getRefFilePathPlug() { return MPlug(thisMObject(), refFilePath); }

  MStatus compute(const MPlug& plug, MDataBlock& data);
  MStatus setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs);
  MStatus shouldSave(const MPlug &plug, bool &isSaving);
  void copyInternalData(MPxNode *node);

  virtual MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
  virtual MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

#if _SPLICE_MAYA_VERSION >= 2016
  SchedulingType schedulingType() const
    { return kParallel; }
  virtual MStatus preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  // node attributes
  static MTypeId id;
  static MObject saveData;
  static MObject evalID;
  static MObject refFilePath;
};

#endif
