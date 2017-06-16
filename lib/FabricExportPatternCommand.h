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

struct FabricExportPatternSettings
{
  MString filePath;
  bool quiet;
  double scale;
  double startFrame;
  double endFrame;
  double fps;
  unsigned int substeps;

  MStringArray objects;

  FabricExportPatternSettings()
  {
    quiet = false;
    scale = 1.0;
    startFrame = 1;
    endFrame = 1;
    fps = 24.0;
    substeps = 1;
  }
};

class FabricExportPatternCommand: public MPxCommand
{
public:

  virtual const char * getName() { return "FabricExportPattern"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
  virtual bool isUndoable() const { return false; }
  MStatus invoke(FabricCore::Client client, FabricCore::DFGBinding binding, const FabricExportPatternSettings & settings);

private:

  bool registerNode(const MObject & node, std::string prefix);
  FabricCore::RTVal createRTValForNode(const MObject & node);
  bool updateRTValForNode(double t, const MObject & node, FabricCore::RTVal & object);
  MString getPathFromDagPath(MDagPath dagPath);

  FabricCore::Client m_client;
  FabricCore::RTVal m_exporterContext;
  FabricCore::RTVal m_importer;
  std::vector< MObject > m_nodes;
  std::map< std::string, size_t > m_nodePaths;
  std::vector< FabricCore::RTVal > m_objects;
  std::map< std::string, size_t > m_objectPaths;

  FabricExportPatternSettings m_settings;
};
