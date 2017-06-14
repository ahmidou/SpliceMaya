//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QFileDialog>
#include <QDialog>

#include "Foundation.h"
#include "FabricImportPatternCommand.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGWidget.h"
#include "FabricDFGConversion.h"
#include "FabricImportPatternDialog.h"
#include "FabricProgressbarDialog.h"

#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MQtUtil.h>
#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagModifier.h>
#include <maya/MFnTransform.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnLambertShader.h>
#include <maya/MCommandResult.h>

#include <FTL/FS.h>

MSyntax FabricImportPatternCommand::newSyntax()
{
  MSyntax syntax;
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  syntax.addFlag( "-f", "-filepath", MSyntax::kString );
  syntax.addFlag( "-q", "-disableDialogs", MSyntax::kBoolean );
  syntax.addFlag( "-k", "-namespace", MSyntax::kString );
  syntax.addFlag( "-x", "-attachToExisting", MSyntax::kBoolean );
  syntax.addFlag( "-m", "-enableMaterials", MSyntax::kBoolean );
  syntax.addFlag( "-s", "-scale", MSyntax::kDouble );
  syntax.addFlag( "-n", "-canvasnode", MSyntax::kString );
  syntax.addFlag( "-r", "-root", MSyntax::kString );
  syntax.addFlag( "-a", "-args", MSyntax::kString );
  syntax.addFlag( "-g", "-geometries", MSyntax::kString );
  return syntax;
}

void* FabricImportPatternCommand::creator()
{
  return new FabricImportPatternCommand;
}

