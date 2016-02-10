#include <QtGui/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include "FabricSpliceToolContext.h"
#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceRenderCallback.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
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

MStatus FabricSpliceManipulationCmd::undoIt()
{
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
    FabricCore::RTVal eventDispatcherHandle = FabricSplice::constructObjectRTVal("EventDispatcherHandle");
    if(eventDispatcherHandle.isValid()){
      mEventDispatcher = eventDispatcherHandle.callMethod("EventDispatcher", "getEventDispatcher", 0, 0);

      if(mEventDispatcher.isValid()){
        mEventDispatcher.callMethod("", "activateManipulation", 0, 0);
        view.refresh(true, true);
      }
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

  MString moduleFolder = getModuleFolder();
  MString imagePath = moduleFolder + "/ui/FE_tool.xpm";
  setImage(imagePath, kImage1);
  setImage(imagePath, kImage2);
  setImage(imagePath, kImage3);

  view.refresh(true, true);
}

void FabricSpliceToolContext::toolOffCleanup()
{
  try
  {
    M3dView view = M3dView::active3dView();

    view.widget()->removeEventFilter(&sEventFilterObject);
    view.widget()->clearFocus();

    if(mEventDispatcher.isValid()){
      // By deactivating the manipulation, we enable the manipulators to perform
      // cleanup, such as hiding paint brushes/gizmos. 
      mEventDispatcher.callMethod("", "deactivateManipulation", 0, 0);
      view.refresh(true, true);

      mEventDispatcher.invalidate();
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
  if(!mEventDispatcher.isValid()){
    mayaLogFunc("Fabric Client not constructed yet.");
    return false;
  }
  // Now we translate the Qt events to FabricEngine events..

  try
  {
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

      klevent = FabricSplice::constructObjectRTVal("KeyEvent");

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

      klevent = FabricSplice::constructObjectRTVal("MouseWheelEvent");

      FabricCore::RTVal klpos = FabricSplice::constructRTVal("Vec2");
      klpos.setMember("x", FabricSplice::constructFloat32RTVal(mouseWheelEvent->pos().x()));
      klpos.setMember("y", FabricSplice::constructFloat32RTVal(mouseWheelEvent->pos().y()));

      klevent.setMember("buttons", FabricSplice::constructUInt32RTVal(mouseWheelEvent->buttons()));
      klevent.setMember("delta", FabricSplice::constructSInt32RTVal(mouseWheelEvent->delta()));
      klevent.setMember("pos", klpos);
    }

    if(klevent.isValid()){

      int eventType = int(event->type());
      QInputEvent *inputEvent = static_cast<QInputEvent *>(event);

      //////////////////////////
      // Setup the viewport
      FabricCore::RTVal inlineViewport = FabricSplice::constructObjectRTVal("InlineViewport");
      {
        double width = view.portWidth();
        double height = view.portHeight();
        // Note: There is a task open to unify the viewports between rendering and manipulation
        // This will mean that resize can occur against the propper draw context. 
        // Here I create a temporary DrawContext, but that should be eliminated.
        FabricCore::RTVal drawContext = FabricSplice::constructObjectRTVal("DrawContext");
        drawContext.setMember("viewport", inlineViewport);
        std::vector<FabricCore::RTVal> args(3);
        args[0] = drawContext;
        args[1] = FabricSplice::constructFloat64RTVal(width);
        args[2] = FabricSplice::constructFloat64RTVal(height);
        inlineViewport.callMethod("", "resize", 3, &args[0]);

        //////////////////////////
        // Setup the Camera
        {
          FabricCore::RTVal inlineCamera = FabricSplice::constructObjectRTVal("InlineCamera");

          MDagPath cameraDag;
          view.getCamera(cameraDag);
          MFnCamera camera(cameraDag);

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

          MMatrix mayaCameraMatrix = cameraDag.inclusiveMatrix();

          FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
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

          inlineViewport.callMethod("", "setCamera", 1, &inlineCamera);
        }
      }

      //////////////////////////
      // Setup the Host
      // We cannot set an interface value via RTVals.
      FabricCore::RTVal host = FabricSplice::constructObjectRTVal("Host");
      host.setMember("hostName", FabricSplice::constructStringRTVal("Maya"));

      //////////////////////////
      // Configure the event...
      std::vector<FabricCore::RTVal> args(4);
      args[0] = host;
      args[1] = inlineViewport;
      args[2] = FabricSplice::constructUInt32RTVal(eventType);
      args[3] = FabricSplice::constructUInt32RTVal(inputEvent->modifiers());
      klevent.callMethod("", "init", 4, &args[0]);

      //////////////////////////
      // Invoke the event...
      mEventDispatcher.callMethod("Boolean", "onEvent", 1, &klevent);

      bool result = klevent.callMethod("Boolean", "isAccepted", 0, 0).getBoolean();
      if(result)
        event->accept();

      // The manipulation system has requested that a node is dirtified.
      // here we use the maya command to dirtify the specified dg node.
      MString dirtifyDCCNode(host.maybeGetMember("dirtifyNode").getStringCString());
      if(dirtifyDCCNode.length() > 0){
        MGlobal::executeCommand(MString("dgdirty \"") + dirtifyDCCNode + MString("\""));
      }

      // The manipulation system has requested that a custom command be invoked.
      // Invoke the custom command passing the speficied args.
      MString customCommand(host.maybeGetMember("customCommand").getStringCString());
      if(customCommand.length() > 0){
        FabricCore::RTVal customCommandParams = host.maybeGetMember("customCommandParams");
        if(customCommandParams.callMethod("Size", "size", 0, 0).getUInt32() > 0)
        {
          if(customCommand == "setAttr")
          {
            FabricCore::RTVal attributeVal = FabricSplice::constructStringRTVal("attribute");
            FabricCore::RTVal valueVal = FabricSplice::constructStringRTVal("value");

            MString attribute = customCommandParams.callMethod("String", "getString", 1, &attributeVal).getStringCString();
            MString valueType = customCommandParams.callMethod("String", "getValueType", 1, &valueVal).getStringCString();
            FabricCore::RTVal value;

            if(attribute.length() > 0)
            {
              MStringArray parts;
              attribute.split('.', parts);
              if(parts.length() == 2)
              {
                FabricDFGBaseInterface * dfgInterf = FabricDFGBaseInterface::getInstanceByName(parts[0].asChar());
                if(dfgInterf)
                {
                  FabricCore::DFGBinding binding = dfgInterf->getDFGBinding();
                  FabricCore::DFGExec exec = binding.getExec();
                  FTL::StrRef portResolvedType = exec.getExecPortResolvedType(parts[1].asChar());

                  if(portResolvedType == "Mat44")
                  {
                    if(valueType == "Mat44")
                    {
                      value = customCommandParams.callMethod("Mat44", "getMat44", 1, &valueVal);
                    }
                    else if(valueType == "Xfo")
                    {
                      value = customCommandParams.callMethod("Xfo", "getXfo", 1, &valueVal);
                      value = value.callMethod("Mat44", "toMat44", 0, 0);
                    }
                  }
                  else if(portResolvedType == "Vec3")
                  {
                    if(valueType == "Vec3")
                    {
                      value = customCommandParams.callMethod("Vec3", "getVec3", 1, &valueVal);
                    }
                    else if(valueType == "Xfo")
                    {
                      value = customCommandParams.callMethod("Xfo", "getXfo", 1, &valueVal);
                      value = value.maybeGetMember("tr");
                    }
                  }
                  else if(portResolvedType == "Euler")
                  {
                    if(valueType == "Euler")
                    {
                      value = customCommandParams.callMethod("Euler", "getEuler", 1, &valueVal);
                    }
                    else if(valueType == "Quat")
                    {
                      value = customCommandParams.callMethod("Quat", "getQuat", 1, &valueVal);
                      value = value.callMethod("Euler", "toEuler", 0, 0);
                    }
                    else if(valueType == "Xfo")
                    {
                      value = customCommandParams.callMethod("Xfo", "getXfo", 1, &valueVal);
                      value = value.maybeGetMember("ori");
                      value = value.callMethod("Euler", "toEuler", 0, 0);
                    }
                  }
                  else
                  {
                    MString message = "Attribute '"+attribute;
                    message += "'to be driven has unsupported type '";
                    message += portResolvedType.data();
                    message += "'.";
                    mayaLogErrorFunc(message);
                    return false;
                  }

                  if(!value.isValid())
                    return false;

                  if(portResolvedType == "Mat44")
                  {
                    // value = value.callMethod("Mat44", "toMat44", 0, 0);
                  }
                  else if(portResolvedType == "Vec3")
                  {
                    bool displayEnabled = true;
                    MString x,y,z;
                    x.set(value.maybeGetMember("x").getFloat32());
                    y.set(value.maybeGetMember("y").getFloat32());
                    z.set(value.maybeGetMember("z").getFloat32());
                    MGlobal::executeCommand(MString("setAttr ") + attribute + "X " + x + ";", displayEnabled);
                    MGlobal::executeCommand(MString("setAttr ") + attribute + "Y " + y + ";", displayEnabled);
                    MGlobal::executeCommand(MString("setAttr ") + attribute + "Z " + z + ";", displayEnabled);
                  }
                  else if(portResolvedType == "Euler")
                  {
                    // value = value.maybeGetMember("tr");
                  }
                }
              }
            }

          }
        }
        else
        {
          FabricCore::RTVal customCommandArgs = host.maybeGetMember("customCommandArgs");
          MString args;
          for(uint32_t i=0; i<customCommandArgs.getArraySize(); i++){
            if(i>0)
              args += MString(" ");
            args += MString(customCommandArgs.getArrayElement(i).getStringCString());
          }
          bool displayEnabled = true;
          MGlobal::executeCommand(customCommand + MString(" ") + args, displayEnabled);
        }
      }

      if(host.maybeGetMember("redrawRequested").getBoolean())
        view.refresh(true, true);

      if(host.callMethod("Boolean", "undoRedoCommandsAdded", 0, 0).getBoolean()){
        // Cache the rtvals in a static variable that the command will then stor in the undo stack.
        FabricSpliceManipulationCmd::s_rtval_commands = host.callMethod("UndoRedoCommand[]", "getUndoRedoCommands", 0, 0);

        bool displayEnabled = true;
        MGlobal::executeCommand(MString("fabricSpliceManipulation"), displayEnabled);
      }

      klevent.invalidate();

      return result;
    }
  }
  catch(FabricCore::Exception e)    {
    mayaLogErrorFunc(e.getDesc_cstr());
    return false;
  }
  catch(FabricSplice::Exception e){
    mayaLogErrorFunc(e.what());
    return false;
  }
  // the event was not handled by FabricEngine manipulation system. 
  return false;
}