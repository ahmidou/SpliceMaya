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
#include <maya/MDagModifier.h>
#include <maya/MFnTransform.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnLambertShader.h>
#include <maya/MCommandResult.h>

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

  if( argParser.isFlagSet("scale") )
  {
    m_settings.scale = argParser.flagArgumentDouble("scale", 0);
  }


  MSelectionList sl;
  if( argParser.isFlagSet("objects") )
  {
    MString objectsJoined = argParser.flagArgumentString("objects", 0);
    MStringArray objectsArray;
    objectsJoined.split(',', objectsArray);
    for(unsigned int i=0;i<objectsArray.length();i++)
    {
      sl.add(objectsArray[i]);

      MObject node;
      sl.getDependNode(i, node);
      if(node.isNull())
      {
        mayaLogErrorFunc("Object '"+objectsArray[i]+"' not found in scene.");
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

      FabricExportPatternDialog * dialog = new FabricExportPatternDialog(mainWindow, binding, &m_settings);
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

  for(size_t i=0;i<m_nodes.size();i++)
  {
    FabricCore::RTVal obj = createRTValForNode(m_nodes[i]);
    m_objects.push_back(obj);
  }

  // FabricCore::Context context;
  // MStringArray result;
  // try
  // {
  //   context = binding.getHost().getContext();
  //   binding.execute();
  // }
  // catch(FabricSplice::Exception e)
  // {
  //   mayaLogErrorFunc(MString(getName()) + ": "+e.what());

  //   if(binding.isValid())
  //   {
  //     MString errors = binding.getErrors(true).getCString();
  //     mayaLogErrorFunc(errors);
  //   }
  //   return mayaErrorOccured();
  // }

  // try
  // {
  //   m_context = FabricCore::RTVal::Construct(context, "ImporterContext", 0, 0);
  //   FabricCore::RTVal contextHost = m_context.maybeGetMember("host");
  //   MString mayaVersion;
  //   mayaVersion.set(MAYA_API_VERSION);
  //   contextHost.setMember("name", FabricCore::RTVal::ConstructString(context, "Maya"));
  //   contextHost.setMember("version", FabricCore::RTVal::ConstructString(context, mayaVersion.asChar()));
  //   m_context.setMember("host", contextHost);

  //   FabricCore::DFGExec exec = binding.getExec();

  //   for(unsigned int i=0;i<exec.getExecPortCount();i++)
  //   {
  //     if(exec.getExecPortType(i) != FabricCore::DFGPortType_Out)
  //       continue;

  //     MString name = exec.getExecPortName(i);
  //     FabricCore::RTVal value = binding.getArgValue(name.asChar());
  //     MString resolvedType = value.getTypeNameCStr();
  //     if(resolvedType != "Ref<ImporterObject>[]")
  //       continue;

  //     for(unsigned j=0;j<value.getArraySize();j++)
  //     {
  //       FabricCore::RTVal obj = value.getArrayElement(j);
  //       obj = FabricCore::RTVal::Create(context, "ImporterObject", 1, &obj);

  //       MString path = obj.callMethod("String", "getInstancePath", 0, 0).getStringCString();
  //       path = m_settings.rootPrefix + simplifyPath(path);

  //       m_objectMap.insert(std::pair< std::string, size_t > (path.asChar(), m_objectList.size()));
  //       m_objectList.push_back(obj);
  //     }
  //   }

  //   // ensure that we have the parents!
  //   for(size_t i=0;i<m_objectList.size();i++)
  //   {
  //     FabricCore::RTVal obj = m_objectList[i];
  //     FabricCore::RTVal transform = FabricCore::RTVal::Create(obj.getContext(), "ImporterTransform", 1, &obj);
  //     if(transform.isNullObject())
  //       continue;

  //     FabricCore::RTVal parent = transform.callMethod("ImporterObject", "getParent", 0, 0);
  //     if(parent.isNullObject())
  //       continue;

  //     MString path = parent.callMethod("String", "getInstancePath", 0, 0).getStringCString();
  //     path = m_settings.rootPrefix + simplifyPath(path);

  //     std::map< std::string, size_t >::iterator it = m_objectMap.find(path.asChar());
  //     if(it != m_objectMap.end())
  //       continue;

  //     m_objectMap.insert(std::pair< std::string, size_t > (path.asChar(), m_objectList.size()));
  //     m_objectList.push_back(parent);
  //   }

  //   // create the groups first
  //   for(size_t i=0;i<m_objectList.size();i++)
  //   {
  //     getOrCreateNodeForObject(m_objectList[i]);
  //   }

  //   // now perform all remaining tasks
  //   for(size_t i=0;i<m_objectList.size();i++)
  //   {
  //     updateTransformForObject(m_objectList[i]);
  //     updateShapeForObject(m_objectList[i]);
  //     updateEvaluatorForObject(m_objectList[i]);
  //     // todo: light, cameras etc..
  //   }
  // }
  // catch(FabricSplice::Exception e)
  // {
  //   mayaLogErrorFunc(MString(getName()) + ": "+e.what());
  // }

  // for(std::map< std::string, MObject >::iterator it = m_nodes.begin(); it != m_nodes.end(); it++)
  // {
  //   MFnDagNode node(it->second);
  //   result.append(node.fullPathName());
  // }

  // setResult(result);
  // mayaLogFunc(MString(getName())+": import done.");
  return MS::kSuccess;
}

FabricCore::RTVal FabricExportPatternCommand::createRTValForNode(const MObject & node)
{
  if(node.isNull())
    return FabricCore::RTVal();

  MFnDependencyNode depNode(node);

  FabricCore::RTVal val;
  try
  {
    switch(depNode.type())
    {
      case MFn::kMesh:
      {
        // todo: make sure that we haven't hit this before...!
        // support for instances
        mayaLogFunc(MString(getName())+": '"+depNode.name()+"' is a kMesh.");

        break;        
      }
      case MFn::kCamera:
      {
        mayaLogFunc(MString(getName())+": '"+depNode.name()+"' is a kCamera.");
        break;        
      }
      case MFn::kPointLight:
      case MFn::kSpotLight:
      case MFn::kDirectionalLight:
      {
        mayaLogFunc(MString(getName())+": '"+depNode.name()+"' is a kCamera.");
        break;        
      }
      case MFn::kTransform:
      {
        mayaLogFunc(MString(getName())+": '"+depNode.name()+"' is a kTransform.");
        break;        
      }
      default:
      {
        mayaLogFunc(MString(getName())+": Warning: Type of '"+depNode.name()+"' is not supported.");
        break;        
      }
    }
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
    return FabricCore::RTVal();
  }

  return val;
}
