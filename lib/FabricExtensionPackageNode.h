//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <maya/MPxNode.h> 
#include <maya/MTypeId.h> 

#include <vector>
#include <FabricCore.h>

class FabricExtensionPackageNode: public MPxNode {

public:
  static void* creator();

  static MStatus initialize();

  FabricExtensionPackageNode();

  ~FabricExtensionPackageNode();

  static FabricExtensionPackageNode * getInstanceByMObject(
    const MObject & obj
    );
  
  static FabricExtensionPackageNode * getInstanceByIndex(
    unsigned int index
    );
  
  static unsigned int getNumInstances();

  MStringArray getExtensionNames() const;
  
  MString getExtensionSuffix() const { return MPlug(thisMObject(), extensionSuffix).asString(); }
  
  MString getExtensionPackage() const { return MPlug(thisMObject(), extensionPackage).asString(); }
  
  void setExtensionNames(
    MStringArray values
    ) const;
  
  void setExtensionSuffix(MString value) const { MPlug(thisMObject(), extensionSuffix).setString(value); }
  
  void setExtensionPackage(MString value) const { MPlug(thisMObject(), extensionPackage).setString(value); }

  MStatus loadPackage(
    FabricCore::Client client
    );

  bool isLoaded() const { return _loaded; }

  // node attributes
  static MTypeId id;
  static MObject extensionNames;
  static MObject extensionSuffix;
  static MObject extensionPackage;

private:
  static std::vector<FabricExtensionPackageNode*> _instances;
  bool _loaded;

};
