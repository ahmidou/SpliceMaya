//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QFileDialog>
#include <QDialog>

#include "Foundation.h"
#include "FabricExportPatternCommand.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGWidget.h"
#include "FabricDFGConversion.h"
#include "FabricExportPatternDialog.h"

#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MQtUtil.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagModifier.h>
#include <maya/MFnTransform.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnLambertShader.h>
#include <maya/MCommandResult.h>
#include <maya/MAnimControl.h>

#include <FTL/FS.h>

MSyntax FabricExportPatternCommand::newSyntax()
{
  MSyntax syntax;
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  syntax.addFlag( "-f", "-filepath", MSyntax::kString );
  syntax.addFlag( "-q", "-disableDialogs", MSyntax::kBoolean );
  syntax.addFlag( "-s", "-scale", MSyntax::kDouble );
  syntax.addFlag( "-a", "-args", MSyntax::kString );
  syntax.addFlag( "-o", "-objects", MSyntax::kString );
  syntax.addFlag( "-b", "-begin", MSyntax::kDouble );
  syntax.addFlag( "-e", "-end", MSyntax::kDouble );
  syntax.addFlag( "-r", "-framerate", MSyntax::kDouble );
  syntax.addFlag( "-t", "-substeps", MSyntax::kLong );
  return syntax;
}

void* FabricExportPatternCommand::creator()
{
  return new FabricExportPatternCommand;
}