MStatus FabricImportPatternCommand::doIt(const MArgList &args)
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

  FabricDFGBaseInterface * interf = NULL;
  MString filepath;

  if ( argParser.isFlagSet("canvasnode") )
  {
    MString canvasnode = argParser.flagArgumentString("canvasnode", 0);
    interf = FabricDFGBaseInterface::getInstanceByName(canvasnode.asChar());
    if(interf == NULL)
    {
      mayaLogErrorFunc("Canvas node \""+canvasnode+"\" does no exist.");
      return mayaErrorOccured();
    }
  }

  if(interf == NULL)
  {
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
  }

  m_settings.quiet = quiet;
  m_settings.filePath = filepath;

  if ( argParser.isFlagSet("root") )
  {
    m_settings.rootPrefix = argParser.flagArgumentString("root", 0);
    if(m_settings.rootPrefix.length() > 0)
    {
      if(m_settings.rootPrefix.asChar()[m_settings.rootPrefix.length()-1] != '/')
        m_settings.rootPrefix += "/";
    }
  }

  if( argParser.isFlagSet("namespace") )
  {
    m_settings.nameSpace = argParser.flagArgumentString("namespace", 0);
    if(m_settings.nameSpace.length() > 0)
    {
      MString suffix = m_settings.nameSpace.substring(m_settings.nameSpace.length()-2, m_settings.nameSpace.length()-1);
      if(suffix != L"::")
        m_settings.nameSpace = m_settings.nameSpace + L"::";
    }
  }
  if( argParser.isFlagSet("scale") )
  {
    m_settings.scale = argParser.flagArgumentDouble("scale", 0);
  }
  if( argParser.isFlagSet("enableMaterials") )
  {
    m_settings.enableMaterials = argParser.flagArgumentBool("enableMaterials", 0);
  }
  if( argParser.isFlagSet("attachToExisting") )
  {
    m_settings.attachToExisting = argParser.flagArgumentBool("attachToExisting", 0);
  }

  MStringArray result;
  FabricCore::DFGBinding binding;

  if(interf != NULL)
  {
    try
    {
      m_client = interf->getCoreClient();
      m_client.loadExtension("AssetPatterns", "", false);
      binding = interf->getDFGBinding();
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
    }

    return invoke(m_client, binding, m_settings);
  }
  else
  {
    try
    {
      m_client = FabricDFGWidget::GetCoreClient();
      m_client.loadExtension("AssetPatterns", "", false);
  
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

      FabricCore::DFGHost dfgHost = m_client.getDFGHost();
      binding = dfgHost.createBindingFromJSON(json.asChar());

      std::map< std::string, MObject > geometryObjects;
      if ( argParser.isFlagSet("geometries") )
      {
        FabricCore::DFGExec exec = binding.getExec();

        MString geometries = argParser.flagArgumentString("geometries", 0);

        FTL::JSONStrWithLoc geometrySrcWithLoc( FTL::StrRef( geometries.asChar(), geometries.length() ) );
        FTL::OwnedPtr<FTL::JSONValue const> geometryValue( FTL::JSONValue::Decode( geometrySrcWithLoc ) );
        FTL::JSONObject const *geometryObject = geometryValue->cast<FTL::JSONObject>();

        for ( FTL::JSONObject::const_iterator it = geometryObject->begin(); it != geometryObject->end(); ++it )
        {
          FTL::CStrRef key = it->key();

          if(!it->value()->isString())
          {
            mayaLogFunc(MString(getName())+": Warning: Provided geometry "+MString(key.c_str())+" is not a string - needs to be the name of the geometry in the scene.");
            continue;
          }
          
          bool found = false;
          for(unsigned int i=0;i<exec.getExecPortCount();i++)
          {
            if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
              continue;

            FTL::CStrRef name = exec.getExecPortName(i);
            if(name != key)
              continue;

            FTL::CStrRef type = exec.getExecPortResolvedType(i);
            if(type != "PolygonMesh" && type != "Curves")
            {
              mayaLogFunc(MString(getName())+": Warning: Geometry "+MString(name.c_str())+" cannot be set since the canvas port is not valid geometry type.");
              continue;
            }

            MString geometryName = it->value()->getStringValue().c_str();
            MSelectionList sl;
            sl.add(geometryName);
            if(sl.length() == 0)
            {
              mayaLogErrorFunc("Provided geometry "+geometryName+" cannot be found in scene.");
              return mayaErrorOccured();
            }

            MDagPath dag;
            sl.getDagPath(0, dag);
            dag.extendToShape();
            MObject obj = dag.node();
            geometryObjects.insert( std::pair< std::string, MObject >(name.c_str(), obj));
            found = true;
            break;
          }

          if(!found)
          {
            mayaLogFunc(MString(getName())+": Geometry argument "+MString(key.c_str())+" does not exist in import pattern.");
            return mayaErrorOccured();
          }
        }
      }
      else
      {
        // parse the selection list for geometries
        MSelectionList sl;
        MGlobal::getActiveSelectionList(sl);

        MSelectionList geometries;
        for(unsigned int i=0;i<sl.length();i++)
        {
          MDagPath dag;
          sl.getDagPath(i, dag);
          dag.extendToShape();
          MObject obj = dag.node();

          MFnDependencyNode node(obj);
          MString typeName = node.typeName();

          if(typeName != "mesh" && typeName != "nurbsCurve")
            continue;

          geometries.add(dag.node());
        }
        
        unsigned int offset = 0;

        FabricCore::DFGExec exec = binding.getExec();
        for(unsigned int i=0;i<exec.getExecPortCount() && offset<geometries.length();i++)
        {
          if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
            continue;

          FTL::CStrRef type = exec.getExecPortResolvedType(i);
          if(type != "PolygonMesh" && type != "Curves")
          {
            continue;
          }

          MObject obj;
          geometries.getDependNode(offset, obj);
          MFnDependencyNode node(obj);

          FTL::CStrRef name = exec.getExecPortName(i);

          if(type == "PolygonMesh")
          {
            if(node.typeName() != "mesh")
            {
              mayaLogErrorFunc("Provided geometry "+node.name()+" is a "+node.typeName()+", but argument "+MString(name.c_str())+" is a "+MString(type.c_str())+".");
              return mayaErrorOccured();
            }
          }
          else if(type == "Curves")
          {
            if(node.typeName() != "nurbsCurve")
            {
              mayaLogErrorFunc("Provided geometry "+node.name()+" is a "+node.typeName()+", but argument "+MString(name.c_str())+" is a "+MString(type.c_str())+".");
              return mayaErrorOccured();
            }
          }

          geometryObjects.insert( std::pair< std::string, MObject >(name.c_str(), obj));
          offset++;
        }
      }

      for(std::map< std::string, MObject >::iterator it = geometryObjects.begin(); it != geometryObjects.end(); it++)
      {
        MString name = it->first.c_str();
        MObject obj = it->second;
        MFnDependencyNode node(obj);
        if(node.typeName() == "mesh")
        {
          MFnMesh mesh(obj);
          FabricCore::RTVal polygonMesh = FabricCore::RTVal::Create(m_client, "PolygonMesh", 0, 0);
          polygonMesh = dfgMFnMeshToPolygonMesh(mesh, polygonMesh);
          binding.setArgValue(name.asChar(), polygonMesh);
        }
        else if(node.typeName() == "nurbsCurve")
        {
          MFnNurbsCurve nurbsCurve(obj);
          FabricCore::RTVal curves = FabricCore::RTVal::Create(m_client, "Curves", 0, 0);
          FabricCore::RTVal curveCountRTVal = FabricCore::RTVal::ConstructUInt32(m_client, 1);
          curves.callMethod( "", "setCurveCount", 1, &curveCountRTVal );
          dfgMFnNurbsCurveToCurves(0, nurbsCurve, curves);
          binding.setArgValue(name.asChar(), curves);
        }
      }

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
            if(type != "String" && type != "FilePath" && !FabricCore::GetRegisteredTypeIsShallow(m_client, type.c_str()))
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

              FabricCore::RTVal jsonVal = FabricCore::RTVal::ConstructString(m_client, json.c_str());
              value = FabricCore::RTVal::Construct(m_client, "FilePath", 1, &jsonVal);
            }
            else
            {
              value = FabricCore::ConstructRTValFromJSON(m_client, type.c_str(), json.c_str());
            }
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

        return invoke(m_client, binding, m_settings);
      }
      else if(!quiet)
      {
        QWidget * mainWindow = MQtUtil().mainWindow();
        mainWindow->setFocus(Qt::ActiveWindowFocusReason);

        QEvent event(QEvent::RequestSoftwareInputPanel);
        QApplication::sendEvent(mainWindow, &event);

        FabricImportPatternDialog * dialog = new FabricImportPatternDialog(mainWindow, m_client, binding, m_settings);
        dialog->setWindowModality(Qt::ApplicationModal);
        dialog->setSizeGripEnabled(true);
        dialog->open();

        if(!dialog->wasAccepted())
        {
          return MS::kSuccess;
        }
      }
      else
      {
        return invoke(m_client, binding, m_settings);
      }
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
    }
  }

  return MS::kSuccess;
}

