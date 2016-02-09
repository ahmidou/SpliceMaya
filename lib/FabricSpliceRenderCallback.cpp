#include "FabricDFGBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MAnimControl.h>

bool gMayaToRTRCallbackEnabled = true;
FabricCore::RTVal FabricSpliceRenderCallback::sDrawContext;
FabricCore::RTVal FabricSpliceRenderCallback::sMayaToRTRCallback;

bool isMayaToRTRCallbackEnabled() {
  return gMayaToRTRCallbackEnabled;
}

void enableMayaToRTRCallback(bool enable) {
  gMayaToRTRCallbackEnabled = enable;
}

void FabricSpliceRenderCallback::invalidateMayaToRTRCallback() {
  sMayaToRTRCallback.invalidate(); 
  sDrawContext.invalidate(); 
}

void FabricSpliceRenderCallback::setCameraTranformFromMaya(M3dView &view, FabricCore::RTVal &camera) {

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

void FabricSpliceRenderCallback::setCameraProjectionFromMaya(M3dView &view, FabricCore::RTVal &camera) {

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

void FabricSpliceRenderCallback::setCameraParamtersFromMaya(M3dView &view, FabricCore::RTVal &camera) {

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
 
FabricCore::RTVal &FabricSpliceRenderCallback::getMayaToRTRCallback(const MString &str, M3dView &view) {
  // sync the time
  if(!sMayaToRTRCallback.isValid()) {
    sMayaToRTRCallback = FabricSplice::constructObjectRTVal("MayaToRTRCallback");
    sMayaToRTRCallback = sMayaToRTRCallback.callMethod("MayaToRTRCallback", "getOrCreateCallback", 0, 0);
  }
  else if(sMayaToRTRCallback.isObject() && sMayaToRTRCallback.isNullObject()) {
    sMayaToRTRCallback = FabricSplice::constructObjectRTVal("MayaToRTRCallback");
    sMayaToRTRCallback = sMayaToRTRCallback.callMethod("MayaToRTRCallback", "getOrCreateCallback", 0, 0);
  }

  if(!sDrawContext.isValid()) {
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    sDrawContext = sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }
  else if(sDrawContext.isObject() && sDrawContext.isNullObject()) {
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    sDrawContext = sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }

  return sMayaToRTRCallback;
}
 
void FabricSpliceRenderCallback::preRenderCallback(const MString &str, void *clientData) {

  if(!gMayaToRTRCallbackEnabled)
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
  //view.beginGL();

  try
  {
    //FabricSplice::Logging::AutoTimer globalTimer("Maya::DrawOpenGL()");

    FabricCore::RTVal callback = getMayaToRTRCallback(str, view);

    //////////////////////////
    // Setup the viewport
    FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(str.asChar());
    FabricCore::RTVal rtrViewport = callback.callMethod("BaseRTRViewport", "getOrAddViewport", 1, &panelNameVal);

    callback.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));
    
    FabricCore::RTVal camera = rtrViewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
    FabricSpliceRenderCallback::setCameraTranformFromMaya(view, camera);
    FabricSpliceRenderCallback::setCameraProjectionFromMaya(view, camera);
    FabricSpliceRenderCallback::setCameraParamtersFromMaya(view, camera);

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
}

void FabricSpliceRenderCallback::postRenderCallback(const MString &str, void *clientData) {

  if(!gMayaToRTRCallbackEnabled)
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

  // draw
  view.beginGL();
  try
  {
    FabricSplice::Logging::AutoTimer globalTimer("Maya::DrawOpenGL()");

    FabricCore::RTVal callback = getMayaToRTRCallback(str, view);
 
    std::vector<FabricCore::RTVal> args(4);
    args[0] = FabricSplice::constructStringRTVal(str.asChar());
    args[1] = FabricSplice::constructFloat64RTVal(view.portWidth());
    args[2] = FabricSplice::constructFloat64RTVal(view.portHeight());
    args[3] = FabricSplice::constructUInt32RTVal(2);
    callback.callMethod("", "render", 4, &args[0]);    
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
