//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include "FabricImportPatternDialog.h"
#include <maya/MFrameContext.h>
#include <maya/MDrawContext.h>
#include <maya/MEventMessage.h>
#include <maya/MViewport2Renderer.h>
#if MAYA_API_VERSION >= 201600
#include "Viewport2Override.h"
#endif

uint32_t FabricSpliceRenderCallback::gPanelId = 0;
bool gRTRPassEnabled = true;
bool FabricSpliceRenderCallback::gCallbackEnabled = true;
FabricCore::RTVal FabricSpliceRenderCallback::sDrawContext;
FabricUI::SceneHub::SHGLRenderer FabricSpliceRenderCallback::shHostGLRenderer;

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

bool FabricSpliceRenderCallback::canDraw() {
  if(!FabricSpliceRenderCallback::gCallbackEnabled)
    return false;
  if(!FabricSplice::SceneManagement::hasRenderableContent() && 
    FabricDFGBaseInterface::getNumInstances() == 0 &&
    !FabricImportPatternDialog::isPreviewRenderingEnabled())
    return false;
  return gRTRPassEnabled;
}

inline MString getActiveRenderName() {
  MString name = MHWRender::MRenderer::theRenderer()->activeRenderOverride();
  //MGlobal::displayError("renderName 1 " + name);
  return name;
}

inline MString getActiveRenderName(const M3dView &view) {
  MString name = getActiveRenderName();
  if(name.numChars() == 0) 
  {
    MStatus status;
    name = view.rendererString(&status);
  }
  //MGlobal::displayError("renderName 2" + name);
  return name;
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

bool FabricSpliceRenderCallback::isRTR2Enable() {
#ifndef FABRIC_SCENEHUB
  return false;
#endif

  if(!canDraw()) 
    return false;
  
  if(!shHostGLRenderer.getSHGLRenderer().isValid())
  {
    const FabricCore::Client *client = 0;
    FECS_DGGraph_getClient(&client);
    if(!client) 
      return false;
    else
      shHostGLRenderer.setClient(*client);
   
    FabricCore::RTVal host = FabricSplice::constructObjectRTVal("SHGLHostRenderer");
    FabricCore::RTVal isValidVal = FabricSplice::constructBooleanRTVal(false);
    host = host.callMethod("SHGLHostRenderer", "get", 1, &isValidVal);
    if(!isValidVal.getBoolean()) 
      return false;
    else
      shHostGLRenderer.setSHGLRenderer(host);
  }
 
  return shHostGLRenderer.getSHGLRenderer().isValid();
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

inline void setCamera(bool id, double width, double height, const MFnCamera &mCamera, FabricCore::RTVal &camera) {
  MDagPath mCameraDag;
  MStatus status = mCamera.getPath(mCameraDag);
  (void)status;
  MMatrix mMatrix = mCameraDag.inclusiveMatrix();

  FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
  FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)cameraMatData.getData();
  setMatrixTranspose(mMatrix, buffer);
  if(!id) camera.callMethod("", "setTransform", 1, &cameraMat);
  else camera.callMethod("", "setFromMat44", 1, &cameraMat);

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
  if(!id) camera.callMethod("", "setRange", 2, &args[0]);
  else
  {
    camera.callMethod("", "setNearDistance", 1, &args[0]);
    camera.callMethod("", "setFarDistance", 1, &args[1]);
  }
}

inline void setProjection(bool id, const MMatrix &projection, FabricCore::RTVal &camera) {
  FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
  FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)projectionData.getData();
  setMatrixTranspose(projection, buffer);
  if(!id) camera.callMethod("", "setProjMatrix", 1, &projectionMat);
  else camera.setMember("projection", projectionMat);
}

