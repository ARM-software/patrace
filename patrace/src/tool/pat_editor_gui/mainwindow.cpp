#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <vector>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusBar->showMessage("Arm");
    this->setAttribute(Qt::WA_QuitOnClose, true);
    ui->treeJsonFile->setSelectionMode(QAbstractItemView::ExtendedSelection);

    //add menu
    connect(ui->actionOpen_Pat_File,SIGNAL(triggered()), this, SLOT(openPatFile()));
    connect(ui->actionOpen_Json_File,SIGNAL(triggered()), this, SLOT(openJsonFile()));
    connect(ui->actionOpen_Json_Directory,SIGNAL(triggered()), this, SLOT(openJsonDir()));
    connect(ui->actionSave_as_PAt,SIGNAL(triggered()), this, SLOT(saveasPat()));
    connect(ui->actionSave_as_Json,SIGNAL(triggered()), this, SLOT(saveasJson()));
    connect(ui->actionSave_Item,SIGNAL(triggered()), this, SLOT(on_actionSetChange_clicked()));
    connect(ui->actionNew_Item,SIGNAL(triggered()), this, SLOT(showTableWidget()));
    connect(ui->actionAdd_Item,SIGNAL(triggered()), this, SLOT(on_actionAdd_clicked()));
    connect(ui->actionDelete_Item,SIGNAL(triggered()), this, SLOT(on_actionDelete_clicked()));

    //add open tool
    QMenu *openMenu = new QMenu(this);
    QAction *openPat = new QAction(tr("Open Pat File"), this);
    QAction *openJson = new QAction(tr("Open Json File"), this);
    QAction *openJson_dir = new QAction(tr("Open Json Directory"), this);
    openMenu->addAction(openPat);
    openMenu->addAction(openJson);
    openMenu->addAction(openJson_dir);
    ui->actionOpen->setMenu(openMenu);  //promote pushbutton to menu

    connect(openPat,SIGNAL(triggered()), this, SLOT(openPatFile()));
    connect(openJson,SIGNAL(triggered()), this, SLOT(openJsonFile()));
    connect(openJson_dir,SIGNAL(triggered()), this, SLOT(openJsonDir()));//add functions for the toolbar's open button
    connect(ui->actionOpen,SIGNAL(clicked()), this, SLOT(openPatFile()));

    ui->actionOpen->setToolTip("Open a Pat file");
    openPat->setStatusTip("Open a Pat file and extract it to a given directory.");
    openJson->setStatusTip("Open one or some Json files and read them all.");
    openJson_dir->setStatusTip("Choose a directory and open all Json files in it.");
    ui->actionOpen->setMenu(openMenu);  //promote pushbutton to menu
    ui->actionOpen->setPopupMode(QToolButton::MenuButtonPopup);
    ui->toolBar->addWidget(ui->actionOpen); //add toolbox to toolbar

    //add save tool
    QMenu *saveMenu = new QMenu(this);
    QAction *savePat = new QAction(tr("Save as Pat"), this);
    QAction *saveJson = new QAction(tr("Save as Json"), this);
    saveMenu->addAction(savePat);
    saveMenu->addAction(saveJson);
    ui->actionSave->setMenu(saveMenu);
    connect(savePat,SIGNAL(triggered()), this, SLOT(saveasPat()));
    connect(saveJson,SIGNAL(triggered()), this, SLOT(saveasJson()));
    connect(ui->actionSave,SIGNAL(clicked()), this, SLOT(saveasPat()));

    ui->actionSave->setMenu(saveMenu);
    ui->actionSave->setToolTip("Save as Pat file");
    savePat->setStatusTip("Merge into a Pat file if a directory or Pat file opened.");
    saveJson->setStatusTip("Save as Json files.");
    ui->actionSave->setPopupMode(QToolButton::MenuButtonPopup);
    ui->toolBar->addWidget(ui->actionSave);

    ui->toolBar->addSeparator();

    //add editor tools
    ui->actionNewItem->setToolTip("New item(Ctrl+N)");
    ui->actionNewItem->setStatusTip("New an item and add it into the opened Json file after edited");
    connect(ui->actionNewItem,SIGNAL(clicked()), this, SLOT(showTableWidget()));
    ui->toolBar->addWidget(ui->actionNewItem);

    ui->actionAdd->setToolTip("Add item(Ctrl+A)");
    ui->actionAdd->setStatusTip("Edit an item in the GUI and add it into the opened Json file.");
    ui->toolBar->addWidget(ui->actionAdd);

    ui->actionDelete->setToolTip("Delete item(Ctrl+D)");
    ui->actionDelete->setStatusTip("Delete one or more items. If a Json file name choosed, delete the whole Json file.");
    ui->toolBar->addWidget(ui->actionDelete);

    ui->actionSetChange->setToolTip("Save the change(Ctrl+s)");
    ui->actionSetChange->setStatusTip("Save the change into the Json, otherwise discard all the change in GUI.");
    ui->toolBar->addWidget(ui->actionSetChange);

    ui->toolBar->addSeparator();

    ui->actionHelp->setToolTip("Review Help");
    ui->toolBar->addWidget(ui->actionHelp);

    ui->ItemInformation->setRowCount(100);
    ui->ItemInformation->setColumnCount(3);
    ui->ItemInformation->setWordWrap(true);
    ui->ItemInformation->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->ItemInformation->resizeColumnsToContents();
    ui->ItemInformation->horizontalHeader()->setStretchLastSection(true);
    ui->ItemInformation->setSpan(0, 1, 1, 2);   //combine cells
    ui->ItemInformation->setSpan(1, 1, 1, 2);
    ui->ItemInformation->setSpan(2, 1, 1, 2);
    connect(this,&MainWindow::itemOpened,this,&MainWindow::record_itemChange);

    this->setWindowTitle("Pat&Json Editor");
    connect(ui->Filter,SIGNAL(returnPressed()), this, SLOT(doSearching()));
    connect(ui->actionHelp,SIGNAL(clicked()), this, SLOT(showhelp()));
}

MainWindow::~MainWindow()
{
    if (o_objThread)
    {
        o_objThread->quit();
        o_objThread->wait();
    }
}

void MainWindow::showhelp()
{
    help_Dialog = new HelpDialog;
    help_Dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    help_Dialog->setAttribute(Qt::WA_QuitOnClose, false);
    help_Dialog->show();
}

