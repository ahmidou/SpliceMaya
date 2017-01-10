//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <QDialog>
#include <DFG/DFGUICmdHandler_QUndo.h>
#include <ModelItems/BindingModelItem.h>
#include <ValueEditor/VEEditorOwner.h>

class FabricImportPatternDialog : public QDialog
{
  Q_OBJECT
  
public:
  FabricImportPatternDialog(QWidget * parent, FabricCore::DFGBinding binding);
  virtual ~FabricImportPatternDialog();

  bool wasAccepted() const { return m_wasAccepted; }

public slots:
  void onAccepted() { m_wasAccepted = true; close(); }
  void onRejected() { m_wasAccepted = false; close(); }

private:
  QUndoStack * m_stack;
  FabricUI::DFG::DFGUICmdHandler * m_handler;
  FabricUI::ModelItems::BindingModelItem * m_bindingItem;
  FabricUI::ValueEditor::VEEditorOwner * m_owner;
  bool m_wasAccepted;
};
