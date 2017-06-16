//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricDFGMayaNode.h"

class FabricDFGMayaNode_Func : public FabricDFGMayaNode
{
  public:
    static void* creator();

    FabricDFGMayaNode_Func();

    static MTypeId id;

  protected:
    static FabricCore::DFGBinding CreateDFGBinding(
      FabricCore::DFGHost &dfgHost
      );
};
