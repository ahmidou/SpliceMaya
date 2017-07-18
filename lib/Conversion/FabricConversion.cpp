//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricConversion.h"
#include <maya/MFnMeshData.h>
#include <maya/MFnNurbsCurveData.h>

using namespace FabricCore;

// ***************** RTVal to Maya ***************** // 
// String
RTVal FabricConversion::MStringToString(MString const&str) {
  RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructStringRTVal(
    str.asChar()
    );

  CONVERSION_CATCH_END("FabricConversion::MStringToString");

  return rtVal;
}

RTVal FabricConversion::MStringArrayToStringArray(MStringArray const&array) {
  RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructVariableArrayRTVal("String");
  rtVal.setArraySize(array.length());

  for(unsigned int i = 0; i < array.length(); ++i)
    rtVal.setArrayElement(
      i, 
      FabricSplice::constructStringRTVal(array[i].asChar())
      );
 
  CONVERSION_CATCH_END("FabricConversion::MStringArrayToStringArray");

  return rtVal;
}

// Xfo 
RTVal FabricConversion::MMatrixToXfoArray(MMatrixArray const&array) { 
  RTVal xfoArrayVal;

  CONVERSION_CATCH_BEGIN;

  RTVal arrayVal = MMatrixToMat44Array(array); 

  xfoArrayVal = FabricSplice::constructVariableArrayRTVal("Xfo");
  xfoArrayVal.setArraySize(arrayVal.getArraySize());
  for(unsigned int i = 0; i < arrayVal.getArraySize(); ++i)
  {
    RTVal matVal = arrayVal.getArrayElement(i);
    xfoArrayVal.setArrayElement(i, FabricSplice::constructRTVal("Xfo", 1, &matVal));
  }

  CONVERSION_CATCH_END("FabricConversion::MMatrixToXfoArray");

  return xfoArrayVal; 
}

RTVal FabricConversion::MMatrixToXfo_dArray(MMatrixArray const&array) { 
  RTVal xfoArrayVal;

  CONVERSION_CATCH_BEGIN;

  RTVal arrayVal = MMatrixToMat44_dArray(array); 

  xfoArrayVal = FabricSplice::constructVariableArrayRTVal("Xfo_d");
  xfoArrayVal.setArraySize(arrayVal.getArraySize());
  for(unsigned int i = 0; i < arrayVal.getArraySize(); ++i)
  {
    RTVal matVal = arrayVal.getArrayElement(i);
    xfoArrayVal.setArrayElement(i, FabricSplice::constructRTVal("Xfo_d", 1, &matVal));
  }

  CONVERSION_CATCH_END("FabricConversion::MMatrixToXfo_dArray");

  return xfoArrayVal; 
}

