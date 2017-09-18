//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricDFGWidget.h"
#include "FabricSpliceHelpers.h"
#include "FabricRenderCallback.h"
#include "FabricDFGBaseInterface.h"

#include <maya/M3dView.h>
#include "FabricImportPatternDialog.h"
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <maya/MUiMessage.h>
#include <maya/MAnimControl.h>
#include <maya/MFrameContext.h>
#include <maya/MEventMessage.h>
#include <maya/MViewport2Renderer.h>
#if MAYA_API_VERSION >= 201600
#include "Viewports/FabricViewport2Override.h"
#endif
#include "FabricCommandManagerCallback.h"

using namespace FabricUI;
using namespace FabricCore;

uint32_t FabricRenderCallback::gViewId = 0;
bool FabricRenderCallback::gCallbackEnabled = true;
RTVal FabricRenderCallback::sDrawContext;
SceneHub::SHGLRenderer FabricRenderCallback::shHostGLRenderer;

bool gRTRPassEnabled = true;

bool FabricRenderCallback::isRTRPassEnabled() 
{
  return gRTRPassEnabled;
}

void FabricRenderCallback::enableRTRPass(
  bool enable) 
{
  gRTRPassEnabled = enable;
}

bool FabricRenderCallback::isEnabled() 
{
  return FabricRenderCallback::gCallbackEnabled;
}

void FabricRenderCallback::enable(
  bool enable) 
{
  FabricRenderCallback::gCallbackEnabled = enable;
}
 
void FabricRenderCallback::disable() 
{
  FabricRenderCallback::sDrawContext.invalidate(); 
}
 
// **************

bool useOpenGLWithoutGradient() {
  if(!MHWRender::MRenderer::theRenderer()->drawAPIIsOpenGL())
  {
    MGlobal::displayError("Fabric can't draw in DirectX, please use OpenGL.");
    return false;
  }

#if MAYA_API_VERSION >= 201600
  if(MHWRender::MRenderer::theRenderer()->useGradient())
  {
    MGlobal::displayError("Fabric can't draw when background gradient is enabled, please disable it.");
    return false;
  }
#endif

  return true;
}

bool FabricRenderCallback::canDraw() 
{
  if(!FabricRenderCallback::gCallbackEnabled)
    return false;

  if(!FabricSplice::SceneManagement::hasRenderableContent() && 
    FabricDFGBaseInterface::getNumInstances() == 0 &&
    !FabricImportPatternDialog::isPreviewRenderingEnabled())
    return false;

  if(!useOpenGLWithoutGradient())
    return false;

  return gRTRPassEnabled;
}

inline MString getActiveRenderName() 
{
  MString name = MHWRender::MRenderer::theRenderer()->activeRenderOverride();
  //MGlobal::displayError("renderName 1 " + name);
  return name;
}

inline MString getActiveRenderName(
  const M3dView &view) 
{
  MString name = getActiveRenderName();
  if(name.numChars() == 0) 
  {
    MStatus status;
    name = view.rendererString(&status);
  }
  //MGlobal::displayError("renderName 2" + name);
  return name;
}

void initID() 
{
 
  if( !FabricRenderCallback::sDrawContext.isValid() || 
      (FabricRenderCallback::sDrawContext.isObject() && FabricRenderCallback::sDrawContext.isNullObject())
    ) 
  {
    FabricCommandManagerCallback::GetManagerCallback()->reset();
    FabricRenderCallback::sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
    FabricRenderCallback::sDrawContext = FabricRenderCallback::sDrawContext.callMethod("DrawContext", "getInstance", 0, 0);
    RTVal::Create(FabricRenderCallback::sDrawContext.getContext(), "Tool::InlineDrawingRender::RenderSetup", 0, 0);
  }
}

bool FabricRenderCallback::isRTR2Enable() 
{
#ifndef FABRIC_SCENEHUB
  return false;
#endif

  if(!canDraw()) 
    return false;
  
  if(!shHostGLRenderer.getSHGLRenderer().isValid())
  {
    FabricCore::Client client = FabricDFGWidget::GetCoreClient();
    shHostGLRenderer.setClient(client);
   
    RTVal host = FabricSplice::constructObjectRTVal("SHGLHostRenderer");
    RTVal isValidVal = FabricSplice::constructBooleanRTVal(false);
    host = host.callMethod("SHGLHostRenderer", "get", 1, &isValidVal);
    if(!isValidVal.getBoolean()) 
      return false;
    else
      shHostGLRenderer.setSHGLRenderer(host);
  }
 
  return shHostGLRenderer.getSHGLRenderer().isValid();
}

