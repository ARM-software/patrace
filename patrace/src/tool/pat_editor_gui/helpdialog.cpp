#include "helpdialog.h"
#include "ui_helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent):
    QDialog(parent),
    helpDialog_Ui(new Ui::HelpDialog)
{
    helpDialog_Ui->setupUi(this);
    this->setWindowTitle("Help Document - Pat&Json Editor");
    connect(helpDialog_Ui->help, SIGNAL(itemSelectionChanged()), this, SLOT(update_helpcontent()));
    //add father point
    QTreeWidgetItem *item0 = new QTreeWidgetItem;
    item0->setText(0, "Help");
    helpDialog_Ui->help->addTopLevelItem(item0);
    //add son point
    QTreeWidgetItem *item0_0 = new QTreeWidgetItem;
    item0_0->setText(0, "No.1");
    item0->addChild(item0_0);

    QTreeWidgetItem *item0_1 = new QTreeWidgetItem;
    item0_1->setText(0, "No.2");
    item0->addChild(item0_1);
}

HelpDialog::~HelpDialog()
{
    delete helpDialog_Ui;
}

void HelpDialog::update_helpcontent()
{
    QTreeWidgetItem * cur_item = helpDialog_Ui->help->currentItem();
    if (cur_item->parent() == NULL)
        return;
    int first_level = helpDialog_Ui->help->indexOfTopLevelItem(cur_item->parent());
    int second_level = cur_item->parent()->indexOfChild(cur_item);
    if(first_level == 0 && second_level == 0)
    {
        helpDialog_Ui->help_content->setText("Welcome to help document N0.1");
    }
    else if (first_level == 0 && second_level == 1) {
        helpDialog_Ui->help_content->setText("Welcome to help document N0.2");
    }
}
