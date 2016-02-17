// Copyright 2015 Autodesk, Inc.  All rights reserved.
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.

#include "viewOverrideSimple.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceRenderCallback.h"

#include <cmath> 
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MFrameContext.h>
#include <maya/MDrawContext.h>
#include <iostream>


ViewOverrideSimple::ViewOverrideSimple(const MString &clientContextID, const MString &name, const MString &viewportName )
  : MRenderOverride( name )
  , mViewportName( viewportName )
  , mUIName("Simple VP2 Override")
  , mCurrentOperation(-1)
{
  MGlobal::displayError( clientContextID );
  mOperations[0] = mOperations[1] = mOperations[2] = mOperations[3] = NULL;
  mOperationNames[0] = "ViewOverrideSimple_Scene";
  mOperationNames[1] = "RTRRender";
  mOperationNames[2] = "ViewOverrideUSER_HUD";
  mOperationNames[3] = "ViewOverrideSimple_HUD";
}

ViewOverrideSimple::~ViewOverrideSimple() {
  for (unsigned int i=0; i<4; i++)
  {
    if (mOperations[i])
    {
      delete mOperations[i];
      mOperations[i] = NULL;
    }
  }
}
  
MHWRender::DrawAPI ViewOverrideSimple::supportedDrawAPIs() const {
  return MHWRender::kAllDevices;
}

bool ViewOverrideSimple::startOperationIterator() {
  mCurrentOperation = 0;
  return true;
}

MHWRender::MRenderOperation* ViewOverrideSimple::renderOperation() {
  if (mCurrentOperation >= 0 && mCurrentOperation < 4)
  {
    if (mOperations[mCurrentOperation])
      return mOperations[mCurrentOperation];
  }
  return NULL;
}

bool ViewOverrideSimple::nextRenderOperation() {
  mCurrentOperation++;
  return (mCurrentOperation < 4);
}

MStatus ViewOverrideSimple::setup( const MString & destination ) {
  MHWRender::MRenderer *theRenderer = MHWRender::MRenderer::theRenderer();
  if(!theRenderer)
    return MStatus::kFailure;

  // Create a new set of operations as required
  if(!mOperations[0])
  {
    mOperations[0] = (MHWRender::MRenderOperation*) new simpleViewRenderSceneRender(mOperationNames[0], mViewportName);
    mOperations[1] = (MHWRender::MRenderOperation*) new RTRRender(mOperationNames[1], mViewportName);
    mOperations[2] = (MHWRender::MRenderOperation*) new MHWRender::MHUDRender();
    mOperations[3] = (MHWRender::MRenderOperation*) new MHWRender::MPresentTarget(mOperationNames[3]);
  }

  if(!mOperations[0] || !mOperations[1] || !mOperations[2] || !mOperations[3])
    return MStatus::kFailure;
  return MStatus::kSuccess;
}

MStatus ViewOverrideSimple::cleanup() {
  mCurrentOperation = -1;
  return MStatus::kSuccess;
}

// *****************

simpleViewRenderSceneRender::simpleViewRenderSceneRender(const MString &name, const MString &viewportName)
  : MSceneRender( name )
{
  //try
  //{
  //  if(!mHostToRTRCallback.isValid()) {
  //    mHostToRTRCallback = FabricSplice::constructObjectRTVal("HostToRTRCallback");
  //    mHostToRTRCallback = mHostToRTRCallback.callMethod("HostToRTRCallback", "getOrCreateCallback", 0, 0);
  //  }
  //  FabricCore::RTVal viewportNameVal = FabricSplice::constructStringRTVal(viewportName.asChar());
  //  mViewport = mHostToRTRCallback.callMethod("BaseRTRViewport", "resetViewport", 1, &viewportNameVal);
  //}
  //catch (FabricCore::Exception e)
  //{
  //  mayaLogErrorFunc(e.getDesc_cstr());
  //  return;
  //}
}

MHWRender::MSceneRender::MSceneFilterOption simpleViewRenderSceneRender::renderFilterOverride() { 
  // Draw only non-shaded items return 
  return MHWRender::MSceneRender::kRenderAllItems; 
}

MHWRender::MClearOperation &simpleViewRenderSceneRender::clearOperation() {
  mClearOperation.setMask((unsigned int)(MHWRender::MClearOperation::kClearAll) );
  return mClearOperation;
}

