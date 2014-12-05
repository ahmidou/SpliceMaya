#include <QtCore/QtCore>

#include "FabricSpliceRenderCallback.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceToolContext.h"

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

FabricCore::RTVal & FabricSpliceRenderCallback::getDrawContext(M3dView & view)
{
  if(!sDrawContext.isValid())
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");
  else if(sDrawContext.isObject() && sDrawContext.isNullObject())
    sDrawContext = FabricSplice::constructObjectRTVal("DrawContext");

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
    if(!FabricSplice::SceneManagement::hasRenderableContent())
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
    FabricSplice::SceneManagement::drawOpenGL(getDrawContext(view));
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