inline void setupIDViewport(
  const MString &panelName, 
  double width, 
  double height, 
  const MFnCamera &mCamera,
  const MMatrix &projection) 
{
  if(!FabricSpliceRenderCallback::canDraw()) return;

  initID();

  FabricSpliceRenderCallback::sDrawContext.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));

  FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(panelName.asChar());
  FabricCore::RTVal viewport = FabricSpliceRenderCallback::sDrawContext.maybeGetMember("viewport");
  viewport.callMethod("", "setName", 1, &panelNameVal);
  FabricCore::RTVal args[3] = {
    FabricSpliceRenderCallback::sDrawContext,
    FabricSplice::constructFloat64RTVal(width),
    FabricSplice::constructFloat64RTVal(height)
  };
  viewport.callMethod("", "resize", 3, &args[0]);
 
  FabricCore::RTVal camera = viewport.callMethod("InlineCamera", "getCamera", 0, 0);
  setCamera(true, width, height, mCamera, camera);
  setProjection(true, projection, camera);
}

MString gRenderName = "NoViewport";
inline void setupRTR2Viewport(
  const MString &renderName, 
  const MString &panelName, 
  double width, 
  double height, 
  const MFnCamera &mCamera,
  const MMatrix &projection) 
{ 
  if(!FabricSpliceRenderCallback::canDraw()) return;

  if(!FabricSpliceRenderCallback::isRTR2Enable()) return;

  FabricSpliceRenderCallback::gPanelId = panelName.substringW(panelName.length()-2, panelName.length()-1).asInt();
  if(gRenderName != renderName)
  {
    gRenderName = renderName;
    FabricSpliceRenderCallback::shHostGLRenderer.removeViewport(FabricSpliceRenderCallback::gPanelId);
  }

  FabricCore::RTVal viewport = FabricSpliceRenderCallback::shHostGLRenderer.getOrAddViewport(FabricSpliceRenderCallback::gPanelId);
  FabricCore::RTVal camera = viewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
  setCamera(false, width, height, mCamera, camera);
  setProjection(false, projection, camera);
}

void FabricSpliceRenderCallback::drawID() {
  if(!canDraw()) return;

  try
  {
    FabricSplice::SceneManagement::drawOpenGL(sDrawContext);
// In maya 2015, there is a bug where the normals are inverted after using the rectangular selection.
// In order to fix it, we force Maya to refresh.
// Now, it just glitchs.
#if MAYA_API_VERSION == 201500
  MGlobal::executeCommandOnIdle("refresh;", false);
#endif
  }
  catch(FabricCore::Exception e)
  {
    MString str("FabricSpliceRenderCallback::drawID: ");
    str += e.getDesc_cstr();
    mayaLogErrorFunc(str.asChar());
  } 
}

MStatus FabricSpliceRenderCallback::drawRTR2(uint32_t width, uint32_t height, uint32_t phase) {
  MStatus status = MStatus::kSuccess;

  if(!canDraw()) return status;

  if(isRTR2Enable())
  {
    FabricSpliceRenderCallback::shHostGLRenderer.render(
      gPanelId,
      width,
      height,
      2,
      phase);
      
// In maya 2015, there is a bug where the normals are inverted after using the rectangular selection.
// In order to fix it, we force Maya to refresh.
// Now, it just glitchs.
#if MAYA_API_VERSION == 201500
  if(phase > 2)    
    MGlobal::executeCommandOnIdle("refresh;", false);
#endif
   
  }
  return status;
}

void FabricSpliceRenderCallback::preDrawCallback(const MString &panelName, void *clientData) {
  
  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);
  MString renderName = getActiveRenderName(view);

// In Maya >= 2016, we deactive the maya viewport 2.0
#if MAYA_API_VERSION >= 201600
  if(renderName == "vp2Renderer")
    return;
#endif

  MDagPath cameraDag; 
  view.getCamera(cameraDag);
  MFnCamera mCamera(cameraDag);
   
  MMatrix projection; 
  view.projectionMatrix(projection);
  
  if(!isRTR2Enable())
  {
    setupIDViewport(panelName, view.portWidth(), view.portHeight(), mCamera, projection);
    //drawID();
  }
  else
  {
    setupRTR2Viewport(renderName, panelName, view.portWidth(), view.portHeight(), mCamera, projection);
    drawRTR2(uint32_t(view.portWidth()), uint32_t(view.portHeight()), 2);
  }
}

