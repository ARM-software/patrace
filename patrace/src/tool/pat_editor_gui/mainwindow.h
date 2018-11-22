#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStatusBar>
#include <QToolBar>
#include <QString>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextBlock>
#include <QWidget>
#include <QHeaderView>
#include <fstream>
#include <string>
#include "dialog.h"
#include "helpdialog.h"

#include <QThread>
#include <QObject>
#include "open_thread.h"
#include "percentage_thread.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void combineJsonContent(const QStringList &result, const int length, const int number);
    void openJsonFile();
    void openPatFile();
    void openJsonDir();
    void readPatContent();
    void saveasJson();
    void saveasPat();
    void on_treeJsonFile_itemSelectionChanged();
    void showTableWidget();
    void doSearching();
    void on_FindNext_clicked();
    void on_FindPrevious_clicked();
    void on_actionSetChange_clicked();
    void on_actionAdd_clicked();
    void on_actionDelete_clicked();
    void showtheChange(int framenum, int callnum, QJsonObject addItemObject, QString addText);
    void showPercentage();
    void showPercentage_saving();
    void record_itemChange();
    void flagChanged(int, int);
    void showhelp();
signals:
    void deliverfilename(const QStringList &);
    void deliverpatinf(const QString &,const QString &);
    void delivermergeinf(const QString &,const QString &);
    void GUIupdate();
    void GUIupdate_saving();
    void itemOpened();
private:
    void startOpenThread();
    void startPercentThread();
private:
    Ui::MainWindow *ui;
    Dialog *new_Dialog;
    HelpDialog *help_Dialog;
    OpenJson_Worker *o_obj = NULL;
    QThread *o_objThread = NULL;

    PercentageThread *percentObj = NULL;
    QThread *percentThread = NULL;

    QStringList resave_string;//save all json content
    std::vector<int> frame_start;//save the start location of each json file
    std::vector<int> frame_end;//save the end location of each json file

    QStringList files;// all selected json name
    QString OriginalDir;// location of the extracted pat
    bool JsonOpened = false;
    bool PatOpened = false;

    QString strTemplate;
    QList<QTreeWidgetItem *> list;
    int findnum;
    int nCount;

    QTreeWidgetItem *newitemlocation;
    bool itemchanged = false;
    QTreeWidgetItem *changed_item;
};

#endif // MAINWINDOW_H
