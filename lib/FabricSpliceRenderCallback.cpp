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
#include "RTRViewport2.h"
#endif

bool FabricSpliceRenderCallback::gCallbackEnabled = true;
FabricCore::RTVal FabricSpliceRenderCallback::sDrawContext;
FabricCore::RTVal FabricSpliceRenderCallback::sRTROGLHostCallback;

bool FabricSpliceRenderCallback::isEnabled() {
  return FabricSpliceRenderCallback::gCallbackEnabled;
}

void FabricSpliceRenderCallback::enable(bool enable) {
  FabricSpliceRenderCallback::gCallbackEnabled = enable;
}
 
void FabricSpliceRenderCallback::disable() {
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 
  FabricSpliceRenderCallback::sRTROGLHostCallback.invalidate(); 
}
 
// **************

inline void init() {
 
  if(!FabricSpliceRenderCallback::sDrawContext.isValid()) {
    FabricSpliceRenderCallback::sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    FabricSpliceRenderCallback::sDrawContext = FabricSpliceRenderCallback::sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }
  else if(FabricSpliceRenderCallback::sDrawContext.isObject() && FabricSpliceRenderCallback::sDrawContext.isNullObject()) {
    FabricSpliceRenderCallback::sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    FabricSpliceRenderCallback::sDrawContext = FabricSpliceRenderCallback::sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
  }

  if(!FabricSpliceRenderCallback::sRTROGLHostCallback.isValid()) {
    FabricSpliceRenderCallback::sRTROGLHostCallback = FabricSplice::constructObjectRTVal("RTROGLHostCallback");
    FabricSpliceRenderCallback::sRTROGLHostCallback = FabricSpliceRenderCallback::sRTROGLHostCallback.callMethod("RTROGLHostCallback", "getOrCreateCallback", 0, 0);
  }
  else if(FabricSpliceRenderCallback::sRTROGLHostCallback.isObject() && FabricSpliceRenderCallback::sRTROGLHostCallback.isNullObject()) {
    FabricSpliceRenderCallback::sRTROGLHostCallback = FabricSplice::constructObjectRTVal("RTROGLHostCallback");
    FabricSpliceRenderCallback::sRTROGLHostCallback = FabricSpliceRenderCallback::sRTROGLHostCallback.callMethod("RTROGLHostCallback", "getOrCreateCallback", 0, 0);
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
  try
  {
    MDagPath mCameraDag;
    MStatus status = mCamera.getPath(mCameraDag);
    MMatrix mMatrix = mCameraDag.inclusiveMatrix();

    FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
    float *buffer = (float*)cameraMatData.getData();
    setMatrixTranspose(mMatrix, buffer);
    camera.callMethod("", "setTransform", 1, &cameraMat);

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
      double fovX, fovY;
      mCamera.getPortFieldOfView(width, height, fovX, fovY);    
      param = FabricSplice::constructFloat64RTVal(fovY);
      camera.callMethod("", "setFovY", 1, &param);
    }

    std::vector<FabricCore::RTVal> args(2);
    args[0] = FabricSplice::constructFloat32RTVal(mCamera.nearClippingPlane());
    args[1] = FabricSplice::constructFloat32RTVal(mCamera.farClippingPlane());
    camera.callMethod("", "setRange", 2, &args[0]);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

inline void setProjection(const MMatrix &projection, FabricCore::RTVal &camera) {
  try
  {
    FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
    FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
    float *buffer = (float*)projectionData.getData();
    setMatrixTranspose(projection, buffer);
    camera.callMethod("", "setProjMatrix", 1, &projectionMat);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

inline MString getActiveRenderName(const M3dView &view) {
  MString name = MHWRender::MRenderer::theRenderer()->activeRenderOverride();
  if(name.numChars() == 0) 
  {
    MStatus status;
    name = view.rendererString(&status);
  }
  MGlobal::displayError("renderName " + name);
  return name;
}

bool FabricSpliceRenderCallback::canDraw() {

  if(!FabricSpliceRenderCallback::gCallbackEnabled)
    return false;

  if(!FabricSplice::SceneManagement::hasRenderableContent() && FabricDFGBaseInterface::getNumInstances() == 0)
    return false;

  return true;
}

void FabricSpliceRenderCallback::draw(double width, double height, const MString &panelName, uint32_t phase) {
  try
  {
    // draw
    FabricSplice::Logging::AutoTimer globalTimer("Maya::DrawOpenGL()"); 
    FabricCore::RTVal args[5] = {
      FabricSplice::constructUInt32RTVal(phase),
      FabricSplice::constructStringRTVal(panelName.asChar()),
      FabricSplice::constructFloat64RTVal(width),
      FabricSplice::constructFloat64RTVal(height),
      FabricSplice::constructUInt32RTVal(2)
    };
    FabricSpliceRenderCallback::sRTROGLHostCallback.callMethod("", "render", 5, &args[0]);    
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

MString gRenderName = "NoViewport";

inline void preDrawCallback(const MString &panelName, void *clientData) {
  
  if(!FabricSpliceRenderCallback::canDraw()) return;

  try
  {
    init();

    M3dView view;
    M3dView::getM3dViewFromModelPanel(panelName, view);

#if _SPLICE_MAYA_VERSION >= 2016
  if(getActiveRenderName(view) == "vp2Renderer")
    return;
#endif

    MStatus status;
    FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(panelName.asChar());
    FabricCore::RTVal viewport;
    if(gRenderName != getActiveRenderName(view))
    {
      gRenderName = getActiveRenderName(view);
      viewport = FabricSpliceRenderCallback::sRTROGLHostCallback.callMethod("BaseRTRViewport", "resetViewport", 1, &panelNameVal);
    }
    else
      viewport = FabricSpliceRenderCallback::sRTROGLHostCallback.callMethod("BaseRTRViewport", "getOrAddViewport", 1, &panelNameVal);
    FabricCore::RTVal camera = viewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
    
    MDagPath cameraDag; view.getCamera(cameraDag);
    MFnCamera mCamera(cameraDag);
    setCamera(view.portWidth(), view.portHeight(), mCamera, camera);
    
    MMatrix projection;
    view.projectionMatrix(projection);
    setProjection(projection, camera);

    // draw
    FabricSpliceRenderCallback::draw(view.portWidth(), view.portHeight(), panelName, 2);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  } 

}

#if _SPLICE_MAYA_VERSION >= 2016
inline void preDrawCallback_2(MHWRender::MDrawContext &context, void* clientData) {
  MString panelName;
  MHWRender::MFrameContext::RenderingDestination destination = 
    context.renderingDestination(panelName);
  preDrawCallback(panelName, 0);
}
#endif;

inline void postDrawCallback(const MString &panelName, void *clientData) {
  if(!FabricSpliceRenderCallback::canDraw()) return;

  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

#if _SPLICE_MAYA_VERSION >= 2016
  if(getActiveRenderName(view) == "vp2Renderer" ||
     getActiveRenderName(view) == "RTRViewport2_name")
    return;
#endif

  //FabricSpliceRenderCallback::draw(view.portWidth(), view.portHeight(), panelName, 4);
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
  for(int p=0; p<5; ++p) 
  {
    MString panelName = MString("modelPanel");
    panelName += p;
    MUiMessage::add3dViewPreRenderMsgCallback(panelName, preDrawCallback, 0, 0);
    MUiMessage::add3dViewPostRenderMsgCallback(panelName, postDrawCallback, 0, 0);
  }

#if _SPLICE_MAYA_VERSION >= 2016
  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  if(renderer) 
  {
    RTRViewport2 *overridePtr = new RTRViewport2(RTRViewport2_name);
    if(overridePtr) renderer->registerOverride(overridePtr);
    renderer->addNotification(preDrawCallback_2, "PreDrawPass", MHWRender::MPassContext::kBeginSceneRenderSemantic, 0);
  }
#endif

}

void FabricSpliceRenderCallback::unplug() {
  MEventMessage::removeCallback(gOnPanelFocusCallbackId);
  for(unsigned int i=0; i<gCallbackCount; i++) 
  {
    MUiMessage::removeCallback(gPreDrawCallbacks[i]);
    MUiMessage::removeCallback(gPostDrawCallbacks[i]);
  }

#if _SPLICE_MAYA_VERSION >= 2016
  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  if(renderer)
  {
    renderer->removeNotification("PreDrawPass", MHWRender::MPassContext::kBeginSceneRenderSemantic);
    const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride(RTRViewport2_name);
    if(overridePtr)
    {
      renderer->deregisterOverride(overridePtr);
      delete overridePtr;
      overridePtr = 0;
    }
  }
#endif

}