// Geometry 
bool FabricConversion::MFnMeshToMesch(MFnMesh &mesh, RTVal rtVal) {
  
  CONVERSION_CATCH_BEGIN;

  if(!rtVal.isValid())
    return false;

  // determine if we need a topology update
  bool requireTopoUpdate = false;
  if(!requireTopoUpdate)
  {
    uint64_t nbPolygons = rtVal.callMethod("UInt64", "polygonCount", 0, 0).getUInt64();
    requireTopoUpdate = nbPolygons != (uint64_t)mesh.numPolygons();
  }
  if(!requireTopoUpdate)
  {
    uint64_t nbSamples = rtVal.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();
    requireTopoUpdate = nbSamples != (uint64_t)mesh.numFaceVertices();
  }

  MPointArray mayaPoints;
  MIntArray mayaCounts, mayaIndices;

  mesh.getPoints(mayaPoints);

  if(requireTopoUpdate)
  {
    // clear the mesh
    rtVal.callMethod("", "clear", 0, NULL);
  }

  if(mayaPoints.length() > 0)
  {
    std::vector<RTVal> args(2);
    args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
    args[1] = FabricSplice::constructUInt32RTVal(4); // components
    rtVal.callMethod("", "setPointsFromExternalArray_d", 2, &args[0]);
    mayaPoints.clear();
  }

  if(requireTopoUpdate)
  {
    mesh.getVertices(mayaCounts, mayaIndices);
    std::vector<RTVal> args(2);
    args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaCounts.length(), &mayaCounts[0]);
    args[1] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.length(), &mayaIndices[0]);
    rtVal.callMethod("", "setTopologyFromCountsIndicesExternalArrays", 2, &args[0]);
  }

  MFloatVectorArray mayaNormals;
  MIntArray mayaNormalsCounts, mayaNormalsIds;
  mesh.getNormals(mayaNormals);
  mesh.getNormalIds(mayaNormalsCounts, mayaNormalsIds);

  if(mayaNormals.length() > 0 && mayaNormalsCounts.length() > 0 && mayaNormalsIds.length() > 0)
  {
    MFloatVectorArray values;
    values.setLength(mayaNormalsIds.length());

    unsigned int offset = 0;
    for(unsigned int i=0;i<mayaNormalsIds.length();i++)
      values[offset++] = mayaNormals[mayaNormalsIds[i]];

    std::vector<RTVal> args(1);
    args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.length() * 3, &values[0]);
    rtVal.callMethod("", "setNormalsFromExternalArray", 1, &args[0]);
    values.clear();
  }

  if(mesh.numUVSets() > 0)
  {
    MFloatArray u, v, values;
    mesh.getUVs(u, v);
    unsigned int offset = 0;

    MIntArray counts, indices;
    mesh.getAssignedUVs(counts, indices);
    counts.clear();
    values.setLength(indices.length() * 2);
    if(values.length() > 0)
    {
      for(unsigned int i=0;i<indices.length(); i++)
      {
        values[offset++] = u[indices[i]];
        values[offset++] = v[indices[i]];
      }
      u.clear();
      v.clear();

      std::vector<RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.length(), &values[0]);
      args[1] = FabricSplice::constructUInt32RTVal(2); // components
      rtVal.callMethod("", "setUVsFromExternalArray", 2, &args[0]);
      values.clear();
    }
  }

  if(mesh.numColorSets() > 0)
  {
    MColorArray faceValues;

    MStringArray colorSetNames;
    mesh.getColorSetNames(colorSetNames);
    MString colorSetName = colorSetNames[0];

    mesh.getFaceVertexColors(faceValues, &colorSetName);
    if(faceValues.length() > 0)
    {
      std::vector<RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("Float32", faceValues.length() * 4, &faceValues[0]);
      args[1] = FabricSplice::constructUInt32RTVal(4); // components
      rtVal.callMethod("", "setVertexColorsFromExternalArray", 2, &args[0]);
      faceValues.clear();
    }
  }

  mayaCounts.clear();
  mayaIndices.clear();

  CONVERSION_CATCH_END("FabricConversion::MFnMeshToMesch");

  return true;
}

RTVal FabricConversion::MFnMeshToMesch(MFnMesh &mesh) {
  RTVal rtVal;
  CONVERSION_CATCH_BEGIN;
  rtVal = FabricSplice::constructObjectRTVal("PolygonMesh");
  MFnMeshToMesch(mesh, rtVal);
  CONVERSION_CATCH_END("FabricConversion::MFnMeshToMesch");
  return rtVal;
}

bool FabricConversion::MFnNurbsCurveToLine(MFnNurbsCurve &curve, RTVal rtVal) {
  
  CONVERSION_CATCH_BEGIN;

  if(!rtVal.isValid())
    return false;

  MPointArray mayaPoints;
  curve.getCVs(mayaPoints);
  std::vector<double> mayaDoubles(mayaPoints.length() * 3);

  size_t nbSegments = (mayaPoints.length() - 1);
  if(curve.form() == MFnNurbsCurve::kClosed)
    nbSegments++;

  std::vector<uint32_t> mayaIndices(nbSegments * 2);

  size_t voffset = 0;
  size_t coffset = 0;
  for(unsigned int i=0;i<mayaPoints.length();i++)
  {
    mayaDoubles[voffset++] = mayaPoints[i].x;
    mayaDoubles[voffset++] = mayaPoints[i].y;
    mayaDoubles[voffset++] = mayaPoints[i].z;
    if(i < mayaPoints.length() - 1)
    {
      mayaIndices[coffset++] = i;
      mayaIndices[coffset++] = i + 1;
    }
    else if(curve.form() == MFnNurbsCurve::kClosed)
    {
      mayaIndices[coffset++] = i;
      mayaIndices[coffset++] = 0;
    }
  }

  RTVal mayaDoublesVal = FabricSplice::constructExternalArrayRTVal("Float64", mayaDoubles.size(), &mayaDoubles[0]);
  rtVal.callMethod("", "_setPositionsFromExternalArray_d", 1, &mayaDoublesVal);

  RTVal mayaIndicesVal = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.size(), &mayaIndices[0]);
  rtVal.callMethod("", "_setTopologyFromExternalArray", 1, &mayaIndicesVal);

  mayaPoints.clear();

  CONVERSION_CATCH_END("FabricConversion::MFnNurbsCurveToLine");
  return true;
}

