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
#include "FabricProgressbarDialog.h"

#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MQtUtil.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagModifier.h>
#include <maya/MItDag.h>
#include <maya/MFnTransform.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnLambertShader.h>
#include <maya/MCommandResult.h>
#include <maya/MAnimControl.h>
#include <maya/MFileObject.h>

#include <FTL/FS.h>

MSyntax FabricExportPatternCommand::newSyntax()
{
  MSyntax syntax;
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  syntax.addFlag( "-f", "-filepath", MSyntax::kString );
  syntax.addFlag( "-q", "-disableDialogs", MSyntax::kBoolean );
  syntax.addFlag( "-m", "-scale", MSyntax::kDouble );
  syntax.addFlag( "-a", "-args", MSyntax::kString );
  syntax.addFlag( "-o", "-objects", MSyntax::kString );
  syntax.addFlag( "-b", "-begin", MSyntax::kDouble );
  syntax.addFlag( "-s", "-stop", MSyntax::kDouble );
  syntax.addFlag( "-r", "-framerate", MSyntax::kDouble );
  syntax.addFlag( "-t", "-substeps", MSyntax::kLong );
  syntax.addFlag( "-u", "-userAttributes", MSyntax::kBoolean );
  syntax.addFlag( "-x", "-stripNameSpaces", MSyntax::kBoolean );
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

  if(!quiet)
  {
    if(MGlobal::mayaState() != MGlobal::kInteractive)
      quiet = true;
  }

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

  m_settings.quiet = quiet;
  m_settings.filePath = filepath;

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
  else
    m_settings.fps = 24.0;

  // initialize substeps, framerate + in and out based on playcontrol
  m_settings.startFrame = MAnimControl::minTime().as(MTime::kSeconds) * m_settings.fps;
  m_settings.endFrame = MAnimControl::maxTime().as(MTime::kSeconds) * m_settings.fps;

  if( argParser.isFlagSet("scale") )
  {
    m_settings.scale = argParser.flagArgumentDouble("scale", 0);
  }

  if( argParser.isFlagSet("begin") )
  {
    if(!argParser.isFlagSet("stop"))
    {
      mayaLogErrorFunc("When specifying -begin you also need to specify -stop.");
      return mayaErrorOccured();
    }
    m_settings.startFrame = argParser.flagArgumentDouble("begin", 0);
  }
  if( argParser.isFlagSet("stop") )
  {
    if(!argParser.isFlagSet("stop"))
    {
      mayaLogErrorFunc("When specifying -stop you also need to specify -begin.");
      return mayaErrorOccured();
    }
    m_settings.endFrame = argParser.flagArgumentDouble("stop", 0);
  }
  if( argParser.isFlagSet("framerate") )
  {
    m_settings.fps = argParser.flagArgumentDouble("framerate", 0);
    if(m_settings.fps <= 0.0)
      m_settings.fps = 24.0;
  }

  if( argParser.isFlagSet("substeps") )
  {
    m_settings.substeps = argParser.flagArgumentInt("substeps", 0);
  }
  if( argParser.isFlagSet("userAttributes") )
  {
    m_settings.userAttributes = argParser.flagArgumentBool("userAttributes", 0);
  }
  if( argParser.isFlagSet("stripNameSpaces") )
  {
    m_settings.stripNameSpaces = argParser.flagArgumentBool("stripNameSpaces", 0);
  }

  if( argParser.isFlagSet("objects") )
  {
    MString objects = argParser.flagArgumentString("objects", 0);
    if(objects.length() > 0)
    {
      if(objects.split(',', m_settings.objects) != MS::kSuccess)
        m_settings.objects.append(objects);
    }
  }

  MStringArray result;
  FabricCore::DFGBinding binding;

  try
  {
    FabricCore::Client client = FabricDFGWidget::GetCoreClient();
    client.loadExtension("AssetPatterns", "", false);

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
          if(type != "String" && type != "FilePath" && !FabricCore::GetRegisteredTypeIsShallow(client, type.c_str()))
          {
            mayaLogFunc(MString(getName())+": Warning: Argument "+MString(name.c_str())+" cannot be set since "+MString(type.c_str())+" is not shallow.");
            continue;
          }

          std::string json = it->value()->encode();
          FabricCore::RTVal value;
          if(type == "FilePath")
          {
            if(json.length() > 2)
            {
              if(json[0] == '"')
                json = json.substr(1);
              if(json[json.length()-1] == '"')
                json = json.substr(0, json.length()-1);
            }

            FabricCore::RTVal jsonVal = FabricCore::RTVal::ConstructString(client, json.c_str());
            value = FabricCore::RTVal::Construct(client, "FilePath", 1, &jsonVal);
          }
          else
          {
            value = FabricCore::ConstructRTValFromJSON(client, type.c_str(), json.c_str());
          }
          binding.setArgValue(name.c_str(), value);
          m_settings.useLastArgValues = false;
          found = true;
          break;
        }

        if(!found)
        {
          mayaLogFunc(MString(getName())+": Argument "+MString(key.c_str())+" does not exist in export pattern.");
          return mayaErrorOccured();
        }
      }

      return invoke(client, binding, m_settings);
    }
    else if(!quiet)
    {
      QWidget * mainWindow = MQtUtil().mainWindow();
      mainWindow->setFocus(Qt::ActiveWindowFocusReason);

      QEvent event(QEvent::RequestSoftwareInputPanel);
      QApplication::sendEvent(mainWindow, &event);

      FabricExportPatternDialog * dialog = new FabricExportPatternDialog(mainWindow, client, binding, m_settings);
      dialog->setWindowModality(Qt::ApplicationModal);
      dialog->setSizeGripEnabled(true);
      dialog->open();

      if(!dialog->wasAccepted())
      {
        cleanup(binding);
        return MS::kSuccess;
      }
    }
    else
    {
      return invoke(client, binding, m_settings);
    }
  }
  catch (FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": " + e.what());
  }
  catch (FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": " + e.getDesc_cstr());
  }

  return MS::kSuccess;
}