void MainWindow::startOpenThread()
{
    if (o_objThread)
    {
        return;
    }
    o_objThread = new QThread();
    o_obj = new OpenJson_Worker();
    o_obj->moveToThread(o_objThread);
    connect(o_objThread, &QThread::finished, o_objThread, &QObject::deleteLater);
    connect(o_objThread, &QThread::finished, o_obj, &QObject::deleteLater);
    connect(this, &MainWindow::deliverfilename, o_obj, &OpenJson_Worker::openJson);
    connect(this, &MainWindow::deliverpatinf, o_obj, &OpenJson_Worker::extractPat);
    connect(this, &MainWindow::delivermergeinf, o_obj, &OpenJson_Worker::mergePat);
    connect(o_obj, &OpenJson_Worker::startShowingPercentage, this, &MainWindow::showPercentage);
    connect(o_obj, &OpenJson_Worker::resultReady, this, &MainWindow::combineJsonContent);
    connect(o_obj, &OpenJson_Worker::extractFinish, this, &MainWindow::readPatContent);
    o_objThread->start();
}

void MainWindow::startPercentThread()
{
    if (percentThread)
    {
        return;
    }
    percentThread = new QThread();
    percentObj = new PercentageThread();
    percentObj->moveToThread(percentThread);
    connect(percentThread, &QThread::finished, percentThread, &QObject::deleteLater);
    connect(percentThread, &QThread::finished, percentObj, &QObject::deleteLater);
    connect(this, &MainWindow::GUIupdate, percentObj, &PercentageThread::dowork);
    connect(this, &MainWindow::GUIupdate_saving, percentObj, &PercentageThread::dowork_saving);
    connect(percentObj, &PercentageThread::startShowingPercentage, this, &MainWindow::showPercentage);
    connect(percentObj, &PercentageThread::startShowingPercentage_saving, this, &MainWindow::showPercentage_saving);
    percentThread->start();
}

