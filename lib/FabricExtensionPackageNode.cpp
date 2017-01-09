//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "Foundation.h"
#include "FabricExtensionPackageNode.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>

MTypeId FabricExtensionPackageNode::id(0x0011AE50);
MObject FabricExtensionPackageNode::extensionNames;
MObject FabricExtensionPackageNode::extensionSuffix;
MObject FabricExtensionPackageNode::extensionPackage;

std::vector<FabricExtensionPackageNode*> FabricExtensionPackageNode::_instances;

FabricExtensionPackageNode::FabricExtensionPackageNode()
{
  _loaded = false;
  _instances.push_back(this);
}

FabricExtensionPackageNode::~FabricExtensionPackageNode()
{
  for(size_t i=0;i<_instances.size();i++){
    if(_instances[i] == this){
      std::vector<FabricExtensionPackageNode*>::iterator iter = _instances.begin() + i;
      _instances.erase(iter);
      break;
    }
  }
}

void* FabricExtensionPackageNode::creator(){
	return new FabricExtensionPackageNode();
}

MStatus FabricExtensionPackageNode::initialize(){

  MFnTypedAttribute typedAttr;
  extensionNames = typedAttr.create("extensionNames", "extensionNames", MFnData::kString);
  typedAttr.setHidden(true);
  typedAttr.setInternal(true);
  addAttribute(extensionNames);

  extensionSuffix = typedAttr.create("extensionSuffix", "extensionSuffix", MFnData::kString);
  typedAttr.setHidden(true);
  typedAttr.setInternal(true);
  addAttribute(extensionSuffix);

  extensionPackage = typedAttr.create("extensionPackage", "extensionPackage", MFnData::kString);
  typedAttr.setHidden(true);
  typedAttr.setInternal(true);
  addAttribute(extensionPackage);

  return MS::kSuccess;
}

FabricExtensionPackageNode * FabricExtensionPackageNode::getInstanceByMObject(const MObject & obj) {

  for(size_t i=0;i<_instances.size();i++)
  {
    if(_instances[i]->thisMObject() == obj)
    {
      return _instances[i];
    }
  }
  return NULL;
}

FabricExtensionPackageNode * FabricExtensionPackageNode::getInstanceByIndex(unsigned int index)
{
  if (index < _instances.size())
    return _instances[index];
  return NULL;
}

unsigned int FabricExtensionPackageNode::getNumInstances()
{
  return (unsigned int)_instances.size();
}

MStringArray FabricExtensionPackageNode::getExtensionNames() const
{
  MString joined = MPlug(thisMObject(), extensionNames).asString();
  MStringArray result;
  joined.split(',', result);
  return result;
}

void FabricExtensionPackageNode::setExtensionNames(MStringArray values) const
{
  MString joined;
  for(unsigned int i=0;i<values.length();i++)
  {
    if(i > 0)
      joined += ",";
    joined += values[i];
  }
  MPlug(thisMObject(), extensionNames).setString(joined);
}

MStatus FabricExtensionPackageNode::loadPackage(FabricCore::Client client)
{
  if(_loaded)
    return MStatus::kSuccess;

  if(!client.isValid())
    return MStatus::kInvalidParameter;

  try
  {
    MString content = getExtensionPackage();
    FabricCore::ImportKLExtensions(client, content.asChar());

    MStringArray names = getExtensionNames();
    for(unsigned int i=0;i<names.length();i++)
    {
      MString name = names[i] + getExtensionSuffix();
      client.loadExtension(name.asChar(), "", false);
    }
  }
  catch(FabricCore::Exception e)
  {
    MString nodeName = MFnDependencyNode(thisMObject()).name();

    // we are not using an error here
    mayaLogFunc(nodeName + ": "+e.getDesc_cstr());
    return mayaErrorOccured();
  }

  _loaded = true;
  return MStatus::kSuccess;
}