MStatus FabricExportPatternCommand::doIt(const MArgList &args)
{
  MStatus status;
  MArgParser argParser(syntax(), args, &status);

  bool quiet = false;
  if ( argParser.isFlagSet("disableDialogs") )
    quiet = argParser.flagArgumentBool("disableDialogs", 0);

  MString filepath;

  if ( argParser.isFlagSet("filepath") )
  {
    filepath = argParser.flagArgumentString("filepath", 0);
  }
  else if(quiet)
  {
    mayaLogErrorFunc("No filepath (-f) specified.");
    return mayaErrorOccured();
  }
  else
  {
    MCommandResult result;
    MString command = "fileDialog2 -ff \"*.canvas\" -ds 2 -fm 1";
    MGlobal::executeCommand(command, result);
    if(result.resultType() == MCommandResult::kString)
    {
      result.getResult(filepath);
    }
    else if(result.resultType() == MCommandResult::kStringArray)
    {
      MStringArray filepaths;
      result.getResult(filepaths);

      if(filepaths.length() > 0)
        filepath = filepaths[0];
    }

    if(filepath.length() == 0)
    {
      mayaLogErrorFunc("No filepath (-f) specified.");
      return mayaErrorOccured();
    }
  }

  if(!FTL::FSExists(filepath.asChar()))
  {
    mayaLogErrorFunc("FilePath \""+filepath+"\" does no exist.");
    return mayaErrorOccured();
  }

  // initialize substeps, framerate + in and out based on playcontrol
  m_settings.startTime = MAnimControl::minTime().as(MTime::kSeconds);
  m_settings.endTime = MAnimControl::maxTime().as(MTime::kSeconds);

  MString timeUnitStr;
  MGlobal::executeCommand("currentUnit -q -time", timeUnitStr);
  if(timeUnitStr == L"game")
    m_settings.fps = 15.0;
  else if(timeUnitStr == L"film")
    m_settings.fps = 24.0;
  else if(timeUnitStr == L"pal")
    m_settings.fps = 25.0;
  else if(timeUnitStr == L"ntsc")
    m_settings.fps = 30.0;
  else if(timeUnitStr == L"show")
    m_settings.fps = 48.0;
  else if(timeUnitStr == L"palf")
    m_settings.fps = 50.0;
  else if(timeUnitStr == L"ntscf")
    m_settings.fps = 60.0;

  if( argParser.isFlagSet("scale") )
  {
    m_settings.scale = argParser.flagArgumentDouble("scale", 0);
  }

  if( argParser.isFlagSet("begin") )
  {
    if(!argParser.isFlagSet("end"))
    {
      mayaLogErrorFunc("When specifying -begin you also need to specify -end.");
      return mayaErrorOccured();
    }
    m_settings.startTime = argParser.flagArgumentDouble("begin", 0);
  }
  if( argParser.isFlagSet("end") )
  {
    if(!argParser.isFlagSet("end"))
    {
      mayaLogErrorFunc("When specifying -end you also need to specify -begin.");
      return mayaErrorOccured();
    }
    m_settings.endTime = argParser.flagArgumentDouble("end", 0);
  }
  if( argParser.isFlagSet("framerate") )
  {
    m_settings.fps = argParser.flagArgumentDouble("framerate", 0);
  }

  if( argParser.isFlagSet("substeps") )
  {
    m_settings.substeps = argParser.flagArgumentInt("substeps", 0);
  }

  if( argParser.isFlagSet("objects") )
  {
    MString objects = argParser.flagArgumentString("objects", 0);
    objects.split(',', m_settings.objects);
  }

  MStringArray result;
  FabricCore::Client client;
  FabricCore::DFGBinding binding;

  try
  {
    client = FabricDFGWidget::GetCoreClient();
    client.loadExtension("GenericImporter", "", false);

    MString json;
    FILE * file = fopen(filepath.asChar(), "rb");
    if(!file)
    {
      mayaLogErrorFunc("FilePath '"+filepath+"' cannot be opened.");
      return MS::kFailure;
    }
    else
    {
      fseek( file, 0, SEEK_END );
      long fileSize = ftell( file );
      rewind( file );

      char * buffer = (char*) malloc(fileSize + 1);
      buffer[fileSize] = '\0';

      size_t readBytes = fread(buffer, 1, fileSize, file);
      assert(readBytes == size_t(fileSize));
      (void)readBytes;

      fclose(file);

      json = buffer;
      free(buffer);
    }

    FabricCore::DFGHost dfgHost = client.getDFGHost();
    binding = dfgHost.createBindingFromJSON(json.asChar());


    std::map< std::string, MObject > geometryObjects;

    if ( argParser.isFlagSet("args") )
    {
      FabricCore::DFGExec exec = binding.getExec();

      MString args = argParser.flagArgumentString("args", 0);

      FTL::JSONStrWithLoc argsSrcWithLoc( FTL::StrRef( args.asChar(), args.length() ) );
      FTL::OwnedPtr<FTL::JSONValue const> argsValue( FTL::JSONValue::Decode( argsSrcWithLoc ) );
      FTL::JSONObject const *argsObject = argsValue->cast<FTL::JSONObject>();

      for ( FTL::JSONObject::const_iterator it = argsObject->begin(); it != argsObject->end(); ++it )
      {
        FTL::CStrRef key = it->key();
        
        bool found = false;
        for(unsigned int i=0;i<exec.getExecPortCount();i++)
        {
          if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
            continue;

          FTL::CStrRef name = exec.getExecPortName(i);
          if(name != key)
            continue;

          FTL::CStrRef type = exec.getExecPortResolvedType(i);
          if(type != "String " && !FabricCore::GetRegisteredTypeIsShallow(client, type.c_str()))
          {
            mayaLogFunc(MString(getName())+": Warning: Argument "+MString(name.c_str())+" cannot be set since "+MString(type.c_str())+" is not shallow.");
            continue;
          }

          std::string json = it->value()->encode();
          FabricCore::RTVal value = FabricCore::ConstructRTValFromJSON(client, type.c_str(), json.c_str());
          binding.setArgValue(name.c_str(), value);
          found = true;
          break;
        }

        if(!found)
        {
          mayaLogFunc(MString(getName())+": Argument "+MString(key.c_str())+" does not exist in import pattern.");
          return mayaErrorOccured();
        }
      }

      return invoke(binding, m_settings);
    }
    else if(!quiet)
    {
      QWidget * mainWindow = MQtUtil().mainWindow();
      mainWindow->setFocus(Qt::ActiveWindowFocusReason);

      QEvent event(QEvent::RequestSoftwareInputPanel);
      QApplication::sendEvent(mainWindow, &event);

      FabricExportPatternDialog * dialog = new FabricExportPatternDialog(mainWindow, binding, m_settings);
      dialog->setWindowModality(Qt::ApplicationModal);
      dialog->setSizeGripEnabled(true);
      dialog->open();

      if(!dialog->wasAccepted())
      {
        return MS::kSuccess;
      }
    }
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
  }

  return MS::kSuccess;
}

