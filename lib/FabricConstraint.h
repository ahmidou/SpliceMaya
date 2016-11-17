//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 

class FabricConstraint: public MPxNode {

public:
  static void* creator();
  static MStatus initialize();

  FabricConstraint();
  ~FabricConstraint();

  MStatus compute(const MPlug& plug, MDataBlock& data);

  // node attributes
  static MTypeId id;
  static MObject mode;
  static MObject input;
  static MObject rotateOrder;
  static MObject offset;
  static MObject translate;
  static MObject rotate;
  static MObject scale;
};