RTVal FabricConversion::MFnNurbsCurveToLine(MFnNurbsCurve &curve) {
  RTVal rtVal;
  CONVERSION_CATCH_BEGIN;
  rtVal = FabricSplice::constructObjectRTVal("Lines");
  MFnNurbsCurveToLine(curve, rtVal);
  CONVERSION_CATCH_END("FabricConversion::MFnNurbsCurveToLine");
  return rtVal;
}

bool FabricConversion::MFnNurbsCurveToCurve(unsigned int index, MFnNurbsCurve &curve, RTVal rtVal) {
  if(!rtVal.isValid())
    return false;

  RTVal args[ 6 ];
  args[0] = FabricSplice::constructUInt32RTVal(index);
  args[1] = FabricSplice::constructUInt8RTVal(uint8_t( curve.degree() ));

  uint8_t curveForm = uint8_t(curve.form());
  if( curveForm == MFnNurbsCurve::kOpen )
    curveForm = 0;//curveForm_open
  else if( curveForm == MFnNurbsCurve::kClosed )
    curveForm = 1;//curveForm_closed
  else if( curveForm == MFnNurbsCurve::kPeriodic )
    curveForm = 2;//curveForm_periodic

  args[2] = FabricSplice::constructUInt8RTVal(curveForm);

  MPointArray mayaPoints;
  curve.getCVs( mayaPoints );
  args[3] = FabricSplice::constructExternalArrayRTVal( "Float64", mayaPoints.length() * 4, &mayaPoints[0] );

  MDoubleArray mayaKnots;
  curve.getKnots( mayaKnots );
  args[4] = FabricSplice::constructExternalArrayRTVal( "Float64", mayaKnots.length(), &mayaKnots[0] );

  rtVal.callMethod( "", "setCurveFromMaya", 5, args );
  return true;
}
// ***************** RTVal to Maya ***************** // 


// ***************** RTVal to Maya ***************** // 
// String
MString FabricConversion::StringToMString(RTVal rtVal) {
  assert( rtVal.hasType( "String" ) );

  MString str;

  CONVERSION_CATCH_BEGIN;

  str = rtVal.getStringCString();

  CONVERSION_CATCH_END("FabricConversion::StringToMString");

  return str;
}

MStringArray FabricConversion::StringArrayToMStringArray(RTVal rtVal) {
  assert( rtVal.hasType( "String[]" ) );

  MStringArray strs;

  CONVERSION_CATCH_BEGIN;

  unsigned int elements = rtVal.isNullObject() 
    ? 0 
    : rtVal.getArraySize();

  strs.setLength(elements);
  for(unsigned int i = 0; i < elements; ++i)
    strs.set(StringToMString(rtVal.getArrayElementRef(i)), i);
 
  CONVERSION_CATCH_END("FabricConversion::StringArrayToMStringArray");

  return strs;
}

// Xfo 
MMatrixArray FabricConversion::XfoArrayToMMatrixArray(RTVal rtVal) {
  assert( rtVal.hasType( "Xfo[]" ) );

  MMatrixArray matrices;

  CONVERSION_CATCH_BEGIN;

  unsigned int elements = rtVal.isNullObject() 
    ? 0 
    : rtVal.getArraySize();

  matrices.setLength(elements);
  for(unsigned int i = 0; i < elements; ++i)
    matrices.set(XfoToMMatrix(rtVal.getArrayElementRef(i)), i);

  CONVERSION_CATCH_END("FabricConversion::XfoArrayToMMatrixArray");

  return matrices;
}

MMatrixArray FabricConversion::Xfo_dArrayToMMatrixArray(RTVal rtVal) {
  assert( rtVal.hasType( "Xfo_d[]" ) );

  MMatrixArray matrices;

  CONVERSION_CATCH_BEGIN;

  unsigned int elements = rtVal.isNullObject() 
    ? 0 
    : rtVal.getArraySize();

  matrices.setLength(elements);
  for(unsigned int i = 0; i < elements; ++i)
    matrices.set(Xfo_dToMMatrix(rtVal.getArrayElementRef(i)), i);

  CONVERSION_CATCH_END("FabricConversion::Xfo_dArrayToMMatrixArray");

  return matrices;
}