MStatus FabricImportPatternCommand::invoke(FabricCore::Client client, FabricCore::DFGBinding binding, const FabricImportPatternSettings & settings)
{
  m_client = client;
  m_settings = settings;

  MStringArray result;
  try
  {
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
    return mayaErrorOccured();
  }

  FabricProgressbarDialog * prog = NULL;
  if(!m_settings.quiet)
  {
    QWidget * mainWindow = MQtUtil().mainWindow();
    mainWindow->setFocus(Qt::ActiveWindowFocusReason);

    QEvent event(QEvent::RequestSoftwareInputPanel);
    QApplication::sendEvent(mainWindow, &event);

    MString title = "Importing...";
    prog = new FabricProgressbarDialog(mainWindow, title.asChar(), (int)10); // count will be changed later
    prog->setWindowModality(Qt::ApplicationModal);
    prog->setSizeGripEnabled(true);
    prog->open();
  }

  try
  {
    m_context = FabricCore::RTVal::Construct(m_client, "ImporterContext", 0, 0);
    FabricCore::RTVal contextHost = m_context.maybeGetMember("host");
    MString mayaVersion;
    mayaVersion.set(MAYA_API_VERSION);
    contextHost.setMember("name", FabricCore::RTVal::ConstructString(m_client, "Maya"));
    contextHost.setMember("version", FabricCore::RTVal::ConstructString(m_client, mayaVersion.asChar()));
    m_context.setMember("host", contextHost);

    FabricCore::DFGExec exec = binding.getExec();

    for(unsigned int i=0;i<exec.getExecPortCount();i++)
    {
      if(exec.getExecPortType(i) != FabricCore::DFGPortType_Out)
        continue;

      MString name = exec.getExecPortName(i);
      FabricCore::RTVal value = binding.getArgValue(name.asChar());
      MString resolvedType = value.getTypeNameCStr();
      if(resolvedType != "Ref<ImporterObject>[]")
        continue;

      for(unsigned j=0;j<value.getArraySize();j++)
      {
        FabricCore::RTVal obj = value.getArrayElement(j);
        obj = FabricCore::RTVal::Create(m_client, "ImporterObject", 1, &obj);

        MString path = obj.callMethod("String", "getInstancePath", 0, 0).getStringCString();
        path = m_settings.rootPrefix + simplifyPath(path);

        m_objectMap.insert(std::pair< std::string, size_t > (path.asChar(), m_objectList.size()));
        m_objectList.push_back(obj);
      }
    }

    // ensure that we have the parents!
    for(size_t i=0;i<m_objectList.size();i++)
    {
      FabricCore::RTVal obj = m_objectList[i];
      FabricCore::RTVal transform = FabricCore::RTVal::Create(obj.getContext(), "ImporterTransform", 1, &obj);
      if(transform.isNullObject())
        continue;

      FabricCore::RTVal parent = transform.callMethod("ImporterObject", "getParent", 0, 0);
      if(parent.isNullObject())
        continue;

      MString path = parent.callMethod("String", "getInstancePath", 0, 0).getStringCString();
      path = m_settings.rootPrefix + simplifyPath(path);

      std::map< std::string, size_t >::iterator it = m_objectMap.find(path.asChar());
      if(it != m_objectMap.end())
        continue;

      m_objectMap.insert(std::pair< std::string, size_t > (path.asChar(), m_objectList.size()));
      m_objectList.push_back(parent);
    }

    if(prog)
      prog->setCount((int)m_objectList.size() * 2);

    // create the nodes first - and go in reverse.
    // this allows us to create the dag nodes for shapes correctly
    for(std::map< std::string, size_t >::reverse_iterator rit=m_objectMap.rbegin();rit!=m_objectMap.rend();rit++)
    {
      size_t index = rit->second;
      getOrCreateNodeForObject(m_objectList[index]);

      if(prog)
      {
        if(prog->wasCancelPressed())
        {
          prog->close();
          mayaLogFunc(MString(getName())+": aborted by user.");
          return MS::kFailure;
        }
        prog->increment();
        QApplication::processEvents();
      }
    }

    // now perform all remaining tasks
    for(size_t i=0;i<m_objectList.size();i++)
    {
      updateTransformForObject(m_objectList[i]);
      updateEvaluatorForObject(m_objectList[i]);
      // todo: light, cameras etc..

      if(prog)
      {
        if(prog->wasCancelPressed())
        {
          prog->close();
          mayaLogFunc(MString(getName())+": aborted by user.");
          return MS::kFailure;
        }
        prog->increment();
        QApplication::processEvents();
      }
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
  }

  if(prog)
    prog->close();

  for(std::map< std::string, MObject >::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
  {
    MFnDagNode node(it->second);
    result.append(node.fullPathName());
  }

  setResult(result);
  mayaLogFunc(MString(getName())+": import done.");
  return MS::kSuccess;
}


MString FabricImportPatternCommand::parentPath(MString path, MString * name)
{
  if(name)
    *name = path;  

  int rindex = path.rindex('/');
  if(rindex > 0)
  {
    if(name)
      *name = path.substring(rindex + 1, path.length()-1);
    return path.substring(0, rindex-1);
  }
  return "";
}

MString FabricImportPatternCommand::simplifyPath(MString path)
{
  if(path.length() == 0)
    return path;

  if (path.asChar()[0] == '/')
    path = path.substring(1, path.length() - 1);

  if(path.index('/') > 0)
  {
    MStringArray parts;
    path.split('/', parts);
    MString concat;
    for(unsigned int i=0;i<parts.length();i++)
    {
      if(concat.length() > 0)
        concat += "/";
      concat += simplifyPath(parts[i]);
    }
    return concat;
  }

  int rindex = path.rindex(':');
  if(rindex > 0)
    path = path.substring(rindex+1, path.length()-1);

  while(path.length() > 0)
  {
    if(path.asChar()[0] == '_' || isdigit(path.asChar()[0]))
    {
      path = path.substring(1, path.length()-1);
    }
    else
    {
      break;
    }
  }

  return path;
}

MObject FabricImportPatternCommand::getOrCreateNodeForPath(MString path, MString type, bool createIfMissing, bool isDagNode)
{
  if(path.length() == 0)
    return MObject();

  path = simplifyPath(path);

  std::map< std::string, MObject >::iterator it = m_nodes.find(path.asChar());
  if(it != m_nodes.end())
    return it->second;

  if(!createIfMissing)
    return MObject();

  MString name = path;
  MObject parentNode;
  int rindex = path.rindex('/');
  if(rindex > 0)
  {
    MString pathForParent = parentPath(path, &name);
    parentNode = getOrCreateNodeForPath(pathForParent, "transform", true);
  }

  // try to find the node in the scene
  if(m_settings.attachToExisting)
  {
    MSelectionList sl;
    std::string pathStr = path.asChar();
    for(size_t i=0;i<pathStr.length();i++)
    {
      if(pathStr[i] != '/')
        continue;
      pathStr[i] = '|';
    }
    sl.add(pathStr.c_str());
    sl.add(name); // also add by name in case the pathStr failed

    if(sl.length() > 0)
    {
      MDagPath dagPath;
      if(sl.getDagPath(0, dagPath) == MS::kSuccess)
      {
        MObject node = dagPath.node();
        m_nodes.insert(std::pair< std::string, MObject > (path.asChar(), node));
        return node;
      }
    }
  }

  MObject node;
  if(isDagNode)
  {
    MDagModifier modif;
    node = modif.createNode(type, parentNode);
    modif.doIt();
    modif.renameNode(node, m_settings.nameSpace + name);
    modif.doIt();
  }
  else
  {
    MDGModifier modif;
    node = modif.createNode(type);
    modif.doIt();
    modif.renameNode(node, m_settings.nameSpace + name);
    modif.doIt();
  }

  m_nodes.insert(std::pair< std::string, MObject > (path.asChar(), node));
  return node;
}

MObject FabricImportPatternCommand::getOrCreateNodeForObject(FabricCore::RTVal obj)
{
  MString type = obj.callMethod("String", "getType", 0, 0).getStringCString();
  MString path = obj.callMethod("String", "getInstancePath", 0, 0).getStringCString();
  path = m_settings.rootPrefix + simplifyPath(path);

  if(type == "Layer" || type == "Group" || type == "Transform" || type == "Instance")
  {
    return getOrCreateNodeForPath(path, "transform");
  }
  else if(type == "Shape")
  {
    return getOrCreateShapeForObject(obj);
  }
  else if(type == "Light")
  {
    // todo
    return MObject();
  }
  else if(type == "Camera")
  {
    // todo
    return MObject();
  }
  else if(type == "Material" || type == "Texture")
  {
    // this is expected - no groups for you!
    return MObject();
  }
  else
  {
    mayaLogErrorFunc("Unsupported ImporterObject type "+type+" for "+path);
    return MObject();
  }

  return MObject();
}

bool FabricImportPatternCommand::updateTransformForObject(FabricCore::RTVal obj, MObject node)
{
  FabricCore::RTVal transform = FabricCore::RTVal::Create(obj.getContext(), "ImporterTransform", 1, &obj);
  if(transform.isNullObject())
    return false;

  if(node.isNull())
  {
    MString path = obj.callMethod("String", "getInstancePath", 0, 0).getStringCString();
    path = m_settings.rootPrefix + simplifyPath(path);

    node = getOrCreateNodeForPath(path, "transform", false);
    if(node.isNull())
      return false;
  }

  MFnTransform transformNode(node);

  FabricCore::RTVal localTransform = transform.callMethod("Mat44", "getLocalTransform", 1, &m_context);
  void * data = localTransform.callMethod("Data", "data", 0, 0).getData();
  float floats[4][4];
  memcpy(floats, data, sizeof(float) * 16);

  MMatrix m = MMatrix(floats).transpose();

  // if this is a root object
  if(transform.callMethod("ImporterObject", "getParent", 0, 0).isNullObject())
  {
    double scale3[3];
    scale3[0] = m_settings.scale;
    scale3[1] = m_settings.scale;
    scale3[2] = m_settings.scale;
    MTransformationMatrix scaleMatrix;
    scaleMatrix.setScale(scale3, MSpace::kWorld);

    m = m * scaleMatrix.asMatrix();
  }

  MTransformationMatrix tfMatrix = m;
  transformNode.set(tfMatrix);

  return true;
}

MObject FabricImportPatternCommand::getOrCreateShapeForObject(FabricCore::RTVal obj)
{
  try
  {
    FabricCore::RTVal shape = FabricCore::RTVal::Create(obj.getContext(), "ImporterShape", 1, &obj);
    if(shape.isNullObject())
      return MObject();

    //const Integer ImporterShape_Mesh = 0;
    //const Integer ImporterShape_Curves = 1;
    //const Integer ImporterShape_Points = 2;
    FabricCore::RTVal geoTypeVal = shape.callMethod("SInt32", "getGeometryType", 1, &m_context);
    int geoType = geoTypeVal.getSInt32();
    if (geoType != 0) // not a mesh
    {
      MString geoTypeStr;
      geoTypeStr.set(geoType);
      mayaLogFunc(MString(getName())+": Shape type " + geoTypeStr + " not yet supported.");
      return MObject();
    }

    MString instancePath = obj.callMethod("String", "getInstancePath", 0, 0).getStringCString();
    MString lookupPath = simplifyPath(instancePath);
    instancePath = m_settings.rootPrefix + simplifyPath(instancePath);

    MString name;
    instancePath = parentPath(instancePath, &name);

    // if this shape has no local transform at all - in maya we can save us one 
    // hierarchy level and go directly to the shape
    if(!shape.callMethod("Boolean", "hasLocalTransform", 0, 0).getBoolean())
    {
      lookupPath = simplifyPath(instancePath);
      instancePath = parentPath(instancePath, &name);
    }
    MObject parentNode = getOrCreateNodeForPath(instancePath, "transform", false);

    // now check if we have already converted this shape before
    std::map< std::string, MObject >::iterator it = m_nodes.find(lookupPath.asChar());
    MObject node;
    if(it == m_nodes.end())
    {
      FabricCore::RTVal polygonMesh = shape.callMethod("PolygonMesh", "getGeometry", 1, &m_context);
      if (polygonMesh.isNullObject())
        return MObject();

      // try to find the node in the scene
      if(m_settings.attachToExisting)
      {
        MSelectionList sl;
        std::string pathStr = lookupPath.asChar();
        for(size_t i=0;i<pathStr.length();i++)
        {
          if(pathStr[i] != '/')
            continue;
          pathStr[i] = '|';
        }
        sl.add(pathStr.c_str());
        if(sl.length() > 0)
        {
          MDagPath dagPath;
          if(sl.getDagPath(0, dagPath) == MS::kSuccess)
          {
            node = dagPath.node();
          }
        }
      }

      if(node.isNull())
      {
        node = dfgPolygonMeshToMFnMesh(polygonMesh, false /* insideCompute */);
      }
      m_nodes.insert(std::pair< std::string, MObject > (lookupPath.asChar(), node));

      MDagModifier modif;
      modif.renameNode(node, m_settings.nameSpace + name);
      modif.doIt();
    
      if(!parentNode.isNull())
      {
        modif.reparentNode(node, parentNode);
        modif.doIt();
      }

      updateTransformForObject(obj, node);
      updateMaterialForObject(obj, node);
    }
    else if(!parentNode.isNull())
    {
      node = it->second;

      MFnDagNode parentDag(parentNode);
      parentDag.addChild(node, MFnDagNode::kNextPos, true /* keepExistingParents */);
    }

    return node;
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.getDesc_cstr());
  }
  return MObject();
}

bool FabricImportPatternCommand::updateMaterialForObject(FabricCore::RTVal obj, MObject node)
{
  MStatus status;
  FabricCore::RTVal shape = FabricCore::RTVal::Create(obj.getContext(), "ImporterShape", 1, &obj);
  if(shape.isNullObject())
    return false;

  if(node.isNull())
    return false;

  if(!m_settings.enableMaterials)
  {
    MObject shadingEngine;

    MSelectionList sl;
    sl.add(L"initialShadingGroup");
    sl.getDependNode(0, shadingEngine);

    MFnSet shadingEngineSet(shadingEngine);
    shadingEngineSet.addMember(node);
    return true;
  }

  FabricCore::RTVal materials = shape.callMethod("Ref<ImporterObject>[]", "getMaterials", 1, &m_context);

  MString materialName;
  MColor materialColor(0.7f, 0.7f, 0.7f);

  if (materials.getArraySize() > 0)
  {
    // todo: for now we only support one material...
    FabricCore::RTVal material = materials.getArrayElement(0);
    materialName = material.callMethod("String", "getName", 0, 0).getStringCString();

    for(unsigned int i=0;i<2;i++)
    {
      FabricCore::RTVal propertyNameVal;
      if(i == 0)
        propertyNameVal = FabricCore::RTVal::ConstructString(obj.getContext(), "diffuse");
      else
        propertyNameVal = FabricCore::RTVal::ConstructString(obj.getContext(), "color");

      MString propType = material.callMethod("String", "getPropertyType", 1, &propertyNameVal).getStringCString();
      if(propType != "Color")
        continue;

      FabricCore::RTVal args[2];
      args[0] = propertyNameVal;
      args[1] = m_context;
      material.callMethod("", "updateProperty", 2, args);
      FabricCore::RTVal colorVal = material.callMethod("Color", "getColorProperty", 1, &propertyNameVal);

      materialColor.r = colorVal.maybeGetMember("r").getFloat32();
      materialColor.g = colorVal.maybeGetMember("g").getFloat32();
      materialColor.b = colorVal.maybeGetMember("b").getFloat32();
      materialColor.a = colorVal.maybeGetMember("a").getFloat32();
    }
  }
  else
  {
    FabricCore::RTVal colorVal = shape.callMethod("Color", "getColor", 1, &m_context);
    materialColor.r = colorVal.maybeGetMember("r").getFloat32();
    materialColor.g = colorVal.maybeGetMember("g").getFloat32();
    materialColor.b = colorVal.maybeGetMember("b").getFloat32();
    materialColor.a = 1.0;

    MString r, g, b;
    r.set(int((float)materialColor.r * 255.0f + 0.5f));
    g.set(int((float)materialColor.g * 255.0f + 0.5f));
    b.set(int((float)materialColor.b * 255.0f + 0.5f));

    materialName = "Material_r"+r+"g"+g+"b"+b+"Color";
  }

  MString sgName = materialName + "SG";

  MObject shadingEngine;
  std::map< std::string, MObject >::iterator it = m_materialSets.find(sgName.asChar());
  if(it == m_materialSets.end())
  {
    MSelectionList sl;
    sl.add(sgName);

    if(sl.length() == 0)
    {
      MString shaderName;
      MString shadingEngineName;
      MGlobal::executeCommand("sets -renderable true -noSurfaceShader true -empty -name \""+sgName+"\";", shadingEngineName);
      MGlobal::executeCommand("shadingNode -asShader lambert -name \"" + materialName + "\";", shaderName);
      MGlobal::executeCommand("connectAttr -f \""+shaderName+".outColor\" \""+ shadingEngineName +".surfaceShader\";");

      MString r, g, b;
      r.set(materialColor.r);
      g.set(materialColor.g);
      b.set(materialColor.b);
      MGlobal::executeCommand("setAttr \""+shaderName+".color\" -type double3 "+r+" "+g+" "+b+";");

      sl.add(shadingEngineName);
      sl.getDependNode(0, shadingEngine);
      m_materialSets.insert(std::pair< std::string, MObject > (shadingEngineName.asChar(), shadingEngine));
    }
    else
    {
      sl.getDependNode(0, shadingEngine);
    }
  }
  else
  {
    shadingEngine = it->second;
  }

  MFnSet shadingEngineSet(shadingEngine);
  shadingEngineSet.addMember(node);

  return true;
}

bool FabricImportPatternCommand::updateEvaluatorForObject(FabricCore::RTVal objRef)
{
  FabricCore::RTVal obj = FabricCore::RTVal::Create(objRef.getContext(), "ImporterObject", 1, &objRef);
  if(obj.isNullObject())
    return false;

  FabricCore::RTVal evaluator = obj.callMethod("Evaluator", "getEvaluator", 0, NULL);
  if(evaluator.isNullObject())
    return false;

  FabricCore::RTVal filePathVal = evaluator.callMethod("FilePath", "getFilePath", 0, 0);
  filePathVal = filePathVal.callMethod("FilePath", "expandEnvVars", 0, 0);
  std::string filePath = filePathVal.callMethod("String", "string", 0, 0).getStringCString();
  for(unsigned int i=0;i<filePath.length();i++)
  {
    if(filePath[i] == '\\')
      filePath[i] = '/';
  }
  MString filePathM = filePath.c_str();
  if(!FTL::FSExists(filePathM.asChar()))
  {
    mayaLogErrorFunc("Evaluator filePath \""+filePathM+"\" does no exist.");
    return mayaErrorOccured();
  }

  FabricCore::RTVal propertiesVal = evaluator.callMethod("String[]", "getProperties", 0, 0);
  if(propertiesVal.getArraySize() == 0)
    return false;

  MString objPath = obj.callMethod("String", "getInstancePath", 0, 0).getStringCString();
  MString evaluatorPath = objPath + "/Evaluator";

  std::map< std::string, MObject >::iterator it = m_nodes.find(simplifyPath(objPath).asChar());
  if(it == m_nodes.end())
  {
    FabricCore::RTVal shape = FabricCore::RTVal::Create(obj.getContext(), "ImporterShape", 1, &obj);
    if(!shape.isNullObject())
    {
      if(!shape.callMethod("Boolean", "hasLocalTransform", 0, 0).getBoolean())
      {
        MString name;
        MString transformPath = parentPath(objPath, &name);
        it = m_nodes.find(simplifyPath(transformPath).asChar());
      }
    }
  }
  if(it == m_nodes.end())
  {
    mayaLogErrorFunc("Missing node for '"+objPath+"'.");
    return false;
  }

  MObject objNode = it->second;
  MFnDependencyNode objDepNode(objNode);

  bool isDeformer = evaluator.callMethod("Boolean", "isDeformer", 0, NULL).getBoolean();

  MObject evaluatorNode;
  if(isDeformer)
  {
    MGlobal::executeCommand("select -r "+objDepNode.name()+";");
    MStringArray results;
    MString deformerCmd = "deformer -type \"canvasFuncDeformer\" -name \"Evaluator\";";
    MGlobal::executeCommand(deformerCmd, results);
    if(results.length() == 0)
    {
      mayaLogErrorFunc("Cannot create deformer for "+objDepNode.name());
      return false;
    }

    // we need to remove the meshes port so that we can load the evaluator code
    MGlobal::executeCommand("FabricCanvasRemovePort -m \""+results[0]+"\" -e \"\" -n \"meshes\";");

    MSelectionList selList;
    MGlobal::getSelectionListByName(results[0], selList);
    selList.getDependNode(0, evaluatorNode);
  }
  else
  {
    evaluatorNode = getOrCreateNodeForPath(evaluatorPath, "canvasFuncNode", true, false /* isDag */);
    if(evaluatorNode.isNull())
    {
      mayaLogErrorFunc("Missing node for '"+evaluatorPath+"'.");
      return false;
    }
  }
  MFnDependencyNode evaluatorDepNode(evaluatorNode);

  // setup the evaluator code
  MStatus st = MGlobal::executeCommand("dfgImportJSON -m "+evaluatorDepNode.name()+" -f \""+filePathM+"\";");
  if(st != MS::kSuccess)
  {
    mayaLogErrorFunc("Failed to load evaluator code '"+filePathM+"'.");
    return false;
  }

  // assign all of the arguments
  unsigned int argCount = evaluator.callMethod("UInt32", "getArgCount", 0, 0).getUInt32();
  for(unsigned int i=0;i<argCount;i++)
  {
    FabricCore::RTVal argIndex = FabricCore::RTVal::ConstructUInt32(obj.getContext(), i);
    MString argName = evaluator.callMethod("String", "getArgName", 1, &argIndex).getStringCString();
    MString argType = evaluator.callMethod("String", "getArgType", 1, &argIndex).getStringCString();

    if(argType == L"Scalar")
      argType = L"Float32";
    if(argType == L"Integer")
      argType = L"SInt32";

    FabricCore::RTVal argValueVal = evaluator.callMethod("RTVal", "getArgValue", 1, &argIndex);
    argValueVal = argValueVal.getUnwrappedRTVal();

    MString argValueType = argValueVal.getTypeNameCStr();
    if(argValueType == L"Scalar")
      argValueType = L"Float32";
    if(argValueType == L"Integer")
      argValueType = L"SInt32";

    if(argValueType != argType)
    {
      mayaLogFunc(MString(getName())+": Warning: Argument value is not a "+argType+".");
      continue;
    }

    MPlug plug = evaluatorDepNode.findPlug(argName);
    if(plug.isNull())
    {
      mayaLogFunc(MString(getName())+": Warning: Cannot find plug for evaluator argument "+argName+".");
      continue;
    }

    // todo: we should probably centralize this code in a function
    // since we are using it in several places - structurally somewhat different but similar.
    if(argType == L"Boolean")
    {
      plug.setValue(argValueVal.getBoolean());
    }
    else if(argType == L"SInt32")
    {
      plug.setValue(argValueVal.getSInt32());
    }
    else if(argType == L"UInt32")
    {
      plug.setValue((int)argValueVal.getUInt32());
    }
    else if(argType == L"Float32")
    {
      if(argName == L"time" || argName == L"timeline")
      {
        // setup an expression to drive the time
        MString exprCmd = "expression -s \""+
          plug.name()+
          " = time;\"  -o "+
          evaluatorDepNode.name()+
          " -ae 1 -uc all ;";
        MGlobal::executeCommand(exprCmd);
      }
      else
      {
        plug.setValue(argValueVal.getFloat32());
      }
    }
    else if(argType == L"Float64")
    {
      plug.setValue(argValueVal.getFloat64());
    }
    else if(argType == L"String")
    {
      plug.setValue(MString(argValueVal.getStringCString()));
    }
    else if(argType == L"Vec2")
    {
      plug.child(0).setValue(argValueVal.maybeGetMember("x").getFloat32());
      plug.child(1).setValue(argValueVal.maybeGetMember("y").getFloat32());
    }
    else if(argType == L"Vec3")
    {
      plug.child(0).setValue(argValueVal.maybeGetMember("x").getFloat32());
      plug.child(1).setValue(argValueVal.maybeGetMember("y").getFloat32());
      plug.child(2).setValue(argValueVal.maybeGetMember("z").getFloat32());
    }
    else if(argType == L"Color")
    {
      plug.child(0).setValue(argValueVal.maybeGetMember("r").getFloat32());
      plug.child(1).setValue(argValueVal.maybeGetMember("g").getFloat32());
      plug.child(2).setValue(argValueVal.maybeGetMember("b").getFloat32());
      plug.child(3).setValue(argValueVal.maybeGetMember("a").getFloat32());
    }
  }

  bool success = false;
  for(unsigned int i=0;i<propertiesVal.getArraySize();i++)
  {
    MString property = propertiesVal.getArrayElement(i).getStringCString();
    if(property == L"localTransform")
    {
      // we also need a node for matrix conversion
      MGlobal::executeCommand("loadPlugin -name \"matrixNodes\" -quiet;");
      MObject decomposeNode = getOrCreateNodeForPath(evaluatorPath+"/Decompose", "decomposeMatrix", true, false /* isDag */);
      if(decomposeNode.isNull())
      {
        mayaLogErrorFunc("Missing node for '"+evaluatorPath+"/Decompose'.");
        return false;
      }

      MFnDependencyNode decomposeDepNode(decomposeNode);

      MDGModifier modif;
      modif.connect(evaluatorDepNode.findPlug("localTransform"), decomposeDepNode.findPlug("inputMatrix"));
      modif.connect(decomposeDepNode.findPlug("outputTranslate"), objDepNode.findPlug("translate"));
      modif.connect(decomposeDepNode.findPlug("outputRotate"), objDepNode.findPlug("rotate"));
      modif.connect(decomposeDepNode.findPlug("outputScale"), objDepNode.findPlug("scale"));
      modif.doIt();

      success = true;
    }
    else if(property == L"geometry")
    {
      // we are already hooked up since this is a deformer?
      if(!isDeformer)
      {
        FabricCore::RTVal shape = FabricCore::RTVal::Create(obj.getContext(), "ImporterShape", 1, &obj);
        if(shape.isNullObject())
          return false;

        //const Integer ImporterShape_Mesh = 0;
        //const Integer ImporterShape_Curves = 1;
        //const Integer ImporterShape_Points = 2;
        FabricCore::RTVal geoTypeVal = shape.callMethod("SInt32", "getGeometryType", 1, &m_context);
        int geoType = geoTypeVal.getSInt32();
        if (geoType == 0) // a mesh
        {
          MDGModifier modif;
          modif.connect(evaluatorDepNode.findPlug("geometry"), objDepNode.findPlug("mesh"));
        }
        else
        {
          // other cases are not yet supported by the shape creation anyway....
        }
      }
    }
    else
    {
      mayaLogErrorFunc("Evaluator uses unsupported property '"+property+"'.");
      continue;
    }
  }
  return success;
}