MStatus FabricExportPatternCommand::invoke(FabricCore::DFGBinding binding, const FabricExportPatternSettings & settings)
{
  m_settings = settings;

  MSelectionList sl;
  if( m_settings.objects.length() > 0 )
  {
    for(unsigned int i=0;i<m_settings.objects.length();i++)
    {
      sl.add(m_settings.objects[i]);

      MObject node;
      sl.getDependNode(i, node);
      if(node.isNull())
      {
        mayaLogErrorFunc("Object '"+m_settings.objects[i]+"' not found in scene.");
        return mayaErrorOccured();
      }
    }
  }
  else
  {
    MGlobal::getActiveSelectionList( sl );
  }

  for(unsigned int i=0;i<sl.length();i++)
  {
    MObject node;
    sl.getDependNode(i, node);
    if(node.isNull())
    {
      MStringArray selString;
      sl.getSelectionStrings(i, selString);
      mayaLogFunc(MString(getName())+": Warning: Skipping Object: '"+selString[0]+"', not a dependency node.");
      return mayaErrorOccured();
    }
    m_nodes.push_back(node);
  }

  FabricCore::Context context;
  try
  {
    context = binding.getHost().getContext();

    m_context = FabricCore::RTVal::Construct(context, "ImporterContext", 0, 0);
    FabricCore::RTVal contextHost = m_context.maybeGetMember("host");
    MString mayaVersion;
    mayaVersion.set(MAYA_API_VERSION);
    contextHost.setMember("name", FabricCore::RTVal::ConstructString(context, "Maya"));
    contextHost.setMember("version", FabricCore::RTVal::ConstructString(context, mayaVersion.asChar()));
    m_context.setMember("host", contextHost);

    m_importer = FabricCore::RTVal::Create(context, "BaseImporter", 0, 0);
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
    return mayaErrorOccured();
  }

  for(size_t i=0;i<m_nodes.size();i++)
  {
    FabricCore::RTVal obj = createRTValForNode(m_nodes[i]);
    m_objects.push_back(obj);
  }

  {
    std::vector< MObject > filteredNodes;
    std::vector< FabricCore::RTVal > filteredObjects;
    std::map<std::string, size_t> pathMap;
    try
    {
      for(size_t i=0;i<m_nodes.size();i++)
      {
        if(!m_objects[i].isValid())
          continue;

        // skip double entries
        std::string path = m_objects[i].callMethod("String", "getPath", 0, NULL).getStringCString();
        if(pathMap.find(path) != pathMap.end())
          continue;
        pathMap.insert(std::pair<std::string, size_t>(path, filteredNodes.size()));

        filteredNodes.push_back(m_nodes[i]);
        filteredObjects.push_back(m_objects[i]);
      }
    }
    catch(FabricSplice::Exception e)
    {
      mayaLogErrorFunc(MString(getName()) + ": "+e.what());
      return mayaErrorOccured();
    }

    m_nodes = filteredNodes;
    m_objects = filteredObjects;

    if(m_nodes.size() == 0)
    {
      mayaLogFunc(MString(getName())+": Warning: nothing to export.");
      return mayaErrorOccured();
    }
  }

  // create a second array of refs
  FabricCore::RTVal refs;
  try
  {
    refs = FabricCore::RTVal::Construct(context, "Ref<ImporterObject>[]", 0, 0);
    for(size_t i=0;i<m_objects.size();i++)
      refs.callMethod("", "push", 1, &m_objects[i]);
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
    return mayaErrorOccured();
  }

  // compute the time step
  double tStep = 1.0;
  if(m_settings.fps > 0.0)
    tStep = 1.0 / m_settings.fps;
  if(m_settings.substeps > 0)
    tStep /= double(m_settings.substeps);

  // loop over all of the frames required,
  // convert all of the objects,
  // set both the objects and the context
  // and execute the binding
  for(double t = m_settings.startTime; t <= m_settings.endTime; t += tStep)
  {
    MAnimControl::setCurrentTime(MTime(t, MTime::kSeconds));

    for(size_t i=0;i<m_objects.size();i++)
    {
      updateRTValForNode(t, m_nodes[i], m_objects[i]);
    }

    try
    {
      binding.setArgValue("context", m_context);
      binding.setArgValue("objects", refs);
      binding.execute();
    }
    catch(FabricSplice::Exception e)
    {
      mayaLogErrorFunc(MString(getName()) + ": "+e.what());

      if(binding.isValid())
      {
        MString errors = binding.getErrors(true).getCString();
        mayaLogErrorFunc(errors);
      }
      return mayaErrorOccured();
    }
  }

  mayaLogFunc(MString(getName())+": export done.");
  return MS::kSuccess;
}

