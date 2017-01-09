//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <map>
#include <ostream>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

class FabricSpliceEditorCmd: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();
  MStatus doIt(const MArgList &args);

private:
};
