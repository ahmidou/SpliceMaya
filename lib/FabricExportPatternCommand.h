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
  double scale;
  MStringArray objects;

  FabricExportPatternSettings()
  {
    scale = 1.0;
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
  MStatus invoke(FabricCore::DFGBinding binding, const FabricExportPatternSettings & settings);

private:

  FabricCore::RTVal createRTValForNode(const MObject & node);

  FabricCore::RTVal m_context;
  std::vector< MObject > m_nodes;
  std::vector< FabricCore::RTVal > m_objects;

  FabricExportPatternSettings m_settings;
};