int Qstringcompare(QString arr1, QString arr2)
{
    QString tmp1, tmp2;
    for (int i = 0; i < arr1.length(); i++)
    {
        if (arr1[i] >='0' && arr1[i] <= '9')
        tmp1.append(arr1[i]);
    }
    for (int j = 0; j < arr2.length(); j++)
    {
        if (arr2[j] >='0' && arr2[j] <= '9')
        tmp2.append(arr2[j]);
    }
    if (tmp1.toInt() > tmp2.toInt())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

QStringList Filenames_sort(QStringList filenames)
{
    int k, j;
    int flag = filenames.size();
    while (flag > 0)
    {
        k = flag;
        flag = 0;
        for (j = 1; j < k; j++)
        {
            if (Qstringcompare(filenames[j - 1], filenames[j]) == 1)
            {
                QString temp = filenames[j - 1];
                filenames[j - 1] = filenames[j];
                filenames[j] = temp;
                flag = j;
            }
        }
    }
    return filenames;
}

void MainWindow::openJsonFile()
{
    files = QFileDialog::getOpenFileNames(this, tr("Select one or more Json files"), "/home", "Json Files(*.json)");
    if (files.isEmpty())
    {
        QMessageBox::warning(this, "Warning", tr("Fail to get a Json file."), QMessageBox::Yes);
        return;
    }
    files = Filenames_sort(files);

    if (JsonOpened == true)
    {
        ui->treeJsonFile->clear();
        resave_string.clear();
        frame_start.clear();
        frame_end.clear();
        files.clear();
    } //clear the frame window when open a new json file

    if (!o_objThread)
    {
        startOpenThread();
    }
    JsonOpened = true;
    emit deliverfilename(files);
    ui->statusBar->showMessage("Start reading Json files......");
}

void MainWindow::openJsonDir()
{
    QString JsonDir = QFileDialog::getExistingDirectory(this, tr("Choose a Json Directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (JsonDir.isEmpty())
    {
        QMessageBox::warning(this, "Warning", tr("Fail to Open a Directory."), QMessageBox::Yes);
        return;
    }

    QDir selected_dir(JsonDir);
    if (!selected_dir.exists()) return;
    QStringList filters;
    filters << "*.json";
    selected_dir.setNameFilters(filters);
    QFileInfoList filtered_list = selected_dir.entryInfoList(); //get all json in the dir
    int filtered_count = filtered_list.count();
    if (filtered_count < 1)
    {
        QMessageBox::warning(this, "Warning", tr("No Json file in the given directory."), QMessageBox::Yes);
        return;
    }
    for (int i = 0; i < filtered_count; i++)
    {
        QFileInfo each_json = filtered_list.at(i);
        files.append(each_json.filePath());
    }
    files = Filenames_sort(files);

    if (JsonOpened == true)
    {
        ui->treeJsonFile->clear();
        resave_string.clear();
        frame_start.clear();
        frame_end.clear();
        files.clear();
    } //clear the frame window when open a new json file

    if (!o_objThread)
    {
        startOpenThread();
    }
    JsonOpened = true;
    emit deliverfilename(files);
    ui->statusBar->showMessage("Start reading Json files......");
}

void MainWindow::combineJsonContent(const QStringList &result, const int length, const int number)
{
    resave_string.append(result);
    QTreeWidgetItem *item = new QTreeWidgetItem;    //add father point

    QFileInfo eachjson = QFileInfo(files[number]);
    QString file_name = eachjson.fileName(); //get file name rather than the path

    item->setText(0, file_name);//update the treewidget
    ui->treeJsonFile->addTopLevelItem(item);
    if (number == 0)
    {
        frame_start.push_back(0);
        frame_end.push_back(length - 1);
    }
    else
    {
        frame_start.push_back(frame_end[number - 1] + 1);
        frame_end.push_back(frame_end[number - 1] + length);
    }
    int percent = (number + 1) * 100 / files.size();

    ui->statusBar->showMessage("Reading Json files......" + QString::number(percent) + "%");
    if (percent == 100)
    {
        ui->statusBar->showMessage("Reading finished.");
    }
}

void MainWindow::openPatFile()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose a Pat File"), "/home", tr("Json Files(*.pat)"));
    if (file.isEmpty())
    {
        QMessageBox::warning(this, "Warning", tr("Fail to get a Pat file."), QMessageBox::Yes);
        return;
    }

    OriginalDir = QFileDialog::getExistingDirectory(this, tr("Choose a Result Directory"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (OriginalDir.isEmpty())
    {
        QMessageBox::warning(this, "Warning", tr("Fail to appoint a Directory."), QMessageBox::Yes);
        return;
    }

    if (JsonOpened == true)
    {
        ui->treeJsonFile->clear();
        resave_string.clear();
        frame_start.clear();
        frame_end.clear();
        files.clear();
    } //clear the frame window when open a new pat file

    if (!o_objThread)
    {
        startOpenThread();
    }

    ui->statusBar->showMessage("Extracting Pat File...");
    emit deliverpatinf(file, OriginalDir);
    Pat_reading = true;
    if (!percentThread)
    {
        startPercentThread();
    }
    emit GUIupdate();
}

void MainWindow::showPercentage()
{
    if (progress_num < 100)
    {
        ui->statusBar->showMessage("Extracting....." + QString::number(progress_num) + "%");
    }
    else
    {
        ui->statusBar->showMessage("Extracting finished, wait for reading Json files.");
    }
}

void MainWindow::readPatContent()
{
    QString JsonDir = OriginalDir + "/GLES_calls";
    if (JsonDir.isEmpty())
    {
        QMessageBox::warning(this, "Warning", tr("Fail to Open a Directory."), QMessageBox::Yes);
        return;
    }

    QDir selected_dir(JsonDir);
    if (!selected_dir.exists())
        return;
    QStringList filters;
    filters << "*.json";
    selected_dir.setNameFilters(filters);
    QFileInfoList filtered_list = selected_dir.entryInfoList(); //get json in the dir
    int filtered_count = filtered_list.count();
    if (filtered_count < 1)
    {
        QMessageBox::warning(this, "Warning", tr("No Json file in the given directory."), QMessageBox::Yes);
        return;
    }

    for (int i = 0; i < filtered_count; i++)
    {
        QFileInfo each_json = filtered_list.at(i);
        files.append(each_json.filePath());
    }
    files = Filenames_sort(files);

    if (!o_objThread)
    {
        startOpenThread();
    }
    JsonOpened = true;
    PatOpened = true;
    emit deliverfilename(files);
}

void MainWindow::showTableWidget()
{
    if (JsonOpened != true)
    {
        QMessageBox::warning(this, "Warning", "No Opened Json", QMessageBox::Yes);
        return;
    }
    MainWindow::new_Dialog = new Dialog;

    newitemlocation = ui->treeJsonFile->currentItem();
    if (newitemlocation == NULL)
    {
        QMessageBox::warning(this, "Warning", "Please choose an insert location", QMessageBox::Yes);
        return;
    }
    int addFrameNum;
    QTreeWidgetItem *parent = newitemlocation->parent();
    int addItemNum;
    if (parent == NULL)
    {
        addFrameNum = ui->treeJsonFile->indexOfTopLevelItem(newitemlocation);
        addItemNum = frame_start[addFrameNum] - 1;
    }
    else {
        addFrameNum = ui->treeJsonFile->indexOfTopLevelItem(parent);
        int frame_start_num = frame_start[addFrameNum];
        int row = parent->indexOfChild(newitemlocation);
        addItemNum = frame_start_num + row;
    }

    newitemlocation->setBackground(0, QColor(0, 255, 0, 60));
    new_Dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
    new_Dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    new_Dialog->setAttribute(Qt::WA_QuitOnClose, false);
    new_Dialog->show();
    new_Dialog->initNewTable(addFrameNum, addItemNum);
    connect(new_Dialog, SIGNAL(sendItem(int, int, QJsonObject, QString)), this, SLOT(showtheChange(int, int, QJsonObject, QString)));
}

void MainWindow::saveasJson()
{
    if (files.size() == 1)   //if only one json file, choose the save location and can change the file name
    {
        QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), "/home", tr("Json Files(*.json)"));
        if (resave_string.isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Fail to save the Json file.", QMessageBox::Yes);
            return;
        }
        QByteArray changed_byte_array;
        changed_byte_array.append("[\xa");
        for (int i = 0; i < resave_string.size() - 1; i++)
        {
            changed_byte_array.append(resave_string[i].toUtf8());
            changed_byte_array.push_back(",\xa");
        }
        changed_byte_array.append(resave_string[resave_string.size() - 1].toUtf8());
        changed_byte_array.append("\xa]");

        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "Warning", "Fail to save the Json file.", QMessageBox::Yes);
            return;
        }
        file.write(changed_byte_array);
        file.close();
    }
    else if (files.size() > 1)   //if more than one json files opened, choose a directory and files name will not change
    {
        QString newJsonDir = QFileDialog::getExistingDirectory(this, tr("Choose a directory to save the opened Json files"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (newJsonDir.isEmpty())
        {
            QMessageBox::warning(this, "Warning", tr("Fail to appoint a Directory."), QMessageBox::Yes);
            return;
        }
        for (int i = 0; i < files.size(); i++)
        {
            QByteArray changed_byte_array;
            changed_byte_array.append("[\xa");
            for (int j = frame_start[i]; j < frame_end[i]; j++)
            {
                changed_byte_array.append(resave_string[j].toUtf8());
                changed_byte_array.push_back(",\xa");
            }
            changed_byte_array.append(resave_string[frame_end[i]].toUtf8());
            changed_byte_array.append("\xa]");

            QFileInfo eachjson = QFileInfo(files[i]);
            QString file_name = eachjson.fileName(); //get file name rather than the path

            QFile file(newJsonDir + "/" + file_name);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::warning(this, "Warning", "Fail to save the NO."+ QString::number(i + 1) + " Json file.", QMessageBox::Yes);
                return;
            }
            file.write(changed_byte_array);
            file.close();
        }
    }
    else {
        QMessageBox::warning(this, "Warning", tr("Fail to save the Json for an unknown reason."), QMessageBox::Yes);
        return;
    }
}

void MainWindow::showPercentage_saving()
{
    if (progress_num < 100)
    {
        ui->statusBar->showMessage("Merging....." + QString::number(progress_num) + "%");
    }
    else
    {
        ui->statusBar->showMessage("Merging finished.");
    }
}

void MainWindow::saveasPat()
{
    if (JsonOpened == true && PatOpened != true)
    {
        QMessageBox::warning(this, "Tips", "Can't save a Json file to Pat direct. Please save it as Json first.", QMessageBox::Yes);
        return;
    }
    else if (JsonOpened == true && PatOpened == true)
    {
        QDir savedpat_dir(OriginalDir + "/GLES_calls");
        if (!savedpat_dir.exists())
        {
            QMessageBox::warning(this, "Warning", "The extracted pat directory missed.", QMessageBox::Yes);
            return;
        }
        if (resave_string.isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Fail to resave the Json file.", QMessageBox::Yes);
            return;
        }
        // first step: save json
        for (int i = 0; i < files.size(); i++)
        {
            QByteArray changed_byte_array;
            changed_byte_array.append("[\xa");
            for (int j = frame_start[i]; j < frame_end[i]; j++)
            {
                changed_byte_array.append(resave_string[j].toUtf8());
                changed_byte_array.push_back(",\xa");
            }
            changed_byte_array.append(resave_string[frame_end[i]].toUtf8());
            changed_byte_array.append("\xa]");

            QFile file(files[i]);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::warning(this, "Warning", "Fail to save the NO." + QString::number(i + 1) +" Json file.", QMessageBox::Yes);
                return;
            }
            file.write(changed_byte_array);
            file.close();
        }
        // second step: merge to pat
        if (files.size() <= 0)
        {
            QMessageBox::warning(this, "Warning", "No Json file in the under-merge directory.", QMessageBox::Yes);
            return;
        }

        QString resave_patname = QFileDialog::getSaveFileName(this, tr("Set a name for the new pat file"), "/home", tr("Pat Files(*.pat)"));

        if (!o_objThread)
        {
            startOpenThread();
        }
        ui->statusBar->showMessage("Start merging Pat File...");
        emit delivermergeinf(OriginalDir, resave_patname);

        Pat_saving = true;
        if (!percentThread)
        {
            startPercentThread();
        }
        emit GUIupdate_saving();
    }
    else // no need to open any Json or Pat file, only merge a directory into pat
    {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory that need to be merged into Pat"), "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (dir.isEmpty())
        {
            QMessageBox::warning(this, "Warning", tr("Fail to appoint a Directory."), QMessageBox::Yes);
            return;
        }

        QDir selected_dir(dir + "/GLES_calls");
        if (!selected_dir.exists()) return;
        QStringList filters;
        filters << "*.json";
        selected_dir.setNameFilters(filters);
        QFileInfoList filtered_list = selected_dir.entryInfoList();
        int filtered_count = filtered_list.count();
        if (filtered_count <= 0)
        {
            QMessageBox::warning(this, "Warning", "No Json file in the given directory.", QMessageBox::Yes);
            return;
        }

        QString filename = QFileDialog::getSaveFileName(this, tr("Set a name for the pat file"), "/home", tr("Pat Files(*.pat)"));

        if (!o_objThread)
        {
            startOpenThread();
        }
        emit delivermergeinf(dir, filename);
    }
}

