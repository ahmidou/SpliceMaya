//
//  Copyright 2010-2015 Fabric Software Inc. All rights reserved.
//

#include "FabricMayaAttrs.h"
#include "FabricSpliceMayaData.h"

#include <assert.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MObjectArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

namespace FabricMaya {

static void ThrowIncompatibleDataArrayTypes(
  FTL::StrRef dataTypeStr,
  FTL::StrRef arrayTypeStr
  )
{
  std::string error;
  error += "DataType '";
  error += dataTypeStr;
  error += "' incompatible with ArrayType '";
  error += dataTypeStr;
  error += "'";
  throw FabricCore::Exception( error.c_str() );
}

static void ThrowInvalidDataType( FTL::StrRef dataTypeStr )
{
  std::string error;
  error += "Unhandled DataType '";
  error += dataTypeStr;
  error += "'";
  throw FabricCore::Exception( error.c_str() );
}

DataType ParseDataType( FTL::StrRef dataTypeStr )
{
  FTL::StrRef dataTypeOverride = dataTypeStr.split('[').first;

  if ( dataTypeOverride == FTL_STR("Boolean" ) )
    return DT_Boolean;
  else if ( dataTypeOverride == FTL_STR("Integer" ) )
    return DT_Integer;
  else if ( dataTypeOverride == FTL_STR("Scalar" ) )
    return DT_Scalar;
  else if ( dataTypeOverride == FTL_STR("String" ) )
    return DT_String;
  else if ( dataTypeOverride == FTL_STR("Vec3" ) )
    return DT_Vec3;
  else if ( dataTypeOverride == FTL_STR("Euler" ) )
    return DT_Euler;
  else if ( dataTypeOverride == FTL_STR("Mat44" ) )
    return DT_Mat44;
  else if ( dataTypeOverride == FTL_STR("Color" ) )
    return DT_Color;
  else if ( dataTypeOverride == FTL_STR("PolygonMesh" ) )
    return DT_PolygonMesh;
  else if ( dataTypeOverride == FTL_STR("Lines" ) )
    return DT_Lines;
  else if ( dataTypeOverride == FTL_STR("KeyframeTrack" ) )
    return DT_KeyframeTrack;
  else if ( dataTypeOverride == FTL_STR("SpliceMayaData" ) )
    return DT_SpliceMayaData;
  else if ( dataTypeOverride == FTL_STR("CompoundParam" ) )
    return DT_CompoundParam;

  ThrowInvalidDataType( dataTypeStr );

  // avoid warnings
  return DT_Boolean;
}

static void ThrowInvalidArrayType( FTL::StrRef arrayTypeStr )
{
  std::string error;
  error += "Unrecognized ArrayType '";
  error += arrayTypeStr;
  error += "'";
  throw FabricCore::Exception( error.c_str() );
}

ArrayType ParseArrayType( FTL::StrRef arrayTypeStr )
{
  if ( arrayTypeStr == FTL_STR("Single Value" ) )
    return AT_Single;
  else if ( arrayTypeStr == FTL_STR("Array (Multi)" ) )
    return AT_Array_Multi;
  else if ( arrayTypeStr == FTL_STR("Array (Native)" ) )
    return AT_Array_Native;

  ThrowInvalidArrayType( arrayTypeStr );

  // avoid warnings
  return AT_Single;
}

static void SetupMayaAttribute(
  MFnAttribute &attr,
  DataType dataType,
  FTL::StrRef dataTypeStr,
  ArrayType arrayType,
  FTL::StrRef arrayTypeStr,
  FabricSplice::Port_Mode portMode
  )
{
  attr.setReadable( portMode != FabricSplice::Port_Mode_IN );
  attr.setWritable( portMode != FabricSplice::Port_Mode_OUT );

  switch ( dataType )
  {
    case DT_PolygonMesh:
    case DT_Lines:
    case DT_SpliceMayaData:
      attr.setStorable( false );
      attr.setKeyable( false );
      break;

    default:
      attr.setStorable( portMode == FabricSplice::Port_Mode_IN );
      attr.setKeyable( portMode == FabricSplice::Port_Mode_IN );
      break;
  }

  switch ( arrayType )
  {
    case AT_Single:
    case AT_Array_Native:
      attr.setArray( false );
      attr.setUsesArrayDataBuilder( false );
      break;

    case AT_Array_Multi:
      attr.setArray( true );
      attr.setUsesArrayDataBuilder( dataType != DT_KeyframeTrack );
      break;

    default:
      assert( false );
      break;
  }
}


static float GetScalarOption(
  const char *key,
  FabricCore::Variant options,
  float defaultValue = 0.0f
  )
{
  if(options.isNull())
    return defaultValue;
  if(!options.isDict())
    return defaultValue;
  const FabricCore::Variant * option = options.getDictValue(key);
  if(!option)
    return defaultValue;
  if(option->isSInt8())
    return (float)option->getSInt8();
  if(option->isSInt16())
    return (float)option->getSInt8();
  if(option->isSInt32())
    return (float)option->getSInt8();
  if(option->isSInt64())
    return (float)option->getSInt8();
  if(option->isUInt8())
    return (float)option->getUInt8();
  if(option->isUInt16())
    return (float)option->getUInt8();
  if(option->isUInt32())
    return (float)option->getUInt8();
  if(option->isUInt64())
    return (float)option->getUInt8();
  if(option->isFloat32())
    return (float)option->getFloat32();
  if(option->isFloat64())
    return (float)option->getFloat64();
  return defaultValue;
} 

static const char *GetStringOption(
  const char *key,
  FabricCore::Variant options,
  const char *defaultValue = ""
  )
{
  if(options.isNull())
    return defaultValue;
  if(!options.isDict())
    return defaultValue;
  const FabricCore::Variant * option = options.getDictValue(key);
  if(!option)
    return defaultValue;
  if(option->isString())
    return option->getString_cstr();
  return defaultValue;
} 

template<typename MFnAttributeTy>
void ApplyMinMax(
  MFnAttributeTy &attr,
  FabricCore::Variant compoundStructure
  )
{
  float uiMin = GetScalarOption("uiMin", compoundStructure);
  float uiMax = GetScalarOption("uiMax", compoundStructure);
  if(uiMin < uiMax) 
  {
    attr.setMin(uiMin);
    attr.setMax(uiMax);
    float uiSoftMin = GetScalarOption("uiSoftMin", compoundStructure);
    float uiSoftMax = GetScalarOption("uiSoftMax", compoundStructure);
    if(uiSoftMin < uiSoftMax) 
    {
      attr.setSoftMin(uiSoftMin);
      attr.setSoftMax(uiSoftMax);
    }
    else
    {
      attr.setSoftMin(uiMin);
      attr.setSoftMax(uiMax);
    }
  }
}

MObject CreateMayaAttribute(
  FTL::CStrRef nameCStr,
  DataType dataType,
  FTL::StrRef dataTypeStr,
  ArrayType arrayType,
  FTL::StrRef arrayTypeStr,
  FabricSplice::Port_Mode portMode,
  FabricCore::Variant compoundStructure
  )
{
  MString name( nameCStr.c_str() );

  MObject obj;

  switch ( dataType )
  {
    case DT_CompoundParam:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          if ( compoundStructure.isNull() )
            throw FabricCore::Exception(
              "CompoundParam used for a maya attribute but no compound structure provided."
              );

          if ( !compoundStructure.isDict() )
            throw FabricCore::Exception(
              "CompoundParam used for a maya attribute but compound structure does not contain a dictionary."
              );

          MObjectArray childAttrs;
          for ( FabricCore::Variant::DictIter childIter(compoundStructure);
            !childIter.isDone(); childIter.next() )
          {
            FTL::CStrRef childNameCStr = childIter.getKey()->getStringData();

            const FabricCore::Variant * value = childIter.getValue();
            if ( !value || value->isNull() )
              continue;

            if ( value->isDict() )
            {
              const FabricCore::Variant * childDataType =
                value->getDictValue("dataType");
              if ( childDataType && childDataType->isString() )
              {
                FTL::StrRef childArrayTypeStr = FTL_STR("Single Value");
                FTL::StrRef childDataTypeStr = childDataType->getStringData();
                if ( childDataTypeStr.find('[') != childDataTypeStr.end() )
                  childArrayTypeStr = FTL_STR("Array (Multi)");

                childAttrs.append(
                  CreateMayaAttribute(
                    childNameCStr,
                    ParseDataType( childDataTypeStr ),
                    childDataTypeStr,
                    ParseArrayType( childArrayTypeStr ),
                    childArrayTypeStr,
                    portMode,
                    *value // compoundStructure
                    )
                  );
              }
              else
              {
                // we assume it's a nested compound param
                childAttrs.append(
                  CreateMayaAttribute(
                    childNameCStr,
                    DT_CompoundParam,
                    FTL_STR("CompoundParam"),
                    AT_Single,
                    FTL_STR("Single Value"),
                    portMode,
                    *value // compoundStructure
                    )
                  );
              }
            }

            // now create the compound attribute
            MFnCompoundAttribute compoundAttr;
            obj = compoundAttr.create( name, name );
            for ( unsigned i=0; i<childAttrs.length(); i++ )
              compoundAttr.addChild( childAttrs[i] );
          }
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Boolean:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnNumericAttribute numAttr;
          obj = numAttr.create( name, name, MFnNumericData::kBoolean );
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Integer:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnNumericAttribute numAttr;
          obj = numAttr.create( name, name, MFnNumericData::kInt );
          ApplyMinMax( numAttr, compoundStructure );
        }
        break;

        case AT_Array_Native:
        {
          MFnTypedAttribute tAttr;
          obj = tAttr.create(name, name, MFnData::kIntArray);
        }
        break;
      }
    }
    break;

    case DT_Scalar:
    {
      if ( arrayType == AT_Array_Native )
      {
        MFnTypedAttribute tAttr;
        obj = tAttr.create(name, name, MFnData::kDoubleArray);
      }
      else
      {
        FTL::CStrRef scalarUnit =
          GetStringOption( "scalarUnit", compoundStructure );
        if ( scalarUnit == FTL_STR("time") )
        {
          MFnUnitAttribute uAttr;
          obj = uAttr.create( name, name, MFnUnitAttribute::kTime );
          ApplyMinMax( uAttr, compoundStructure );
        }
        else if ( scalarUnit == FTL_STR("angle") )
        {
          MFnUnitAttribute uAttr;
          obj = uAttr.create( name, name, MFnUnitAttribute::kAngle );
          ApplyMinMax( uAttr, compoundStructure );
        }
        else if ( scalarUnit == FTL_STR("distance") )
        {
          MFnUnitAttribute uAttr;
          obj = uAttr.create( name, name, MFnUnitAttribute::kDistance );
          ApplyMinMax( uAttr, compoundStructure );
        }
        else
        {
          MFnNumericAttribute numAttr;
          obj = numAttr.create( name, name, MFnNumericData::kFloat );
          ApplyMinMax( numAttr, compoundStructure );
        }
      }
    }
    break;

    case DT_String:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnStringData emptyStringData;
          MObject emptyStringObject = emptyStringData.create("");

          MFnTypedAttribute tAttr;
          obj = tAttr.create( name, name, MFnData::kString, emptyStringObject );
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Color:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnNumericAttribute numAttr;
          obj = numAttr.createColor(name, name);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Vec3:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnNumericAttribute numAttr;
          obj = numAttr.createPoint(name, name);
        }
        break;

        case AT_Array_Native:
        {
          MFnTypedAttribute tAttr;
          obj = tAttr.create(name, name, MFnData::kVectorArray);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Euler:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnUnitAttribute uAttrX;
          MObject objX = uAttrX.create( name+"X", name+"X", MFnUnitAttribute::kAngle );
          SetupMayaAttribute(
            uAttrX,
            DT_Scalar,
            FTL_STR("Float32"),
            AT_Single,
            FTL_STR("Single Value"),
            portMode
            );
          MFnUnitAttribute uAttrY;
          MObject objY = uAttrY.create( name+"Y", name+"Y", MFnUnitAttribute::kAngle );
          SetupMayaAttribute(
            uAttrY,
            DT_Scalar,
            FTL_STR("Float32"),
            AT_Single,
            FTL_STR("Single Value"),
            portMode
            );
          MFnUnitAttribute uAttrZ;
          MObject objZ = uAttrZ.create( name+"Z", name+"Z", MFnUnitAttribute::kAngle );
          SetupMayaAttribute(
            uAttrZ,
            DT_Scalar,
            FTL_STR("Float32"),
            AT_Single,
            FTL_STR("Single Value"),
            portMode
            );
          MFnNumericAttribute numAttr;
          obj = numAttr.create(name, name, objX, objY, objZ);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Mat44:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnMatrixAttribute mAttr;
          obj = mAttr.create(name, name, MFnMatrixAttribute::kFloat);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_PolygonMesh:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnTypedAttribute tAttr;
          obj = tAttr.create(name, name, MFnData::kMesh);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_Lines:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnTypedAttribute tAttr;
          obj = tAttr.create(name, name, MFnData::kNurbsCurve);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_KeyframeTrack:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnMessageAttribute msgAttr;
          obj = msgAttr.create(name, name);
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    case DT_SpliceMayaData:
    {
      switch ( arrayType )
      {
        case AT_Single:
        case AT_Array_Multi:
        {
          MFnTypedAttribute tyAttr;
          obj = tyAttr.create( name, name, FabricSpliceMayaData::id );
        }
        break;

        default: ThrowIncompatibleDataArrayTypes( dataTypeStr, arrayTypeStr );
      }
    }
    break;

    default:
      assert( false );
      break;
  }

  assert( !obj.isNull() );
  MFnAttribute attr( obj );
  SetupMayaAttribute(
    attr,
    dataType,
    dataTypeStr,
    arrayType,
    arrayTypeStr,
    portMode
    );

  return obj;
}

} // namespace FabricMaya
