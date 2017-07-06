//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricMayaException.h"
#include "FabricSpliceHelpers.h"

FabricMayaException::FabricMayaException(
  QString const&message)
  : FabricUI::Application::FabricException(message)
{
}

FabricMayaException::~FabricMayaException() throw() 
{
}

void FabricMayaException::log() const throw()
{
  mayaLogErrorFunc(
    m_message.c_str()
    );
}
