//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "Viewport2Override.h"
using namespace MHWRender;

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
