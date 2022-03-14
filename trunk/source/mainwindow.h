/*
   SensHub

   Copyright (C) 2009:
         Daniel Roggen, droggen@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDir>
#include <QTime>
#include <QLabel>
#include <QTableWidgetItem>
#include <QAbstractTableModel>
#include <QAbstractItemDelegate>
#include <QItemDelegate>
#include <QGridLayout>
#include <QListWidget>

#include "Reader/SimSensReader.h"
#include "Reader/Hub.h"
#include "TableDisplayWidget.h"

namespace Ui
{
    class MainWindow;
}





class MainWindow : public QMainWindow//, public QThread
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    Hub MyHub;
    QDir path;

    QTableDisplayWidget *StatTable,*StatTableM;

    std::vector<DeviceSpec> DeviceSpecification,DeviceSpecificationAbs;
    QPixmap PixmapConnected,PixmapConnectedSRE,PixmapDisconnected,PixmapInProgress;
    QLabel *SBStart,*SBDuration;
    QTime HubStartTime;
    bool HubStarted;

    int StatUpdateCounter0,StatUpdateCounter1,StatUpdateCounter2,StatUpdatePeriod;

protected:
     void timerEvent(QTimerEvent *event);
     int ParseDeviceString(QString devicestring,std::vector<DeviceSpec> &devspec,QString &err);
     void InitTable();
     QPixmap Hist2Pixmap(const std::vector<double> &hist);

     QString GetLabelFile();
     QString GetLabelFileAbs();
     DeviceSpec CheckDSValid(bool &ok,QString &err);
     void SetDSList();
     void SetDSListFromString(QString str);
     void loadConfiguration(QString fileName);
     bool ApplyConfiguration();
     void UpdateStatusBar();
     void OfflineMerge();
     void DeviceSpecification2DeviceSpecificationAbs();
     QString CtrSize2String(unsigned v);
     QString DeviceSpecification2String(DeviceSpec ds);

private slots:
    void on_actionHowto_triggered();
    void on_pushButton_PIDAutoSet_clicked();
    void on_pushButton_clicked();
    void on_action_About_triggered();
    void on_pushButton_StartStop_clicked();
    void on_action_Load_configuration_triggered();
    void on_action_Save_configuration_triggered();
    void on_pushButton_DSDefine_clicked();
    void on_pushButton_DSDown_clicked();
    void on_pushButton_DSUp_clicked();
    void on_pushButton_DSModify_clicked();
    void on_listWidget_DSList_currentRowChanged(int currentRow);
    void on_pushButton_DSDelete_clicked();
    void on_pushButton_DSAdd_clicked();
    void on_toolButtonWorkingDirectory_clicked();

    void StatisticsChanged(int type, int reader);
    //void keyPressEvent(QKeyEvent * event);

};






#endif // MAINWINDOW_H
