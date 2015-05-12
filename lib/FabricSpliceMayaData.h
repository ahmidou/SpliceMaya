
#ifndef _CREATIONSPLICEMAYADATA_H_
#define _CREATIONSPLICEMAYADATA_H_

#include <istream>
#include <ostream>

#include <maya/MPxData.h>
#include <maya/MTypeId.h>
#include <maya/MArgList.h>
#include <maya/MString.h>
#include <maya/MFnPluginData.h>

#include <FabricSplice.h>

class FabricSpliceMayaData : public MPxData
{
public:
  FabricSpliceMayaData();
  virtual ~FabricSpliceMayaData();

  virtual MStatus readASCII( const MArgList & argList, unsigned & idx );
  virtual MStatus readBinary( std::istream & in, unsigned length );
  virtual MStatus writeASCII( std::ostream & out );
  virtual MStatus writeBinary( std::ostream & out );

  virtual void copy( const MPxData & other );

  virtual MTypeId typeId() const;
  virtual MString name() const;

  static const MString typeName;
  static const MTypeId id;
  static void * creator();

  // custom data accessors
  void setRTVal(const FabricCore::RTVal & value);
  FabricCore::RTVal getRTVal() const;

private:
  FabricCore::RTVal mValue;
};

class  MFnFabricSpliceMayaData : public MFnPluginData {

public:
  static MString classNameString ;

public:
  MFnFabricSpliceMayaData();
  MFnFabricSpliceMayaData (MObject &object, MStatus *ReturnStatus);
  ~MFnFabricSpliceMayaData();
  
  MFn::Type type () const;
  const char *className ();

  virtual MStatus setObject (MObject &object);
  virtual MStatus setObject (const MObject &object);
  FabricCore::RTVal getRTVal () const;
  MStatus setRTVal (const FabricCore::RTVal &newVal);

  MObject create (MStatus *stat =NULL);
  MObject create (const FabricCore::RTVal &in, MStatus *stat =NULL);
} ;

#endif
