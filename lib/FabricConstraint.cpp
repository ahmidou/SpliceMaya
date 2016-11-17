//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricConstraint.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MVector.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>

MTypeId FabricConstraint::id(0x0011AE49);
MObject FabricConstraint::input;
MObject FabricConstraint::offset;
MObject FabricConstraint::rotateOrder;
MObject FabricConstraint::translate;
MObject FabricConstraint::rotate;
MObject FabricConstraint::scale;

FabricConstraint::FabricConstraint()
{
}

FabricConstraint::~FabricConstraint()
{
}

void* FabricConstraint::creator(){
	return new FabricConstraint();
}

MStatus FabricConstraint::initialize(){

  MFnNumericAttribute nAttr;
  MFnMatrixAttribute mAttr;
  MFnUnitAttribute uAttr;

  MObject x, y, z;

  input = mAttr.create("input", "input");
  mAttr.setWritable(true);
  mAttr.setReadable(false);
  mAttr.setKeyable(true);
  mAttr.setConnectable(true);
  addAttribute(input);

  offset = mAttr.create("offset", "offset");
  mAttr.setWritable(true);
  mAttr.setReadable(false);
  mAttr.setKeyable(false);
  mAttr.setKeyable(true);
  mAttr.setConnectable(true);
  addAttribute(offset);

  rotateOrder = nAttr.create("rotateOrder", "rotateOrder", MFnNumericData::kInt, 0);
  nAttr.setWritable(true);
  nAttr.setReadable(false);
  nAttr.setKeyable(false);
  nAttr.setKeyable(false);
  nAttr.setConnectable(false);
  nAttr.setMin(0);
  nAttr.setMax(5);
  addAttribute(rotateOrder);

  x = nAttr.create("translateX", "translateX", MFnNumericData::kDouble, 0.0);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  y = nAttr.create("translateY", "translateY", MFnNumericData::kDouble, 0.0);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  z = nAttr.create("translateZ", "translateZ", MFnNumericData::kDouble, 0.0);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  translate = nAttr.create("translate", "translate", x, y, z);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  addAttribute(translate);

  x = uAttr.create("rotateX", "rotateX", MFnUnitAttribute::kAngle);
  uAttr.setWritable(false);
  uAttr.setReadable(true);
  y = uAttr.create("rotateY", "rotateY", MFnUnitAttribute::kAngle);
  uAttr.setWritable(false);
  uAttr.setReadable(true);
  z = uAttr.create("rotateZ", "rotateZ", MFnUnitAttribute::kAngle);
  uAttr.setWritable(false);
  uAttr.setReadable(true);
  rotate = nAttr.create("rotate", "rotate", x, y, z);      
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  addAttribute(rotate);

  x = nAttr.create("scaleX", "scaleX", MFnNumericData::kDouble, 1.0);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  y = nAttr.create("scaleY", "scaleY", MFnNumericData::kDouble, 1.0);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  z = nAttr.create("scaleZ", "scaleZ", MFnNumericData::kDouble, 1.0);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  scale = nAttr.create("scale", "scale", x, y, z);
  nAttr.setWritable(false);
  nAttr.setReadable(true);
  addAttribute(scale);

  attributeAffects(input, translate);
  attributeAffects(input, rotate);
  attributeAffects(input, scale);
  attributeAffects(offset, translate);
  attributeAffects(offset, rotate);
  attributeAffects(offset, scale);
  attributeAffects(rotateOrder, rotate);

  return MS::kSuccess;
}

MStatus FabricConstraint::compute(const MPlug& plug, MDataBlock& data){

  MMatrix inputValue = data.inputValue(input).asMatrix();
  MMatrix offsetValue = data.inputValue(offset).asMatrix();
  MTransformationMatrix result = offsetValue * inputValue;

  if(plug.attribute() == translate)
  {
    MVector value = result.getTranslation(MSpace::kTransform);
    MDataHandle handle = data.outputValue(plug);
    handle.set(value.x, value.y, value.z);
    handle.setClean();
  }
  else if(plug.attribute() == rotate)
  {
    int rotateOrderValue = data.inputValue(rotateOrder).asInt();
    result.reorderRotation((MTransformationMatrix::RotationOrder)(rotateOrderValue+1));

    double value[3];
    MTransformationMatrix::RotationOrder ro;
    result.getRotation(value, ro);
    MDataHandle handle = data.outputValue(plug);
    handle.set(value[0], value[1], value[2]);
    handle.setClean();
  }
  else if(plug.attribute() == scale)
  {
    double value[3];
    result.getScale(value, MSpace::kTransform);
    MDataHandle handle = data.outputValue(plug);
    handle.set(value[0], value[1], value[2]);
    handle.setClean();
  }

  return MStatus::kSuccess;
}
