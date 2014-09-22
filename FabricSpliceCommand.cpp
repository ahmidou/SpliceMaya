#include <QtGui/QFileDialog>

#include "FabricSpliceCommand.h"
#include "FabricSpliceConversion.h"

#include <maya/MStringArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MFnAttribute.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MQtUtil.h>

#include <FabricSplice.h>

#include "FabricSpliceBaseInterface.h"
#include "FabricSpliceEditorCmd.h"
#include "FabricSpliceRenderCallback.h"

#define kActionFlag "-a"
#define kActionFlagLong "-action"
#define kReferenceFlag "-r"
#define kReferenceFlagLong "-reference"
#define kDataFlag "-data"
#define kDataFlagLong "-data"
#define kAuxiliaryFlag "-x"
#define kAuxiliaryFlagLong "-auxiliary"

MSyntax FabricSpliceCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kActionFlag, kActionFlagLong, MSyntax::kString);
  syntax.addFlag(kReferenceFlag, kReferenceFlagLong, MSyntax::kString);
  syntax.addFlag(kDataFlag, kDataFlagLong, MSyntax::kString);
  syntax.addFlag(kAuxiliaryFlag, kAuxiliaryFlagLong, MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricSpliceCommand::creator()
{
  return new FabricSpliceCommand;
}

MStatus FabricSpliceCommand::doIt(const MArgList &args)
{
  mayaClearError();

  try
  {
    FabricCore::Variant scriptArgs = FabricSplice::Scripting::parseScriptingArguments(
      args.asString(0).asChar(), args.asString(1).asChar(), args.asString(2).asChar(), args.asString(3).asChar());

    // fetch the relevant arguments
    MString actionStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "action").c_str();

    // optional arguments
    MString referenceStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "reference", "", true).c_str();

    // perform actions which don't require a reference
    if(actionStr == "constructClient"){
      bool clientCreated = FabricSplice::ConstructClient().isValid();
      setResult(clientCreated);
      return mayaErrorOccured();
    }
    else if(actionStr == "destroyClient"){
      bool clientDestroyed = FabricSplice::DestroyClient();
      setResult(clientDestroyed);
      return mayaErrorOccured();
    }
    else if(actionStr == "getClientContextID"){
      MString clientContextID = FabricSplice::GetClientContextID();
      setResult(clientContextID);
      return mayaErrorOccured();
    }
    else if(actionStr == "registerKLType"){
      MString rtStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "rt").c_str();
      MString extStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "extension", "", true).c_str();
      bool res = true;
      if(extStr.length() > 0)
      {
        FabricSplice::DGGraph::loadExtension(rtStr.asChar());
      }
      FabricSplice::DGGraph::loadExtension(extStr.asChar());
      setResult(res);
      return mayaErrorOccured();
    }
    else if(actionStr == "toggleRenderer")
    {
      enableRTRPass(!isRTRPassEnabled());
      mayaRefreshFunc();
      return mayaErrorOccured();
    }
    else if(actionStr == "startProfiling")
    {
      FabricSplice::Logging::enableTimers();
      for(unsigned int i=0;i<FabricSplice::Logging::getNbTimers();i++)
      {
        FabricSplice::Logging::resetTimer(FabricSplice::Logging::getTimerName(i));
      }    
      return mayaErrorOccured();
    }
    else if(actionStr == "stopProfiling")
    {
      for(unsigned int i=0;i<FabricSplice::Logging::getNbTimers();i++)
      {
        FabricSplice::Logging::logTimer(FabricSplice::Logging::getTimerName(i));
      }    
      FabricSplice::Logging::disableTimers();
      return mayaErrorOccured();
    }

    // find interface
    FabricSpliceBaseInterface * interf = FabricSpliceBaseInterface::getInstanceByName(referenceStr.asChar());
    if(!interf)
    {
      mayaLogErrorFunc("Splice interface could not be found: '"+referenceStr+"'.");
      return mayaErrorOccured();
    }

    // find the maya node
    MSelectionList selList;
    MGlobal::getSelectionListByName(referenceStr, selList);
    MObject spliceMayaNodeObj;
    selList.getDependNode(0, spliceMayaNodeObj);
    if(spliceMayaNodeObj.isNull())
    {
      mayaLogErrorFunc("Splice node could not be found: '"+referenceStr+"'.");
      return mayaErrorOccured();
    }
    MFnDependencyNode spliceMayaNodeFn(spliceMayaNodeObj);

    if(actionStr == "addInputPort" || actionStr == "addOutputPort" || actionStr == "addIOPort")
    {

      // parse args
      MString portNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "portName").c_str();
      MString dataTypeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dataType").c_str();
      MString arrayTypeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "arrayType", "Single Value", true).c_str();
      bool addSpliceMayaAttr = FabricSplice::Scripting::consumeBooleanArgument(scriptArgs, "addSpliceMayaAttr", false, true);
      bool addMayaAttr = FabricSplice::Scripting::consumeBooleanArgument(scriptArgs, "addMayaAttr", addSpliceMayaAttr, true);
      bool autoInitObjects = FabricSplice::Scripting::consumeBooleanArgument(scriptArgs, "autoInitObjects", true, true);
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode", "DGNode", true).c_str();
      MString extStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "extension", "", true).c_str();
      MString auxiliaryStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "auxiliary", "", true).c_str();

      // check if the arrayType and dataType match
      if(arrayTypeStr != "Single Value")
      {
        MStringArray dataTypeStrParts;
        dataTypeStr.split('[', dataTypeStrParts);
        if(dataTypeStrParts.length() == 1)
        {
          mayaLogErrorFunc("dataType '"+dataTypeStr+"' and arrayType '"+arrayTypeStr+"' don't match. (Did you forget '[]'?)");
          return mayaErrorOccured();
        }
      }

      FabricSplice::Port_Mode portMode = FabricSplice::Port_Mode_OUT;
      if(actionStr == "addInputPort")
        portMode = FabricSplice::Port_Mode_IN;
      else if(actionStr == "addIOPort")
        portMode = FabricSplice::Port_Mode_IO;

      FabricCore::Variant defaultValue;
      if(auxiliaryStr.length() > 0)
      {
        if(dataTypeStr == "String")
          defaultValue = FabricCore::Variant::CreateString(auxiliaryStr.asChar());
        else if( dataTypeStr == "SInt16" || dataTypeStr == "UInt16" )
          defaultValue = FabricCore::Variant::CreateSInt32(auxiliaryStr.asShort());
        else if(dataTypeStr == "Integer" || dataTypeStr == "SInt32" || dataTypeStr == "SInt64" )
          defaultValue = FabricCore::Variant::CreateSInt32(auxiliaryStr.asInt());
        else if(dataTypeStr == "Size"  || dataTypeStr == "Index" || dataTypeStr == "UInt32" || dataTypeStr == "UInt64")
          defaultValue = FabricCore::Variant::CreateSInt32(auxiliaryStr.asUnsigned());
        else if(dataTypeStr == "Float32")
          defaultValue = FabricCore::Variant::CreateFloat64(auxiliaryStr.asFloat());
        else if(dataTypeStr == "Scalar" || dataTypeStr == "Float64")
          defaultValue = FabricCore::Variant::CreateFloat64(auxiliaryStr.asDouble());
        else if(dataTypeStr != "CompoundParam" && dataTypeStr != "CompoundParam[]")
          defaultValue = FabricCore::Variant::CreateFromJSON(auxiliaryStr.asChar());
      }
      interf->addPort(portNameStr, dataTypeStr, portMode, dgNodeStr, autoInitObjects, extStr, defaultValue);

      // add all additional arguments as flags on the port
      FabricSplice::DGPort port = interf->getPort(portNameStr);
      if(port.isValid())
      {
        FabricCore::Variant compoundStructure = FabricCore::Variant::CreateDict();
        compoundStructure.setDictValue("dataType", FabricCore::Variant::CreateString(dataTypeStr.asChar()));
        for(FabricCore::Variant::DictIter keyIter(scriptArgs); !keyIter.isDone(); keyIter.next())
        {
          std::string key = keyIter.getKey()->getStringData();
          FabricCore::Variant value = *keyIter.getValue();
          if(value.isNull())
            continue;
          port.setOption(key.c_str(), value);
          compoundStructure.setDictValue(key.c_str(), value);
        }

        if(arrayTypeStr == "Array (Native)")
          port.setOption("nativeArray", FabricCore::Variant::CreateBoolean(true));

        if(addMayaAttr)
        {
          if(addSpliceMayaAttr)
          {
            dataTypeStr = "SpliceMayaData";
            port.setOption("opaque", FabricCore::Variant::CreateBoolean(true));
          }
          else if((dataTypeStr == "CompoundParam" || dataTypeStr == "CompoundParam[]") && auxiliaryStr.length() > 0)
          {
            compoundStructure = FabricCore::Variant::CreateFromJSON(auxiliaryStr.asChar());
            port.setOption("compoundStructure", compoundStructure);
          }
          interf->addMayaAttribute(portNameStr, dataTypeStr, arrayTypeStr, portMode, false, compoundStructure);
        }
        else
        {
          port.setOption("internal", FabricCore::Variant::CreateBoolean(true));
        }
      }
    }
    else if(actionStr == "removePort")
    {
      MString portNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "portName").c_str();
      interf->removePort(portNameStr);
    }
    else if(actionStr == "setDirtyMechanism")
    {
      bool enabled = FabricSplice::Scripting::consumeBooleanArgument(scriptArgs, "enabled");
      interf->setDgDirtyEnabled(enabled);
    }
    else if(actionStr == "addDGNode")
    {
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode").c_str();
      interf->getSpliceGraph().constructDGNode(dgNodeStr.asChar());
      FabricSpliceEditorWidget::postUpdateAll();
      return mayaErrorOccured();
    }
    else if(actionStr == "removeDGNode")
    {
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode").c_str();
      interf->getSpliceGraph().removeDGNode(dgNodeStr.asChar());
      FabricSpliceEditorWidget::postUpdateAll();
      return mayaErrorOccured();
    }
    else if(actionStr == "setDGNodeDependency")
    {
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode", "DGNode", true).c_str();
      MString dependencyStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dependency").c_str();
      interf->getSpliceGraph().setDGNodeDependency(dgNodeStr.asChar(), dependencyStr.asChar());
      FabricSpliceEditorWidget::postUpdateAll();
      return mayaErrorOccured();
    }
    else if(actionStr == "removeDGNodeDependency")
    {
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode", "DGNode", true).c_str();
      MString dependencyStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dependency").c_str();
      interf->getSpliceGraph().removeDGNodeDependency(dgNodeStr.asChar(), dependencyStr.asChar());
      FabricSpliceEditorWidget::postUpdateAll();
      return MS::kSuccess;
    }
    else if(actionStr == "addKLOperator")
    {
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      MString klCodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "auxiliary", "", true).c_str();
      MString entryStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "entry", "", true).c_str();
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode", "DGNode", true).c_str();
      FabricCore::Variant portMap = FabricSplice::Scripting::consumeVariantArgument(scriptArgs, "portMap", FabricCore::Variant::CreateDict(), true);
      interf->addKLOperator(opNameStr, klCodeStr, entryStr, dgNodeStr, portMap);
    }
    else if(actionStr == "removeKLOperator")
    {
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      MString dgNodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "dgNode", "DGNode", true).c_str();
      interf->removeKLOperator(opNameStr, dgNodeStr);
    }
    else if(actionStr == "getKLOperatorCode")
    {
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      
      MStatus stat;
      MString code = interf->getKLOperatorCode(opNameStr, &stat).c_str();
      if(stat == MStatus::kFailure){
        return MS::kFailure;
      }

      setResult(code);
    }
    else if(actionStr == "setKLOperatorCode")
    {
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      MString klCodeStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "auxiliary", "", true).c_str();
      MString entryStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "entry", "", true).c_str();
      
      MStatus stat;
      interf->setKLOperatorCode(opNameStr, klCodeStr, entryStr, &stat);
      if(stat == MStatus::kFailure){
        return MS::kFailure;
      }
    }
    else if(actionStr == "setKLOperatorFile")
    {
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      MString fileNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "fileName").c_str();
      MString entryStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "entry", "", true).c_str();
      interf->setKLOperatorFile(opNameStr, fileNameStr, entryStr);
    }
    else if(actionStr == "setKLOperatorEntry")
    {
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      MString entryStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "entry").c_str();
      interf->setKLOperatorEntry(opNameStr, entryStr);
    }
    else if(actionStr == "setKLOperatorIndex")
    {
      // parse args
      MString opNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "opName").c_str();
      int opIndex = FabricSplice::Scripting::consumeIntegerArgument(scriptArgs, "index");

      interf->setKLOperatorIndex(opNameStr, opIndex);
    } 
    else if(actionStr == "saveSplice")
    {
      MString fileNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "fileName", "", true).c_str();
      if(fileNameStr.length() == 0)
      {
        QString qFileName = QFileDialog::getSaveFileName( 
          MQtUtil::mainWindow(), 
          "Choose Splice file", 
          QDir::currentPath(), 
          "Splice files (*.splice);;All files (*.*)"
        );
        
        if(qFileName.isNull())
        {
          mayaLogErrorFunc("No filename specified.");
          return mayaErrorOccured();
        }
#ifdef _WIN32
        fileNameStr = qFileName.toLocal8Bit().constData();
#else
        fileNameStr = qFileName.toUtf8().constData();
#endif      
      }
      interf->saveToFile(fileNameStr);
    }
    else if(actionStr == "loadSplice")
    {
      MString fileNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "fileName", "", true).c_str();

      int portLimit = 0;
      if(spliceMayaNodeFn.typeName() == "spliceMayaDeformer")
        portLimit = 1;
      if(interf->getPortNames().length() > portLimit)
      {
        mayaLogErrorFunc("SpliceNode '"+referenceStr+"' already contains ports. Please create a new one.");
        return mayaErrorOccured();
      }
      if(fileNameStr.length() == 0)
      {
        QString qFileName = QFileDialog::getOpenFileName( 
          MQtUtil::mainWindow(), 
          "Choose Splice file", 
          QDir::currentPath(), 
          "Splice files (*.splice);;All files (*.*)"
        );
        
        if(qFileName.isNull())
        {
          mayaLogErrorFunc("No filename specified.");
          return mayaErrorOccured();
        }
    #ifdef _WIN32
        fileNameStr = qFileName.toLocal8Bit().constData();
    #else
        fileNameStr = qFileName.toUtf8().constData();
    #endif      
      }
      interf->loadFromFile(fileNameStr);
      return mayaErrorOccured();
    }
    else if(actionStr == "getPortInfo")
    {
      MString portInfo = interf->getSpliceGraph().getDGPortInfo().c_str();
      setResult(portInfo);
    }
    else if(actionStr == "setPortPersistence")
    {
      MString portNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "portName").c_str();
      bool persistence = FabricSplice::Scripting::consumeBooleanArgument(scriptArgs, "persistence");
      interf->setPortPersistence(portNameStr, persistence);
    }
    else if(actionStr == "getPortData")
    {
      MString portNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "portName").c_str();

      FabricSplice::DGPort port = interf->getSpliceGraph().getDGPort(portNameStr.asChar());
      if(!port.isValid())
      {
        mayaLogErrorFunc("Port '"+portNameStr+"' does not exist.");
        return mayaErrorOccured();
      }
      if(port.getSliceCount() != 1)
      {
        mayaLogErrorFunc("Port '"+portNameStr+"' has multiple slices.");
        return mayaErrorOccured();
      }
      FabricCore::Variant portDataVar = port.getVariant();
      MString portData = portDataVar.getJSONEncoding().getStringData();
      setResult(portData);
    }
    else if(actionStr == "setPortData")
    {
      // parse args
      MString portNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "portName").c_str();
      MString auxiliaryStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "auxiliary").c_str();

      FabricSplice::DGPort port = interf->getSpliceGraph().getDGPort(portNameStr.asChar());
      if(!port.isValid())
      {
        mayaLogErrorFunc("Port '"+portNameStr+"' does not exist.");
        return mayaErrorOccured();
      }
      if(port.getSliceCount() != 1)
      {
        mayaLogErrorFunc("Port '"+portNameStr+"' has multiple slices.");
        return mayaErrorOccured();
      }
      if(port.getMode() == FabricSplice::Port_Mode_OUT)
      {
        mayaLogErrorFunc("Port '"+portNameStr+"' is an output port.");
        return mayaErrorOccured();
      }
      MString dataTypeStr = port.getDataType();
      FabricCore::Variant portDataVar;
      bool isArray = port.isArray();
      if(!port.isArray())
      {
        if(dataTypeStr == "String" && !isArray)
          portDataVar = FabricCore::Variant::CreateString(auxiliaryStr.asChar());
        else if( dataTypeStr == "SInt16" || dataTypeStr == "UInt16" )
          portDataVar = FabricCore::Variant::CreateSInt32(auxiliaryStr.asShort());
        else if(dataTypeStr == "Integer" || dataTypeStr == "SInt32" || dataTypeStr == "SInt64" )
          portDataVar = FabricCore::Variant::CreateSInt32(auxiliaryStr.asInt());
        else if(dataTypeStr == "Size"  || dataTypeStr == "Index" || dataTypeStr == "UInt32" || dataTypeStr == "UInt64")
          portDataVar = FabricCore::Variant::CreateSInt32(auxiliaryStr.asUnsigned());
        else if(dataTypeStr == "Float32")
          portDataVar = FabricCore::Variant::CreateFloat64(auxiliaryStr.asFloat());
        else if(dataTypeStr == "Scalar" || dataTypeStr == "Float64")
          portDataVar = FabricCore::Variant::CreateFloat64(auxiliaryStr.asDouble());
        else
          portDataVar = FabricCore::Variant::CreateFromJSON(auxiliaryStr.asChar());
      }
      else
        portDataVar = FabricCore::Variant::CreateFromJSON(auxiliaryStr.asChar());
      port.setVariant(portDataVar);
      interf->setPortPersistence(portNameStr, true);
    }
    // else if(actionStr == "setManipulationCommand"){
    //   MString commandNameStr = FabricSplice::Scripting::consumeStringArgument(scriptArgs, "commandName").c_str();
    //   interf->setManipulationCommand(commandNameStr);
    // }
    else
    {
      mayaLogErrorFunc("Action '" + actionStr + "' not supported.");
    }
  }
  catch(FabricSplice::Exception e)
  {
    return MS::kFailure;
  }

  // inform our UI
  FabricSpliceEditorWidget::postUpdateAll();
  return mayaErrorOccured();
}

