//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QtGui/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include "FabricSpliceToolContext.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include <FabricCore.h>
#include <FabricSplice.h>

#include <maya/MCursor.h>
#include <maya/MDagPath.h>
#include <maya/MFnCamera.h>
#include <maya/MAnimControl.h>
#include <maya/MTime.h>
#include <FTL/StrRef.h>

#include <map>


/////////////////////////////////////////////////////
// FabricSpliceManipulationCmd
FabricCore::RTVal FabricSpliceManipulationCmd::s_rtval_commands;

FabricSpliceManipulationCmd::FabricSpliceManipulationCmd() {
  m_rtval_commands = s_rtval_commands;
}

FabricSpliceManipulationCmd::~FabricSpliceManipulationCmd() {}

void* FabricSpliceManipulationCmd::creator() {
  return new FabricSpliceManipulationCmd;
}

MStatus FabricSpliceManipulationCmd::doIt(const MArgList &args) {
  return MStatus::kSuccess;
}

MStatus FabricSpliceManipulationCmd::redoIt() {
  try
  {
    if(m_rtval_commands.isValid()){
      for(uint32_t i=0; i<m_rtval_commands.getArraySize(); i++){
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

MStatus FabricSpliceManipulationCmd::undoIt() {
  try
  {
    if(m_rtval_commands.isValid()){
      for(uint32_t i=0; i<m_rtval_commands.getArraySize(); i++){
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

bool FabricSpliceManipulationCmd::isUndoable() const {
  return true;
}


/////////////////////////////////////////////////////
// FabricSpliceManipulationCmd
FabricSpliceToolCmd::FabricSpliceToolCmd() {
  setCommandString("FabricSpliceToolCmd");
}

FabricSpliceToolCmd::~FabricSpliceToolCmd() {
}

void* FabricSpliceToolCmd::creator() {
  return new FabricSpliceToolCmd;
}

MStatus FabricSpliceToolCmd::doIt(const MArgList &args) {
  return redoIt();
}

MStatus FabricSpliceToolCmd::redoIt() {
  // we don't do anything during the tool really
  return MStatus::kSuccess;
}

MStatus FabricSpliceToolCmd::undoIt() {
  return MStatus::kSuccess;
}

bool FabricSpliceToolCmd::isUndoable() const {
  return false;
}


/////////////////////////////////////////////////////
// FabricSpliceToolContext
class EventFilterObject : public QObject {
  public:
    FabricSpliceToolContext *tool;
    bool eventFilter(QObject *object, QEvent *event);
};

bool EventFilterObject::eventFilter(QObject *object, QEvent *event) {
  return tool->onEvent(event);
}

static EventFilterObject sEventFilterObject;

const char helpString[] = "Click and drag to interact with Fabric:Splice.";

FabricSpliceToolContext::FabricSpliceToolContext() {}

void FabricSpliceToolContext::getClassName(MString & name) const {
  name.set("FabricSpliceTool");
}

void FabricSpliceToolContext::toolOnSetup(MEvent &event) {

  M3dView view = M3dView::active3dView();
  const FabricCore::Client * client = NULL;
  FECS_DGGraph_getClient(&client);
  if(!client)
  {
    mayaLogFunc("Fabric Client not constructed yet. A Splice Node must be created before the manipulation tool can be activated.");
    return;
  }
  
  sEventFilterObject.tool = this;
  view.widget()->installEventFilter(&sEventFilterObject);
  view.widget()->setFocus();

  setCursor(MCursor::editCursor);
  setHelpString(helpString);
  setTitleString("FabricSplice Tool");

  MString moduleFolder = getModuleFolder();
  MString imagePath = moduleFolder + "/ui/FE_tool.xpm";
  setImage(imagePath, kImage1);
  setImage(imagePath, kImage2);
  setImage(imagePath, kImage3);
  view.refresh(true, true);
}

void FabricSpliceToolContext::toolOffCleanup() {
  M3dView view = M3dView::active3dView();
  view.widget()->removeEventFilter(&sEventFilterObject);
  view.widget()->clearFocus();
  view.refresh(true, true);
}

MStatus FabricSpliceToolContext::doPress(MEvent & event) {
  return MS::kSuccess;
}

MStatus FabricSpliceToolContext::doDrag(MEvent & event) {
  return MS::kSuccess;
}

MStatus FabricSpliceToolContext::doRelease(MEvent & event) {
  return MS::kSuccess;
}

MStatus FabricSpliceToolContext::doEnterRegion(MEvent & event) {
  return setHelpString( helpString );
}

MPxContext* FabricSpliceToolContextCmd::makeObj() {
  return new FabricSpliceToolContext;
}

void* FabricSpliceToolContextCmd::creator() {
  return new FabricSpliceToolContextCmd;
}

inline FabricCore::RTVal QtToKLMousePosition(const QPoint &pos, FabricCore::RTVal viewport, bool swapAxis) {
  FabricCore::RTVal klViewportDim = viewport.callMethod("Vec2", "getDimensions", 0, 0);
  FabricCore::RTVal klpos = FabricSplice::constructRTVal("Vec2");
  klpos.setMember("x",  FabricSplice::constructFloat32RTVal(pos.x()));
  // We must inverse the y coordinate to match Qt/RTR viewport system of coordonates
  if(swapAxis)
    klpos.setMember("y", FabricSplice::constructFloat32RTVal(klViewportDim.maybeGetMember("y").getFloat32() - pos.y()));
  else
    klpos.setMember("y", FabricSplice::constructFloat32RTVal(pos.y()));
  return klpos;
}

bool FabricSpliceToolContext::onEvent(QEvent *event) {

  FabricCore::RTVal callback;
  if(!FabricSpliceRenderCallback::getCallback(callback))
    return false;

  if(FabricSpliceRenderCallback::sPanelName.length() == 0)
    return false;
  
  // Now we translate the Qt events to FabricEngine events..
  FabricCore::RTVal panelNameVal = FabricSplice::constructStringRTVal(FabricSpliceRenderCallback::sPanelName.asChar());
  FabricCore::RTVal viewport = callback.callMethod("Viewport_Virtual", "getViewport", 1, &panelNameVal);

  FabricCore::RTVal klevent;
  if(event->type() == QEvent::Enter || event->type() == QEvent::Leave)
    klevent = FabricSplice::constructObjectRTVal("MouseEvent");
      
  else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) 
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    klevent = FabricSplice::constructObjectRTVal("KeyEvent");
    klevent.setMember("key", FabricSplice::constructUInt32RTVal(keyEvent->key()));
    klevent.setMember("count", FabricSplice::constructUInt32RTVal(keyEvent->count()));
    klevent.setMember("isAutoRepeat", FabricSplice::constructBooleanRTVal(keyEvent->isAutoRepeat()));
  }

  else if ( event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonDblClick || 
            event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) 
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    klevent = FabricSplice::constructObjectRTVal("MouseEvent");
    klevent.setMember("pos", QtToKLMousePosition(mouseEvent->pos(), viewport, true));
    klevent.setMember("button", FabricSplice::constructUInt32RTVal(mouseEvent->button()));
    klevent.setMember("buttons", FabricSplice::constructUInt32RTVal(mouseEvent->buttons()));
  }

  else if (event->type() == QEvent::Wheel) 
  {
    QWheelEvent *mouseWheelEvent = static_cast<QWheelEvent *>(event);
    klevent = FabricSplice::constructObjectRTVal("MouseWheelEvent");
    klevent.setMember("pos", QtToKLMousePosition(mouseWheelEvent->pos(), viewport, true));
    klevent.setMember("buttons", FabricSplice::constructUInt32RTVal(mouseWheelEvent->buttons()));
    klevent.setMember("delta", FabricSplice::constructSInt32RTVal(mouseWheelEvent->delta()));
  }

  if(klevent.isValid()) 
  {
    int eventType = int(event->type());
    QInputEvent *inputEvent = static_cast<QInputEvent *>(event);
  
    FabricCore::RTVal host = FabricSplice::constructObjectRTVal("Host");
    host.setMember("hostName", FabricSplice::constructStringRTVal("Maya"));
 
    FabricCore::RTVal args[4] = {
      host, viewport,
      FabricSplice::constructUInt32RTVal(eventType),
      FabricSplice::constructUInt32RTVal(inputEvent->modifiers())
    };
    klevent.callMethod("", "init", 4, &args[0]);
    callback.callMethod("", "onEvent", 1, &klevent);

    bool result = klevent.callMethod("Boolean", "isAccepted", 0, 0).getBoolean();
    if(result) 
    {
      event->accept();
      M3dView view = M3dView::active3dView();
      view.refresh(true, true);
      view.widget()->setFocus();
    }
    return result;
  }

  // the event was not handled by FabricEngine manipulation system. 
  return false;
}