MStatus FabricExportPatternCommand::invoke(FabricCore::Client client, FabricCore::DFGBinding binding, const FabricExportPatternSettings & settings)
{
  m_client = client;
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

    // if there's nothing selected - export everything
    if(sl.length() == 0)
    {
      MObject rootObj = MItDag().root();
      MFnDagNode rootNode(rootObj);
      for(unsigned int i=0;i<rootNode.childCount();i++)
      {
        MStatus dagStatus;
        MFnDagNode dagNode(rootNode.child(i), &dagStatus);
        if(dagStatus != MS::kSuccess)
          continue;
        sl.add(dagNode.object());
      }
    }
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
    MFnDagNode dagNode(node);
    MString prefix;
    getParentDagNode(node, &prefix);
    registerNode(node, prefix.asChar());
  }

  // compute the time step
  double tStep = 1.0;
  if(m_settings.substeps > 0)
    tStep /= double(m_settings.substeps);
  std::vector<double> timeSamples;
  for(double t = settings.startFrame; t <= settings.endFrame; t += tStep)
    timeSamples.push_back(t / m_settings.fps); // store as seconds always

  try
  {
    m_client.loadExtension("AssetPatterns", "", false);

    m_exporterContext = FabricCore::RTVal::Construct(m_client, "ExporterContext", 0, 0);
    FabricCore::RTVal contextHost = m_exporterContext.maybeGetMember("host");
    MString mayaVersion;
    mayaVersion.set(MAYA_API_VERSION);
    contextHost.setMember("name", FabricCore::RTVal::ConstructString(m_client, "Maya"));
    contextHost.setMember("version", FabricCore::RTVal::ConstructString(m_client, mayaVersion.asChar()));
    m_exporterContext.setMember("host", contextHost);

    FabricCore::RTVal timeSamplesVal = FabricCore::RTVal::ConstructVariableArray(m_client, "Float32");
    FabricCore::RTVal countVal = FabricCore::RTVal::ConstructUInt32(m_client, (unsigned int)timeSamples.size());
    timeSamplesVal.callMethod("", "resize", 1, &countVal);
    FabricCore::RTVal timeSamplesData = timeSamplesVal.callMethod("Data", "data", 0, 0);
    float * timeSamplesPtr = (float*)timeSamplesData.getData();
    for(size_t i=0;i<timeSamples.size();i++)
      timeSamplesPtr[i] = (float)timeSamples[i];

    m_exporterContext.setMember("timeSamples", timeSamplesVal);

    FabricCore::RTVal willCallUpdateForEachTimeSampleVal = FabricCore::RTVal::ConstructBoolean(m_client, true);
    m_exporterContext.setMember("willCallUpdateForEachTimeSample", willCallUpdateForEachTimeSampleVal);

    m_importer = FabricCore::RTVal::Create(m_client, "BaseImporter", 0, 0);
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
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
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
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
    refs = FabricCore::RTVal::Construct(m_client, "Ref<ImporterObject>[]", 0, 0);
    for(size_t i=0;i<m_objects.size();i++)
      refs.callMethod("", "push", 1, &m_objects[i]);
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
    cleanup(binding);
    return mayaErrorOccured();
  }

  FabricProgressbarDialog * prog = NULL;
  if(!m_settings.quiet)
  {
    QWidget * mainWindow = MQtUtil().mainWindow();
    mainWindow->setFocus(Qt::ActiveWindowFocusReason);

    QEvent event(QEvent::RequestSoftwareInputPanel);
    QApplication::sendEvent(mainWindow, &event);

    MString title = "Exporting...";
    prog = new FabricProgressbarDialog(mainWindow, title.asChar(), (int)timeSamples.size());
    prog->setWindowModality(Qt::ApplicationModal);
    prog->setSizeGripEnabled(true);
    prog->open();
  }

  // loop over all of the frames required,
  // convert all of the objects,
  // set both the objects and the context
  // and execute the binding
  for(size_t t=0;t<timeSamples.size();t++)
  {
    MAnimControl::setCurrentTime(MTime(timeSamples[t], MTime::kSeconds));

    for(size_t i=0;i<m_objects.size();i++)
    {
      updateRTValForNode(timeSamples[t], m_nodes[i], m_objects[i]);
    }

    try
    {
      FabricCore::RTVal sampleIndexVal = FabricCore::RTVal::ConstructUInt32(m_client, (unsigned int)t);
      m_exporterContext.callMethod("", "stepToTimeSample", 1, &sampleIndexVal);

      binding.setArgValue("context", m_exporterContext);
      binding.setArgValue("objects", refs);
      binding.execute();
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());

      if(binding.isValid())
      {
        MString errors = binding.getErrors(true).getCString();
        mayaLogErrorFunc(errors);
      }
      if(prog)
        prog->close();
      cleanup(binding);
      return mayaErrorOccured();
    }

    if(prog)
    {
      if(prog->wasCancelPressed())
      {
        prog->close();
        mayaLogFunc(MString(getName())+": aborted by user.");
        cleanup(binding);
        return MS::kFailure;
      }
      prog->increment();
      QApplication::processEvents();
    }
    else
    {
      MString index, total;
      index.set((int)t+1);
      total.set((int)timeSamples.size());
      mayaLogFunc("Exported timesample "+index+" of "+total);
    }
  }

  if(prog)
    prog->close();


  mayaLogFunc(MString(getName())+": export finished.");
  cleanup(binding);
  return MS::kSuccess;
}

