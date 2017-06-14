//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QVBoxLayout>
#include <QProgressBar>
#include <QDialogButtonBox>

#include "FabricProgressbarDialog.h"

FabricProgressbarDialog::FabricProgressbarDialog(QWidget * parent, QString title, int count)
: QDialog(parent)
, m_cancelPressed(false)
, m_progressBar(NULL)
{
  setWindowTitle(title);
  setMinimumWidth(350);

  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);

  m_progressBar = new QProgressBar(this);
  layout->addWidget(m_progressBar);

  m_progressBar->setMinimum(0);
  setCount(count);
  m_progressBar->setValue(0);

  QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, this);
  layout->addWidget(buttons);

  QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(onCancelPressed()));
}

FabricProgressbarDialog::~FabricProgressbarDialog()
{
}

void FabricProgressbarDialog::increment()
{
  m_progressBar->setValue(m_progressBar->value() + 1);
}

void FabricProgressbarDialog::setCount(int count)
{
  m_progressBar->setMaximum(count);
}

bool FabricProgressbarDialog::wasCancelPressed() const
{
  return m_cancelPressed;
}

void FabricProgressbarDialog::onCancelPressed()
{
  m_cancelPressed = true;
  emit(cancelPressed());
}
