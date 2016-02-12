//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <FabricCore.h>
#include <maya/M3dView.h>

bool isHostToRTRCallbackEnabled();
void enableHostToRTRCallback(bool enable);


class FabricSpliceRenderCallback
{
public:
  static void draw(const MString &str, void *clientData);
  static FabricCore::RTVal & getDrawContext(const MString &str, M3dView & view);
  static FabricCore::RTVal sDrawContext;
  static FabricCore::RTVal sHostToRTRCallback;
  static void invalidateHostToRTRCallback();
  static void setCameraTranformFromMaya(M3dView &view, FabricCore::RTVal &camera);
  static void setCameraProjectionFromMaya(M3dView &view, FabricCore::RTVal &camera);
  static void setCameraParamtersFromMaya(M3dView &view, FabricCore::RTVal &camera);
  static FabricCore::RTVal& getHostToRTRCallback(const MString &str, M3dView &view);
  static void preRenderCallback(const MString &str, void *clientData);
  static void postRenderCallback(const MString &str, void *clientData);
};
