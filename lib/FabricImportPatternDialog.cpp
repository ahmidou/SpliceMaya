//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "FabricImportPatternDialog.h"
#include "FabricImportPatternCommand.h"

FabricImportPatternDialog::FabricImportPatternDialog(QWidget * parent, FabricCore::DFGBinding binding, MString rootPrefix)
: QDialog(parent)
, m_rootPrefix(rootPrefix)
, m_binding(binding)
, m_wasAccepted(false)
{
  setWindowTitle("Fabric Import Pattern");
  m_stack = new QUndoStack();
  m_handler = new FabricUI::DFG::DFGUICmdHandler_QUndo(m_stack);
  m_bindingItem = new FabricUI::ModelItems::BindingModelItem(m_handler, binding);
  m_owner = new FabricUI::ValueEditor::VEEditorOwner();
  m_owner->initConnections();
  m_owner->emitReplaceModelRoot( m_bindingItem );

  setSizePolicy(QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  setMinimumWidth(500);
  setMaximumWidth(1200);
  setMinimumHeight(800);

  QWidget * widget = m_owner->getWidget();
  widget->setSizePolicy(QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  widget->setParent(this);

  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);
  layout->addWidget(widget);

  QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  layout->addWidget(buttons);

  QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(onAccepted()));
  QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(onRejected()));
}

FabricImportPatternDialog::~FabricImportPatternDialog()
{
  delete m_owner;
  delete m_bindingItem;
  delete m_handler;
  delete m_stack;
}

void FabricImportPatternDialog::onAccepted()
{
  FabricCore::DFGBinding binding = m_binding;
  close();
  FabricImportPatternCommand().invoke(binding, m_rootPrefix);
}
