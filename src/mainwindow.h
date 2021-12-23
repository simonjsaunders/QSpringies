/*
 * Copyright (c) 2020 Simon J. Saunders
 *
 * This file is part of QSpringies.
 *
 * QSpringies is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QSpringies is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QSpringies.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "system.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QTimer;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void fileOpen();
    void fileInsert();
    void fileSave();
    void fileSaveAs();
    void start();
    void setRestLen();
    void setCenter();
    void forceComboChanged(int);
    void forceCheckBoxChanged(int);
    void editDelete();
    void editDuplicate();
    void editSelectAll();
    void stateRestore();
    void stateReset();
    void stateSave();
    void about();
    void aboutQt();
    void editButtonClicked();
    void massButtonClicked();
    void springButtonClicked();
    void fixedMassChanged(int);
    void showSpringsChanged(int);
    void adaptiveTimeStepChanged(int);
    void gridSnapChanged(int);
    void northChanged(int);
    void southChanged(int);
    void eastChanged(int);
    void westChanged(int);
    void collideChanged(int);
    void massValueChanged(double);
    void elasticityValueChanged(double);
    void kSpringValueChanged(double);
    void kDampValueChanged(double);
    void forceValueChanged(double);
    void miscValueChanged(double);
    void viscosityValueChanged(double);
    void stickinessValueChanged(double);
    void timeStepValueChanged(double);
    void precisionValueChanged(double);
    void gridSnapValueChanged(int);
    void tick();
    void updateControls();

protected:
    void closeEvent(QCloseEvent*) override;

private:
    void setCurrentFile(const QString&);
    void updateForceControls();
    void readSettings();
    void writeSettings();
    std::unique_ptr<Ui::MainWindow> ui;
    int force_;
    QString fileName_;
    QString currentDirectory_;
    System system_;
    System savedSystem_;
    QTimer* timer_;
};
#endif // MAINWINDOW_H
