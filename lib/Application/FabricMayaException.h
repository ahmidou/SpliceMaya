//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef __FABRIC_MAYA_MAYA_EXCEPTION__
#define __FABRIC_MAYA_MAYA_EXCEPTION__

#include <FabricSplice.h>
#include <FabricUI/Application/FabricException.h>

namespace FabricMaya {
namespace Application {

class FabricMayaException : public FabricUI::Application::FabricException
{
  /**
    FabricMayaException specializes FabricException for Maya.
  
    It defines a macro FABRIC_MAYA_CATCH_BEGIN() - FABRIC_MAYA_CATCH_END() that 
    catchs and then logs: 
    - JSON Exception
    - Fabric Exception
    - FabricCore Exception
    - FabricSplice Exception
  */
  public: 
    FabricMayaException(
      QString const&message
      );
    
    virtual ~FabricMayaException() throw();
    
    /// Implementation of FabricException.
    /// Logs the error in the maya editor.
    virtual void log() const throw();
};

} // namespace FabricMaya
} // namespace FabricUI

#define FABRIC_MAYA_CATCH_BEGIN() \
  try {

#define FABRIC_MAYA_CATCH_END(methodName) \
  } \
  catch (FabricCore::Exception &e) \
  { \
    FabricUI::Application::FabricException::Throw( \
      methodName, \
      "Caught Core Exception", \
      e.getDesc_cstr(), \
      FabricUI::Application::FabricException::LOG \
      ); \
  } \
  catch (FTL::JSONException &je) \
  { \
    FabricUI::Application::FabricException::Throw( \
      methodName, \
      "Caught JSON Exception", \
      je.getDescCStr(), \
      FabricUI::Application::FabricException::LOG \
      ); \
  } \
  catch (FabricUI::Application::FabricException &e) \
  { \
    FabricUI::Application::FabricException::Throw( \
      methodName, \
      "Caught App Exception", \
      e.what(), \
      FabricUI::Application::FabricException::LOG \
      ); \
  } \
  catch (FabricSplice::Exception &e) \
  { \
    FabricUI::Application::FabricException::Throw( \
      methodName, \
      "Caught Splice Exception", \
      e.what(), \
      FabricUI::Application::FabricException::LOG \
      ); \
  } 

#endif // __FABRIC_MAYA_MAYA_EXCEPTION__