void MainWindow::on_treeJsonFile_itemSelectionChanged()
{
    disconnect(ui->ItemInformation, SIGNAL(cellChanged(int, int)), this, SLOT(flagChanged(int, int)));
    if (itemchanged == true)
    {
        itemchanged = false;
        int flag = QMessageBox::warning(this, tr("Warning"), QString("Do you want to save the change?"), QMessageBox::No, QMessageBox::Yes);
        if (flag == QMessageBox::Yes)
        {
            QTreeWidgetItem *item = changed_item;
            QTreeWidgetItem *parent = item->parent();
            if (parent == NULL) return;
            int num = ui->treeJsonFile->indexOfTopLevelItem(parent);
            int frame_start_num = frame_start[num];
            int row=parent->indexOfChild(item);
            int changedItemNum = frame_start_num + row;
            int currentTableRow = 0;

            QJsonObject changedItemObject;
            if (ui->ItemInformation->item(currentTableRow, 1) == NULL) {return;}
            QString str0 = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
            int newCall_no = str0.toInt();
            changedItemObject.insert("0 call_no", newCall_no);
            currentTableRow++;

            if (ui->ItemInformation->item(currentTableRow, 1) == NULL) {return;}
            QString str1 = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
            int newTid_no = str1.toInt();
            changedItemObject.insert("1 tid", newTid_no);
            currentTableRow++;

            if (ui->ItemInformation->item(currentTableRow, 1) == NULL) {return;}
            QString str2= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
            changedItemObject.insert("2 func_name", str2);
            currentTableRow++;

            if (ui->ItemInformation->item(currentTableRow, 1) == NULL) {return;}
            QString str3= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
            changedItemObject.insert("3 return_type", str3);
            if (ui->ItemInformation->item(currentTableRow, 2) != NULL){
                QString str4 = ui->ItemInformation->item(currentTableRow, 2)->text().trimmed();
                if (str4 == "null"){
                    changedItemObject.insert("4 return_value", QJsonValue::Null);
                }
                else if (('0' <= str4[0] && str4[0] <= '9') || str4[0] == '-') {
                    int temp = str4.toDouble();
                    changedItemObject.insert("4 return_value", temp);
                }
                else {
                    changedItemObject.insert("4 return_value", str4);
                }
            }
            else if (ui->ItemInformation->item(currentTableRow, 2) == NULL &&
                     ui->ItemInformation->item(currentTableRow + 1, 0) == NULL &&
                     ui->ItemInformation->item(currentTableRow + 1, 1) != NULL &&
                     ui->ItemInformation->item(currentTableRow + 1, 2) != NULL)
            {
                currentTableRow++;
                QJsonObject returnobject;
                QString str4_pointertype = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
                QString str4_pointervalue = ui->ItemInformation->item(currentTableRow, 2)->text().trimmed();
                returnobject.insert(str4_pointertype, str4_pointervalue);
                changedItemObject.insert("4 return_value", returnobject);
            }
            else {
                return;
            }
            currentTableRow++;

            int arg_num = -1;
            QJsonArray changed_argtype;
            QJsonObject changed_argvalue;
            if (ui->ItemInformation->item(currentTableRow, 0) != NULL && ui->ItemInformation->item(currentTableRow, 0)->text().contains("arg"))
            {
                currentTableRow++;
                for (int line = currentTableRow; line < ui->ItemInformation->rowCount(); line++)
                {
                    if (ui->ItemInformation->item(line, 0) != NULL && ui->ItemInformation->item(line, 1) != NULL)
                    {
                        arg_num++;
                        QString arg = ui->ItemInformation->item(line, 0)->text().trimmed();  //In JSON:arg_value\type
                        if ('0' > arg[0] || arg[0] > '9')
                        {
                            QMessageBox::warning(this, "Warning", "Args should start with a number and a blank", QMessageBox::Yes);
                            return;
                        }
                        QString arg_type_first = ui->ItemInformation->item(line, 1)->text().trimmed();   //In JSON:arg_type
                        changed_argtype.insert(arg_num, arg_type_first);
                        if (ui->ItemInformation->item(line, 2) != NULL && ui->ItemInformation->item(line, 2)->text() != "") //arg is not object type
                        {
                            QString arg_value = ui->ItemInformation->item(line, 2)->text().trimmed();    //In JSON:arg_value\value
                            if (arg_value == "null")
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
                        else if ((ui->ItemInformation->item(line, 2) == NULL && ui->ItemInformation->item(line + 1, 0) == NULL &&
                                  ui->ItemInformation->item(line + 1, 1) != NULL) ||
                                 (ui->ItemInformation->item(line, 2) != NULL && ui->ItemInformation->item(line, 2)->text() == "")) {//arg is object type
                            QJsonObject argvalue_object;
                            QString arg_type_second = ui->ItemInformation->item(line + 1, 1)->text().trimmed();
                            if (arg_type_first != "Array" && arg_type_first != "array" ) // objectarg contains pointer
                            {
                                if (ui->ItemInformation->item(line + 1, 2) != NULL)
                                {
                                    QString arg_value = ui->ItemInformation->item(line + 1, 2)->text().trimmed();
                                    if (('0'<=arg_value[0]&&arg_value[0]<='9') || arg_value[0] == '-')
                                    {
                                        double temp = arg_value.toDouble();
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
                            else{  // objectarg contains array
                                QJsonArray argvalueobject_array;
                                int array_arg_length = 0;
                                if (ui->ItemInformation->item(line + 1, 2) != NULL)
                                {
                                    for (int i = line + 1; i< ui->ItemInformation->rowCount(); i++)
                                    {
                                        if (ui->ItemInformation->item(i + 1, 1) == NULL && ui->ItemInformation->item(i + 1, 2) != NULL) {
                                            array_arg_length++;
                                        }
                                        else{
                                            array_arg_length++;
                                            break;
                                        }
                                    }
                                    for (int i =0; i<array_arg_length; i++)
                                    {
                                        QString temp = ui->ItemInformation->item(line + 1 + i, 2)->text().trimmed();
                                        if (('0'<=temp[0]&&temp[0]<='9') || temp[0] == '-')
                                        {
                                            float temp_double = temp.toDouble();
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
                        else{return;}
                    }
                }
                if (arg_num == -1)
                {
                    changedItemObject.insert("5 arg_type", changed_argtype);
                    changedItemObject.insert("6 arg_value", QJsonValue::Null);
                }
                else{
                    changedItemObject.insert("5 arg_type", changed_argtype);
                    changedItemObject.insert("6 arg_value", changed_argvalue);
                }
            }
            resave_string.removeAt(changedItemNum);
            QJsonDocument document(changedItemObject);
            QString changedString = document.toJson(QJsonDocument::Indented);
            changedString.remove(changedString.size() - 1, 1);
            resave_string.insert(changedItemNum, changedString);
        }
    }

    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    QTreeWidgetItem *parent = item->parent();

    if (parent == NULL)
    {
        int childrencount = item->childCount();
        for (int i =0; i< childrencount; i++)
        {
            QTreeWidgetItem *child = item->takeChild(0);
            delete child;
        }
        int num = ui->treeJsonFile->indexOfTopLevelItem(item);
        for (int i = frame_start[num]; i<=frame_end[num]; i++)
        {
            QJsonDocument eachFunction = QJsonDocument::fromJson(resave_string[i].toUtf8());
            QJsonObject eachFunction_obj = eachFunction.object();
            QJsonValue callno = eachFunction_obj.value(QString("0 call_no"));
            QString callnum = QString::number(callno.toInt(), 10);
            QJsonValue functionname = eachFunction_obj.value(QString("2 func_name"));
            QString functionname_str = functionname.toString();
            QString abbreviation = "(" + callnum + ")" + functionname_str;
            QTreeWidgetItem *item1=new QTreeWidgetItem;
            item1->setText(0, abbreviation);
            item->addChild(item1);
        }
    }
    else{
        ui->ItemInformation->clear();
        int num =ui->treeJsonFile->indexOfTopLevelItem(parent);
        int frame_start_num = frame_start[num];
        int row=parent->indexOfChild(item); //get the row number in father point(start from 0)
        int currentRowNo = frame_start_num + row;

        int currentTableRow = 0;
        QString selectString = resave_string[currentRowNo];
        QJsonDocument selectItem = QJsonDocument::fromJson(selectString.toUtf8());
        QJsonObject selectItemObject = selectItem.object();

        //the 1st
        QJsonValue callno = selectItemObject.value(QString("0 call_no"));
        QString callnum = QString::number(callno.toInt(), 10);
        QTableWidgetItem *item0 = new QTableWidgetItem("call_no:");
        item0->setFlags(item0->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow, 0, item0);
        ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(callnum));
        currentTableRow++;

        //the 2nd
        QJsonValue tidno = selectItemObject.value(QString("1 tid"));
        QString tidnum = QString::number(tidno.toInt(), 10);
        QTableWidgetItem *item1 = new QTableWidgetItem("tid:");
        item1->setFlags(item1->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow, 0, item1);
        ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(tidnum));
        currentTableRow++;

        //the 3rd
        QJsonValue functionname = selectItemObject.value(QString("2 func_name"));
        QTableWidgetItem *item2 = new QTableWidgetItem("func_name:");
        item2->setFlags(item2->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow, 0, item2);
        ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(functionname.toString()));
        currentTableRow++;

        //the 4th
        QJsonValue returntype = selectItemObject.value(QString("3 return_type"));
        QTableWidgetItem *item3 = new QTableWidgetItem("return:");
        item3->setFlags(item3->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow, 0, item3);
        ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(returntype.toString()));

        //the 5th
        QJsonValue returnvalue = selectItemObject.value(QString("4 return_value"));
        if (returnvalue.isNull()){
            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem("null"));
        }
        else if (returnvalue.isDouble()) {
            QString temp = QString::number(returnvalue.toDouble(), 'g', 16);
            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(temp));
        }
        else if (returnvalue.isObject()){
            currentTableRow++;
            QJsonObject temp = returnvalue.toObject();
            QStringList tempkey = temp.keys();
            QJsonValue tempvalue = *temp.begin();
            ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(tempkey[0]));
            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(tempvalue.toString()));
        }
        else {
            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(returnvalue.toString()));
        }
        currentTableRow++;
        ui->ItemInformation->setSpan(currentTableRow, 1, 1, 2);

        //the 6th + 7th
        QTableWidgetItem *item4 = new QTableWidgetItem("arg:");
        item4->setFlags(item4->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow, 0, item4);
        QTableWidgetItem *item5 = new QTableWidgetItem();
        item5->setFlags(item5->flags()&(~Qt::ItemIsEditable));
        ui->ItemInformation->setItem(currentTableRow, 1, item5);
        QJsonArray argtype = selectItemObject["5 arg_type"].toArray();
        QJsonValue argvalue = selectItemObject.value(QString("6 arg_value"));
        QJsonObject argvalueobj = argvalue.toObject();
        QStringList argList = argvalueobj.keys();
        if (argtype.size() >= 1)
        {
            for (int argNum = 0; argNum < argtype.size(); argNum++)
            {
                currentTableRow++;
                ui->ItemInformation->setItem(currentTableRow, 0, new QTableWidgetItem(argList[argNum]));
                ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(argtype[argNum].toString()));
                QJsonValue eacharg = *(argvalueobj.begin() + argNum);
                if (eacharg.isNull())
                {
                    ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem("null"));
                }
                if (eacharg.isDouble())
                {
                    QString temp = QString::number(eacharg.toDouble(), 'g', 16);
                    ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(temp));
                }
                if (eacharg.isString())
                {
                    ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(eacharg.toString()));
                }

                if (eacharg.isObject())
                {
                    QJsonObject objectarg = eacharg.toObject();
                    QStringList objectargList = objectarg.keys();
                    for (int i = 0; i< objectargList.size(); i++)
                    {
                        ++currentTableRow;
                        ui->ItemInformation->setItem(currentTableRow, 1, new QTableWidgetItem(objectargList[i]));
                        QJsonValue objectargItem = *(objectarg.begin() + i);
                        if (objectargItem.isNull())
                        {
                            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem("null"));
                        }
                        if (objectargItem.isDouble())
                        {
                            QString temp = QString::number(objectargItem.toDouble(), 'g', 16);
                            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(temp));
                        }
                        if (objectargItem.isString())
                        {
                            ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(objectargItem.toString()));
                        }

                        if (objectargItem.isArray())
                        {
                            QJsonArray arrayargItem = objectargItem.toArray();
                            if (arrayargItem.size() >= 1)
                            {
                                for (int i = 0; i < arrayargItem.size() - 1; i++)
                                {
                                    QString temp = QString::number(arrayargItem[i].toDouble(), 'g', 16);
                                    ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(temp));
                                    currentTableRow++;
                                }
                                QString temp = QString::number(arrayargItem[arrayargItem.size() - 1].toDouble(), 'g', 16);
                                ui->ItemInformation->setItem(currentTableRow, 2, new QTableWidgetItem(temp));
                            }
                        }
                    }
                }
            }
        }
    }
    emit itemOpened();
}

