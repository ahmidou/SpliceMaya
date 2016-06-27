//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "Viewport2Override.h"
using namespace MHWRender;

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
      if(FabricSpliceRenderCallback::isRTR2Enable())
      {
        int originX, originY, width, height;
        context.getViewportDimensions(originX, originY, width, height);
        FabricSpliceRenderCallback::drawRTR2(width, height, 4);
      }
      else 
        FabricSpliceRenderCallback::drawID();
      return MStatus::kSuccess;
    }
};


Viewport2Override::Viewport2Override(const MString &name) : MRenderOverride(name) {
  mCurrentOp = -1;
  mOps[0] = mOps[1] = mOps[2] = mOps[3] = NULL;
  mOpNames[0] = "SceneRenderOverride";
  mOpNames[1] = "UserRenderOperationOverride";
  mOpNames[2] = "HudOverride";
  mOpNames[3] = "SimpleTargetOverride";
}

Viewport2Override::~Viewport2Override() {
  for(uint32_t i=0; i<4; i++) {
    delete mOps[i];
    mOps[i] = NULL;
  }
}

MString Viewport2Override::uiName() const { 
  return "Fabric Viewport 2.0"; 
}
    
DrawAPI Viewport2Override::supportedDrawAPIs() const { 
  return kAllDevices; 
};

MStatus Viewport2Override::setup(const MString &destination) {
  MRenderer *theRenderer = MRenderer::theRenderer();
  if(!theRenderer) return MStatus::kFailure;
 
  if(!mOps[0]) {
    mOps[0] = (MRenderOperation*) new SceneRenderOverride(mOpNames[0]);
    mOps[1] = (MRenderOperation*) new UserRenderOperationOverride(mOpNames[1]);
    mOps[2] = (MRenderOperation*) new MHUDRender();
    mOps[3] = (MRenderOperation*) new MPresentTarget(mOpNames[3]);
  }
  return (!mOps[0] || !mOps[1] || !mOps[2] || !mOps[3]) ? MStatus::kFailure : MStatus::kSuccess;
}

MStatus Viewport2Override::cleanup() {
  mCurrentOp = -1;
  return MStatus::kSuccess;
}
  
bool Viewport2Override::startOperationIterator() {
  mCurrentOp = 0;
  return true;
}

MRenderOperation* Viewport2Override::renderOperation() {
  return (mCurrentOp >= 0 && mCurrentOp < 4) ? mOps[mCurrentOp] : NULL;
}

bool Viewport2Override::nextRenderOperation() {
  mCurrentOp++;
  return (mCurrentOp < 4);
}
