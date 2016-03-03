//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceMayaData.h"

#include <maya/MGlobal.h>

const MTypeId FabricSpliceMayaData::id( 0x0011AE45 );
const MString FabricSpliceMayaData::typeName( "FabricSpliceMayaData" );
MString MFnFabricSpliceMayaData::classNameString ("MFnFabricSpliceMayaData") ;

FabricSpliceMayaData::FabricSpliceMayaData(){
}

FabricSpliceMayaData::~FabricSpliceMayaData(){
}

MStatus FabricSpliceMayaData::readASCII( const MArgList & argList, unsigned & index ){
    return MS::kSuccess;
}

MStatus FabricSpliceMayaData::readBinary( std::istream & in, unsigned length ){
    return MS::kSuccess;
    
}

MStatus FabricSpliceMayaData::writeASCII( std::ostream & out ){
  return MS::kSuccess;
}

MStatus FabricSpliceMayaData::writeBinary( std::ostream & out ){
  return MS::kSuccess;
}

void FabricSpliceMayaData::copy ( const MPxData & other ){
  FabricSpliceMayaData &otherSpliceData = ( FabricSpliceMayaData &)other;
  mValue = otherSpliceData.mValue;
}

MTypeId FabricSpliceMayaData::typeId() const{
  return id;
}

MString FabricSpliceMayaData::name() const{
  return typeName;
}

void * FabricSpliceMayaData::creator(){
    return new FabricSpliceMayaData();
}

void FabricSpliceMayaData::setRTVal(const FabricCore::RTVal &value){
  mValue = value;
}

FabricCore::RTVal FabricSpliceMayaData::getRTVal() const{
  return mValue;
}

MFnFabricSpliceMayaData::MFnFabricSpliceMayaData (){
}

MFnFabricSpliceMayaData::MFnFabricSpliceMayaData (MObject &object, MStatus *ReturnStatus) : MFnPluginData (){
  MStatus status =setObject (object) ;
  if ( status != MS::kSuccess ){
    if ( ReturnStatus )
      *ReturnStatus =status ;
    return ;
  }
  if ( ReturnStatus )
    *ReturnStatus =MS::kSuccess ;
}

MFnFabricSpliceMayaData::~MFnFabricSpliceMayaData (){
}

MFn::Type MFnFabricSpliceMayaData::type () const {
  return (MFn::kPluginData) ;
}

const char *MFnFabricSpliceMayaData::className (){
  return (classNameString.asChar ()) ;
}

MStatus MFnFabricSpliceMayaData::setObject (MObject &object){
  MStatus status ;
  MObject oldObj =MFnPluginData::object () ;
  status =MFnPluginData::setObject (object) ;
  if ( status != MS::kSuccess )
    return (status) ;
  if ( MFnPluginData::typeId () != FabricSpliceMayaData::id ){
    (void)MFnPluginData::setObject (oldObj) ;
    return (MS::kInvalidParameter) ;
  }
  return (MS::kSuccess) ;
}

MStatus MFnFabricSpliceMayaData::setObject (const MObject &object){
  MObject oldObj =MFnPluginData::object () ;
  MStatus status =MFnPluginData::setObject (object) ;
  if ( status != MS::kSuccess )
    return (status) ;
  if ( MFnPluginData::typeId () != FabricSpliceMayaData::id ){
    (void)MFnPluginData::setObject (oldObj) ;
    return (MS::kInvalidParameter) ;
  }
  return (MS::kSuccess) ;
}

FabricCore::RTVal MFnFabricSpliceMayaData::getRTVal () const{
  if ( object ().isNull () )
    return FabricCore::RTVal();
  const FabricSpliceMayaData *myDataObj =static_cast<const FabricSpliceMayaData *>(this->constData ()) ;
  return (myDataObj->getRTVal()) ;
}

MStatus MFnFabricSpliceMayaData::setRTVal (const FabricCore::RTVal &newVal){
  if ( object ().isNull () )
    return (MS::kFailure) ;
  FabricSpliceMayaData *myDataObj =static_cast<FabricSpliceMayaData *>(this->data ()) ;
  myDataObj->setRTVal (newVal) ;
  return (MS::kSuccess) ;
}

MObject MFnFabricSpliceMayaData::create (MStatus *stat){
  return (MFnPluginData::create (FabricSpliceMayaData::id, stat)) ;
}

MObject MFnFabricSpliceMayaData::create (const FabricCore::RTVal &in, MStatus *ReturnStatus){
  MStatus status ;
  MObject newObj =MFnPluginData::create (FabricSpliceMayaData::id, &status) ;
  if ( status != MS::kSuccess ){ 
    if ( ReturnStatus )
      *ReturnStatus =status ;
    return (MObject::kNullObj) ;
  }
  FabricSpliceMayaData *myDataObj =static_cast<FabricSpliceMayaData *>(this->data ()) ;
  myDataObj->setRTVal (in) ;
  if ( ReturnStatus )
    *ReturnStatus =MS::kSuccess ;
  return (newObj) ;
}
