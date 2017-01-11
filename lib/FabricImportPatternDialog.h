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
  FabricImportPatternDialog(QWidget * parent, FabricCore::DFGBinding binding, MString rootPrefix);
  virtual ~FabricImportPatternDialog();

  bool wasAccepted() const { return m_wasAccepted; }

public slots:
  void onAccepted();
  void onRejected() { m_wasAccepted = false; close(); }

private:
  MString m_rootPrefix;
  QUndoStack * m_stack;
  FabricCore::DFGBinding m_binding;
  FabricUI::DFG::DFGUICmdHandler * m_handler;
  FabricUI::ModelItems::BindingModelItem * m_bindingItem;
  FabricUI::ValueEditor::VEEditorOwner * m_owner;
  bool m_wasAccepted;
};