// Geometry 
MObject FabricConversion::MeschToMFnMesh(RTVal rtVal, bool insideCompute, MFnMesh &mesh) {
  assert( rtVal.hasType( "PolygonMesh" ) );

  MObject result;
  
  CONVERSION_CATCH_BEGIN;

  unsigned int nbPoints   = 0;
  unsigned int nbPolygons = 0;
  unsigned int nbSamples  = 0;
  if(!rtVal.isNullObject())
  {
    nbPoints   = rtVal.callMethod("UInt64", "pointCount",         0, 0).getUInt64();
    nbPolygons = rtVal.callMethod("UInt64", "polygonCount",       0, 0).getUInt64();
    nbSamples  = rtVal.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();
  }

  MPointArray  mayaPoints;
  MVectorArray mayaNormals;
  MIntArray    mayaCounts;
  MIntArray    mayaIndices;

  #if MAYA_API_VERSION < 201500         // FE-5118 ("crash when saving scene with an empty polygon mesh")

  if (nbPoints < 3 || nbPolygons == 0)
  {
    // the rtVal is either empty or has no polygons, so in order to
    // avoid a crash in Maya 2013 and 2014 we create a mesh with a
    // single triangle (and try to preserve the vertices, if any).

    if (nbPoints < 3)
    {
      // we only create the three vertices if there aren't enough.
      // (note: Maya correctly sets the vertices if at least one triangle is present).
      mayaPoints.setLength(3);
      mayaPoints[0] = MPoint(0, 0, 0, 0);
      mayaPoints[1] = MPoint(0, 0, 0, 0);
      mayaPoints[2] = MPoint(0, 0, 0, 0);
    }

    mayaCounts.setLength(1);
    mayaCounts[0] = 3;

    mayaIndices.setLength(3);
    mayaIndices[0] = 0;
    mayaIndices[1] = 1;
    mayaIndices[2] = 2;

    MFnMeshData meshDataFn;

    if(insideCompute)
    {
      MObject meshObject = meshDataFn.create();
      mesh.create( mayaPoints.length(), mayaCounts.length(), mayaPoints, mayaCounts, mayaIndices, meshObject );
      result = meshObject;
    }
    else
    {
      result = mesh.create( mayaPoints.length(), mayaCounts.length(), mayaPoints, mayaCounts, mayaIndices, MObject::kNullObj );
    }
    mesh.updateSurface();
  }
  else

  #endif
  {
    mayaPoints.setLength(nbPoints);
    if(mayaPoints.length() > 0)
    {
      std::vector<RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
      args[1] = FabricSplice::constructUInt32RTVal(4); // components
      rtVal.callMethod("", "getPointsAsExternalArray_d", 2, &args[0]);
    }

    mayaNormals.setLength(nbSamples);
    if(mayaNormals.length() > 0)
    {
      RTVal normalsVar = 
      FabricSplice::constructExternalArrayRTVal("Float64", mayaNormals.length() * 3, &mayaNormals[0]);
      rtVal.callMethod("", "getNormalsAsExternalArray_d", 1, &normalsVar);
    }

    mayaCounts.setLength(nbPolygons);
    mayaIndices.setLength(nbSamples);
    if(mayaCounts.length() > 0 && mayaIndices.length() > 0)
    {
      std::vector<RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaCounts.length(),  &mayaCounts[0]);
      args[1] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.length(), &mayaIndices[0]);
      rtVal.callMethod("", "getTopologyAsCountsIndicesExternalArrays", 2, &args[0]);
    }

    MFnMeshData meshDataFn;

    MIntArray normalFace, normalVertex;
    normalFace.setLength( mayaIndices.length() );
    normalVertex.setLength( mayaIndices.length() );

    int face = 0;
    int vertex = 0;
    int offset = 0;

    for( unsigned int i = 0; i < mayaIndices.length(); i++ ) {
      normalFace[i] = face;
      normalVertex[i] = mayaIndices[offset + vertex];
      vertex++;

      if( vertex == mayaCounts[face] ) {
        offset += mayaCounts[face];
        face++;
        vertex = 0;
      }
    }

    if(insideCompute)
    {
      MObject meshObject = meshDataFn.create();
      mesh.create( mayaPoints.length(), mayaCounts.length(), mayaPoints, mayaCounts, mayaIndices, meshObject );
      result = meshObject;
    }
    else
    {
      result = mesh.create( mayaPoints.length(), mayaCounts.length(), mayaPoints, mayaCounts, mayaIndices, MObject::kNullObj );
    }

    mesh.updateSurface();
    mayaPoints.clear();
    mesh.setFaceVertexNormals( mayaNormals, normalFace, normalVertex );

    if( !rtVal.isNullObject() ) {

      if( rtVal.callMethod( "Boolean", "hasUVs", 0, 0 ).getBoolean() ) {
        MFloatArray values( nbSamples * 2 );
        std::vector<RTVal> args( 2 );
        args[0] = FabricSplice::constructExternalArrayRTVal( "Float32", values.length(), &values[0] );
        args[1] = FabricSplice::constructUInt32RTVal( 2 ); // components
        rtVal.callMethod( "", "getUVsAsExternalArray", 2, &args[0] );

        MFloatArray u, v;
        u.setLength( nbSamples );
        v.setLength( nbSamples );
        unsigned int offset = 0;
        for( unsigned int i = 0; i < u.length(); i++ ) {
          u[i] = values[offset++];
          v[i] = values[offset++];
        }
        values.clear();
        MString setName( "map1" );
        mesh.createUVSet( setName );
        mesh.setCurrentUVSetName( setName );

        mesh.setUVs( u, v );

        MIntArray indices( nbSamples );
        for( unsigned int i = 0; i < nbSamples; i++ )
          indices[i] = i;
        mesh.assignUVs( mayaCounts, indices );
      }

      if( rtVal.callMethod( "Boolean", "hasVertexColors", 0, 0 ).getBoolean() ) {
        MColorArray values( nbSamples );
        std::vector<RTVal> args( 2 );
        args[0] = FabricSplice::constructExternalArrayRTVal( "Float32", values.length() * 4, &values[0] );
        args[1] = FabricSplice::constructUInt32RTVal( 4 ); // components
        rtVal.callMethod( "", "getVertexColorsAsExternalArray", 2, &args[0] );

        MString setName( "colorSet" );
        mesh.createColorSet( setName );
        mesh.setCurrentColorSetName( setName );

        MIntArray face( nbSamples );

        unsigned int offset = 0;
        for( unsigned int i = 0; i < mayaCounts.length(); i++ ) {
          for( int j = 0; j < mayaCounts[i]; j++, offset++ ) {
            face[offset] = i;
          }
        }

        mesh.setFaceVertexColors( values, face, mayaIndices );
      }
    }
  }

  CONVERSION_CATCH_END("FabricConversion::MeschToMFnMesh_single");

  return result;
}

