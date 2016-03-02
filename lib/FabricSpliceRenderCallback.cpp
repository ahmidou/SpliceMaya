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


/// *************** InlineDrawing ***************/
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



/// *************** RTR2 ***************/
inline void setMatrixTranspose(const MMatrix &mMatrix, float *buffer) {
  buffer[0]  = (float)mMatrix[0][0]; buffer[1]  = (float)mMatrix[1][0];
  buffer[2]  = (float)mMatrix[2][0]; buffer[3]  = (float)mMatrix[3][0];
  buffer[4]  = (float)mMatrix[0][1]; buffer[5]  = (float)mMatrix[1][1];
  buffer[6]  = (float)mMatrix[2][1]; buffer[7]  = (float)mMatrix[3][1];
  buffer[8]  = (float)mMatrix[0][2]; buffer[9]  = (float)mMatrix[1][2];
  buffer[10] = (float)mMatrix[2][2]; buffer[11] = (float)mMatrix[3][2];
  buffer[12] = (float)mMatrix[0][3]; buffer[13] = (float)mMatrix[1][3];
  buffer[14] = (float)mMatrix[2][3]; buffer[15] = (float)mMatrix[3][3];
}

inline void setCamera(double width, double height, const MFnCamera &mCamera, FabricCore::RTVal &camera) {
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
    mCamera.getPortFieldOfView(int(width), int(height), fovX, fovY);    
    param = FabricSplice::constructFloat64RTVal(fovY);
    camera.callMethod("", "setFovY", 1, &param);
  }

  std::vector<FabricCore::RTVal> args(2);
  args[0] = FabricSplice::constructFloat32RTVal(mCamera.nearClippingPlane());
  args[1] = FabricSplice::constructFloat32RTVal(mCamera.farClippingPlane());
  camera.callMethod("", "setRange", 2, &args[0]);
}

inline void setProjection(const MMatrix &projection, FabricCore::RTVal &camera) {
  FabricCore::RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
  FabricCore::RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)projectionData.getData();
  setMatrixTranspose(projection, buffer);
  camera.callMethod("", "setProjMatrix", 1, &projectionMat);
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

bool FabricSpliceRenderCallback::getCallback(FabricCore::RTVal &callback) {
  FabricCore::RTVal isValid = FabricSplice::constructBooleanRTVal(false);
  callback = FabricSplice::constructObjectRTVal("RTROGLHostCallback");
  callback = callback.callMethod("RTROGLHostCallback", "getCallback", 1, &isValid);
  return isValid.getBoolean();
}

void FabricSpliceRenderCallback::draw(double width, double height, uint32_t phase) {
  FabricCore::RTVal callback;
  if(getCallback(callback))
  {
    FabricSplice::Logging::AutoTimer globalTimer("Maya::DrawOpenGL()"); 
    FabricCore::RTVal args[5] = {
      FabricSplice::constructUInt32RTVal(phase),
      FabricSplice::constructStringRTVal(sPanelName.asChar()),
      FabricSplice::constructUInt32RTVal(uint32_t(width)),
      FabricSplice::constructUInt32RTVal(uint32_t(height)),
      FabricSplice::constructUInt32RTVal(2)
    };
    callback.callMethod("", "render", 5, &args[0]);    
  }
}

MString gRenderName = "NoViewport";
MString FabricSpliceRenderCallback::sPanelName = "";
void FabricSpliceRenderCallback::preDrawCallback(const MString &panelName, void *clientData) {
  
  FabricCore::RTVal callback;
  if(!getCallback(callback)) return;

  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

#if _SPLICE_MAYA_VERSION >= 2016
  if(getActiveRenderName(view) == "vp2Renderer")
    return;
#endif

  sPanelName = panelName;

  MStatus status;
  FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(panelName.asChar());
  FabricCore::RTVal viewport;
  if(gRenderName != getActiveRenderName(view))
  {
    gRenderName = getActiveRenderName(view);
    viewport = callback.callMethod("BaseRTRViewport", "resetViewport", 1, &panelNameVal);
  }
  else
    viewport = callback.callMethod("BaseRTRViewport", "getOrAddViewport", 1, &panelNameVal);
  FabricCore::RTVal camera = viewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
  
  MDagPath cameraDag; view.getCamera(cameraDag);
  MFnCamera mCamera(cameraDag);
  setCamera(view.portWidth(), view.portHeight(), mCamera, camera);
  
  MMatrix projection;
  view.projectionMatrix(projection);
  setProjection(projection, camera);

  // draw
  draw(view.portWidth(), view.portHeight(), 2);
}

#if _SPLICE_MAYA_VERSION >= 2016
void FabricSpliceRenderCallback::preDrawCallback_2(MHWRender::MDrawContext &context, void* clientData) {
  MString panelName;
  context.renderingDestination(panelName);
  FabricSpliceRenderCallback::preDrawCallback(panelName, 0);
}
#endif

void FabricSpliceRenderCallback::postDrawCallback(const MString &panelName, void *clientData) {
  
  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

#if _SPLICE_MAYA_VERSION >= 2016
  if(getActiveRenderName(view) == "vp2Renderer" ||
     getActiveRenderName(view) == "RTRViewport2_name")
    return;
#endif

  uint32_t drawPhase = (getActiveRenderName(view) == "vp2Renderer") ? 3 : 4;
  FabricSpliceRenderCallback::draw(view.portWidth(), view.portHeight(), drawPhase);
}

// **************

#define gCallbackCount 32
MCallbackId gOnPanelFocusCallbackId;
MCallbackId gPostDrawCallbacks[gCallbackCount];
MCallbackId gPreDrawCallbacks[gCallbackCount];
 
inline void onModelPanelSetFocus2(void *client) {
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
  gOnPanelFocusCallbackId = MEventMessage::addEventCallback("ModelPanelSetFocus", &onModelPanelSetFocus2);
  
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
    RTRViewport2 *overridePtr = new RTRViewport2(RTRViewport2_name);
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

