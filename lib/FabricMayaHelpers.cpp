//
//  Copyright 2010-2015 Fabric Software Inc. All rights reserved.
//

#include "FabricMayaHelpers.h"

#include <assert.h>

namespace FabricMaya {

static void ThrowInvalidDataType( FTL::StrRef dataTypeStr )
{
  str::string error;
  error += "Unhandled DataType '";
  error += dataTypeStr;
  error += "'";
  throw FabricCore::Exception( error.c_str() );
}

DataType ParseDataType( FTL::StrRef dataTypeStr )
{
  FTL::StrRef dataTypeOverride = dataTypeStr.split('[').first;

  else if ( dataTypeOverride == FTL_STR("Boolean" ) )
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
  else ThrowInvalidDataType( dataTypeStr );
}

static void ThrowIncompatibleDataArrayTypes(
  FTL::StrRef dataTypeStr,
  FTL::StrRef arrayTypeStr
  )
{
  str::string error;
  error += "DataType '";
  error += dataTypeStr;
  error += "' incompatible with ArrayType '";
  error += dataTypeStr;
  error += "'";
  throw FabricCore::Exception( error.c_str() );
}

static void ThrowInvalidArrayType( FTL::StrRef arrayTypeStr )
{
  str::string error;
  error += "Unrecognized ArrayType '";
  error += arrayTypeStr;
  error += "'";
  throw FabricCore::Exception( error.c_str() );
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

  switch ( arrayType )
  {
    case AT_Single:
    case AT_Array_Native:
      // Do nothing
      break;

    case AT_Array_Multi:
      attr.setArray( true );
      if ( dataType != DT_KeyframeTrack )
        attr.setUsesArrayDataBuilder( true );
      break;

    default:
      assert( false );
      break;
  }
}

template<typename MFnAttributeTy>
void ApplyMinMax(
  MFnAttributeTy &attr,
  FabricCore::Variant compoundStructure
  )
{
  float uiMin = getScalarOption("uiMin", compoundStructure);
  float uiMax = getScalarOption("uiMax", compoundStructure);
  if(uiMin < uiMax) 
  {
    attr.setMin(uiMin);
    attr.setMax(uiMax);
    float uiSoftMin = getScalarOption("uiSoftMin", compoundStructure);
    float uiSoftMax = getScalarOption("uiSoftMax", compoundStructure);
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

MObject CreateMayaAttriubte(
  FTL::StrRef name,
  DataType dataType,
  FTL::StrRef dataTypeStr,
  ArrayType arrayType,
  FTL::StrRef arrayTypeStr,
  FabricSplice::Port_Mode portMode,
  FabricCore::Variant compoundStructure
  )
{
  bool storable = true;

  MFnAttribute attr;

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
            MString childName = childIter.getKey()->getStringData();

            const FabricCore::Variant * value = childIter.getValue();
            if ( !value || value->isNull() )
              continue;

            if ( value->isDict() )
            {
              const FabricCore::Variant * childDataType =
                value->getDictValue("dataType");
              if ( childDataType && childDataType->isString() )
              {
                MString childArrayTypeStr = "Single Value";
                MString childDataTypeStr = childDataType->getStringData();
                MStringArray childDataTypeStrParts;
                childDataTypeStr.split('[', childDataTypeStrParts);
                if(childDataTypeStrParts.length() > 1)
                  childArrayTypeStr = "Array (Multi)";

                childAttrs.append(
                  CreateMayaAttriubte(
                    childName,
                    childDataTypeStr,
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
                  CreateMayaAttriubte(
                    childName,
                    "CompoundParam",
                    "Single Value",
                    portMode,
                    *value // compoundStructure
                    )
                  );
              }
            }

            // now create the compound attribute
            MFnCompoundAttribute cAttr;
            attr = cAttr.create( name, name );
            for ( unsigned i=0; i<children.length(); i++ )
              cAttr.addChild(children[i]);
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
          MFnNumericAttribute nAttr;
          attr = nAttr.create( name, name, MFnNumericData::kBoolean );
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
          MFnNumericAttribute nAttr;
          attr = nAttr.create( name, name, MFnNumericData::kInt );
          ApplyMinMax( nAttr, compoundStructure );
        }
        break;

        case AT_Array_Native:
        {
          MFnTypedAttriubte tAttr;
          attr = tAttr.create(name, name, MFnData::kIntArray);
        }
        break;
      }
    }
    break;

    case DT_Scalar:
    {
      if ( arrayType == AT_Array_Native )
      {
        MFnTypedAttriubte tAttr;
        attr = tAttr.create(name, name, MFnData::kDoubleArray);
      }
      else
      {
        std::string scalarUnit =
          getStringOption( "scalarUnit", compoundStructure );
        if ( scalarUnit == "time" )
        {
          MFnUnitAttribute uAttr;
          attr = uAttr.create( name, name, MFnUnitAttribute::kTime );
          ApplyMinMax( uAttr, compoundStructure );
        }
        else if ( scalarUnit == "angle" )
        {
          MFnUnitAttribute uAttr;
          attr = uAttr.create( name, name, MFnUnitAttribute::kAngle );
          ApplyMinMax( uAttr, compoundStructure );
        }
        else if ( scalarUnit == "distance" )
        {
          MFnUnitAttribute uAttr;
          attr = uAttr.create( name, name, MFnUnitAttribute::kDistance );
          ApplyMinMax( uAttr, compoundStructure );
        }
        else
        {
          MFnNumericAttribute nAttr;
          attr = nAttr.create( name, name, MFnNumericData::kDouble );
          ApplyMinMax( nAttr, compoundStructure );
        }
      }
    }
    break;

    case DT_String:
    {
      MFnStringData emptyStringData;
      MObject emptyStringObject = emptyStringData.create("");

      if(arrayType == "Single Value")
      {
        MFnTypedAttriubte tAttr;
        attr = tAttr.create( name, name, MFnData::kString, emptyStringObject );
      }
      else if(arrayType == "Array (Multi)"){
        newAttribute = tAttr.create(name, name, MFnData::kString, emptyStringObject);
        tAttr.setArray(true);
        tAttr.setUsesArrayDataBuilder(true);
      }
      else
      {
        mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
        return newAttribute;
      }
    }
    break;

  else if(dataTypeOverride == "Color")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = nAttr.createColor(name, name);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = nAttr.createColor(name, name);
      nAttr.setArray(true);
      nAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Vec3")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = nAttr.createPoint(name, name);
      nAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      nAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
    }
    else if(arrayType == "Array (Multi)")
    {
      MObject x = nAttr.create(name+"_x", name+"_x", MFnNumericData::kDouble);
      nAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      nAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      MObject y = nAttr.create(name+"_y", name+"_y", MFnNumericData::kDouble);
      nAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      nAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      MObject z = nAttr.create(name+"_z", name+"_z", MFnNumericData::kDouble);
      nAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      nAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      newAttribute = cAttr.create(name, name);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
      cAttr.setArray(true);
      cAttr.setUsesArrayDataBuilder(true);
    }
    else if(arrayType == "Array (Native)")
    {
      newAttribute = tAttr.create(name, name, MFnData::kVectorArray);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Euler")
  {
    if(arrayType == "Single Value")
    {
      MObject x = uAttr.create(name+"_x", name+"_x", MFnUnitAttribute::kAngle);
      uAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      uAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      uAttr.setStorable(true);
      uAttr.setKeyable(true);
      MObject y = uAttr.create(name+"_y", name+"_y", MFnUnitAttribute::kAngle);
      uAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      uAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      uAttr.setStorable(true);
      uAttr.setKeyable(true);
      MObject z = uAttr.create(name+"_z", name+"_z", MFnUnitAttribute::kAngle);
      uAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      uAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      uAttr.setStorable(true);
      uAttr.setKeyable(true);
      newAttribute = nAttr.createPoint(name, name);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
    }
    else if(arrayType == "Array (Multi)")
    {
      MObject x = uAttr.create(name+"_x", name+"_x", MFnUnitAttribute::kAngle);
      uAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      uAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      uAttr.setStorable(true);
      MObject y = uAttr.create(name+"_y", name+"_y", MFnUnitAttribute::kAngle);
      uAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      uAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      uAttr.setStorable(true);
      MObject z = uAttr.create(name+"_z", name+"_z", MFnUnitAttribute::kAngle);
      uAttr.setReadable(portMode != FabricSplice::Port_Mode_IN);
      uAttr.setWritable(portMode != FabricSplice::Port_Mode_OUT);
      uAttr.setStorable(true);
      newAttribute = cAttr.create(name, name);
      cAttr.addChild(x);
      cAttr.addChild(y);
      cAttr.addChild(z);
      cAttr.setArray(true);
      cAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Mat44")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = mAttr.create(name, name);
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = mAttr.create(name, name);
      mAttr.setArray(true);
      mAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "PolygonMesh")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(name, name, MFnData::kMesh);
      storable = false;
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = tAttr.create(name, name, MFnData::kMesh);
      storable = false;
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "Lines")
  {
    if(arrayType == "Single Value")
    {
      newAttribute = tAttr.create(name, name, MFnData::kNurbsCurve);
      storable = false;
    }
    else if(arrayType == "Array (Multi)")
    {
      newAttribute = tAttr.create(name, name, MFnData::kNurbsCurve);
      storable = false;
      tAttr.setArray(true);
      tAttr.setUsesArrayDataBuilder(true);
    }
    else
    {
      mayaLogErrorFunc("DataType '"+dataType+"' incompatible with ArrayType '"+arrayType+"'.");
      return newAttribute;
    }
  }
  else if(dataTypeOverride == "KeyframeTrack"){
    
    if(arrayType == "Single Value")
    {
      if(_spliceGraph.getDGPort(name.asChar()).isValid()){
        newAttribute = pAttr.create(name, name);
        pAttr.setStorable(true);
        pAttr.setKeyable(true);
        pAttr.setCached(false);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + name);
        return newAttribute;
      }
    }
    else
    {
      if(_spliceGraph.getDGPort(name.asChar()).isValid()){
        newAttribute = pAttr.create(name, name);
        pAttr.setStorable(true);
        pAttr.setKeyable(true);
        pAttr.setArray(true);
        pAttr.setCached(false);
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + name);
        return newAttribute;
      }
    }
  }
  else if(dataTypeOverride == "SpliceMayaData"){
    
    FabricSplice::DGPort port = _spliceGraph.getDGPort(name.asChar());
    if(arrayType == "Single Value")
    {
      if(port.isValid()){
        newAttribute = tAttr.create(name, name, FabricSpliceMayaData::id);
        mSpliceMayaDataOverride.push_back(name.asChar());
        storable = false;

        // disable input conversion by default
        // only enable it again if there is a connection to the port
        port.setOption("disableSpliceMayaDataConversion", FabricCore::Variant::CreateBoolean(true));
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + name);
        return newAttribute;
      }
    }
    else
    {
      if(port.isValid()){
        newAttribute = tAttr.create(name, name, FabricSpliceMayaData::id);
        mSpliceMayaDataOverride.push_back(name.asChar());
        storable = false;
        tAttr.setArray(true);
        tAttr.setUsesArrayDataBuilder(true);

        // disable input conversion by default
        // only enable it again if there is a connection to the port
        port.setOption("disableSpliceMayaDataConversion", FabricCore::Variant::CreateBoolean(true));
      }
      else{
        mayaLogErrorFunc("Creating maya attribute failed, No port found with name " + name);
        return newAttribute;
      }
    }
  }
  else
  {
    mayaLogErrorFunc("DataType '"+dataType+"' not supported.");
    return newAttribute;
  }

  // set the mode
  if(!newAttribute.isNull())
  {
    bool readable = portMode != FabricSplice::Port_Mode_IN;
    nAttr.setReadable(readable);
    tAttr.setReadable(readable);
    mAttr.setReadable(readable);
    uAttr.setReadable(readable);
    cAttr.setReadable(readable);
    pAttr.setReadable(readable);

    bool writable = portMode != FabricSplice::Port_Mode_OUT;
    nAttr.setWritable(writable);
    tAttr.setWritable(writable);
    mAttr.setWritable(writable);
    uAttr.setWritable(writable);
    cAttr.setWritable(writable);
    pAttr.setWritable(writable);

    if(portMode == FabricSplice::Port_Mode_IN && storable)
    {
      nAttr.setKeyable(true);
      tAttr.setKeyable(true);
      mAttr.setKeyable(true);
      uAttr.setKeyable(true);
      cAttr.setKeyable(true);
      pAttr.setKeyable(true);

      nAttr.setStorable(true);
      tAttr.setStorable(true);
      mAttr.setStorable(true);
      uAttr.setStorable(true);
      cAttr.setStorable(true);
      pAttr.setStorable(true);
    }
  }

  assert( !attr.isNull() );

  SetupMayaAttribute(
    attr,
    dataType,
    dataTypeOverride,
    arrayType
    );

  return attr;
}
