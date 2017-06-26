//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QDialogButtonBox>

#include "FabricExportPatternDialog.h"
#include "FabricExportPatternCommand.h"
#include "FabricImportPatternDialog.h"

FabricExportPatternDialog::FabricExportPatternDialog(QWidget * parent, FabricCore::Client client, FabricCore::DFGBinding binding, const FabricExportPatternSettings & settings)
: QDialog(parent)
, m_settings(settings)
, m_qSettings(NULL)
, m_client(client)
, m_binding(binding)
, m_wasAccepted(false)
{
  setWindowTitle("Fabric Export Pattern");

  if(m_settings.useLastArgValues)
    FabricImportPatternDialog::restoreSettings(client, m_settings.filePath, m_binding, &m_qSettings);

  m_stack = new QUndoStack();
  m_handler = new FabricUI::DFG::DFGUICmdHandler_QUndo(m_stack);
  m_bindingItem = new FabricUI::ModelItems::BindingModelItem(m_handler, binding, true, false, false);
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
  scaleLineEdit->setText(QString::number(m_settings.scale));
  optionsLayout->addWidget(scaleLineEdit, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(scaleLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onScaleChanged(const QString &)));

  QLabel * startFrameLabel = new QLabel("Start Frame", optionsWidget);
  optionsLayout->addWidget(startFrameLabel, 2, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * startFrameLineEdit = new QLineEdit(optionsWidget);
  startFrameLineEdit->setValidator(new QDoubleValidator());
  startFrameLineEdit->setText(QString::number(m_settings.startFrame));
  optionsLayout->addWidget(startFrameLineEdit, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(startFrameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onStartFrameChanged(const QString &)));

  QLabel * endFrameLabel = new QLabel("End Frame", optionsWidget);
  optionsLayout->addWidget(endFrameLabel, 3, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * endFrameLineEdit = new QLineEdit(optionsWidget);
  endFrameLineEdit->setValidator(new QDoubleValidator());
  endFrameLineEdit->setText(QString::number(m_settings.endFrame));
  optionsLayout->addWidget(endFrameLineEdit, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(endFrameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onEndFrameChanged(const QString &)));

  QLabel * fpsLabel = new QLabel("FPS", optionsWidget);
  optionsLayout->addWidget(fpsLabel, 4, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * fpsLineEdit = new QLineEdit(optionsWidget);
  fpsLineEdit->setValidator(new QDoubleValidator());
  fpsLineEdit->setText(QString::number(m_settings.fps));
  optionsLayout->addWidget(fpsLineEdit, 4, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(fpsLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onFPSChanged(const QString &)));

  QLabel * subStepsLabel = new QLabel("Sub Steps", optionsWidget);
  optionsLayout->addWidget(subStepsLabel, 5, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * subStepsLineEdit = new QLineEdit(optionsWidget);
  subStepsLineEdit->setValidator(new QIntValidator(1, 32));
  subStepsLineEdit->setText(QString::number(1.0 / m_settings.substeps));
  optionsLayout->addWidget(subStepsLineEdit, 5, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(subStepsLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onSubStepsChanged(const QString &)));

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
  if(m_qSettings)
    delete(m_qSettings);
}

void FabricExportPatternDialog::onAccepted()
{
  FabricCore::Client client = m_client;
  FabricCore::DFGBinding binding = m_binding;
  FabricExportPatternSettings settings = m_settings;
  FabricImportPatternDialog::storeSettings(client, settings.filePath, binding, &m_qSettings);
  close();
  FabricExportPatternCommand().invoke(client, binding, settings);
}

void FabricExportPatternDialog::onScaleChanged(const QString & text)
{
  m_settings.scale = text.toDouble();
}

void FabricExportPatternDialog::onStartFrameChanged(const QString & text)
{
  m_settings.startFrame = text.toDouble();
}

void FabricExportPatternDialog::onEndFrameChanged(const QString & text)
{
  m_settings.endFrame = text.toDouble();
}

void FabricExportPatternDialog::onFPSChanged(const QString & text)
{
  m_settings.fps = text.toDouble();
}

void FabricExportPatternDialog::onSubStepsChanged(const QString & text)
{
  m_settings.substeps = (unsigned int)text.toDouble();
}