FabricCore::RTVal FabricExportPatternCommand::createRTValForNode(const MObject & node)
{
  if(node.isNull())
    return FabricCore::RTVal();

  MFnDagNode dagNode(node);
  MString typeName = dagNode.typeName();

  MDagPathArray dagPaths;
  MDagPath::getAllPathsTo(node, dagPaths);
  if(dagPaths.length() == 0)
  {
    mayaLogFunc(MString(getName())+": Warning: Node '"+dagNode.name()+"' is not part of the dag.");
    return FabricCore::RTVal();
  }

  MDagPath dagPath = dagPaths[0];
  MString path = getPathFromDagPath(dagPath);

  FabricCore::Context context = m_context.getContext();

  FabricCore::RTVal val;
  try
  {
    if(typeName == L"mesh") // or curves or points
    {
      std::vector<FabricCore::RTVal> args(3);
      args[0] = FabricCore::RTVal::ConstructString(context, "Shape");
      args[1] = FabricCore::RTVal::ConstructString(context, path.asChar());
      args[2] = args[1];
      val = m_importer.callMethod("ImporterShape", "getOrCreateObject", args.size(), &args[0]);

      FabricCore::RTVal geometryTypeVal = FabricCore::RTVal::ConstructSInt32(context, 0); //ImporterShape_Mesh = 0
      val.callMethod("", "setGeometryType", 1, &geometryTypeVal);
    }
    else if(typeName == L"camera")
    {
      mayaLogFunc(MString(getName())+": Warning: '"+dagNode.name()+"' is a camera - to be implemented.");
    }
    else if(typeName == L"pointlight" ||
      typeName == L"spotlight" ||
      typeName == L"directionallight")
    {
      mayaLogFunc(MString(getName())+": Warning: '"+dagNode.name()+"' is a light - to be implemented.");
    }
    else if(typeName == L"transform")
    {
      MString objType = "Transform";

      // look at shape
      MDagPath shapeDagPath = dagPath;
      if(MS::kSuccess == shapeDagPath.extendToShape())
      {
        MFnDagNode shapeDagNode(shapeDagPath.node());

        // rewire to the first dag path
        // to identify instances
        MDagPathArray shapeDagPaths;
        MDagPath::getAllPathsTo(node, shapeDagPaths);
        if(shapeDagPaths.length() > 1)
          objType = L"Instance";

        // push the shape so that we will process it
        m_nodes.push_back(shapeDagNode.object());
      }

      std::vector<FabricCore::RTVal> args(3);
      args[0] = FabricCore::RTVal::ConstructString(context, objType.asChar());
      args[1] = FabricCore::RTVal::ConstructString(context, path.asChar());
      args[2] = args[1];
      val = m_importer.callMethod("ImporterObject", "getOrCreateObject", args.size(), &args[0]);
    }
    else
    {
      mayaLogFunc(MString(getName())+": Warning: Type '"+dagNode.typeName()+"' of '"+dagNode.name()+"' is not yet supported.");
    }
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
    return FabricCore::RTVal();
  }

  // visit all children
  for(unsigned int i=0;i<dagNode.childCount();i++)
  {
    MStatus childStatus;
    MFnDagNode childDagNode(dagNode.child(i), &childStatus);
    if(childStatus == MS::kSuccess)
      m_nodes.push_back(childDagNode.object());
  }

  return val;
}

bool FabricExportPatternCommand::updateRTValForNode(double t, const MObject & node, FabricCore::RTVal & object)
{
  bool isStart = t <= m_settings.startTime;

  try
  {
    FabricCore::Context context = m_context.getContext();

    MString objectType = object.callMethod("String", "getType", 0, 0).getStringCString();
    MString objectPath = object.callMethod("String", "getPath", 0, 0).getStringCString();

    if(objectType == "Transform")
    {
      MFnTransform transformNode(node);
      MMatrix localMatrix = transformNode.transformation().asMatrix();
      
      FabricCore::RTVal matrixVal = FabricCore::RTVal::Construct(context, "Mat44", 0, 0);
      MMatrixToMat44(localMatrix, matrixVal);

      FabricCore::RTVal transform = FabricCore::RTVal::Create(context, "ImporterTransform", 1, &object);
      transform.callMethod("", "setLocalTransform", 1, &matrixVal);
    }
    else if(objectType == "Camera")
    {
      mayaLogFunc(MString(getName())+": Warning: Lights still need to be implemented");
      return false;
    }
    else if(objectType == "Light")
    {
      mayaLogFunc(MString(getName())+": Warning: Lights still need to be implemented");
      return false;
    }
    else if(objectType == "Shape")
    {
      mayaLogFunc(MString(getName())+": Warning: Shapes still need to be implemented");
      return false;
    }
    else
    {
      mayaLogFunc(MString(getName())+": Warning: Object type 'Importer"+objectType+"' is not yet supported.");
      return false;
    }
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
    return false;
  }

  return false;
}

MString FabricExportPatternCommand::getPathFromDagPath(MDagPath dagPath)
{
  std::string path = dagPath.fullPathName().asChar();
  for(size_t i=0;i<path.size();i++)
  {
    if(path[i] == '|')
      path[i] = '/';
  }
  return path.c_str();
}
