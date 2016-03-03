//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>

class FabricUpgradeAttrCommand: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();

  MStatus doIt(const MArgList &args);

private:
};
