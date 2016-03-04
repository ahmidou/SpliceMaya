//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <FabricCore.h>
#include <maya/M3dView.h>

bool isRTRPassEnabled();
void enableRTRPass(bool enable);

class FabricSpliceRenderCallback
{
public:
  static void draw(const MString &str, void *clientData);
  static FabricCore::RTVal & getDrawContext(const MString &str, M3dView & view);
  static FabricCore::RTVal sDrawContext;
};
