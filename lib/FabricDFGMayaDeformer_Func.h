//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricDFGMayaDeformer.h"

class FabricDFGMayaDeformer_Func : public FabricDFGMayaDeformer
{
public:

  static void* creator();

  FabricDFGMayaDeformer_Func();

  static MTypeId id;

protected:

  static FabricCore::DFGBinding CreateDFGBinding(
    FabricCore::DFGHost &dfgHost
    );
};
