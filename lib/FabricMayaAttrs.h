//
//  Copyright 2010-2015 Fabric Software Inc. All rights reserved.
//

#ifndef _FabricMayaAttrs_h
#define _FabricMayaAttrs_h

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
  FabricSplice::Port_Mode portMode,
  FabricCore::Variant compoundStructure
  );

} // namespace FabricMaya

#endif //_FabricMayaAttrs_h
