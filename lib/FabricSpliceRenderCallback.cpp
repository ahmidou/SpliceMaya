//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricDFGBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceToolContext.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MAnimControl.h>

bool gRTRPassEnabled = true;

FabricCore::RTVal FabricSpliceRenderCallback::sDrawContext;

bool isRTRPassEnabled()
{
  return gRTRPassEnabled;
}

void enableRTRPass(bool enable)
{
  gRTRPassEnabled = enable;
}

FabricCore::RTVal & FabricSpliceRenderCallback::getDrawContext(const MString &str, M3dView & view)
{
  if(!sDrawContext.isValid()) {
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    sDrawContext = sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }
  else if(sDrawContext.isObject() && sDrawContext.isNullObject()) {
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    sDrawContext = sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }

  // sync the time
  sDrawContext.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));

  //////////////////////////
  // Setup the viewport
  FabricCore::RTVal inlineViewport = sDrawContext.maybeGetMember("viewport");
  double width = view.portWidth();
  double height = view.portHeight();
  std::vector<FabricCore::RTVal> args(3);
  args[0] = sDrawContext;
  args[1] = FabricSplice::constructFloat64RTVal(width);
  args[2] = FabricSplice::constructFloat64RTVal(height);
  inlineViewport.callMethod("", "resize", 3, &args[0]);

  FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(str.asChar());
  inlineViewport.callMethod("", "setName", 1, &panelNameVal);

  {
    FabricCore::RTVal inlineCamera = inlineViewport.callMethod("InlineCamera", "getCamera", 0, 0);

    MDagPath cameraDag;
    view.getCamera(cameraDag);
    MFnCamera camera(cameraDag);

    MMatrix projectionMatrix;
    view.projectionMatrix(projectionMatrix);
    projectionMatrix = projectionMatrix.transpose();

    FabricCore::RTVal projectionMatrixExtArray = FabricSplice::constructExternalArrayRTVal("Float64", 16, &projectionMatrix.matrix);
    FabricCore::RTVal projectionVal = inlineCamera.maybeGetMember("projection");
    projectionVal.callMethod("", "set", 1, &projectionMatrixExtArray);
    inlineCamera.setMember("projection", projectionVal);

    MMatrix mayaCameraMatrix = cameraDag.inclusiveMatrix();
    FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");

    try
    {
      FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
      float * cameraMatFloats = (float*)cameraMatData.getData();
      if(cameraMat) {
        cameraMatFloats[0] = (float)mayaCameraMatrix[0][0];
        cameraMatFloats[1] = (float)mayaCameraMatrix[1][0];
        cameraMatFloats[2] = (float)mayaCameraMatrix[2][0];
        cameraMatFloats[3] = (float)mayaCameraMatrix[3][0];
        cameraMatFloats[4] = (float)mayaCameraMatrix[0][1];
        cameraMatFloats[5] = (float)mayaCameraMatrix[1][1];
        cameraMatFloats[6] = (float)mayaCameraMatrix[2][1];
        cameraMatFloats[7] = (float)mayaCameraMatrix[3][1];
        cameraMatFloats[8] = (float)mayaCameraMatrix[0][2];
        cameraMatFloats[9] = (float)mayaCameraMatrix[1][2];
        cameraMatFloats[10] = (float)mayaCameraMatrix[2][2];
        cameraMatFloats[11] = (float)mayaCameraMatrix[3][2];
        cameraMatFloats[12] = (float)mayaCameraMatrix[0][3];
        cameraMatFloats[13] = (float)mayaCameraMatrix[1][3];
        cameraMatFloats[14] = (float)mayaCameraMatrix[2][3];
        cameraMatFloats[15] = (float)mayaCameraMatrix[3][3];

        inlineCamera.callMethod("", "setFromMat44", 1, &cameraMat);
      }

      bool isOrthographic = camera.isOrtho();
      FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(isOrthographic);
      inlineCamera.callMethod("", "setOrthographic", 1, &param);

      if(isOrthographic){
        double windowAspect = width/height;
        double left = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        double top = 0.0;
        bool  applyOverscan = 0.0;
        bool  applySqueeze = 0.0;
        bool  applyPanZoom = 0.0;
        camera.getViewingFrustum ( windowAspect, left, right, bottom, top, applyOverscan, applySqueeze, applyPanZoom );
        param = FabricSplice::constructFloat64RTVal(top-bottom);
        inlineCamera.callMethod("", "setOrthographicFrustumHeight", 1, &param);
      }
      else{
        double fovX, fovY;
        camera.getPortFieldOfView(view.portWidth(), view.portHeight(), fovX, fovY);    
        param = FabricSplice::constructFloat64RTVal(fovY);
        inlineCamera.callMethod("", "setFovY", 1, &param);
      }

      param = FabricSplice::constructFloat64RTVal(camera.nearClippingPlane());
      inlineCamera.callMethod("", "setNearDistance", 1, &param);
      param = FabricSplice::constructFloat64RTVal(camera.farClippingPlane());
      inlineCamera.callMethod("", "setFarDistance", 1, &param);
    }
    catch (FabricCore::Exception e)
    {
      mayaLogErrorFunc(e.getDesc_cstr());
    }
  }
  
  return sDrawContext;
}

void FabricSpliceRenderCallback::draw(const MString &str, void *clientData){

  if(!gRTRPassEnabled)
    return;

  try
  {
    if(!FabricSplice::SceneManagement::hasRenderableContent() && FabricDFGBaseInterface::getNumInstances() == 0)
      return;
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(e.what());
  }

  M3dView view;
  M3dView::getM3dViewFromModelPanel(str, view);
  view.beginGL();

  // draw all gizmos
  try
  {
    FabricSplice::Logging::AutoTimer globalTimer("Maya::DrawOpenGL()");
    FabricSplice::SceneManagement::drawOpenGL(getDrawContext(str, view));
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(e.what());
    return;
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }

  view.endGL();
}
