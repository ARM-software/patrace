#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent):
    QDialog(parent),
    new_UiDialog(new Ui::Dialog)
{
    new_UiDialog->setupUi(this);
    this->setWindowTitle("Add a New Item");
    new_UiDialog->NewTableWidget->setRowCount(100);
    new_UiDialog->NewTableWidget->setColumnCount(3);
    new_UiDialog->NewTableWidget->setWordWrap(true);
    new_UiDialog->NewTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    new_UiDialog->NewTableWidget->resizeColumnsToContents();
    new_UiDialog->NewTableWidget->horizontalHeader()->setStretchLastSection(true);
    new_UiDialog->NewTableWidget->setSpan(0, 1, 1, 2);//combine cells
    new_UiDialog->NewTableWidget->setSpan(1, 1, 1, 2);
    new_UiDialog->NewTableWidget->setSpan(2, 1, 1, 2);
    new_UiDialog->NewTableWidget->setSpan(4, 1, 1, 2);

    connect(new_UiDialog->addNew, SIGNAL(clicked()), this, SLOT(finish_edit()));
    connect(new_UiDialog->clearNew, SIGNAL(clicked()), this, SLOT(clear_edit()));
}

Dialog::~Dialog()
{
    delete new_UiDialog;
}

void Dialog::initNewTable(int i, int j)
{
    framenum = i;
    callnum = j;

    new_UiDialog->NewTableWidget->clear();
    QTableWidgetItem *item0 = new QTableWidgetItem("call_no:");
    item0->setFlags(item0->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(0, 0, item0);
    QTableWidgetItem *item1 = new QTableWidgetItem("tid:");
    item1->setFlags(item1->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(1, 0, item1);
    QTableWidgetItem *item2 = new QTableWidgetItem("func_name:");
    item2->setFlags(item2->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(2, 0, item2);
    QTableWidgetItem *item3 = new QTableWidgetItem("return:");
    item3->setFlags(item3->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(3, 0, item3);

    QTableWidgetItem *item4 = new QTableWidgetItem("arg:");
    item4->setFlags(item4->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(4, 0, item4);

    QTableWidgetItem *item5 = new QTableWidgetItem();
    item5->setFlags(item5->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(4, 1, item5);
}

void Dialog::finish_edit()
{
    int currentTableRow = 0;

    if(new_UiDialog->NewTableWidget->item(currentTableRow, 1) == NULL)
        return;
    QString str0 = new_UiDialog->NewTableWidget->item(currentTableRow, 1)->text().trimmed();
    int newCall_no = str0.toInt();
    addItemObject.insert("0 call_no", newCall_no);
    currentTableRow++;

    if(new_UiDialog->NewTableWidget->item(currentTableRow, 1) == NULL)
        return;
    QString str1 = new_UiDialog->NewTableWidget->item(currentTableRow, 1)->text().trimmed();
    int newTid_no = str1.toInt();
    addItemObject.insert("1 tid", newTid_no);
    currentTableRow++;

    if(new_UiDialog->NewTableWidget->item(currentTableRow, 1) == NULL)
        return;
    QString str2= new_UiDialog->NewTableWidget->item(currentTableRow, 1)->text().trimmed();
    addItemObject.insert("2 func_name", str2);
    currentTableRow++;

    if(new_UiDialog->NewTableWidget->item(currentTableRow, 1) == NULL)
        return;
    QString str3= new_UiDialog->NewTableWidget->item(currentTableRow, 1)->text().trimmed();
    addItemObject.insert("3 return_type", str3);
    if (new_UiDialog->NewTableWidget->item(currentTableRow, 2) != NULL) {
        QString str4 = new_UiDialog->NewTableWidget->item(currentTableRow, 2)->text().trimmed();
        if(str4 == "null") {
            addItemObject.insert("4 return_value",QJsonValue::Null);
        }
        else if (('0' <= str4[0] && str4[0] <= '9') || str4[0] == '-') {
            int temp = str4.toDouble();
            addItemObject.insert("4 return_value", temp);
        }
        else {
            addItemObject.insert("4 return_value", str4);
        }
    }
    else if (new_UiDialog->NewTableWidget->item(currentTableRow, 2) == NULL &&
             new_UiDialog->NewTableWidget->item(currentTableRow + 1, 0) == NULL &&
             new_UiDialog->NewTableWidget->item(currentTableRow + 1, 1) != NULL &&
             new_UiDialog->NewTableWidget->item(currentTableRow + 1, 2) != NULL ){
        currentTableRow++;
        QJsonObject returnobject;
        QString str4_pointertype= new_UiDialog->NewTableWidget->item(currentTableRow, 1)->text().trimmed();
        QString str4_pointervalue= new_UiDialog->NewTableWidget->item(currentTableRow, 2)->text().trimmed();
        returnobject.insert(str4_pointertype, str4_pointervalue);
        addItemObject.insert("4 return_value", returnobject);
    }
    else {
        return;
    }
    currentTableRow++;

    int arg_num = -1;
    QJsonArray changed_argtype;
    QJsonObject changed_argvalue;
    if(new_UiDialog->NewTableWidget->item(currentTableRow, 0) != NULL && new_UiDialog->NewTableWidget->item(currentTableRow, 0)->text().contains("arg"))
    {
        currentTableRow++;
        for(int line = currentTableRow; line < new_UiDialog->NewTableWidget->rowCount(); line++)
        {
            if(new_UiDialog->NewTableWidget->item(line, 0) != NULL && new_UiDialog->NewTableWidget->item(line, 1) != NULL )
            {
                arg_num++;
                QString arg = new_UiDialog->NewTableWidget->item(line, 0)->text().trimmed();  //In JSON:arg_value\type
                if('0' > arg[0] || arg[0] > '9')
                {
                    QMessageBox::warning(this,"Warning","Args should start with a number and a blank",QMessageBox::Yes);
                    return;
                }
                QString arg_type_first = new_UiDialog->NewTableWidget->item(line, 1)->text().trimmed();   //In JSON:arg_type
                changed_argtype.insert(arg_num, arg_type_first);
                if(new_UiDialog->NewTableWidget->item(line, 2) != NULL) //arg is not object type
                {
                    QString arg_value = new_UiDialog->NewTableWidget->item(line, 2)->text().trimmed();    //In JSON:arg_value\value
                    if(arg_value == "null")
                    {
                        changed_argvalue.insert(arg, QJsonValue::Null);
                    }
                    else if (('0' <= arg_value[0] && arg_value[0] <= '9') || arg_value[0] == '-')
                    {
                        double temp = arg_value.toDouble();
                        changed_argvalue.insert(arg, temp);
                    }
                    else
                    {
                        changed_argvalue.insert(arg, arg_value);
                    }
                }
                else if(new_UiDialog->NewTableWidget->item(line, 2) == NULL && new_UiDialog->NewTableWidget->item(line + 1, 0) == NULL &&
                        new_UiDialog->NewTableWidget->item(line + 1, 1) != NULL) {//arg is object type
                    QJsonObject argvalue_object;
                    QString arg_type_second = new_UiDialog->NewTableWidget->item(line + 1, 1)->text().trimmed();
                    if(arg_type_first != "Array" && arg_type_first != "array" )// objectarg contains pointer
                    {
                        if(new_UiDialog->NewTableWidget->item(line + 1, 2) != NULL)
                        {
                            QString arg_value = new_UiDialog->NewTableWidget->item(line + 1, 2)->text().trimmed();
                            if (('0' <= arg_value[0] && arg_value[0] <= '9') || arg_value[0] == '-')
                            {
                                double temp = arg_value.toFloat();
                                argvalue_object.insert(arg_type_second, temp);
                            }
                            else{
                                argvalue_object.insert(arg_type_second, arg_value);
                            }
                        }
                        else{
                            argvalue_object.insert(arg_type_second, QJsonValue::Null);
                        }
                    }
                    else {   // objectarg contains array
                        QJsonArray argvalueobject_array;
                        int array_arg_length = 0;
                        if (new_UiDialog->NewTableWidget->item(line + 1, 2) != NULL)
                        {
                            for (int i = line + 1; i< new_UiDialog->NewTableWidget->rowCount(); i++)
                            {
                                if (new_UiDialog->NewTableWidget->item(i + 1, 1) == NULL && new_UiDialog->NewTableWidget->item(i + 1 , 2) != NULL) {
                                    array_arg_length++;
                                }
                                else {
                                    array_arg_length++;
                                    break;
                                }
                            }
                            for(int i = 0; i < array_arg_length; i++)
                            {
                                QString temp = new_UiDialog->NewTableWidget->item(line + 1 + i, 2)->text().trimmed();
                                if (('0' <= temp[0] && temp[0] <= '9') || temp[0] == '-')
                                {
                                    double temp_double = temp.toDouble();
                                    argvalueobject_array.insert(i, temp_double);
                                }
                                else
                                {
                                    argvalueobject_array.insert(i, temp);
                                }
                            }
                        }
                        argvalue_object.insert(arg_type_second, argvalueobject_array);
                    }
                    changed_argvalue.insert(arg, argvalue_object);
                }
                else {
                    return;
                }
            }
        }
        if(arg_num == -1) {
            addItemObject.insert("5 arg_type", changed_argtype);
            addItemObject.insert("6 arg_value", QJsonValue::Null);
        }
        else {
            addItemObject.insert("5 arg_type", changed_argtype);
            addItemObject.insert("6 arg_value", changed_argvalue);
        }
    }

    addText = "(" + str0 + ")" + str2;

    emit sendItem(framenum, callnum, addItemObject, addText);
}

void Dialog::clear_edit()
{
    new_UiDialog->NewTableWidget->clear();
    QTableWidgetItem *item0 = new QTableWidgetItem("call_no:");
    item0->setFlags(item0->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(0, 0, item0);
    QTableWidgetItem *item1 = new QTableWidgetItem("tid:");
    item1->setFlags(item1->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(1, 0, item1);
    QTableWidgetItem *item2 = new QTableWidgetItem("func_name:");
    item2->setFlags(item2->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(2, 0, item2);
    QTableWidgetItem *item3 = new QTableWidgetItem("return:");
    item3->setFlags(item3->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(3, 0, item3);

    QTableWidgetItem *item4 = new QTableWidgetItem("arg:");
    item4->setFlags(item4->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(4, 0, item4);

    QTableWidgetItem *item5 = new QTableWidgetItem();
    item5->setFlags(item5->flags() & (~Qt::ItemIsEditable));
    new_UiDialog->NewTableWidget->setItem(4, 1, item5);
}