inline void setMatrixTranspose(
  const MMatrix &mMatrix, 
  float *buffer) 
{
  buffer[0]  = (float)mMatrix[0][0];  buffer[1]  = (float)mMatrix[1][0];
  buffer[2]  = (float)mMatrix[2][0];  buffer[3]  = (float)mMatrix[3][0];
  buffer[4]  = (float)mMatrix[0][1];  buffer[5]  = (float)mMatrix[1][1];
  buffer[6]  = (float)mMatrix[2][1];  buffer[7]  = (float)mMatrix[3][1];
  buffer[8]  = (float)mMatrix[0][2];  buffer[9]  = (float)mMatrix[1][2];
  buffer[10] = (float)mMatrix[2][2];  buffer[11] = (float)mMatrix[3][2];
  buffer[12] = (float)mMatrix[0][3];  buffer[13] = (float)mMatrix[1][3];
  buffer[14] = (float)mMatrix[2][3];  buffer[15] = (float)mMatrix[3][3];
}

void setCamera(
  bool id, 
  double width, 
  double height, 
  const MFnCamera &mCamera, 
  RTVal &camera) 
{
  MDagPath mCameraDag;
  mCamera.getPath(mCameraDag);
  //(void)status;
  MMatrix mMatrix = mCameraDag.inclusiveMatrix();

  RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
  RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)cameraMatData.getData();
  setMatrixTranspose(mMatrix, buffer);
  if(!id) camera.callMethod("", "setTransform", 1, &cameraMat);
  else camera.callMethod("", "setFromMat44", 1, &cameraMat);

  bool isOrthographic = mCamera.isOrtho();
  RTVal param = FabricSplice::constructBooleanRTVal(isOrthographic);
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

  RTVal args[2] = {
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

inline void setProjection(
  bool id, 
  const MMatrix &projection, 
  RTVal &camera) 
{
  RTVal projectionMat = FabricSplice::constructRTVal("Mat44");
  RTVal projectionData = projectionMat.callMethod("Data", "data", 0, 0);
  float *buffer = (float*)projectionData.getData();
  setMatrixTranspose(projection, buffer);
  if(!id) camera.callMethod("", "setProjMatrix", 1, &projectionMat);
  else camera.setMember("projection", projectionMat);
}

MString gPanelName = "";
std::map<std::string, MString> gPanelNameToRenderName;

RTVal gViewport;
void setupIDViewport(
  const MString &panelName, 
  double width, 
  double height, 
  const MFnCamera &mCamera,
  const MMatrix &projection,
  bool toolEventSetup )
{
  initID();

  FabricRenderCallback::sDrawContext.setMember("time", FabricSplice::constructFloat32RTVal(MAnimControl::currentTime().as(MTime::kSeconds)));

  RTVal panelNameVal = FabricSplice::constructStringRTVal(panelName.asChar());

  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);
  MString renderName = getActiveRenderName(view);

  if(gPanelName != panelName)
  {
    RTVal drawing = FabricSplice::constructObjectRTVal( "InlineDrawingScope" );
    drawing = drawing.callMethod( "InlineDrawing", "getDrawing", 0, 0 );
    gViewport = drawing.callMethod( "Viewport", "getOrCreateViewport", 1, &panelNameVal );
    gPanelName = panelName;
  }

  //if( !toolEventSetup ) {
  //  // under toolEventSetup, renderer name is not consistant (getting vp2 instead of vpOverride)
  //  if( gPanelNameToRenderName.find( panelName.asChar() ) == gPanelNameToRenderName.end() )
  //    gPanelNameToRenderName[panelName.asChar()] = renderName;
  //  else if( gPanelNameToRenderName[panelName.asChar()] != renderName ) {
  //    gPanelNameToRenderName[panelName.asChar()] = renderName;
  //    // Code to reset viewport specific data - put here
  //  }
  //}

  FabricRenderCallback::sDrawContext.setMember( "viewport", gViewport );

  RTVal args[3] = 
  {
    FabricRenderCallback::sDrawContext,
    FabricSplice::constructFloat64RTVal(width),
    FabricSplice::constructFloat64RTVal(height)
  };
  gViewport.callMethod("", "resize", 3, args);

  RTVal camera = gViewport.callMethod("InlineCamera", "getCamera", 0, 0);
  setCamera(true, width, height, mCamera, camera);
  setProjection(true, projection, camera);
}

void setupRTR2Viewport(
  const MString &renderName, 
  const MString &panelName, 
  double width, 
  double height, 
  const MFnCamera &mCamera,
  const MMatrix &projection) 
{ 
  if(!FabricRenderCallback::canDraw()) return;

  if(!FabricRenderCallback::isRTR2Enable()) return;

  FabricRenderCallback::gViewId = panelName.substringW(panelName.length()-2, panelName.length()-1).asInt();

  // under toolEventSetup, renderer name is not consistant (getting vp2 instead of vpOverride)
  if( gPanelNameToRenderName.find( panelName.asChar() ) == gPanelNameToRenderName.end() )
    gPanelNameToRenderName[panelName.asChar()] = renderName;
  else if( gPanelNameToRenderName[panelName.asChar()] != renderName ) {
    gPanelNameToRenderName[panelName.asChar()] = renderName;
    FabricRenderCallback::shHostGLRenderer.removeViewport(FabricRenderCallback::gViewId);
  }

  RTVal viewport = FabricRenderCallback::shHostGLRenderer.getOrAddViewport(FabricRenderCallback::gViewId);
  RTVal camera = viewport.callMethod("RTRBaseCamera", "getRTRCamera", 0, 0);
  setCamera(false, width, height, mCamera, camera);
  setProjection(false, projection, camera);
}