MObject FabricConversion::MeschToMFnMesh(RTVal rtVal, bool insideCompute) {
  MFnMesh mesh;
  return MeschToMFnMesh(rtVal, insideCompute, mesh);
}

MObject FabricConversion::LinesToMFnNurbsCurve(RTVal rtVal, MFnNurbsCurve &curve) {
  MObject curveObject;

  CONVERSION_CATCH_BEGIN;

  unsigned int nbPoints = 0;
  unsigned int nbSegments = 0;
  if(!rtVal.isNullObject())
  {
    nbPoints   = rtVal.callMethod("UInt64", "pointCount", 0, 0).getUInt64();
    nbSegments = rtVal.callMethod("UInt64", "lineCount",  0, 0).getUInt64();
  }

  MPointArray mayaPoints(nbPoints);
  std::vector<double> mayaDoubles(nbPoints * 3);
  std::vector<uint32_t> mayaIndices(nbSegments*2);
  MDoubleArray mayaKnots(nbPoints);

  if(nbPoints > 0)
  {
    RTVal mayaDoublesVal = FabricSplice::constructExternalArrayRTVal("Float64", mayaDoubles.size(), &mayaDoubles[0]);
    rtVal.callMethod("", "_getPositionsAsExternalArray_d", 1, &mayaDoublesVal);
  }

  if(nbSegments > 0)
  {
    RTVal mayaIndicesVal = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.size(), &mayaIndices[0]);
    rtVal.callMethod("", "_getTopologyAsExternalArray", 1, &mayaIndicesVal);
  }

  size_t offset = 0;
  for(unsigned int i=0;i<nbPoints;i++) {
    mayaPoints[i].x = mayaDoubles[offset++];
    mayaPoints[i].y = mayaDoubles[offset++];
    mayaPoints[i].z = mayaDoubles[offset++];
    mayaPoints[i].w = 1.0;
    mayaKnots[i] = (double)i;
  }

  MFnNurbsCurve::Form form = MFnNurbsCurve::kOpen;
  if(mayaIndices.size() > 1)
  {
    if(mayaIndices[0] == mayaIndices[mayaIndices.size()-1])
      form = MFnNurbsCurve::kClosed; 
  }

  MFnNurbsCurveData curveDataFn;
  curveObject = curveDataFn.create();

  curve.create(
    mayaPoints, mayaKnots, 1, 
    form,
    false,
    false,
    curveObject);

  CONVERSION_CATCH_END("FabricConversion::LinesToMFnNurbsCurve_single");

  return curveObject;
}

