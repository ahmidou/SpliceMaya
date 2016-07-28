//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMenu>

#include "FabricSpliceKLSyntaxHighlighter.h"
#include <FabricSplice.h>

class FabricSpliceKLLineNumberWidget : public QWidget {
public:
  FabricSpliceKLLineNumberWidget(QFont font, QWidget * parent);
  ~FabricSpliceKLLineNumberWidget();
  void paintEvent ( QPaintEvent * event );
  void setLineOffset(int lineOffset) { mLineOffset = lineOffset; update(); }
  float lineHeight() const { return mFontMetrics->lineSpacing(); }
private:
  QFont mFont;
  QFontMetrics * mFontMetrics;
  int mLineOffset;
};

class FabricSpliceCodeCompletionMenu : public QMenu {
public:
  FabricSpliceCodeCompletionMenu(QFont font, QWidget * parent = NULL);
  ~FabricSpliceCodeCompletionMenu() {}

  void addItem(const std::string & name, const std::string & desc, const std::string & comments);
  void setPrefix(const std::string & seq) { mPrefix = seq; setPressedKeySequence(seq); }
  void setPressedKeySequence(const std::string & seq) { mPressedKeySequence = seq; }
  void clearItems();
  void show(QPoint pos);
  void update() { if(mShown)show(mPos); }
  void hide();
  bool shown() { return mShown; }
  void keyPressEvent(QKeyEvent  *e);
  std::string completionSuffix();
private:
  struct Item{
    QAction * action;
    std::string name;
    std::string desc;
    std::string comments;
  };
  QFont mFont;
  bool mShown;
  QPoint mPos;
  std::vector<Item> mItems;
  std::string mPrefix;
  std::string mPressedKeySequence;
};

class FabricSpliceKLPlainTextWidget : public QPlainTextEdit {
public:
  FabricSpliceKLPlainTextWidget(QFont font, FabricSpliceKLLineNumberWidget * lineNumbers, QWidget * parent);
  ~FabricSpliceKLPlainTextWidget();
  bool event(QEvent * e);
  void keyPressEvent(QKeyEvent  *e);
  void paintEvent ( QPaintEvent * event );

  std::string getStdText();
  void setStdText(const std::string & code);
private:
  FabricSpliceKLLineNumberWidget * mLineNumbers;
  FabricSpliceKLSyntaxHighlighter * mHighlighter;
  FabricSpliceCodeCompletionMenu * mCodeCompletionMenu;
  int mLastScrollOffset;
};

class FabricSpliceKLSourceCodeWidget : public QWidget {
  friend class FabricSpliceKLPlainTextWidget;
public:
  FabricSpliceKLSourceCodeWidget(QWidget * parent, int fontSize = 12);

  std::string getSourceCode();
  void setSourceCode(const std::string & opName, const std::string & code);
  void setEnabled(bool enabled);
  FabricSplice::KLParser getKLParser() { return mParser; }
  void updateKLParser();

private:
  FabricSpliceKLLineNumberWidget * mLineNumbers;
  FabricSpliceKLPlainTextWidget * mTextEdit;
  std::string mOperator;
  FabricSplice::KLParser mParser;
};
