//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include <QMouseEvent>
#include <FTL/StrRef.h>
#include <maya/MCursor.h>
 
#include <FabricSplice.h>
#include "FabricMayaException.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include "FabricToolContext.h"
#include "FabricRenderCallback.h"
#include <FabricUI/Viewports/QtToKLEvent.h>
#include <FabricUI/Commands/KLCommandManager.h>
  
using namespace FabricCore;
using namespace FabricUI::Commands;

/////////////////////////////////////////////////////
// FabricManipulationCmd
RTVal FabricManipulationCmd::s_rtval_commands;

FabricManipulationCmd::FabricManipulationCmd() 
{
  m_rtval_commands = s_rtval_commands;
}

FabricManipulationCmd::~FabricManipulationCmd() 
{
}

void* FabricManipulationCmd::creator() 
{
  return new FabricManipulationCmd;
}

MStatus FabricManipulationCmd::doIt(
  const MArgList &args) 
{
  return MStatus::kSuccess;
}

MStatus FabricManipulationCmd::redoIt() 
{
  FABRIC_MAYA_CATCH_BEGIN();

  if(m_rtval_commands.isValid())
  {
    for(uint32_t i=0; i<m_rtval_commands.getArraySize(); i++)
      m_rtval_commands.getArrayElement(i).callMethod("", "doAction", 0, 0);
  }
  M3dView::scheduleRefreshAllViews();
  return MStatus::kSuccess;
 
  FABRIC_MAYA_CATCH_END("FabricManipulationCmd::redoIt");

  return MStatus::kFailure;
}

MStatus FabricManipulationCmd::undoIt() 
{
  FABRIC_MAYA_CATCH_BEGIN();

  if(m_rtval_commands.isValid())
  {
    for(uint32_t i=0; i<m_rtval_commands.getArraySize(); i++)
      m_rtval_commands.getArrayElement(i).callMethod("", "undoAction", 0, 0);
  }
  M3dView::scheduleRefreshAllViews();
  return MStatus::kSuccess;

  FABRIC_MAYA_CATCH_END("FabricManipulationCmd::undoIt");
 
  return MStatus::kFailure;
}

bool FabricManipulationCmd::isUndoable() const 
{
  return true;
}


/////////////////////////////////////////////////////
// FabricManipulationCmd
FabricToolCmd::FabricToolCmd() 
{
  setCommandString("FabricToolCmd");
}

FabricToolCmd::~FabricToolCmd() 
{
}

void* FabricToolCmd::creator() 
{
  return new FabricToolCmd;
}

MStatus FabricToolCmd::doIt(
  const MArgList &args) 
{
  return redoIt();
}

MStatus FabricToolCmd::redoIt() 
{
  // we don't do anything during the tool really
  return MStatus::kSuccess;
}

MStatus FabricToolCmd::undoIt() 
{
  return MStatus::kSuccess;
}

bool FabricToolCmd::isUndoable() const 
{
  return false;
}



/////////////////////////////////////////////////////
// FabricToolContext
class EventFilterObject : public QObject 
{
  public:
    FabricToolContext *tool;

    M3dView view;
    MString panelName;

    bool eventFilter(
      QObject *object, 
      QEvent *event
      );
};

static std::map<void*, EventFilterObject*> sEventFilterObjectMap;

const char helpString[] = "Click and drag to interact with Fabric:Splice.";

FabricToolContext::FabricToolContext() 
{
}

void FabricToolContext::getClassName(
  MString & name) const 
{
  name.set("FabricSpliceTool");
}

