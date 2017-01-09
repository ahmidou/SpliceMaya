//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <FabricSplice.h>
#include <FTL/StrRef.h>
#include <maya/MObject.h>

namespace FabricMaya {

enum DataType
{
  DT_Boolean,
  DT_Integer,
  DT_Scalar,
  DT_String,
  DT_Vec3,
  DT_Euler,
  DT_Mat44,
  DT_Color,
  DT_PolygonMesh,
  DT_Curves,
  DT_Curve,
  DT_Lines,
  DT_KeyframeTrack,
  DT_SpliceMayaData,
  DT_CompoundParam
};

DataType ParseDataType(
  FTL::StrRef dataTypeStr
  );

enum ArrayType
{
  AT_Single,
  AT_Array_Multi,
  AT_Array_Native
};

ArrayType ParseArrayType(
  FTL::StrRef arrayTypeStr
  );

MObject CreateMayaAttribute(
  FTL::CStrRef name,
  DataType dataType,
  FTL::StrRef dataTypeStr,
  ArrayType arrayType,
  FTL::StrRef arrayTypeStr,
  bool isInput,
  bool isOutput,
  FabricCore::Variant compoundStructure
  );

} // namespace FabricMaya
