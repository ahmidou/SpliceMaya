//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include <QMap>
#include <string>
#include <QString>
#include <iostream>
#include "FabricCanvasExecuteCommand.h"
#include <FabricUI/Commands/CommandManager.h>

inline QString encodeMELStringChars(
  QString str)
{
  QString result;

  QByteArray ba = str.toUtf8();
  for ( int i = 0; i < ba.size(); ++i )
  {
    switch ( ba[i] )
    {
      case '\"':
        result += "\\\"";
        break;
      
      case '\\':
        result += "\\\\";
        break;
      
      case '\t':
        result += "\\t";
        break;
      
      case '\r':
        result += "\\r";
        break;
      
      case '\n':
        result += "\\n";
        break;
      
      default:
        result += ba[i];
        break;
    }
  }

  return result;
}

inline QString encodeMELString(
  QString str)
{
  return "\'" + encodeMELStringChars( str ) + "\'";
}

void* FabricCanvasExecuteCommand::creator()
{
  return new FabricCanvasExecuteCommand;
}

bool FabricCanvasExecuteCommand::isUndoable() const
{
  return false;
}

MStatus FabricCanvasExecuteCommand::doIt(
  const MArgList &args)
{
  MStatus status;

  try
  {
    setHistoryOn(false);

    // Get the command name.
    MString name = args.asString(0, &status);
    std::cout << "FabricCanvasExecuteCommand::doIt " << name.asChar() << std::endl;

    // Get the command args.
    QMap<QString, QString > cmdArgs;
    for(unsigned int i=1; i<args.length(&status); ++i)
    {
      QString key = args.asString(i, &status).asChar();
      QString value = args.asString(++i, &status).asChar();

      std::cout << "key " << key.toUtf8().constData() << std::endl;
      std::cout << "value " << value.toUtf8().constData() << std::endl;
      std::cout << "value2 " << encodeMELString(value).toUtf8().constData() << std::endl;

      cmdArgs[key] = encodeMELString(value);
    }

    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

    // Create the fabric command.
    manager->createCommand(
      name.asChar(),
      cmdArgs
    );
   
    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    printf(
      "FabricCanvasExecuteCommand::doIt, exception: %s\n", 
      e.c_str());
  }

  return status;
}
