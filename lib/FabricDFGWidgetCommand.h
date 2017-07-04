//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <maya/MArgList.h>
#include <maya/MPxCommand.h>

class FabricDFGWidgetCommand: public MPxCommand{
  public:
    static void* creator();
    
    static MSyntax newSyntax();

    MStatus doIt(
      const MArgList &args
      );
};