void FabricRenderCallback::drawID() 
{
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
  catch(Exception e)
  {
    MString str("FabricRenderCallback::drawID: ");
    str += e.getDesc_cstr();
    mayaLogErrorFunc(str.asChar());
  } 
}

MStatus FabricRenderCallback::drawRTR2(
  uint32_t width, 
  uint32_t height, 
  uint32_t phase) 
{
  MStatus status = MStatus::kSuccess;

  if(!canDraw()) return status;

  if(isRTR2Enable())
  {
    FabricRenderCallback::shHostGLRenderer.render(
      gViewId,
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

void FabricRenderCallback::prepareViewport(
  const MString &panelName,
  M3dView &view,
  bool toolEventSetup
) {

  MDagPath cameraDag;
  view.getCamera( cameraDag );
  MFnCamera mCamera( cameraDag );

  MMatrix projection;
  view.projectionMatrix( projection );

  if( !isRTR2Enable() )
    setupIDViewport( panelName, view.portWidth(), view.portHeight(), mCamera, projection, toolEventSetup );
  else
    setupRTR2Viewport( getActiveRenderName( view ), panelName, view.portWidth(), view.portHeight(), mCamera, projection );
}

void FabricRenderCallback::preDrawCallback(
  const MString &panelName, 
  void *clientData) 
{  

  if( !isRTR2Enable() && !FabricRenderCallback::canDraw() ) return;

  M3dView view;
  M3dView::getM3dViewFromModelPanel( panelName, view );

  // In Maya >= 2016, we deactive the maya viewport 2.0
  MString renderName = getActiveRenderName( view );
#if MAYA_API_VERSION >= 201600
  if( renderName == "vp2Renderer" )
    return;
#endif

  prepareViewport( panelName, view, false );

  if( isRTR2Enable() )
    drawRTR2( uint32_t( view.portWidth() ), uint32_t( view.portHeight() ), 2 );
}

// Special preDraw Callback for FabricViewport2Override in Maya >= 2016
#if MAYA_API_VERSION >= 201600
void FabricRenderCallback::viewport2OverridePreDrawCallback(
  MHWRender::MDrawContext &context, 
  void* clientData) 
{ 
  if( !FabricRenderCallback::canDraw() ) return;

  MString panelName;
  context.renderingDestination(panelName);

  MString renderName = getActiveRenderName();
  if(renderName != FabricViewport2Override_name)
  {
    MGlobal::displayError("Fabric can draw in " + FabricViewport2Override_name + " only.");
    return;
  }

  int oriX, oriY, width, height;
  MStatus status = context.getViewportDimensions(oriX, oriY, width, height);

  MDagPath cameraDag = context.getCurrentCameraPath(&status);
  MFnCamera mCamera(cameraDag);
  MMatrix projection = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);

  if(!isRTR2Enable())
  {
    setupIDViewport(panelName, (double)width, (double)height, mCamera, projection, false);
    //drawID();
  }
  else
  {
    setupRTR2Viewport(renderName, panelName, (double)width, (double)height, mCamera, projection);
    drawRTR2(width, height, 2);
  }
}
#endif

void FabricRenderCallback::postDrawCallback(
  const MString &panelName, 
  void *clientData) 
{
  M3dView view;
  M3dView::getM3dViewFromModelPanel(panelName, view);

// In Maya >= 2016, we deactive the maya viewport 2.0
// And the postDraw Callback is done within the override, cf Viewport2Override class.
#if MAYA_API_VERSION >= 201600
  if(
      getActiveRenderName(view) == "vp2Renderer" ||   
      getActiveRenderName(view) == FabricViewport2Override_name
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
 
inline void onModelPanelSetFocus(
  void *client) 
{
  MString panelName;
  MGlobal::executeCommandOnIdle("refresh;", false);
}

void FabricRenderCallback::plug() 
{
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
// within the override, cf FabricViewport2Override class.
#if MAYA_API_VERSION >= 201600
  MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
  if(renderer) 
  {
    FabricViewport2Override *overridePtr = new FabricViewport2Override(
      FabricViewport2Override_name);
    
    if(overridePtr) 
      renderer->registerOverride(
        overridePtr);
    
    renderer->addNotification(
      viewport2OverridePreDrawCallback, 
      "FabricViewport2OverridePreDrawPass", 
      MHWRender::MPassContext::kBeginSceneRenderSemantic, 
      0);
  }
#endif
}

void FabricRenderCallback::unplug() 
{
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
    renderer->removeNotification(
      "FabricViewport2OverridePreDrawPass", 
      MHWRender::MPassContext::kBeginSceneRenderSemantic);

    const MHWRender::MRenderOverride* overridePtr = renderer->findRenderOverride(
      FabricViewport2Override_name);

    if(overridePtr)
    {
      renderer->deregisterOverride(
        overridePtr);
      
      delete overridePtr;
      overridePtr = 0;
    }
  }
#endif
}
