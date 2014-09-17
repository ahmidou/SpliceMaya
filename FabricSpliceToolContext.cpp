#include <QtGui/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include "FabricSpliceToolContext.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include <FabricSplice.h>
#include <maya/MCursor.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <maya/MAnimControl.h>
#include <maya/MTime.h>

#include <map>


/////////////////////////////////////////////////////
// FabricSpliceManipulationCmd

FabricCore::RTVal FabricSpliceManipulationCmd::s_rtval_commands;

FabricSpliceManipulationCmd::FabricSpliceManipulationCmd()
{
  m_rtval_commands = s_rtval_commands;
}

FabricSpliceManipulationCmd::~FabricSpliceManipulationCmd()
{
}

void* FabricSpliceManipulationCmd::creator()
{
  return new FabricSpliceManipulationCmd;
}

MStatus FabricSpliceManipulationCmd::doIt(const MArgList &args)
{
  return MStatus::kSuccess;
}

MStatus FabricSpliceManipulationCmd::redoIt()
{
  try
  {
    if(m_rtval_commands.isValid()){
      for(int i=0; i<m_rtval_commands.getArraySize(); i++){
        m_rtval_commands.getArrayElement(i).callMethod("", "doAction", 0, 0);
      }
    }
    M3dView view = M3dView::active3dView();
    view.refresh(true, true);
    return MStatus::kSuccess;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return MStatus::kFailure;
  }
}

MStatus FabricSpliceManipulationCmd::undoIt()
{
  try
  {
    if(m_rtval_commands.isValid()){
      for(int i=0; i<m_rtval_commands.getArraySize(); i++){
        m_rtval_commands.getArrayElement(i).callMethod("", "undoAction", 0, 0);
      }
    }
    M3dView view = M3dView::active3dView();
    view.refresh(true, true);
    return MStatus::kSuccess;
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return MStatus::kFailure;
  }
}

bool FabricSpliceManipulationCmd::isUndoable() const
{
  return true;
}


/////////////////////////////////////////////////////
// FabricSpliceManipulationCmd


FabricSpliceToolCmd::FabricSpliceToolCmd()
{
  setCommandString("FabricSpliceToolCmd");
}

FabricSpliceToolCmd::~FabricSpliceToolCmd()
{
}

void* FabricSpliceToolCmd::creator()
{
  return new FabricSpliceToolCmd;
}

MStatus FabricSpliceToolCmd::doIt(const MArgList &args)
{
  return redoIt();
}

MStatus FabricSpliceToolCmd::redoIt()
{
  // we don't do anything during the tool really
  return MStatus::kSuccess;
}

MStatus FabricSpliceToolCmd::undoIt()
{
  return MStatus::kSuccess;
}

bool FabricSpliceToolCmd::isUndoable() const
{
  return false;
}



/////////////////////////////////////////////////////
// FabricSpliceToolContext

class EventFilterObject : public QObject
{
public:
  FabricSpliceToolContext *tool;
  bool eventFilter(QObject *object, QEvent *event);
};

static EventFilterObject sEventFilterObject;

const char helpString[] = "Click and drag to interact with Fabric:Splice.";

FabricSpliceToolContext::FabricSpliceToolContext() 
{
}

void FabricSpliceToolContext::getClassName( MString & name ) const
{
  name.set("FabricSpliceTool");
}