MObject FabricConversion::LinesToMFnNurbsCurve(RTVal rtVal) {
  MFnNurbsCurve curve;
  return LinesToMFnNurbsCurve(rtVal, curve);
}

MObject FabricConversion::CurveToMFnNurbsCurve(RTVal rtVal, int index, MFnNurbsCurve &curve) {
  MObject curveObject;

  CONVERSION_CATCH_BEGIN;

  MFnNurbsCurve::Form form = MFnNurbsCurve::kOpen;
  unsigned int nbPoints = 0;
  unsigned int nbKnots = 0;
  unsigned int degree = 1;
  bool isRational = false;

  RTVal args[6];
  args[0] = FabricSplice::constructUInt32RTVal( index );//curveIndex

  if( !rtVal.isNullObject() ) {
    args[1] = FabricSplice::constructUInt8RTVal( 1 );//degree
    args[2] = FabricSplice::constructUInt8RTVal( 0 );//curveForm
    args[3] = FabricSplice::constructBooleanRTVal( false );//isRational
    args[4] = FabricSplice::constructUInt32RTVal( 0 );//knotCount
    args[5] = FabricSplice::constructUInt32RTVal( 0 );//pointCountWithDuplicates

    rtVal.callMethod( "", "getCurveInfoForMaya", 6, args );

    degree = args[1].getUInt8();

    int fabricForm = args[2].getUInt8();
    if( fabricForm == 1 )
      form = MFnNurbsCurve::kClosed;
    else if( fabricForm == 2 )
      form = MFnNurbsCurve::kPeriodic;

    isRational = args[3].getBoolean();

    nbKnots = args[4].getUInt32();
    nbPoints = args[5].getUInt32();
  }

  MPointArray mayaPoints( nbPoints );
  MDoubleArray mayaKnots( nbKnots );

  if( nbPoints > 0 ) {
    args[1] = FabricSplice::constructExternalArrayRTVal( "Float64", mayaPoints.length() * 4, &mayaPoints[0] );//points
    args[2] = FabricSplice::constructExternalArrayRTVal( "Float64", mayaKnots.length(), &mayaKnots[0] );//knots

    rtVal.callMethod( "", "getCurveDataForMaya", 3, args );
  }

  MFnNurbsCurveData curveDataFn;
  curveObject = curveDataFn.create();

  curve.create(
    mayaPoints, mayaKnots, degree,
    form,
    false,
    isRational,
    curveObject
    );

  CONVERSION_CATCH_END("FabricConversion::CurveToMFnNurbsCurve");

  return curveObject;
}

MObject FabricConversion::CurveToMFnNurbsCurve(RTVal rtVal, int index) {
  MFnNurbsCurve curve;
  return CurveToMFnNurbsCurve(rtVal, index, curve);
}

MObject FabricConversion::CurveToMFnNurbsCurve(RTVal curveRTVal, MFnNurbsCurve &curve) {

  MObject mFnNurbsCurve;

  CONVERSION_CATCH_BEGIN;

  RTVal curvesContainerRTVal = curveRTVal.callMethod( 
    "Curves", 
    "createCurvesContainerIfNone", 
    0, 0);

  unsigned int indexInCurvesContainer = curveRTVal.callMethod( 
    "UInt32", 
    "getCurveIndex", 
    0, 0).getUInt32();

  mFnNurbsCurve = CurveToMFnNurbsCurve( 
    curvesContainerRTVal, 
    indexInCurvesContainer, 
    curve);

  CONVERSION_CATCH_END("FabricConversion::CurveToMFnNurbsCurve");

  return mFnNurbsCurve;
}

MObject FabricConversion::CurveToMFnNurbsCurve(RTVal rtVal) {
  MFnNurbsCurve curve;
  return CurveToMFnNurbsCurve(rtVal, curve);
}
// ***************** RTVal to Maya ***************** // 