void MainWindow::doSearching()
{
    if (list.size()>0)
    {
        for (int i = 0; i<nCount; i++)
        {
            list[i]->setBackground(0, QColor(0, 0, 0, 0));
        }
        list.clear();
    }
    findnum = 0;
    strTemplate = ui->Filter->text();
    if (strTemplate.isEmpty())
    {
        return;
    }

    list = ui->treeJsonFile->findItems(strTemplate, Qt::MatchContains | Qt::MatchRecursive, 0);

    nCount = list.count();
    if (nCount < 1)
    {
        QMessageBox::information(this, tr("Results"), tr("No matching"));
        return;
    }

    for (int i = 0; i<nCount; i++)
    {
        list[i]->setBackground(0, QColor(0, 0, 255, 31));
    }

    ui->treeJsonFile->setCurrentItem(list[findnum]);
    ui->treeJsonFile->scrollToItem(list[findnum]);
    ui->treeJsonFile->setFocus();

}

void MainWindow::on_FindNext_clicked()
{
    QString nextTemplate = ui->Filter->text();
    if (nextTemplate != strTemplate) {
        QMessageBox::information(this, tr("Results"), tr("Keywords changed"));
        return;
    }
    else{
        if (findnum == nCount - 1){
            findnum = 0;
        }
        else{
            ++findnum;
        }
        ui->treeJsonFile->setCurrentItem(list[findnum]);
        ui->treeJsonFile->scrollToItem(list[findnum]);
        ui->treeJsonFile->setFocus();
    }
}