void FabricToolContext::toolOnSetup(
  MEvent &) 
{
  M3dView view = M3dView::active3dView();
  //setCursor(MCursor::editCursor);
  setHelpString(helpString);
  setTitleString("FabricSplice Tool");

  MString moduleFolder = getModuleFolder();
  MString imagePath = moduleFolder + "/ui/FE_tool.xpm";
  setImage(imagePath, kImage1);
  setImage(imagePath, kImage2);
  setImage(imagePath, kImage3);

  const Client *client = 0;
  FECS_DGGraph_getClient(&client);

  if(!client)
  {
    mayaLogFunc("Fabric Client not constructed yet. A Splice Node must be created before the manipulation tool can be activated.");
    return;
  }

  FABRIC_MAYA_CATCH_BEGIN();

  RTVal eventDispatcherHandle = FabricSplice::constructObjectRTVal("EventDispatcherHandle");
  if(eventDispatcherHandle.isValid())
  {
    mEventDispatcher = eventDispatcherHandle.callMethod("EventDispatcher", "getEventDispatcher", 0, 0);
    if(mEventDispatcher.isValid())
    {
      mEventDispatcher.callMethod("", "activateManipulation", 0, 0);
      M3dView::scheduleRefreshAllViews();
    }
  }

  // Install filters on all views
  MStringArray modelPanels;
  MGlobal::executeCommand( MString( "getPanel -type \"modelPanel\"" ), modelPanels );
  for( unsigned int i = 0; i < modelPanels.length(); i++ ) 
  {
    M3dView panelView;
    if( MStatus::kSuccess == M3dView::getM3dViewFromModelPanel( modelPanels[i], panelView ) ) 
    {
      EventFilterObject* filter = new EventFilterObject();
      filter->tool = this;
      filter->view = panelView;
      filter->panelName = modelPanels[i];
      sEventFilterObjectMap[panelView.widget()] = filter;
      panelView.widget()->installEventFilter( filter );
    }
  }

  view.widget()->setFocus();
  M3dView::scheduleRefreshAllViews();

  FABRIC_MAYA_CATCH_END("FabricToolContext::toolOnSetup");
}

void FabricToolContext::toolOffCleanup() 
{
  
  FABRIC_MAYA_CATCH_BEGIN();

  for( std::map<void*, EventFilterObject*>::iterator it = sEventFilterObjectMap.begin(); it != sEventFilterObjectMap.end(); ++it ) 
  {
    EventFilterObject* filter = it->second;
    filter->view.widget()->removeEventFilter( filter );
    delete filter;
  }

  sEventFilterObjectMap.clear();
  if(mEventDispatcher.isValid())
  {
    // By deactivating the manipulation, we enable the manipulators to perform
    // cleanup, such as hiding paint brushes/gizmos. 
    mEventDispatcher.callMethod("", "deactivateManipulation", 0, 0);
    mEventDispatcher.invalidate();
  }

  M3dView view = M3dView::active3dView();
  view.widget()->clearFocus();
  M3dView::scheduleRefreshAllViews();
  FABRIC_MAYA_CATCH_END("FabricToolContext::toolOffCleanup");
}

MStatus FabricToolContext::doPress(
  MEvent & event) 
{
  return MS::kSuccess;
}

MStatus FabricToolContext::doDrag(
  MEvent & event) 
{
  return MS::kSuccess;
}

MStatus FabricToolContext::doRelease(
  MEvent & event) 
{
  return MS::kSuccess;
}

MStatus FabricToolContext::doEnterRegion(
  MEvent & event) 
{
  return setHelpString(helpString);
}

MPxContext* FabricToolContextCmd::makeObj() 
{
  return new FabricToolContext;
}

void* FabricToolContextCmd::creator() 
{
  return new FabricToolContextCmd;
}

bool EventFilterObject::eventFilter(
  QObject *object, 
  QEvent *event) 
{
  return tool->onEvent(event, panelName, view);
}
 