void FabricSpliceToolContext::toolOnSetup(MEvent &)
{
  M3dView view = M3dView::active3dView();

  const FabricCore::Client * client = NULL;
  FECS_DGGraph_getClient(&client);

  if(!client){
    mayaLogFunc("Fabric Client not constructed yet. A Splice Node must be created before the manipulation tool can be activated.");
    return;
  }

  try{
    mEventDispatcherHandle = FabricSplice::constructObjectRTVal("EventDispatcherHandle");

    if(mEventDispatcherHandle.isValid()){
      mEventDispatcherHandle.callMethod("", "activateManipulation", 0, 0);
      view.refresh(true, true);
    }
  }
  catch(FabricCore::Exception e)    {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
  catch(FabricSplice::Exception e){
    mayaLogErrorFunc(e.what());
    return;
  }

  sEventFilterObject.tool = this;
  view.widget()->installEventFilter(&sEventFilterObject);
  view.widget()->setFocus();

  setCursor( MCursor::editCursor);
  setHelpString(helpString);
  setTitleString("FabricSplice Tool");

  view.refresh(true, true);
}

void FabricSpliceToolContext::toolOffCleanup()
{
  try
  {
    M3dView view = M3dView::active3dView();

    view.widget()->removeEventFilter(&sEventFilterObject);
    view.widget()->clearFocus();
     
    if(mEventDispatcherHandle.isValid()){
      // By deactivating the manipulation, we enable the manipulators to perform
      // cleanup, such as hiding paint brushes/gizmos. 
      mEventDispatcherHandle.callMethod("", "deactivateManipulation", 0, 0);
      view.refresh(true, true);

      mEventDispatcherHandle.invalidate();
    }

    view.refresh(true, true);
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
}

MStatus FabricSpliceToolContext::doPress(MEvent & event)
{
  return MS::kSuccess;
}

MStatus FabricSpliceToolContext::doDrag(MEvent & event)
{
  return MS::kSuccess;
}

MStatus FabricSpliceToolContext::doRelease( MEvent & event)
{
  return MS::kSuccess;
}

MStatus FabricSpliceToolContext::doEnterRegion( MEvent & event)
{
  return setHelpString( helpString );
}

MPxContext* FabricSpliceToolContextCmd::makeObj()
{
  return new FabricSpliceToolContext;
}

void* FabricSpliceToolContextCmd::creator()
{
  return new FabricSpliceToolContextCmd;
}


bool EventFilterObject::eventFilter(QObject *object, QEvent *event)
{
  return tool->onEvent(event);
}



bool FabricSpliceToolContext::onEvent(QEvent *event)
{
  if(!mEventDispatcherHandle.isValid()){
    mayaLogFunc("Fabric Client not constructed yet.");
    return false;
  }
  // Now we translate the Qt events to FabricEngine events..

  M3dView view = M3dView::active3dView();
  FabricCore::RTVal klevent;

  if(event->type() == QEvent::Enter){
    klevent = FabricSplice::constructObjectRTVal("MouseEvent");
  }
  else if(event->type() == QEvent::Leave){
    klevent = FabricSplice::constructObjectRTVal("MouseEvent");
  }
  else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    try{
      klevent = FabricSplice::constructObjectRTVal("KeyEvent");
    }
    catch(FabricCore::Exception e)    {
      mayaLogErrorFunc(e.getDesc_cstr());
      return false;
    }
    catch(FabricSplice::Exception e){
      mayaLogErrorFunc(e.what());
      return false;
    }

    klevent.setMember("key", FabricSplice::constructUInt32RTVal(keyEvent->key()));
    klevent.setMember("count", FabricSplice::constructUInt32RTVal(keyEvent->count()));
    klevent.setMember("isAutoRepeat", FabricSplice::constructBooleanRTVal(keyEvent->isAutoRepeat()));

  } 
  else if ( event->type() == QEvent::MouseMove || 
            event->type() == QEvent::MouseButtonDblClick || 
            event->type() == QEvent::MouseButtonPress || 
            event->type() == QEvent::MouseButtonRelease
  ) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

    klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    FabricCore::RTVal klpos = FabricSplice::constructRTVal("Vec2");
    klpos.setMember("x", FabricSplice::constructFloat32RTVal(mouseEvent->pos().x()));
    klpos.setMember("y", FabricSplice::constructFloat32RTVal(mouseEvent->pos().y()));

    klevent.setMember("button", FabricSplice::constructUInt32RTVal(mouseEvent->button()));
    klevent.setMember("buttons", FabricSplice::constructUInt32RTVal(mouseEvent->buttons()));
    klevent.setMember("pos", klpos);
  } 
  else if (event->type() == QEvent::Wheel) {
    QWheelEvent *mouseWheelEvent = static_cast<QWheelEvent *>(event);

    try{
      klevent = FabricSplice::constructObjectRTVal("MouseWheelEvent");
    }
    catch(FabricCore::Exception e)    {
      mayaLogErrorFunc(e.getDesc_cstr());
      return false;
    }
    catch(FabricSplice::Exception e){
      mayaLogErrorFunc(e.what());
      return false;
    }

    FabricCore::RTVal klpos = FabricSplice::constructRTVal("Vec2");
    klpos.setMember("x", FabricSplice::constructFloat32RTVal(mouseWheelEvent->pos().x()));
    klpos.setMember("y", FabricSplice::constructFloat32RTVal(mouseWheelEvent->pos().y()));

    klevent.setMember("buttons", FabricSplice::constructUInt32RTVal(mouseWheelEvent->buttons()));
    klevent.setMember("delta", FabricSplice::constructSInt32RTVal(mouseWheelEvent->delta()));
    klevent.setMember("pos", klpos);
  }

  if(klevent.isValid()){

    int eventType = int(event->type());
    klevent.setMember("eventType", FabricSplice::constructUInt32RTVal(eventType));

    QInputEvent *inputEvent = static_cast<QInputEvent *>(event);
    klevent.setMember("modifiers", FabricSplice::constructUInt32RTVal(inputEvent->modifiers()));

    //////////////////////////
    // Setup the Camera
    {

      //////////////////////////
      // Setup the viewport
      FabricCore::RTVal inlineViewport = FabricSplice::constructObjectRTVal("InlineViewport");
      {
        FabricCore::RTVal viewportDim = FabricSplice::constructRTVal("Vec2");
        viewportDim.setMember("x", FabricSplice::constructFloat64RTVal(view.portWidth()));
        viewportDim.setMember("y", FabricSplice::constructFloat64RTVal(view.portHeight()));
        inlineViewport.setMember("dimensions", viewportDim);
      }

      {
        FabricCore::RTVal inlineCamera = FabricSplice::constructObjectRTVal("InlineCamera");

        MDagPath cameraDag;
        view.getCamera(cameraDag);
        MFnCamera camera(cameraDag);

        inlineCamera.setMember("isOrthographic", FabricSplice::constructBooleanRTVal(camera.isOrtho()));

        double fovX, fovY;
        camera.getPortFieldOfView(view.portWidth(), view.portHeight(), fovX, fovY);    
        inlineCamera.setMember("fovY", FabricSplice::constructFloat64RTVal(fovY));
        inlineCamera.setMember("nearDistance", FabricSplice::constructFloat64RTVal(camera.nearClippingPlane()));
        inlineCamera.setMember("farDistance", FabricSplice::constructFloat64RTVal(camera.farClippingPlane()));

        MMatrix mayaCameraMatrix = cameraDag.inclusiveMatrix();

        FabricCore::RTVal cameraMat = inlineCamera.maybeGetMember("mat44");

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

            inlineCamera.setMember("mat44", cameraMat);
          }
        }
        catch (FabricCore::Exception e)
        {
          mayaLogErrorFunc(e.getDesc_cstr());
        }

        inlineViewport.setMember("camera", inlineCamera);
      }
      klevent.setMember("viewport", inlineViewport);
    }

    //////////////////////////
    // Setup the Host
    // We cannot set an interface value via RTVals.
    FabricCore::RTVal host = FabricSplice::constructObjectRTVal("Host");
    host.setMember("hostName", FabricSplice::constructStringRTVal("Maya"));
    klevent.setMember("host", host);

    //////////////////////////
    // Invoke the event...
    try{
      mEventDispatcherHandle.callMethod("Boolean", "onEvent", 1, &klevent);

      bool result = klevent.callMethod("Boolean", "isAccepted", 0, 0).getBoolean();
      if(result)
        event->accept();

      if(host.maybeGetMember("redrawRequested").getBoolean())
        view.refresh(true, true);

      if(host.callMethod("Boolean", "undoRedoCommandsAdded", 0, 0).getBoolean()){
        // Cache the rtvals in a static variable that the command will then stor in the undo stack.
        FabricSpliceManipulationCmd::s_rtval_commands = host.callMethod("UndoRedoCommand[]", "getUndoRedoCommands", 0, 0);

        bool displayEnabled = true;
        MGlobal::executeCommandOnIdle(MString("fabricSpliceManipulation"), displayEnabled);
      }

      klevent.invalidate();

      return result;
    }
    catch(FabricCore::Exception e)    {
      mayaLogErrorFunc(e.getDesc_cstr());
      return false;
    }
    catch(FabricSplice::Exception e){
      mayaLogErrorFunc(e.what());
      return false;
    }
  }
  // the event was not handled by FabricEngine manipulation system. 
  return false;
}