void MainWindow::on_FindPrevious_clicked()
{
    QString nextTemplate = ui->Filter->text();
    if (nextTemplate != strTemplate) {
        QMessageBox::information(this, tr("Results"), tr("Keywords changed"));
        return;
    }
    else {
        if (findnum == 0) {
            findnum = nCount - 1;
        }
        else {
            --findnum;
        }
        ui->treeJsonFile->setCurrentItem(list[findnum]);
        ui->treeJsonFile->scrollToItem(list[findnum]);
        ui->treeJsonFile->setFocus();
    }
}

void MainWindow::on_actionSetChange_clicked()
{
    if (JsonOpened != true)
    {
        QMessageBox::warning(this, "Warning", "No Opened Json", QMessageBox::Yes);
        return;
    }
    itemchanged = false;
    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    QTreeWidgetItem *parent = item->parent();
    if (parent == NULL) return;
    int num = ui->treeJsonFile->indexOfTopLevelItem(parent);
    int frame_start_num = frame_start[num];
    int row=parent->indexOfChild(item);
    int changedItemNum = frame_start_num + row;
    int currentTableRow = 0;

    QJsonObject changedItemObject;
    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str0 = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    int newCall_no = str0.toInt();
    changedItemObject.insert("0 call_no", newCall_no);
    currentTableRow++;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str1 = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    int newTid_no = str1.toInt();
    changedItemObject.insert("1 tid", newTid_no);
    currentTableRow++;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str2= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    changedItemObject.insert("2 func_name", str2);
    currentTableRow++;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str3= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    changedItemObject.insert("3 return_type", str3);
    if (ui->ItemInformation->item(currentTableRow, 2) != NULL){
        QString str4= ui->ItemInformation->item(currentTableRow, 2)->text().trimmed();
        if (str4 == "null"){
            changedItemObject.insert("4 return_value", QJsonValue::Null);
        }
        else if (('0' <= str4[0] && str4[0] <= '9') || str4[0] == '-') {
            int temp = str4.toDouble();
            changedItemObject.insert("4 return_value", temp);
        }
        else {
            changedItemObject.insert("4 return_value", str4);
        }
    }
    else if (ui->ItemInformation->item(currentTableRow, 2) == NULL && ui->ItemInformation->item(currentTableRow + 1, 0) == NULL &&
       ui->ItemInformation->item(currentTableRow + 1, 1) != NULL && ui->ItemInformation->item(currentTableRow + 1, 2) != NULL ){
        currentTableRow++;
        QJsonObject returnobject;
        QString str4_pointertype= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
        QString str4_pointervalue= ui->ItemInformation->item(currentTableRow, 2)->text().trimmed();
        returnobject.insert(str4_pointertype, str4_pointervalue);
        changedItemObject.insert("4 return_value", returnobject);
    }
    else{
        return;
    }
    currentTableRow++;

    int arg_num = -1;
    QJsonArray changed_argtype;
    QJsonObject changed_argvalue;
    if (ui->ItemInformation->item(currentTableRow, 0) != NULL && ui->ItemInformation->item(currentTableRow, 0)->text().contains("arg"))
    {
        currentTableRow++;
        for (int line = currentTableRow; line < ui->ItemInformation->rowCount(); line++)
        {
            if (ui->ItemInformation->item(line, 0) != NULL && ui->ItemInformation->item(line, 1) != NULL )
            {
                arg_num++;
                QString arg = ui->ItemInformation->item(line, 0)->text().trimmed();  //In JSON:arg_value\type
                if ('0'>arg[0] || arg[0]>'9')
                {
                    QMessageBox::warning(this, "Warning", "Args should start with a number and a blank", QMessageBox::Yes);
                    return;
                }
                QString arg_type_first = ui->ItemInformation->item(line, 1)->text().trimmed();   //In JSON:arg_type
                changed_argtype.insert(arg_num, arg_type_first);
                if (ui->ItemInformation->item(line, 2) != NULL && ui->ItemInformation->item(line, 2)->text() != "") //arg is not object type
                {
                    QString arg_value = ui->ItemInformation->item(line, 2)->text().trimmed();    //In JSON:arg_value\value
                    if (arg_value == "null")
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
                else if ((ui->ItemInformation->item(line, 2) == NULL && ui->ItemInformation->item(line + 1, 0) == NULL &&
                          ui->ItemInformation->item(line + 1, 1) != NULL) ||
                         (ui->ItemInformation->item(line, 2) != NULL && ui->ItemInformation->item(line, 2)->text() == "")){//arg is object type
                    QJsonObject argvalue_object;
                    QString arg_type_second = ui->ItemInformation->item(line + 1, 1)->text().trimmed();
                    if (arg_type_first != "Array" && arg_type_first != "array" ) // objectarg contains pointer
                    {
                        if (ui->ItemInformation->item(line + 1, 2) != NULL)
                        {
                            QString arg_value = ui->ItemInformation->item(line + 1, 2)->text().trimmed();
                            if (('0'<=arg_value[0]&&arg_value[0]<='9') || arg_value[0] == '-')
                            {
                                double temp = arg_value.toDouble();
                                argvalue_object.insert(arg_type_second, temp);
                            }
                            else {
                                argvalue_object.insert(arg_type_second, arg_value);
                            }
                        }
                        else {
                            argvalue_object.insert(arg_type_second, QJsonValue::Null);
                        }
                    }
                    else {  // objectarg contains array
                        QJsonArray argvalueobject_array;
                        int array_arg_length = 0;
                        if (ui->ItemInformation->item(line + 1, 2) != NULL)
                        {
                            for (int i = line + 1; i< ui->ItemInformation->rowCount(); i++)
                            {
                                if (ui->ItemInformation->item(i + 1, 1) == NULL && ui->ItemInformation->item(i + 1, 2) != NULL) {
                                    array_arg_length++;
                                }
                                else {
                                    array_arg_length++;
                                    break;
                                }
                            }
                            for (int i =0; i<array_arg_length; i++)
                            {
                                QString temp = ui->ItemInformation->item(line + 1 + i, 2)->text().trimmed();
                                if (('0' <= temp[0] && temp[0] <= '9') || temp[0] == '-')
                                {
                                    float temp_double = temp.toDouble();
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
                else {return;}
            }
        }
        if (arg_num == -1)
        {
            changedItemObject.insert("5 arg_type", changed_argtype);
            changedItemObject.insert("6 arg_value", QJsonValue::Null);
        }
        else {
            changedItemObject.insert("5 arg_type", changed_argtype);
            changedItemObject.insert("6 arg_value", changed_argvalue);
        }
    }
    resave_string.removeAt(changedItemNum);
    QJsonDocument document(changedItemObject);
    QString changedString = document.toJson(QJsonDocument::Indented);
    changedString.remove(changedString.size() - 1, 1);
    resave_string.insert(changedItemNum, changedString);
    ui->treeJsonFile->clearSelection();
    ui->treeJsonFile->setCurrentItem(item, 0);
}

void MainWindow::showtheChange(int framenum, int callnum, QJsonObject addItemObject, QString addText)
{
    newitemlocation->setBackground(0, QColor(0, 0, 0, 0));
    frame_end[framenum] = frame_end[framenum] + 1;
    for (int i = framenum + 1; i < int(frame_end.size()); i++)
    {
        frame_start[i] = frame_start[i] + 1;
        frame_end[i] = frame_end[i] + 1;
    }

    QJsonDocument document(addItemObject);
    QString addString = document.toJson(QJsonDocument::Indented);
    addString.remove(addString.size() - 1, 1);
    resave_string.insert(callnum, addString);

    QTreeWidgetItem *item1=new QTreeWidgetItem;
    item1->setText(0, addText);
    if (newitemlocation->parent()==NULL)
    {
        newitemlocation->insertChild(0, item1);
    }
    else {
        newitemlocation->parent()->insertChild(callnum - frame_start[framenum] + 1, item1);
    }
    ui->treeJsonFile->clearSelection();
    ui->treeJsonFile->setCurrentItem(item1);
    ui->treeJsonFile->scrollToItem(item1);
    new_Dialog->close();    //close the subwindow
}

void MainWindow::on_actionAdd_clicked()
{
    if (JsonOpened != true)
    {
        QMessageBox::warning(this, "Warning", "No Opened Json", QMessageBox::Yes);
        return;
    }
    QTreeWidgetItem *item = ui->treeJsonFile->currentItem();
    QTreeWidgetItem *parent = item->parent();
    if (parent == NULL) return;
    int num = ui->treeJsonFile->indexOfTopLevelItem(parent);
    int frame_start_num = frame_start[num];
    int row=parent->indexOfChild(item);
    int addItemNum = frame_start_num + row;

    QJsonObject addItemObject;
    int currentTableRow = 0;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str0 = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    int newCall_no = str0.toInt();
    addItemObject.insert("0 call_no", newCall_no);
    currentTableRow++;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str1 = ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    int newTid_no = str1.toInt();
    addItemObject.insert("1 tid", newTid_no);
    currentTableRow++;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str2= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    addItemObject.insert("2 func_name", str2);
    currentTableRow++;

    if (ui->ItemInformation->item(currentTableRow, 1) == NULL){return;}
    QString str3= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
    addItemObject.insert("3 return_type", str3);
    if (ui->ItemInformation->item(currentTableRow, 2) != NULL){
        QString str4= ui->ItemInformation->item(currentTableRow, 2)->text().trimmed();
        if (str4 == "null"){
            addItemObject.insert("4 return_value", QJsonValue::Null);
        }
        else if (('0' <= str4[0] && str4[0] <= '9') || str4[0] == '-') {
            int temp = str4.toDouble();
            addItemObject.insert("4 return_value", temp);
        }
        else {
            addItemObject.insert("4 return_value", str4);
        }
    }
    else if (ui->ItemInformation->item(currentTableRow, 2) == NULL && ui->ItemInformation->item(currentTableRow + 1, 0) == NULL &&
       ui->ItemInformation->item(currentTableRow + 1, 1) != NULL && ui->ItemInformation->item(currentTableRow + 1, 2) != NULL ){
        currentTableRow++;
        QJsonObject returnobject;
        QString str4_pointertype= ui->ItemInformation->item(currentTableRow, 1)->text().trimmed();
        QString str4_pointervalue= ui->ItemInformation->item(currentTableRow, 2)->text().trimmed();
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
    if (ui->ItemInformation->item(currentTableRow, 0) != NULL && ui->ItemInformation->item(currentTableRow, 0)->text().contains("arg"))
    {
        currentTableRow++;
        for (int line = currentTableRow; line< ui->ItemInformation->rowCount(); line++)
        {
            if (ui->ItemInformation->item(line, 0) != NULL && ui->ItemInformation->item(line, 1) != NULL )
            {
                arg_num++;
                QString arg = ui->ItemInformation->item(line, 0)->text().trimmed();  //In JSON:arg_value\type
                if ('0' > arg[0] || arg[0] > '9')
                {
                    QMessageBox::warning(this, "Warning", "Args should start with a number", QMessageBox::Yes);
                    return;
                }
                QString arg_type_first = ui->ItemInformation->item(line, 1)->text().trimmed();   //In JSON:arg_type
                changed_argtype.insert(arg_num, arg_type_first);
                if (ui->ItemInformation->item(line, 2) != NULL) //arg is not object type
                {
                    QString arg_value = ui->ItemInformation->item(line, 2)->text().trimmed();    //In JSON:arg_value\value
                    if (arg_value == "null")
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
                else if (ui->ItemInformation->item(line, 2) == NULL && ui->ItemInformation->item(line + 1, 0) == NULL &&
                         ui->ItemInformation->item(line + 1, 1) != NULL){  //arg is object type
                    QJsonObject argvalue_object;
                    QString arg_type_second = ui->ItemInformation->item(line + 1, 1)->text().trimmed();
                    if (arg_type_first != "Array" && arg_type_first != "array" ) // objectarg contains pointer
                    {
                        if (ui->ItemInformation->item(line + 1, 2) != NULL)
                        {
                            QString arg_value = ui->ItemInformation->item(line + 1, 2)->text().trimmed();
                            if (('0' <= arg_value[0] && arg_value[0] <= '9') || arg_value[0] == '-')
                            {
                                double temp = arg_value.toFloat();
                                argvalue_object.insert(arg_type_second, temp);
                            }
                            else {
                                argvalue_object.insert(arg_type_second, arg_value);
                            }
                        }
                        else {
                            argvalue_object.insert(arg_type_second, QJsonValue::Null);
                        }
                    }
                    else {  // objectarg contains array
                        QJsonArray argvalueobject_array;
                        int array_arg_length = 0;
                        if (ui->ItemInformation->item(line + 1, 2) != NULL)
                        {
                            for (int i = line + 1; i< ui->ItemInformation->rowCount(); i++)
                            {
                                if (ui->ItemInformation->item(i + 1, 1) == NULL && ui->ItemInformation->item(i + 1, 2) != NULL) {
                                    array_arg_length++;
                                }
                                else {
                                    array_arg_length++;
                                    break;
                                }
                            }
                            for (int i =0; i<array_arg_length; i++)
                            {
                                QString temp = ui->ItemInformation->item(line + 1 + i, 2)->text().trimmed();
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
                else {return;}
            }
        }
        if (arg_num == -1)
        {
            addItemObject.insert("5 arg_type", changed_argtype);
            addItemObject.insert("6 arg_value", QJsonValue::Null);
        }
        else{
            addItemObject.insert("5 arg_type", changed_argtype);
            addItemObject.insert("6 arg_value", changed_argvalue);
        }
    }

    QJsonDocument document(addItemObject);
    QString addString = document.toJson(QJsonDocument::Indented);
    addString.remove(addString.size() - 1, 1);
    resave_string.insert(addItemNum, addString);

    QTreeWidgetItem *addItem=new QTreeWidgetItem;
    QString addText = "(" + str0 + ")" + str2;
    addItem->setText(0, addText);
    parent->insertChild(row + 1, addItem);

    frame_end[num] = frame_end[num] + 1;
    for (int i=num + 1; i<int(frame_end.size()); i++)
    {
        frame_start[i] = frame_start[i] + 1;
        frame_end[i] = frame_end[i] + 1;
    }

    ui->treeJsonFile->clearSelection();
    ui->treeJsonFile->setCurrentItem(addItem);
}

void MainWindow::on_actionDelete_clicked()
{
    if (JsonOpened != true)
    {
        QMessageBox::warning(this, "Warning", "No Opened Json", QMessageBox::Yes);
        return;
    }
    QList<QTreeWidgetItem*> selectedItemList = ui->treeJsonFile->selectedItems();
    if (selectedItemList.size() < 1) return;

    for (int index =0; index<selectedItemList.size(); index++)
    {
        QTreeWidgetItem *item = selectedItemList[index];
        QTreeWidgetItem *parent = item->parent();
        int row;

        if (parent == NULL)
        {
            int flag = QMessageBox::warning(this, tr("Warning"), QString("This operation will delete the Json file"), QMessageBox::Yes, QMessageBox::No);
            if (flag == QMessageBox::Yes)
            {
                int FrameNum = ui->treeJsonFile->indexOfTopLevelItem(item);
                int frame_start_num = frame_start[FrameNum];
                int frame_end_num = frame_end[FrameNum];

                for (int i = frame_start_num; i <= frame_end_num; i++)
                {
                  delete item->takeChild(0); //delete son point from father point
                  resave_string.removeAt(frame_start_num - 1);
                }

                std::vector<int>::iterator i1 = frame_start.begin();
                std::vector<int>::iterator i2 = frame_end.begin();
                frame_start.erase(i1 + FrameNum);
                frame_end.erase(i2 + FrameNum);
                for (int j=FrameNum; j<int(frame_end.size()); j++)
                {
                    frame_start[j] = frame_start[j] - frame_end_num + frame_start_num - 1;
                    frame_end[j] = frame_end[j] - frame_end_num + frame_start_num - 1;
                }

                delete ui->treeJsonFile->takeTopLevelItem(FrameNum); //delete father point
                //delete item;
                QFile::remove(files[FrameNum]);
                files.removeAt(FrameNum);
            }
            else {return;}
        }
        else{
            int FrameNum = ui->treeJsonFile->indexOfTopLevelItem(parent);
            int frame_start_num = frame_start[FrameNum];
            row = parent->indexOfChild(item);
            int deleteItemNum = frame_start_num + row;
            resave_string.removeAt(deleteItemNum);
            delete parent->takeChild(row);
            frame_end[FrameNum] = frame_end[FrameNum] - 1;
            for (int i = FrameNum + 1; i < int(frame_end.size()); i++)
            {
                frame_start[i] = frame_start[i] - 1;
                frame_end[i] = frame_end[i] - 1;
            }
        }
    }
    ui->treeJsonFile->clearSelection();
    if (selectedItemList[selectedItemList.size() - 1]->parent() != NULL)
    {
        ui->treeJsonFile->setCurrentItem(selectedItemList[selectedItemList.size() - 1]->parent());
    }
}

void MainWindow::record_itemChange()
{
    changed_item = ui->treeJsonFile->currentItem();
    connect(ui->ItemInformation, SIGNAL(cellChanged(int, int)), this, SLOT(flagChanged(int, int)));
}

void MainWindow::flagChanged(int, int)
{
    itemchanged = true;
}
