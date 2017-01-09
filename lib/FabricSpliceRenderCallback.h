//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef __FABRIC_SPLICE_RENDER_CALLBACK_H__
#define __FABRIC_SPLICE_RENDER_CALLBACK_H__

#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceBaseInterface.h"
 
#include <cmath> 
#include <iostream>

#include "Foundation.h"
#include <FabricCore.h>
#include <FabricSplice.h>

#include <maya/MColor.h>
#include <maya/MString.h>
#include <maya/M3dView.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <maya/MUiMessage.h>
#include <maya/MFnDagNode.h>
#include <maya/MAnimControl.h>
#include <maya/MDrawContext.h>
#include <maya/MFrameContext.h>
#include <maya/MViewport2Renderer.h>
#include <FabricUI/SceneHub/SHGLRenderer.h>

bool isRTRPassEnabled();

void enableRTRPass(bool enable);

class FabricSpliceRenderCallback {

  public:

    static uint32_t gPanelId;

    static bool gCallbackEnabled;
    
    static FabricCore::RTVal sDrawContext;
    
    static FabricUI::SceneHub::SHGLRenderer shHostGLRenderer;

    static void plug(); 
    
    static void unplug();

    static void enable(bool enable);
    
    static void disable();
    
    static bool isEnabled();
    
    static bool isRTR2Enable();

    static bool canDraw();
    
    static void drawID();

    static MStatus drawRTR2(uint32_t width, uint32_t height, uint32_t phase);

    static void preDrawCallback(const MString &panelName, void *clientData);
   
    static void postDrawCallback(const MString &panelName, void *clientData);

#if MAYA_API_VERSION >= 201600
    static void viewport2OverridePreDrawCallback(MHWRender::MDrawContext &context, void* clientData);
#endif
    
};


#endif // __FABRIC_SPLICE_RENDER_CALLBACK_H__