void simpleViewRenderSceneRender::preSceneRender(const MHWRender::MDrawContext &context) {
  /*
  try
  {
    FabricCore::RTVal camera = mViewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
       
    MStatus status;
    MMatrix mayaMatrix = context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
    float *cameraMatFloats = (float*)cameraMatData.getData();
    if(cameraMat) 
    {
      cameraMatFloats[0]  = (float)mayaMatrix[0][0];
      cameraMatFloats[1]  = (float)mayaMatrix[1][0];
      cameraMatFloats[2]  = (float)mayaMatrix[2][0];
      cameraMatFloats[3]  = (float)mayaMatrix[3][0];
      cameraMatFloats[4]  = (float)mayaMatrix[0][1];
      cameraMatFloats[5]  = (float)mayaMatrix[1][1];
      cameraMatFloats[6]  = (float)mayaMatrix[2][1];
      cameraMatFloats[7]  = (float)mayaMatrix[3][1];
      cameraMatFloats[8]  = (float)mayaMatrix[0][2];
      cameraMatFloats[9]  = (float)mayaMatrix[1][2];
      cameraMatFloats[10] = (float)mayaMatrix[2][2];
      cameraMatFloats[11] = (float)mayaMatrix[3][2];
      cameraMatFloats[12] = (float)mayaMatrix[0][3];
      cameraMatFloats[13] = (float)mayaMatrix[1][3];
      cameraMatFloats[14] = (float)mayaMatrix[2][3];
      cameraMatFloats[15] = (float)mayaMatrix[3][3];
    }
    camera.callMethod("", "setTransform", 1, &cameraMat);


    MMatrix projectionMatrix = context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    projectionMatrix = projectionMatrix.transpose();
    FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
    float* projectionFloats = (float*)projectionData.getData();
    if(projectionMat) 
    {
      projectionFloats[0]  = (float)projectionMatrix[0][0];
      projectionFloats[1]  = (float)projectionMatrix[0][1];
      projectionFloats[2]  = (float)projectionMatrix[0][2];
      projectionFloats[3]  = (float)projectionMatrix[0][3];
      projectionFloats[4]  = (float)projectionMatrix[1][0];
      projectionFloats[5]  = (float)projectionMatrix[1][1];
      projectionFloats[6]  = (float)projectionMatrix[1][2];
      projectionFloats[7]  = (float)projectionMatrix[1][3];
      projectionFloats[8]  = (float)projectionMatrix[2][0];
      projectionFloats[9]  = (float)projectionMatrix[2][1];
      projectionFloats[10] = (float)projectionMatrix[2][2];
      projectionFloats[11] = (float)projectionMatrix[2][3];
      projectionFloats[12] = (float)projectionMatrix[3][0];
      projectionFloats[13] = (float)projectionMatrix[3][1];
      projectionFloats[14] = (float)projectionMatrix[3][2];
      projectionFloats[15] = (float)projectionMatrix[3][3];
    }
    camera.callMethod("", "setProjMatrix", 1, &projectionMat);

    FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(false);
    camera.callMethod("", "setOrthographic", 1, &param);
 

    //MHWRender::MFrameContext frameContext = context;
    MFnCamera mayaCamera(context.getCurrentCameraPath(&status));

    double fovX, fovY;
    int originX, originY, width, height;
    status = context.getViewportDimensions(originX, originY, width, height);
    mayaCamera.getPortFieldOfView(width, height, fovX, fovY);    
    param = FabricSplice::constructFloat64RTVal(fovY);
    camera.callMethod("", "setFovY", 1, &param);
 
    FabricCore::RTVal args[2] = {
      FabricSplice::constructFloat32RTVal(mayaCamera.nearClippingPlane()),
      FabricSplice::constructFloat32RTVal(mayaCamera.farClippingPlane())
    };
    camera.callMethod("", "setRange", 2, &args[0]);

    //FabricCore::RTVal parameters[5] = {
    //  FabricSplice::constructUInt32RTVal(2),
    //  FabricSplice::constructStringRTVal("modelPanel0"),
    //  FabricSplice::constructFloat64RTVal((double)width),
    //  FabricSplice::constructFloat64RTVal((double)height),
    //  FabricSplice::constructUInt32RTVal(2)
    //}; 
    //mHostToRTRCallback.callMethod("", "render", 5, &parameters[0]); 
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }  
  */
}

void simpleViewRenderSceneRender::postSceneRender(const MHWRender::MDrawContext &context) {
  /*
  try
  {
    int originX, originY, width, height;
    MStatus status = context.getViewportDimensions(originX, originY, width, height);
    FabricCore::RTVal parameters[5] = {
      FabricSplice::constructUInt32RTVal(2),
      FabricSplice::constructStringRTVal("modelPanel0"),
      FabricSplice::constructFloat64RTVal((double)width),
      FabricSplice::constructFloat64RTVal((double)height),
      FabricSplice::constructUInt32RTVal(2)
    }; 
    mHostToRTRCallback.callMethod("", "render", 5, &parameters[0]); 
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
  */
}

// *****************

