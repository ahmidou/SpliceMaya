
#ifndef _FABRICDFGCOMMANDSTACK_H_
#define _FABRICDFGCOMMANDSTACK_H_

#include <Commands/CommandStack.h>
#include <DFG/Commands/DFGCommand.h>
#include <maya/MString.h>

class FabricDFGCommandStack: public FabricServices::Commands::CommandStack
{
public:

  struct Info
  {
    std::string name;
    unsigned int id;
    bool undoable;

    Info()
    {
      name = "";
      id = UINT_MAX;
      undoable = false;
    }

    Info(const std::string & in_name,  unsigned int in_id, bool in_undoable)
    {
      name = in_name;
      id = in_id;
      undoable = in_undoable;
    }
  };

  virtual bool add(FabricServices::Commands::Command * command);
  static void addCommandToIgnore(const std::string & name, unsigned id, bool undoable);
  static Info consumeCommandToIgnore(const std::string & name);
  static FabricDFGCommandStack * getStack();
  static MString getNodeNameFromCommand(FabricUI::DFG::DFGCommand * command);
  static void enableMayaCommands(bool state);

private:

  bool logMayaCommand(FabricServices::Commands::Command * genericCommand, unsigned int id, bool undoable = true, FabricServices::Commands::Command * lastCommand = NULL);
  std::string escapeString(const std::string & unescaped);

  static bool s_mayaCommandsEnabled;
  static std::vector<Info> s_pendingCommandsToIgnore;
};

#endif 
