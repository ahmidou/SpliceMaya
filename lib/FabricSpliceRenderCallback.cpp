//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include <maya/MFrameContext.h>
#include <maya/MEventMessage.h>
#include <maya/MViewport2Renderer.h>
#if _SPLICE_MAYA_VERSION >= 2016
#include "Viewport2Override.h"
#endif

uint32_t gPanelId = 0;
bool gRTRPassEnabled = true;

bool FabricSpliceRenderCallback::gCallbackEnabled = true;
FabricCore::RTVal FabricSpliceRenderCallback::sDrawContext;

bool isRTRPassEnabled() {
  return gRTRPassEnabled;
}

void enableRTRPass(bool enable) {
  gRTRPassEnabled = enable;
}

bool FabricSpliceRenderCallback::isEnabled() {
  return FabricSpliceRenderCallback::gCallbackEnabled;
}

void FabricSpliceRenderCallback::enable(bool enable) {
  FabricSpliceRenderCallback::gCallbackEnabled = enable;
}
 
void FabricSpliceRenderCallback::disable() {
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 
}
 
// **************

inline bool canDraw() {
  if(!FabricSpliceRenderCallback::gCallbackEnabled)
    return false;
  if(!FabricSplice::SceneManagement::hasRenderableContent() && FabricDFGBaseInterface::getNumInstances() == 0)
    return false;
  return true;
}

inline void initID() {
 
  if(!FabricSpliceRenderCallback::sDrawContext.isValid()) {
    FabricSpliceRenderCallback::sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    FabricSpliceRenderCallback::sDrawContext = FabricSpliceRenderCallback::sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }
  else if(FabricSpliceRenderCallback::sDrawContext.isObject() && FabricSpliceRenderCallback::sDrawContext.isNullObject()) {
    FabricSpliceRenderCallback::sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    FabricSpliceRenderCallback::sDrawContext = FabricSpliceRenderCallback::sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }
}

inline void setMatrixTranspose(const MMatrix &mMatrix, float *buffer) {
  buffer[0]  = (float)mMatrix[0][0];  buffer[1]  = (float)mMatrix[1][0];
  buffer[2]  = (float)mMatrix[2][0];  buffer[3]  = (float)mMatrix[3][0];
  buffer[4]  = (float)mMatrix[0][1];  buffer[5]  = (float)mMatrix[1][1];
  buffer[6]  = (float)mMatrix[2][1];  buffer[7]  = (float)mMatrix[3][1];
  buffer[8]  = (float)mMatrix[0][2];  buffer[9]  = (float)mMatrix[1][2];
  buffer[10] = (float)mMatrix[2][2];  buffer[11] = (float)mMatrix[3][2];
  buffer[12] = (float)mMatrix[0][3];  buffer[13] = (float)mMatrix[1][3];
  buffer[14] = (float)mMatrix[2][3];  buffer[15] = (float)mMatrix[3][3];
}

inline void setCamera(double width, double height, const MFnCamera &mCamera, FabricCore::RTVal &camera) {
  MDagPath mCameraDag;
  MStatus status = mCamera.getPath(mCameraDag);
  (void)status;
  MMatrix mMatrix = mCameraDag.inclusiveMatrix();

  FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
  FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)cameraMatData.getData();
  setMatrixTranspose(mMatrix, buffer);

  camera.callMethod("", "setFromMat44", 1, &cameraMat);

  bool isOrthographic = mCamera.isOrtho();
  FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(isOrthographic);
  camera.callMethod("", "setOrthographic", 1, &param);

  if(isOrthographic) 
  {
    double windowAspect = width/height;
    double left = 0.0, right = 0.0, bottom = 0.0, top = 0.0;
    bool applyOverscan = false, applySqueeze = false, applyPanZoom = false;
    mCamera.getViewingFrustum ( windowAspect, left, right, bottom, top, applyOverscan, applySqueeze, applyPanZoom );
    param = FabricSplice::constructFloat32RTVal(top-bottom);
    camera.callMethod("", "setOrthographicFrustumHeight", 1, &param);
  }
  else 
  {
    int w = (int)width;
    int h = (int)height;
    double fovX = 0.0, fovY = 0.0;
    mCamera.getPortFieldOfView(w, h, fovX, fovY);    
    param = FabricSplice::constructFloat64RTVal(fovY);
    camera.callMethod("", "setFovY", 1, &param);
  }

  FabricCore::RTVal args[2] = {
    FabricSplice::constructFloat32RTVal(mCamera.nearClippingPlane()),
    FabricSplice::constructFloat32RTVal(mCamera.farClippingPlane())
  };
 
  camera.callMethod("", "setNearDistance", 1, &args[0]);
  camera.callMethod("", "setFarDistance", 1, &args[1]);
}

inline void setProjection(const MMatrix &projection, FabricCore::RTVal &camera) {
  FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
  FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)projectionData.getData();
  setMatrixTranspose(projection, buffer);
  camera.setMember("projection", projectionMat);
}

inline MString getActiveRenderName(const M3dView &view) {
  MString name = MHWRender::MRenderer::theRenderer()->activeRenderOverride();
  if(name.numChars() == 0) 
  {
    MStatus status;
    name = view.rendererString(&status);
  }
  //MGlobal::displayError("renderName " + name);
  return name;
}