RTRRender::RTRRender(const MString &name, const MString &viewportName)
  : MUserRenderOperation( name )
{
  try
  {
    if(!mHostToRTRCallback.isValid()) 
    {
      mHostToRTRCallback = FabricSplice::constructObjectRTVal("HostToRTRCallback");
      mHostToRTRCallback = mHostToRTRCallback.callMethod("HostToRTRCallback", "getOrCreateCallback", 0, 0);
    }
    FabricCore::RTVal viewportNameVal = FabricSplice::constructStringRTVal(viewportName.asChar());
    mViewport = mHostToRTRCallback.callMethod("BaseRTRViewport", "resetViewport", 1, &viewportNameVal);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}
 
MStatus RTRRender::execute(const MHWRender::MDrawContext &context) {
    
  try
  {
    FabricCore::RTVal camera = mViewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
   
    MStatus status;
    MFnCamera mayaCamera(context.getCurrentCameraPath(&status));

    MMatrix mayaMatrix = context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);

    FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
    float *cameraMatFloats = (float*)cameraMatData.getData();
    if(cameraMat) 
    {
      cameraMatFloats[0]  = (float)mayaMatrix[0][0];
      cameraMatFloats[1]  = (float)mayaMatrix[1][0];
      cameraMatFloats[2]  = (float)mayaMatrix[2][0];
      cameraMatFloats[3]  = (float)mayaMatrix[3][0];
      cameraMatFloats[4]  = (float)mayaMatrix[0][1];
      cameraMatFloats[5]  = (float)mayaMatrix[1][1];
      cameraMatFloats[6]  = (float)mayaMatrix[2][1];
      cameraMatFloats[7]  = (float)mayaMatrix[3][1];
      cameraMatFloats[8]  = (float)mayaMatrix[0][2];
      cameraMatFloats[9]  = (float)mayaMatrix[1][2];
      cameraMatFloats[10] = (float)mayaMatrix[2][2];
      cameraMatFloats[11] = (float)mayaMatrix[3][2];
      cameraMatFloats[12] = (float)mayaMatrix[0][3];
      cameraMatFloats[13] = (float)mayaMatrix[1][3];
      cameraMatFloats[14] = (float)mayaMatrix[2][3];
      cameraMatFloats[15] = (float)mayaMatrix[3][3];
    }
    camera.callMethod("", "setTransform", 1, &cameraMat);


    MMatrix projectionMatrix = context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    projectionMatrix = projectionMatrix.transpose();
    FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
    float* projectionFloats = (float*)projectionData.getData();
    if(projectionMat) 
    {
      projectionFloats[0]  = (float)projectionMatrix[0][0];
      projectionFloats[1]  = (float)projectionMatrix[0][1];
      projectionFloats[2]  = (float)projectionMatrix[0][2];
      projectionFloats[3]  = (float)projectionMatrix[0][3];
      projectionFloats[4]  = (float)projectionMatrix[1][0];
      projectionFloats[5]  = (float)projectionMatrix[1][1];
      projectionFloats[6]  = (float)projectionMatrix[1][2];
      projectionFloats[7]  = (float)projectionMatrix[1][3];
      projectionFloats[8]  = (float)projectionMatrix[2][0];
      projectionFloats[9]  = (float)projectionMatrix[2][1];
      projectionFloats[10] = (float)projectionMatrix[2][2];
      projectionFloats[11] = (float)projectionMatrix[2][3];
      projectionFloats[12] = (float)projectionMatrix[3][0];
      projectionFloats[13] = (float)projectionMatrix[3][1];
      projectionFloats[14] = (float)projectionMatrix[3][2];
      projectionFloats[15] = (float)projectionMatrix[3][3];
    }
    camera.callMethod("", "setProjMatrix", 1, &projectionMat);

    FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(false);
    camera.callMethod("", "setOrthographic", 1, &param);
 

    double fovX, fovY;
    int originX, originY, width, height;
    status = context.getViewportDimensions(originX, originY, width, height);
    mayaCamera.getPortFieldOfView(width, height, fovX, fovY);    
    param = FabricSplice::constructFloat64RTVal(fovY);
    camera.callMethod("", "setFovY", 1, &param);
 
    FabricCore::RTVal args[2] = {
      FabricSplice::constructFloat32RTVal(mayaCamera.nearClippingPlane()),
      FabricSplice::constructFloat32RTVal(mayaCamera.farClippingPlane())
    };
    camera.callMethod("", "setRange", 2, &args[0]);

    FabricCore::RTVal parameters[5] = {
      FabricSplice::constructUInt32RTVal(2),
      FabricSplice::constructStringRTVal("modelPanel0"),
      FabricSplice::constructFloat64RTVal((double)width),
      FabricSplice::constructFloat64RTVal((double)height),
      FabricSplice::constructUInt32RTVal(2)
    }; 
    mHostToRTRCallback.callMethod("", "render", 5, &parameters[0]); 
    return MStatus::kSuccess;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return MStatus::kFailure;
  }
  
  return MStatus::kSuccess;
}