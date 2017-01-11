//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <iostream>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MPxCommand.h>

#include <FabricCore.h>

#include <vector>
#include <map>

class FabricImportPatternCommand: public MPxCommand
{
public:

  virtual const char * getName() { return "FabricImportPattern"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
  virtual bool isUndoable() const { return false; }
  MStatus invoke(FabricCore::DFGBinding binding, MString rootPrefix);

private:
  FabricCore::RTVal m_context;
  std::vector< FabricCore::RTVal > m_objectList;
  std::map< std::string, size_t > m_objectMap;
  std::map< std::string, MObject > m_nodes;
  std::map< std::string, MObject > m_materialSets;

  MString m_rootPrefix;

  MString parentPath(MString path, MString * name = NULL);
  MString simplifyPath(MString path);
  MObject getOrCreateNodeForPath(MString path, MString type="transform", bool createIfMissing = true);
  MObject getOrCreateNodeForObject(FabricCore::RTVal obj);
  bool updateTransformForObject(FabricCore::RTVal obj, MObject node = MObject::kNullObj);
  bool updateShapeForObject(FabricCore::RTVal obj);
  bool updateMaterialForObject(FabricCore::RTVal obj, MObject node);
};