// Special preDraw Callback for Viewport2Override in Maya >= 2016
#if MAYA_API_VERSION >= 201600
void FabricSpliceRenderCallback::viewport2OverridePreDrawCallback(MHWRender::MDrawContext &context, void* clientData) {

  MString panelName;
  context.renderingDestination(panelName);

  MString renderName = getActiveRenderName();
  if(renderName != "Viewport2Override")
    return;

  int oriX, oriY, width, height;
  MStatus status = context.getViewportDimensions(oriX, oriY, width, height);

  MDagPath cameraDag = context.getCurrentCameraPath(&status);
  MFnCamera mCamera(cameraDag);
  MMatrix projection = context.getMatrix(MDrawContext::kProjectionMtx, &status);

  if(!isRTR2Enable())
  {
    setupIDViewport(panelName, (double)width, (double)height, mCamera, projection);
    //drawID();
  }
  else
  {
    setupRTR2Viewport(renderName, panelName, (double)width, (double)height, mCamera, projection);
    drawRTR2(width, height, 2);
  }
}
#endif

void FabricSpliceRenderCallback::postDrawCallback(const MString &panelName, void *clientData) {

  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

// In Maya >= 2016, we deactive the maya viewport 2.0
// And the postDraw Callback is done within the override, cf Viewport2Override class.
#if MAYA_API_VERSION >= 201600
  if(
      getActiveRenderName(view) == "vp2Renderer" ||   
      getActiveRenderName(view) == "Viewport2Override"
    )
    return;
#endif

  if(isRTR2Enable()) 
  {
    uint32_t drawPhase = (getActiveRenderName(view) == "vp2Renderer") ? 3 : 4;
    drawRTR2(uint32_t(view.portWidth()), uint32_t(view.portHeight()), drawPhase);
  }
  else
    drawID();
}

// **************

#define gCallbackCount 32
MCallbackId gOnPanelFocusCallbackId;
MCallbackId gPostDrawCallbacks[gCallbackCount];
MCallbackId gPreDrawCallbacks[gCallbackCount];
 
inline void onModelPanelSetFocus(void *client) {
  MString panelName;
  MGlobal::executeCommandOnIdle("refresh;", false);
}

void FabricSpliceRenderCallback::plug() {
  
  gOnPanelFocusCallbackId = MEventMessage::addEventCallback("ModelPanelSetFocus", &onModelPanelSetFocus);
  
  MStatus status;
  for(int p=0; p<gCallbackCount; ++p) 
  {
    MString panelName = MString("modelPanel");
    panelName += p;
    gPreDrawCallbacks[p] = MUiMessage::add3dViewPreRenderMsgCallback(panelName, preDrawCallback, 0, &status);
    gPostDrawCallbacks[p] = MUiMessage::add3dViewPostRenderMsgCallback(panelName, postDrawCallback, 0, &status);
  }

// In Maya >= 2016, we override the viewport 2.0
// Create a pretDraw Callback, the postDraw Callback is done 
// within the override, cf Viewport2Override class.
#if MAYA_API_VERSION >= 201600
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if(renderer) 
    {
      Viewport2Override *overridePtr = new Viewport2Override("Viewport2Override");
      if(overridePtr) renderer->registerOverride(overridePtr);
      renderer->addNotification(viewport2OverridePreDrawCallback, "Viewport2OverridePreDrawPass", MHWRender::MPassContext::kBeginSceneRenderSemantic, 0);
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

#if MAYA_API_VERSION >= 201600
    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if(renderer)
    {
      renderer->removeNotification("Viewport2OverridePreDrawPass", MHWRender::MPassContext::kBeginSceneRenderSemantic);
      const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride(Viewport2Override_name);
      if(overridePtr)
      {
        renderer->deregisterOverride(overridePtr);
        delete overridePtr;
        overridePtr = 0;
      }
    }
#endif
}
