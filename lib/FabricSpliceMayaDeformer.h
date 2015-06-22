

#ifndef _CREATIONSPLICEMAYADEFORMER_H_
#define _CREATIONSPLICEMAYADEFORMER_H_

#include "FabricSpliceBaseInterface.h"

#include <maya/MPxDeformerNode.h> 
#include <maya/MTypeId.h> 
#include <maya/MItGeometry.h>

class FabricSpliceMayaDeformer: public MPxDeformerNode, public FabricSpliceBaseInterface{
public:
  static void* creator();
  static MStatus initialize();

  FabricSpliceMayaDeformer();
  void postConstructor();
  ~FabricSpliceMayaDeformer();

  // implement pure virtual functions
  virtual MObject getThisMObject() { return thisMObject(); }
  virtual MPlug getSaveDataPlug() { return MPlug(thisMObject(), saveData); }

  MStatus deform(MDataBlock& block, MItGeometry& iter, const MMatrix&, unsigned int multiIndex);
  MStatus setDependentsDirty(MPlug const &inPlug, MPlugArray &affectedPlugs);
  MStatus shouldSave(const MPlug &plug, bool &isSaving);
  void copyInternalData(MPxNode *node);

  virtual MStatus connectionMade(const MPlug &plug, const MPlug &otherPlug, bool asSrc);
  virtual MStatus connectionBroken(const MPlug &plug, const MPlug &otherPlug, bool asSrc);

#if _SPLICE_MAYA_VERSION >= 2016
  virtual MStatus preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode);
#endif

  // node attributes
  static MTypeId id;
  static MObject saveData;
  static MObject evalID;

protected:
  virtual void invalidateNode();

private:
  int initializePolygonMeshPorts(MPlug &meshPlug, MDataBlock &data);
  // void initializeGeometry(MObject &meshObj);
  int mGeometryInitialized;
};

#endif

