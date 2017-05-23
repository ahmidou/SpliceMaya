//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QDialogButtonBox>

#include "FabricExportPatternDialog.h"
#include "FabricExportPatternCommand.h"

FabricExportPatternDialog::FabricExportPatternDialog(QWidget * parent, FabricCore::DFGBinding binding, FabricExportPatternSettings * settings)
: QDialog(parent)
, m_settings(settings)
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

  QWidget * optionsWidget = new QWidget(this);
  optionsWidget->setContentsMargins(0, 0, 0, 0);
  QGridLayout * optionsLayout = new QGridLayout();
  optionsWidget->setLayout(optionsLayout);
  layout->addWidget(optionsWidget);

  QLabel * scaleLabel = new QLabel("Scale", optionsWidget);
  optionsLayout->addWidget(scaleLabel, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * scaleLineEdit = new QLineEdit(optionsWidget);
  scaleLineEdit->setValidator(new QDoubleValidator());
  scaleLineEdit->setText(QString::number(m_settings->scale));
  optionsLayout->addWidget(scaleLineEdit, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(scaleLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onScaleChanged(const QString &)));

  QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  layout->addWidget(buttons);

  QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(onAccepted()));
  QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(onRejected()));
}

FabricExportPatternDialog::~FabricExportPatternDialog()
{
  delete m_owner;
  delete m_bindingItem;
  delete m_handler;
  delete m_stack;
}

void FabricExportPatternDialog::onAccepted()
{
  FabricCore::DFGBinding binding = m_binding;
  close();
  FabricExportPatternCommand().invoke(binding, *m_settings);
}

void FabricExportPatternDialog::onScaleChanged(const QString & text)
{
  m_settings->scale = text.toDouble();
}