inline void setupIDViewport(M3dView &view, const MString &panelName) {
 
  // sync the time
  FabricSpliceRenderCallback::sDrawContext.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));

  //////////////////////////
  // Setup the viewport
  FabricCore::RTVal viewport = FabricSpliceRenderCallback::sDrawContext.maybeGetMember("viewport");
  double width = view.portWidth();
  double height = view.portHeight();
  FabricCore::RTVal args[3] = {
    FabricSpliceRenderCallback::sDrawContext,
    FabricSplice::constructFloat64RTVal(width),
    FabricSplice::constructFloat64RTVal(height)
  };
  viewport.callMethod("", "resize", 3, &args[0]);

  FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(panelName.asChar());
  viewport.callMethod("", "setName", 1, &panelNameVal);
  FabricCore::RTVal camera = viewport.callMethod("InlineCamera", "getCamera", 0, 0);

  MDagPath cameraDag; view.getCamera(cameraDag);
  MFnCamera mCamera(cameraDag);
  setCamera(view.portWidth(), view.portHeight(), mCamera, camera);
  
  MMatrix projection;
  view.projectionMatrix(projection);
  setProjection(projection, camera);
}

void FabricSpliceRenderCallback::drawID() {
  try
  {
    FabricSplice::SceneManagement::drawOpenGL(sDrawContext);
  }
  catch(FabricCore::Exception e)
  {
    MString str("FabricSpliceRenderCallback::drawID: ");
    str += e.getDesc_cstr();
    mayaLogErrorFunc(str.asChar());
  } 
}

void FabricSpliceRenderCallback::preDrawCallback(const MString &panelName, void *clientData) {
  
  if(!canDraw()) return;
 
  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

   
  initID();
  setupIDViewport(view, panelName);
   
#if _SPLICE_MAYA_VERSION >= 2016
  if(getActiveRenderName(view) == "vp2Renderer")
    return;
#endif

}

#if _SPLICE_MAYA_VERSION >= 2016
void FabricSpliceRenderCallback::preDrawCallback_2(MHWRender::MDrawContext &context, void* clientData) {
  MString panelName;
  context.renderingDestination(panelName);
  FabricSpliceRenderCallback::preDrawCallback(panelName, 0);
}
#endif

void FabricSpliceRenderCallback::postDrawCallback(const MString &panelName, void *clientData) {
  if(!canDraw()) return;

  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

#if _SPLICE_MAYA_VERSION >= 2016
  if(
      getActiveRenderName(view) == "vp2Renderer" ||   
      getActiveRenderName(view) == "Viewport2Override"
    )
    return;
#endif

  drawID();
}

// **************

#define gCallbackCount 32
MCallbackId gOnPanelFocusCallbackId;
MCallbackId gPostDrawCallbacks[gCallbackCount];
MCallbackId gPreDrawCallbacks[gCallbackCount];
 
inline void onModelPanelSetFocus(void *client) {
  MString panelName;
  MGlobal::executeCommand("getPanel -wf;", panelName, false);
  //for(int p=0; p<gCallbackCount; ++p) 
  //{
  //  MString modelPanel = MString("modelPanel");
  //  modelPanel += p;
  //  if(panelName == modelPanel) 
  //    addViewport(p, panelName);
  //}
  MGlobal::executeCommandOnIdle("refresh;", false);
}

void FabricSpliceRenderCallback::plug() {
  gOnPanelFocusCallbackId = MEventMessage::addEventCallback("ModelPanelSetFocus", &onModelPanelSetFocus);
  
  MStatus status;
  for(int p=0; p<5; ++p) 
  {
    MString panelName = MString("modelPanel");
    panelName += p;
    gPreDrawCallbacks[p] = MUiMessage::add3dViewPreRenderMsgCallback(panelName, preDrawCallback, 0, &status);
    gPostDrawCallbacks[p] = MUiMessage::add3dViewPostRenderMsgCallback(panelName, postDrawCallback, 0, &status);
  }

  #if _SPLICE_MAYA_VERSION >= 2016
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if(renderer) 
    {
      Viewport2Override *overridePtr = new Viewport2Override("Viewport2Override");
      if(overridePtr) renderer->registerOverride(overridePtr);
      renderer->addNotification(preDrawCallback_2, "PreDrawPass", MHWRender::MPassContext::kBeginSceneRenderSemantic, 0);
    }
  #endif

}

void FabricSpliceRenderCallback::unplug() {
  MEventMessage::removeCallback(gOnPanelFocusCallbackId);
  for(int i=0; i<gCallbackCount; i++) 
  {
    MUiMessage::removeCallback(gPreDrawCallbacks[i]);
    MUiMessage::removeCallback(gPostDrawCallbacks[i]);
  }

  #if _SPLICE_MAYA_VERSION >= 2016
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if(renderer)
    {
      renderer->removeNotification("PreDrawPass", MHWRender::MPassContext::kBeginSceneRenderSemantic);
      const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride("Viewport2Override");
      if(overridePtr)
      {
        renderer->deregisterOverride(overridePtr);
        delete overridePtr;
        overridePtr = 0;
      }
    }
  #endif
}
