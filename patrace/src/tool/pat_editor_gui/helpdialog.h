#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui{
    class HelpDialog;
}

class HelpDialog: public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = 0);
    ~HelpDialog();

public slots:
    void update_helpcontent();

private:
    Ui::HelpDialog *helpDialog_Ui;
};

#endif // HELPDIALOG_H
