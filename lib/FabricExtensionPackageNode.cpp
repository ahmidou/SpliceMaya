//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricExtensionPackageNode.h"

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
