//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 

#include <vector>

class FabricExtensionPackageNode: public MPxNode {

public:
  static void* creator();
  static MStatus initialize();

  FabricExtensionPackageNode();
  ~FabricExtensionPackageNode();

  static FabricExtensionPackageNode * getInstanceByMObject(const MObject & obj);
  static FabricExtensionPackageNode * getInstanceByIndex(unsigned int index);
  static unsigned int getNumInstances();

  MString getExtensionNames() const { return MPlug(thisMObject(), extensionNames).asString(); }
  MString getExtensionSuffix() const { return MPlug(thisMObject(), extensionSuffix).asString(); }
  MString getExtensionPackage() const { return MPlug(thisMObject(), extensionPackage).asString(); }
  void setExtensionNames(MString value) const { MPlug(thisMObject(), extensionNames).setString(value); }
  void setExtensionSuffix(MString value) const { MPlug(thisMObject(), extensionSuffix).setString(value); }
  void setExtensionPackage(MString value) const { MPlug(thisMObject(), extensionPackage).setString(value); }

  // node attributes
  static MTypeId id;
  static MObject extensionNames;
  static MObject extensionSuffix;
  static MObject extensionPackage;

private:
  static std::vector<FabricExtensionPackageNode*> _instances;

};
