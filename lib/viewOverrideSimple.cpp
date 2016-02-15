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

// For override creation we return a UI name so that it shows up in as a
// renderer in the 3d viewport menus.
ViewOverrideSimple::ViewOverrideSimple(const MString &clientContextID, const MString &name, const MString &viewportName )
: MRenderOverride( name )
, mViewportName( viewportName )
, mUIName("Simple VP2 Override")
, mCurrentOperation(-1)
{
  MGlobal::displayError( clientContextID );
  mOperations[0] = mOperations[1] = mOperations[2] = NULL;
  mOperationNames[0] = "ViewOverrideSimple_Scene";
  mOperationNames[1] = "ViewOverrideUSER_HUD";
  mOperationNames[2] = "ViewOverrideSimple_HUD";
}

// On destruction all operations are deleted.
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
  
// Drawing uses all internal code so will support all draw APIs 
MHWRender::DrawAPI ViewOverrideSimple::supportedDrawAPIs() const {
  return MHWRender::kAllDevices;
}

// Basic iterator methods which returns a list of operations in order
// The operations are not executed at this time only queued for execution
// - startOperationIterator() : to start iterating
// - renderOperation() : will be called to return the current operation
// - nextRenderOperation() : when this returns false we've returned all operations
bool ViewOverrideSimple::startOperationIterator() {
  mCurrentOperation = 0;
  return true;
}

MHWRender::MRenderOperation* ViewOverrideSimple::renderOperation() {
  if (mCurrentOperation >= 0 && mCurrentOperation < 3)
  {
    if (mOperations[mCurrentOperation])
      return mOperations[mCurrentOperation];
  }
  return NULL;
}

bool ViewOverrideSimple::nextRenderOperation() {
  mCurrentOperation++;
  return (mCurrentOperation < 3);
}