void FabricExportPatternCommand::cleanup(FabricCore::DFGBinding binding)
{
  binding.setNotificationCallback(NULL, NULL);
  binding.deallocValues();
}

bool FabricExportPatternCommand::registerNode(const MObject & node, MString prefix, bool addChildren)
{
  MStatus status;
  MFnDagNode dagNode(node, &status);
  if(status != MS::kSuccess)
    return false;
  if(dagNode.isIntermediateObject())
    return false;

  MString path = dagNode.name().asChar();
  if(prefix.length() > 0)
    path = prefix + "|" + path;

  if(m_nodePaths.find(path.asChar()) != m_nodePaths.end())
    return false;

  // filter out texture reference objects
  MDagPath dagPath;
  dagNode.getPath(dagPath);
  dagPath.extendToShape();
  MFnDependencyNode shapeNode(dagPath.node());
  MPlug messagePlug = shapeNode.findPlug("message");
  if(!messagePlug.isNull())
  {
    MPlugArray connections;
    messagePlug.connectedTo(connections, false, true);
    for(unsigned int i=0;i<connections.length();i++)
    {
      MPlug connection = connections[i];
      if(connection.partialName(false /*includeNodeName*/) == "rob") // referenceObject
      {
        MFnDependencyNode connectedNode(connection.node());
        if(connectedNode.typeName() == shapeNode.typeName())
        {
          return false;
        }
      }
    }
  }
  MStatus parentStatus;

  MObject parentNode = getParentDagNode(node);
  if(!parentNode.isNull())
  {
    MString grandParentPrefix = prefix;
    int pipeIndex = grandParentPrefix.rindex('|');
    if(pipeIndex > 1)
      grandParentPrefix = grandParentPrefix.substring(0, pipeIndex - 1);
    else
      grandParentPrefix = "";
    registerNode(parentNode, grandParentPrefix, false /* addChildren */);
  }


  m_nodePaths.insert(std::pair< std::string, size_t >(path.asChar(), m_nodes.size()));
  m_nodes.push_back(node);

  if(addChildren)
  {
    for(unsigned int i=0;i<dagNode.childCount();i++)
      registerNode(dagNode.child(i), path);
  }

  return true;
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

  if(m_objectPaths.find(path.asChar()) != m_objectPaths.end())
    return FabricCore::RTVal();
  m_objectPaths.insert(std::pair< std::string, size_t >(path.asChar(), m_objects.size()));

  FabricCore::Context context = m_exporterContext.getContext();

  FabricCore::RTVal val;
  try
  {
    if(typeName == L"mesh" ||
      typeName == L"nurbsCurve") // or points
    {
      FabricCore::RTVal args[3];
      args[0] = FabricCore::RTVal::ConstructString(m_client, "Shape");
      args[1] = FabricCore::RTVal::ConstructString(m_client, path.asChar());
      args[2] = args[1];
      val = m_importer.callMethod("ImporterShape", "getOrCreateObject", 3, &args[0]);

      FabricCore::RTVal geometryTypeVal;
      if(typeName == L"mesh")
        geometryTypeVal = FabricCore::RTVal::ConstructSInt32(m_client, 0); //ImporterShape_Mesh = 0
      else if(typeName == L"nurbsCurve")
        geometryTypeVal = FabricCore::RTVal::ConstructSInt32(m_client, 1); //ImporterShape_Curves = 1
      val.callMethod("", "setGeometryType", 1, &geometryTypeVal);
    }
    else if(typeName == L"camera")
    {
      FabricCore::RTVal args[3];
      args[0] = FabricCore::RTVal::ConstructString(m_client, "Camera");
      args[1] = FabricCore::RTVal::ConstructString(m_client, path.asChar());
      args[2] = args[1];
      val = m_importer.callMethod("ImporterCamera", "getOrCreateObject", 3, &args[0]);
    }
    else if(typeName == L"pointlight" ||
      typeName == L"spotlight" ||
      typeName == L"directionallight")
    {
      mayaLogFunc(MString(getName())+": Warning: '"+dagNode.name()+"' is a light - to be implemented.");
    }
    else if(typeName == L"world" ||
      typeName == L"pointConstraint" ||
      typeName == L"orientConstraint" ||
      typeName == L"scaleConstraint" ||
      typeName == L"parentConstraint" ||
      typeName == L"aimConstraint" ||
      typeName == L"poleVectorConstraint" ||
      typeName == L"distanceDimShape"
      )
    {
      // ignore those
      return FabricCore::RTVal();
    }
    else if(typeName == L"transform" ||
      typeName == L"locator" ||
      typeName == L"joint" ||
      typeName == L"ikEffector" ||
      typeName == L"ikHandle"
      )
    {
      MString objType = "Transform";

      FabricCore::RTVal args[3];
      args[0] = FabricCore::RTVal::ConstructString(m_client, objType.asChar());
      args[1] = FabricCore::RTVal::ConstructString(m_client, path.asChar());
      args[2] = args[1];
      val = m_importer.callMethod("ImporterObject", "getOrCreateObject", 3, &args[0]);
    }
    else
    {
      mayaLogFunc(MString(getName())+": Warning: Type '"+dagNode.typeName()+"' of '"+dagNode.name()+"' is not yet supported.");
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
    return FabricCore::RTVal();
  }

  return val;
}

bool FabricExportPatternCommand::updateRTValForNode(double t, const MObject & node, FabricCore::RTVal & object)
{
  bool isStart = t <= (m_settings.startFrame / m_settings.fps);

  try
  {
    if(!object.isValid())
      return false;

    FabricCore::Context context = m_exporterContext.getContext();

    MString objectType = object.callMethod("String", "getType", 0, 0).getStringCString();
    MString objectPath = object.callMethod("String", "getPath", 0, 0).getStringCString();

    if(objectType == "Transform")
    {
      FabricCore::RTVal transform = FabricCore::RTVal::Create(m_client, "ImporterTransform", 1, &object);
      if(!transform.isValid())
        return false;
      if(transform.isNullObject())
        return false;

      MFnTransform transformNode(node);
      MMatrix localMatrix = transformNode.transformation().asMatrix();
      
      FabricCore::RTVal matrixVal = FabricCore::RTVal::Construct(m_client, "Mat44", 0, 0);
      MMatrixToMat44(localMatrix, matrixVal);

      // make sure to also mark the property as varying
      // todo: figure out if it is changing over time
      FabricCore::RTVal arg;
      arg = FabricCore::RTVal::ConstructString(m_client, "localTransform");
      transform.callMethod("", "setPropertyVarying", 1, &arg);

      // now set the transform
      transform.callMethod("", "setLocalTransform", 1, &matrixVal);
    }
    else if(objectType == "Camera")
    {
      FabricCore::RTVal camera = FabricCore::RTVal::Create(m_client, "ImporterCamera", 1, &object);
      if(!camera.isValid())
        return false;
      if(camera.isNullObject())
        return false;

      MFnCamera cameraNode(node);

      FabricCore::RTVal args[2];

      args[0] = FabricCore::RTVal::ConstructString(m_client, "fovY");
      args[1] = FabricCore::RTVal::ConstructFloat32(m_client, (float)cameraNode.verticalFieldOfView());
      camera.callMethod("", "setProperty", 2, args);

      args[0] = FabricCore::RTVal::ConstructString(m_client, "focalLength");
      args[1] = FabricCore::RTVal::ConstructFloat32(m_client, (float)cameraNode.focalLength());
      camera.callMethod("", "setProperty", 2, args);

      args[0] = FabricCore::RTVal::ConstructString(m_client, "focusDistance");
      args[1] = FabricCore::RTVal::ConstructFloat32(m_client, (float)cameraNode.focusDistance());
      camera.callMethod("", "setProperty", 2, args);

      args[0] = FabricCore::RTVal::ConstructString(m_client, "near");
      args[1] = FabricCore::RTVal::ConstructFloat32(m_client, (float)cameraNode.nearClippingPlane());
      camera.callMethod("", "setProperty", 2, args);

      args[0] = FabricCore::RTVal::ConstructString(m_client, "far");
      args[1] = FabricCore::RTVal::ConstructFloat32(m_client, (float)cameraNode.farClippingPlane());
      camera.callMethod("", "setProperty", 2, args);
    }
    else if(objectType == "Light")
    {
      mayaLogFunc(MString(getName())+": Warning: Lights still need to be implemented");
      return false;
    }
    else if(objectType == "Shape")
    {
      FabricCore::RTVal shape = FabricCore::RTVal::Create(m_client, "ImporterShape", 1, &object);
      if(!shape.isValid())
        return false;
      if(shape.isNullObject())
        return false;

      int geometryType = shape.callMethod("SInt32", "getGeometryType", 1, &m_exporterContext).getSInt32();
      switch(geometryType)
      {
        case 0: // ImporterShape_Mesh
        {
          MFnDagNode shapeNode(node);

          // check if the mesh has a deformer
          // if not let's exit if this is not the first frame...!
          bool isDeforming = isShapeDeforming(shape, node, isStart);
          if((!isStart) && (!isDeforming))
            return true;

          MPlug meshPlug = shapeNode.findPlug("outMesh");
          if(meshPlug.isNull())
          {
            mayaLogFunc(MString(getName())+": Warning: Node "+MString(shapeNode.fullPathName())+") does not have a 'mesh' plug.");
            return false;
          }

          MObject meshObj;
          meshPlug.getValue(meshObj);
          MStatus meshStatus;
          MFnMesh meshData(meshObj, &meshStatus);
          if(meshStatus != MS::kSuccess)
          {
            mayaLogFunc(MString(getName())+": Warning: Node "+MString(shapeNode.fullPathName())+" does not have a valid 'mesh' plug.");
            return false;
          }

          FabricCore::RTVal meshVal = shape.callMethod("PolygonMesh", "getGeometry", 1, &m_exporterContext);
          if(meshVal.isNullObject())
          {
            meshVal = FabricCore::RTVal::Create(m_client, "PolygonMesh", 0, 0);
            shape.callMethod("", "setGeometry", 1, &meshVal);
          }
          
          if(dfgMFnMeshToPolygonMesh(meshData, meshVal).isNullObject())
            return false;

          // look for texture references
          if(isStart)
          {
            MPlug refObjectPlug = shapeNode.findPlug("referenceObject");
            if(!refObjectPlug.isNull())
            {
              MPlugArray connections;
              refObjectPlug.connectedTo(connections, true, false);
              for(unsigned int i=0;i<connections.length();i++)
              {
                MPlug connection = connections[i];
                MFnDependencyNode connectedNode(connection.node());

                if(connectedNode.typeName() == shapeNode.typeName())
                {
                  FabricCore::RTVal refObjectMeshVal = FabricCore::RTVal::Create(m_client, "PolygonMesh", 0, 0);
                  MPlug refObjectMeshPlug = connectedNode.findPlug("outMesh");
                  MObject refObjectMeshObj;
                  refObjectMeshPlug.getValue(refObjectMeshObj);
                  MFnMesh refObjectMeshData(refObjectMeshObj);

                  if(!dfgMFnMeshToPolygonMesh(refObjectMeshData, refObjectMeshVal).isNullObject())
                  {
                    meshVal.callMethod("", "setTextureReference", 1, &refObjectMeshVal);
                  }
                }
              }
            }
          }
          break;
        }
        case 1: // ImporterShape_Curves
        {
          MFnDagNode shapeNode(node);

          // check if the curves has a deformer
          // if not let's exit if this is not the first frame...!
          bool isDeforming = isShapeDeforming(shape, node, isStart);
          if((!isStart) && (!isDeforming))
            return true;

          MPlug localPlug = shapeNode.findPlug("local");
          if(localPlug.isNull())
          {
            mayaLogFunc(MString(getName())+": Warning: Node "+MString(shapeNode.fullPathName())+") does not have a 'local' plug.");
            return false;
          }

          MObject curveObj;
          localPlug.getValue(curveObj);
          MStatus curveStatus;
          MFnNurbsCurve curveData(curveObj, &curveStatus);
          if(curveStatus != MS::kSuccess)
          {
            mayaLogFunc(MString(getName())+": Warning: Node "+MString(shapeNode.fullPathName())+" does not have a valid 'local' plug.");
            return false;
          }

          FabricCore::RTVal curvesVal = shape.callMethod("Curves", "getGeometry", 1, &m_exporterContext);
          if(curvesVal.isNullObject())
          {
            curvesVal = FabricCore::RTVal::Create(m_client, "Curves", 0, 0);

            FabricCore::RTVal curveCountVal = FabricCore::RTVal::ConstructUInt32(m_client, 1);
            curvesVal.callMethod( "", "setCurveCount", 1, &curveCountVal );

            shape.callMethod("", "setGeometry", 1, &curvesVal);
          }

          if(!dfgMFnNurbsCurveToCurves(0, curveData, curvesVal))
            return false;
          break;
        }
        default:
        {
          MString geometryTypeStr;
          geometryTypeStr.set(geometryType);
          mayaLogFunc(MString(getName())+": Warning: Geometry type "+geometryTypeStr+" not yet supported.");
          return false;
        }
      }
    }
    else if(objectType == "FaceSet")
    {
      mayaLogFunc(MString(getName())+": Warning: FaceSets still need to be implemented");
      return false;
    }
    else
    {
      mayaLogFunc(MString(getName())+": Warning: Object type 'Importer"+objectType+"' is not yet supported.");
      return false;
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
    return false;
  }

  processUserAttributes(object, node, isStart);
  return true;
}

MString FabricExportPatternCommand::getPathFromDagPath(MDagPath dagPath)
{
  MString fullPath = dagPath.fullPathName();
  MStringArray parts;
  if(fullPath.split('|', parts) != MS::kSuccess)
    parts.append(dagPath.fullPathName());

  MString result;
  for(unsigned int i=0;i<parts.length();i++)
  {
    MString part = parts[i];
    if(part.length() == 0)
      continue;    
    if(m_settings.stripNameSpaces)
    {
      if(part.rindex(':') > 0)
        part = part.substring(part.rindex(':') + 1, part.length()-1);
    }
    result += "/" + part;
  }
  return result;
}

bool FabricExportPatternCommand::isShapeDeforming(FabricCore::RTVal shapeVal, MObject node, bool isStart)
{
  MFnDagNode shapeNode(node);
  bool isDeforming = false;
  if(isStart)
  {
    MStringArray history;
    MGlobal::executeCommand("listHistory "+shapeNode.fullPathName(), history);
    if(history.length() > 0)
    {
      MString deformHistoryCmd = "ls -type \"geometryFilter\" -long";
      for(unsigned int i=0;i<history.length();i++)
        deformHistoryCmd += " \""+history[i]+"\"";

      MStringArray deformHistory;
      MGlobal::executeCommand(deformHistoryCmd, deformHistory);
      isDeforming = deformHistory.length() > 0;
    }

    if(isDeforming)
    {
      // also mark the property as varying (constant == false)
      FabricCore::RTVal arg = FabricCore::RTVal::ConstructString(m_client, "geometry");
      shapeVal.callMethod("", "setPropertyVarying", 1, &arg);
    }
  }
  else
  {
    isDeforming = !shapeVal.callMethod("Boolean", "isConstant", 1, &m_exporterContext).getBoolean();
  }
  return isDeforming;
}

void FabricExportPatternCommand::processUserAttributes(FabricCore::RTVal obj, const MObject & node, bool isStart)
{
  if(!m_settings.userAttributes)
    return;
  if(node.isNull())
    return;
  MFnDependencyNode depNode(node);

  try
  {
    FabricCore::RTVal userProperties = FabricCore::RTVal::ConstructVariableArray(m_client, "String");

    for(unsigned int i=0;i<depNode.attributeCount();i++)
    {
      MObject attribute = depNode.attribute(i);
      if(depNode.attributeClass(attribute) != MFnDependencyNode::kLocalDynamicAttr)
        continue;

      MString name = MFnAttribute(attribute).name();
      FabricCore::RTVal nameVal = FabricCore::RTVal::ConstructString(m_client, name.asChar());
      FabricCore::RTVal args[2];
      args[0] = nameVal;

      MPlug plug(node, attribute);

      MStatus castStatus;

      MFnNumericAttribute numericAttr(attribute, &castStatus);
      if(castStatus == MS::kSuccess)
      {
        switch(numericAttr.unitType())
        {
          case MFnNumericData::kBoolean:
          {
            bool value;
            plug.getValue(value);
            args[1] = FabricCore::RTVal::ConstructBoolean(m_client, value);
            obj.callMethod("", "setProperty", 2, args);
            break;
          }
          case MFnNumericData::kInt:
          {
            int value;
            plug.getValue(value);
            args[1] = FabricCore::RTVal::ConstructSInt32(m_client, value);
            obj.callMethod("", "setProperty", 2, args);
            break;
          }
          case MFnNumericData::kFloat:
          {
            float value;
            plug.getValue(value);
            args[1] = FabricCore::RTVal::ConstructFloat32(m_client, value);
            obj.callMethod("", "setProperty", 2, args);
            break;
          }
          case MFnNumericData::kDouble:
          {
            double value;
            plug.getValue(value);
            args[1] = FabricCore::RTVal::ConstructFloat32(m_client, (float)value);
            obj.callMethod("", "setProperty", 2, args);
            break;
          }
          default:
          {
            if(isStart)
              mayaLogFunc(MString(getName())+": Warning: User attribute '"+name+"'' is not supported.");
            continue;
          }
        }

        userProperties.callMethod("", "push", 1, &nameVal);
        continue;
      }

      MFnTypedAttribute typedAttr(attribute, &castStatus);
      if(castStatus == MS::kSuccess)
      {
        switch(typedAttr.attrType())
        {
          case MFnData::kString:
          {
            MString value;
            plug.getValue(value);
            args[1] = FabricCore::RTVal::ConstructString(m_client, value.asChar());
            obj.callMethod("", "setProperty", 2, args);
            break;
          }
          default:
          {
            if(isStart)
              mayaLogFunc(MString(getName())+": Warning: User attribute '"+name+"'' is not supported.");
            continue;
          }
        }

        userProperties.callMethod("", "push", 1, &nameVal);
        continue;
      }

      if(isStart)
        mayaLogFunc(MString(getName())+": Warning: User attribute '"+name+"'' is not supported.");
    }

    if(userProperties.getArraySize() > 0)
    {
      obj.callMethod("", "setUserProperties", 1, &userProperties);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
  }
}

MObject FabricExportPatternCommand::getParentDagNode(MObject node, MString * parentPrefix)
{
  MStatus status;

  if(parentPrefix != NULL)
    (*parentPrefix) = "";

  MFnDagNode dagNode(node, &status);
  if(status != MS::kSuccess)
    return MObject();

  MDagPathArray dagPaths;
  MDagPath::getAllPathsTo(node, dagPaths);
  if(dagPaths.length() == 0)
    return MObject();

  MDagPath dagPath = dagPaths[0];
  if(dagPath.pop() != MS::kSuccess)
    return MObject();

  if(parentPrefix != NULL)
  {
    (*parentPrefix) = dagPath.fullPathName();
    int pipeIndex = (*parentPrefix).rindex('|');
    if(pipeIndex > 0)
      (*parentPrefix) = (*parentPrefix).substring(0, pipeIndex - 1);
    else
      (*parentPrefix) = "";
  }

  return dagPath.node();
}