bool FabricToolContext::onIDEvent(
  QEvent *event, 
  const MString &panelName,
  M3dView &view) 
{
  
  const Client *client = 0;
  FECS_DGGraph_getClient(&client);

  if(!client)
  {
    mayaLogFunc("Fabric Client not constructed yet. A Splice Node must be created before the manipulation tool can be activated.");
    return false;
  }

  if(!FabricRenderCallback::sDrawContext.isValid())
  {
    mayaLogFunc("InlineDrawing not constructed yet. A DrawingHandle Node must be created before the manipulation tool can be activated.");
    return false;
  }

  FABRIC_MAYA_CATCH_BEGIN();

  //Setup the right viewport
  FabricRenderCallback::prepareViewport( panelName, view, true );

  RTVal viewport = FabricRenderCallback::sDrawContext.maybeGetMember("viewport");
  RTVal klevent = QtToKLEvent(event, viewport, "Maya" );
   
  if(klevent.isValid() && !klevent.isNullObject())
  {
    //////////////////////////
    // Invoke the event...
    mEventDispatcher.callMethod("Boolean", "onEvent", 1, &klevent);
    bool result = klevent.callMethod("Boolean", "isAccepted", 0, 0).getBoolean();

    // The manipulation system has requested that a node is dirtified.
    // here we use the maya command to dirtify the specified dg node.
    RTVal host = klevent.maybeGetMember("host");
    MString dirtifyDCCNode(host.maybeGetMember("dirtifyNode").getStringCString());
    if(dirtifyDCCNode.length() > 0)
      MGlobal::executeCommand(MString("dgdirty \"") + dirtifyDCCNode + MString("\""));
    
    // The manipulation system has requested that a custom command be invoked.
    // Invoke the custom command passing the speficied args.
    MString customCommand(host.maybeGetMember("customCommand").getStringCString());
    if(customCommand.length() > 0)
    {
      RTVal customCommandParams = host.maybeGetMember("customCommandParams");
      if(customCommandParams.callMethod("Size", "size", 0, 0).getUInt32() > 0)
      {
        if(customCommand == "setAttr")
        {
          RTVal attributeVal = FabricSplice::constructStringRTVal("attribute");
          RTVal valueVal = FabricSplice::constructStringRTVal("value");

          MString attribute = customCommandParams.callMethod("String", "getString", 1, &attributeVal).getStringCString();
          MString valueType = customCommandParams.callMethod("String", "getValueType", 1, &valueVal).getStringCString();
          RTVal value;

          if(attribute.length() > 0)
          {
            MStringArray parts;
            attribute.split('.', parts);
            if(parts.length() == 2)
            {
              FabricDFGBaseInterface * dfgInterf = FabricDFGBaseInterface::getInstanceByName(parts[0].asChar());
              if(dfgInterf)
              {
                DFGBinding binding = dfgInterf->getDFGBinding();
                DFGExec exec = binding.getExec();
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
        RTVal customCommandArgs = host.maybeGetMember("customCommandArgs");
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

    if(host.callMethod("Boolean", "undoRedoCommandsAdded", 0, 0).getBoolean())
    {
      // Cache the rtvals in a static variable that the command will then stor in the undo stack.
      FabricManipulationCmd::s_rtval_commands = host.callMethod(
        "UndoRedoCommand[]", 
        "getUndoRedoCommands", 
        0, 
        0);

      bool displayEnabled = true;
      MGlobal::executeCommand(MString("fabricSpliceManipulation"), displayEnabled);
    }

    if(result)
    {
      KLCommandManager *manager = qobject_cast<KLCommandManager*>(
        CommandManager::getCommandManager());
      manager->synchronizeKL();
      event->accept();
    }

    if(host.maybeGetMember("redrawRequested").getBoolean())
      M3dView::scheduleRefreshAllViews();

    klevent.invalidate();
    return result;
  }

  FABRIC_MAYA_CATCH_END("FabricToolContext::onEvent");

  return false;
}

bool FabricToolContext::onRTR2Event(
  QEvent *event, 
  M3dView &view) 
{

  MString panelName;
  M3dView::getM3dViewFromModelPanel(panelName, view);
  unsigned int panelId = panelName.substringW(panelName.length()-2, panelName.length()-1).asInt();
 
  bool res = FabricRenderCallback::shHostGLRenderer.onEvent(
    panelId, 
    event, 
    false);

  if(event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if(mouseEvent->button() & Qt::MidButton)
    {
      if(mouseEvent->modifiers() & Qt::ShiftModifier)
      {
        FabricRenderCallback::shHostGLRenderer.emitShowContextualMenu(
          panelId,
          mouseEvent->pos(), 
          view.widget());
        event->accept();
      }
    }
  }

  if(res)M3dView::scheduleRefreshAllViews();
  return res;
}

bool FabricToolContext::onEvent(
  QEvent *event, const MString &panelName, M3dView& view)
{
  if(!FabricRenderCallback::canDraw()) 
  {
    mayaLogFunc("Viewport not constructed yet.");
    return false;
  }

  if(!mEventDispatcher.isValid()) 
  {
    mayaLogFunc("Fabric Client not constructed yet.");
    return false;
  }
 
  FABRIC_MAYA_CATCH_BEGIN();

  if(!FabricRenderCallback::isRTR2Enable())
  {
    bool res = onIDEvent(event, panelName, view);

    if(event->type() == QEvent::MouseButtonRelease)
    {
      KLCommandManager *manager = qobject_cast<KLCommandManager*>(
        CommandManager::getCommandManager());
      manager->synchronizeKL();
    }

    return res;
  }
  else
    return onRTR2Event(event, view);

  FABRIC_MAYA_CATCH_END("FabricToolContext::onEvent");

  // the event was not handled by FabricEngine manipulation system. 
  return false;
}
