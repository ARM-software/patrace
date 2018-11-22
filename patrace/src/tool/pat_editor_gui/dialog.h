#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QString>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QListWidget>
#include <QListWidgetItem>

namespace Ui {
    class Dialog;
}

class Dialog: public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

public slots:
    void initNewTable(int i, int j);
    void clear_edit();
    void finish_edit();

signals:
    void sendItem(int, int, QJsonObject, QString);

private:
    Ui::Dialog *new_UiDialog;

    int framenum;
    int callnum;
    QJsonObject addItemObject;
    QString addText;
};

#endif // DIALOG_H
