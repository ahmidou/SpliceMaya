//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#ifndef __RTR_OVERRIDE_H__
#define __RTR_OVERRIDE_H__
 
#include "FabricSpliceRenderCallback.h"
#include <maya/MUiMessage.h>
#include <maya/MDrawContext.h>
#include <maya/MViewport2Renderer.h>

using namespace MHWRender;

const MString Viewport2Override_name = "Viewport2Override";

class Viewport2Override : public MRenderOverride {

  public:
    Viewport2Override(const MString &name);
    virtual ~Viewport2Override();
    virtual MStatus setup(const MString& destination);
    virtual MStatus cleanup();
    virtual bool startOperationIterator();
    virtual MRenderOperation* renderOperation();
    virtual bool nextRenderOperation();

    virtual MString uiName() const { return "Fabric Viewport2.0"; }
    virtual DrawAPI supportedDrawAPIs() const { return kAllDevices; };
        
  private:
    int mCurrentOp;
    MString mOpNames[4];
    MRenderOperation* mOps[4];
};

class SceneRenderOverride : public MSceneRender {
  public:
    SceneRenderOverride(const MString &name) : MSceneRender(name) {}
    virtual MClearOperation& clearOperation() {
      MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
      bool gradient = renderer->useGradient();
      MColor color1 = renderer->clearColor();
      MColor color2 = renderer->clearColor2();
      float c1[4] = { color1[0], color1[1], color1[2], 1.0f };
      float c2[4] = { color2[0], color2[1], color2[2], 1.0f };
      mClearOperation.setClearColor( c1 );
      mClearOperation.setClearColor2( c2 );
      mClearOperation.setClearGradient( gradient);
      return mClearOperation;
    }
};
 
class UserRenderOperationOverride : public MUserRenderOperation {
  public:
    UserRenderOperationOverride(const MString &name) : MUserRenderOperation(name) {}
    
    virtual MStatus execute(const MDrawContext &context) {
      int originX, originY, width, height;
      MStatus status = context.getViewportDimensions(originX, originY, width, height);
      (void)status;
      FabricSpliceRenderCallback::drawID();//width, height, 4);
      return MStatus::kSuccess;
    }
};

#endif // __RTR_OVERRIDE_H__