// On setup we make sure that we have created the appropriate operations
// These will be returned via the iteration code above.
// The only thing that is required here is to create:
//  - One scene render operation to draw the scene.
//  - One "stock" HUD render operation to draw the HUD over the scene
//  - One "stock" presentation operation to be able to see the results in the viewport
MStatus ViewOverrideSimple::setup( const MString & destination ) {
  MHWRender::MRenderer *theRenderer = MHWRender::MRenderer::theRenderer();
  if (!theRenderer)
    return MStatus::kFailure;

  // Create a new set of operations as required
  if (!mOperations[0])
  {
    mOperations[0] = (MHWRender::MRenderOperation *) new simpleViewRenderSceneRender( mOperationNames[0], mViewportName );
    mOperations[1] = (MHWRender::MRenderOperation *) new MHWRender::MHUDRender();
    mOperations[2] = (MHWRender::MRenderOperation *) new MHWRender::MPresentTarget( mOperationNames[2] );
  }

  if (!mOperations[0] || !mOperations[1] || !mOperations[2])
    return MStatus::kFailure;
  return MStatus::kSuccess;
}

  
// On cleanup we just return the list of operations for the next render
MStatus ViewOverrideSimple::cleanup() {
  mCurrentOperation = -1;
  return MStatus::kSuccess;
}
// The only customization for the scene render (and hence derivation)
// is to be able to set the background color.
simpleViewRenderSceneRender::simpleViewRenderSceneRender(const MString &name, const MString &viewportName)
: MSceneRender( name )
{
  try
  {
    if(!mHostToRTRCallback.isValid()) {
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

// Background color override. We get the current colors from the 
// renderer and use them
MHWRender::MClearOperation &simpleViewRenderSceneRender::clearOperation() {
  mClearOperation.setMask((unsigned int)(MHWRender::MClearOperation::kClearAll) );
  return mClearOperation;
}

inline void cameraTranformFromMaya(M3dView &view, FabricCore::RTVal &camera/*, FabricCore::RTVal &camera*/) {

  try
  {
    MDagPath mayaCameraDag;
    view.getCamera(mayaCameraDag);

    MMatrix mayaCameraMatrix = mayaCameraDag.inclusiveMatrix();
    FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");

    FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
    float * cameraMatFloats = (float*)cameraMatData.getData();
    if(cameraMat) {
      cameraMatFloats[0]  = (float)mayaCameraMatrix[0][0];
      cameraMatFloats[1]  = (float)mayaCameraMatrix[1][0];
      cameraMatFloats[2]  = (float)mayaCameraMatrix[2][0];
      cameraMatFloats[3]  = (float)mayaCameraMatrix[3][0];
      cameraMatFloats[4]  = (float)mayaCameraMatrix[0][1];
      cameraMatFloats[5]  = (float)mayaCameraMatrix[1][1];
      cameraMatFloats[6]  = (float)mayaCameraMatrix[2][1];
      cameraMatFloats[7]  = (float)mayaCameraMatrix[3][1];
      cameraMatFloats[8]  = (float)mayaCameraMatrix[0][2];
      cameraMatFloats[9]  = (float)mayaCameraMatrix[1][2];
      cameraMatFloats[10] = (float)mayaCameraMatrix[2][2];
      cameraMatFloats[11] = (float)mayaCameraMatrix[3][2];
      cameraMatFloats[12] = (float)mayaCameraMatrix[0][3];
      cameraMatFloats[13] = (float)mayaCameraMatrix[1][3];
      cameraMatFloats[14] = (float)mayaCameraMatrix[2][3];
      cameraMatFloats[15] = (float)mayaCameraMatrix[3][3];
    }

    camera.callMethod("", "setTransform", 1, &cameraMat);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

inline void cameraProjectionFromMaya(M3dView &view, FabricCore::RTVal &camera/*, FabricCore::RTVal &camera*/) {

  try
  {
    MMatrix projectionMatrix;
    view.projectionMatrix(projectionMatrix);
    projectionMatrix = projectionMatrix.transpose();

    FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
    float * projectionFloats = (float*)projectionData.getData();
    if(projectionMat) {
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
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

inline void cameraParamtersFromMaya(M3dView &view, FabricCore::RTVal &camera/*, FabricCore::RTVal &camera*/) {

  try
  {
    MDagPath mayaCameraDag;
    view.getCamera(mayaCameraDag);

    MFnCamera mayaCamera(mayaCameraDag);
    bool isOrthographic = mayaCamera.isOrtho();
    FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(isOrthographic);
    camera.callMethod("", "setOrthographic", 1, &param);

    if(isOrthographic) 
    {
      double windowAspect = view.portWidth()/view.portHeight();
      double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0;
      bool applyOverscan = false, applySqueeze = false, applyPanZoom = false;
      mayaCamera.getViewingFrustum ( windowAspect, left, right, bottom, top, applyOverscan, applySqueeze, applyPanZoom );
      param = FabricSplice::constructFloat32RTVal(top-bottom);
      camera.callMethod("", "setOrthographicFrustumHeight", 1, &param);
    }
    else 
    {
      double fovX, fovY;
      mayaCamera.getPortFieldOfView(view.portWidth(), view.portHeight(), fovX, fovY);    
      param = FabricSplice::constructFloat64RTVal(fovY);
      camera.callMethod("", "setFovY", 1, &param);
    }

    std::vector<FabricCore::RTVal> args(2);
    args[0] = FabricSplice::constructFloat32RTVal(mayaCamera.nearClippingPlane());
    args[1] = FabricSplice::constructFloat32RTVal(mayaCamera.farClippingPlane());
    camera.callMethod("", "setRange", 2, &args[0]);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

// Background color override. We get the current colors from the 
// renderer and use them
void simpleViewRenderSceneRender::preSceneRender(const MHWRender::MDrawContext &context) {
  
  try
  {

    FabricCore::RTVal camera = mViewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
       
    MStatus status;
    MMatrix mayaMatrix = context.getMatrix(MHWRender::MFrameContext::kWorldMtx, &status);
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

    //MBoundingBox box = context.getFrustumBox(&status);
    //param = FabricSplice::constructFloat32RTVal((float)box.height());
    //camera.callMethod("", "setOrthographicFrustumHeight", 1, &param);       

    // float fov = 2 * actan (frame size/(focal length * 2))

    //MHWRender::MFrameContext frameContext = context;
    MFnCamera mayaCamera(context.getCurrentCameraPath(&status));

    //double fovX, fovY;
    //int originX, originY, width, height;
    //status = context.getViewportDimensions(originX, originY, width, height);
    //mayaCamera.getPortFieldOfView(width, height, fovX, fovY);    
    //param = FabricSplice::constructFloat64RTVal(fovY);
    //camera.callMethod("", "setFovY", 1, &param);
 
    //std::vector<FabricCore::RTVal> args(2);
    //args[0] = FabricSplice::constructFloat32RTVal(mayaCamera.nearClippingPlane());
    //args[1] = FabricSplice::constructFloat32RTVal(mayaCamera.farClippingPlane());
    //camera.callMethod("", "setRange", 2, &args[0]);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

void simpleViewRenderSceneRender::postSceneRender(const MHWRender::MDrawContext &context) {

  int originX, originY, width, height;
  context.getViewportDimensions(originX, originY, width, height);
  FabricCore::RTVal args[5] = 
  {
    FabricSplice::constructUInt32RTVal(2),
    FabricSplice::constructStringRTVal("modelPanel0"),
    FabricSplice::constructFloat64RTVal((double)width),
    FabricSplice::constructFloat64RTVal((double)height),
    FabricSplice::constructUInt32RTVal(2)
  }; 
  mHostToRTRCallback.callMethod("", "render", 5, &args[0]); 
}
 