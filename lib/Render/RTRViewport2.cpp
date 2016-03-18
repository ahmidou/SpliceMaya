//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "RTRViewport2.h"
using namespace MHWRender;

RTRViewport2::RTRViewport2(const MString &name) : MRenderOverride(name) {
  mCurrentOp = -1;
  mOps[0] = mOps[1] = mOps[2] = mOps[3] = NULL;
  mOpNames[0] = "RTRSceneRender";
  mOpNames[1] = "RTRUserRenderOperation";
  mOpNames[2] = "RTRHud";
  mOpNames[3] = "RTRSimpleTarget";
}

RTRViewport2::~RTRViewport2() {
  for(uint32_t i=0; i<4; i++) {
    delete mOps[i];
    mOps[i] = NULL;
  }
}

MStatus RTRViewport2::setup(const MString &destination) {
  MRenderer *theRenderer = MRenderer::theRenderer();
  if(!theRenderer) return MStatus::kFailure;
 
  if(!mOps[0]) {
    mOps[0] = (MRenderOperation*) new RTRSceneRender(mOpNames[0]);
    mOps[1] = (MRenderOperation*) new RTRUserRenderOperation(mOpNames[1]);
    mOps[2] = (MRenderOperation*) new MHUDRender();
    mOps[3] = (MRenderOperation*) new MPresentTarget(mOpNames[3]);
  }
  return (!mOps[0] || !mOps[1] || !mOps[2] || !mOps[3]) ? MStatus::kFailure : MStatus::kSuccess;
}

MStatus RTRViewport2::cleanup() {
  mCurrentOp = -1;
  return MStatus::kSuccess;
}
  
bool RTRViewport2::startOperationIterator() {
  mCurrentOp = 0;
  return true;
}

MRenderOperation* RTRViewport2::renderOperation() {
  return (mCurrentOp >= 0 && mCurrentOp < 4) ? mOps[mCurrentOp] : NULL;
}

bool RTRViewport2::nextRenderOperation() {
  mCurrentOp++;
  return (mCurrentOp < 4);
}
