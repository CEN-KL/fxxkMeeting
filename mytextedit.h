#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QWidget>
#include <QPlainTextEdit>
// QCompleter能实现自动填充功能,方便用户输入,提升用户的体验,一般和QLineEdit与QComboBox搭配起来使用.
#include <QCompleter>
#include <QVector>
#include <QPair>
#include <QString>
#include <QStringList>

class Completer : public QCompleter
{
    Q_OBJECT
public:
    explicit Completer(QObject *par = nullptr) :QCompleter(par) { }
};


class MyTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit MyTextEdit(QWidget *par = nullptr);
    QString toPlainTextEdit();
    void setPlainTextEdit(QString);
    void setPlaceholderText(QString);
    void setCompleter(QStringList);
private:
    QPlainTextEdit *edit;
    Completer *completer;
    QVector<QPair<int, int>> ipspan;

    QString textUnderCursor();
    bool eventFilter(QObject *, QEvent *) override;

private slots:
    void changeCompletion(QString);
public slots:
    void complete();
};

#endif // MYTEXTEDIT_H
