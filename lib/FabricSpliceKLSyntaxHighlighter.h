//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QHash>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextDocument>

#include <FabricCore.h>

class FabricSpliceKLSyntaxHighlighter : public QSyntaxHighlighter {
public:

  FabricSpliceKLSyntaxHighlighter(QTextDocument * document);
  bool isKeyWord(const QString & word);

protected:
  void highlightBlock(const QString &text);

private:
  struct HighlightingRule
  {
    QRegExp pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QRegExp commentStartExpression;
  QRegExp commentEndExpression;
  QStringList keywordPatterns;
  QStringList classPatterns;

  QTextCharFormat keywordFormat;
  QTextCharFormat classFormat;
  QTextCharFormat singleLineCommentFormat;
  QTextCharFormat multiLineCommentFormat;
  QTextCharFormat quotationFormat;
  QTextCharFormat functionFormat;
};
