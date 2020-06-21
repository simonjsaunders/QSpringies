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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "file.h"
#include "misc.h"
#include "phys.h"
#include "version.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>

bool updateBoolValue(bool& value, int state) {
    bool newValue = (state == Qt::Checked);
    if (value != newValue) {
        value = newValue;
        return true;
    }
    return false;
}

void setChecked(QCheckBox* checkBox, bool checked) {
    QSignalBlocker blocker(checkBox);
    checkBox->setChecked(checked);
}

void setValue(QDoubleSpinBox* spinBox, double value) {
    QSignalBlocker blocker(spinBox);
    spinBox->setValue(value);
}

void setValue(QSpinBox* spinBox, int value) {
    QSignalBlocker blocker(spinBox);
    spinBox->setValue(value);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    force_(0) {
    ui->setupUi(this);

    ui->canvas->setSystem(&system_);

    MouseMode mode = ui->canvas->mouseMode();
    ui->editButton->setChecked(mode == ModeEdit);
    ui->massButton->setChecked(mode == ModeMass);
    ui->springButton->setChecked(mode == ModeSpring);
    updateControls();
    ui->forceComboBox->addItem(tr("Gravity"));
    ui->forceComboBox->addItem(tr("Center of Mass"));
    ui->forceComboBox->addItem(tr("Point Attraction"));
    ui->forceComboBox->addItem(tr("Wall Repel/Attract"));
    forceComboChanged(force_);

    connect(ui->actionFileOpen, &QAction::triggered, this, &MainWindow::fileOpen);
    connect(ui->actionFileInsert, &QAction::triggered, this, &MainWindow::fileInsert);
    connect(ui->actionFileSave, &QAction::triggered, this, &MainWindow::fileSave);
    connect(ui->actionFileSaveAs, &QAction::triggered, this, &MainWindow::fileSaveAs);

    connect(ui->actionDuplicate, &QAction::triggered, this, &MainWindow::editDuplicate);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::editDelete);
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::editSelectAll);

    connect(ui->actionRestoreState, &QAction::triggered, this, &MainWindow::stateRestore);
    connect(ui->actionResetState, &QAction::triggered, this, &MainWindow::stateReset);
    connect(ui->actionSaveState, &QAction::triggered, this, &MainWindow::stateSave);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::aboutQt);

    connect(ui->startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(ui->setRestLengthButton, SIGNAL(clicked()), this, SLOT(setRestLen()));
    connect(ui->setCenterButton, SIGNAL(clicked()), this, SLOT(setCenter()));

    connect(ui->editButton, SIGNAL(clicked()), this, SLOT(editButtonClicked()));
    connect(ui->massButton, SIGNAL(clicked()), this, SLOT(massButtonClicked()));
    connect(ui->springButton, SIGNAL(clicked()), this, SLOT(springButtonClicked()));

    connect(ui->fixedMassCheckBox, SIGNAL(stateChanged(int)), this, SLOT(fixedMassChanged(int)));
    connect(ui->showSpringsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(showSpringsChanged(int)));
    connect(ui->adaptiveCheckBox, SIGNAL(stateChanged(int)), this, SLOT(adaptiveTimeStepChanged(int)));
    connect(ui->gridSnapCheckBox, SIGNAL(stateChanged(int)), this, SLOT(gridSnapChanged(int)));
    connect(ui->northCheckBox, SIGNAL(stateChanged(int)), this, SLOT(northChanged(int)));
    connect(ui->southCheckBox, SIGNAL(stateChanged(int)), this, SLOT(southChanged(int)));
    connect(ui->eastCheckBox, SIGNAL(stateChanged(int)), this, SLOT(eastChanged(int)));
    connect(ui->westCheckBox, SIGNAL(stateChanged(int)), this, SLOT(westChanged(int)));
    connect(ui->collideCheckBox, SIGNAL(stateChanged(int)), this, SLOT(collideChanged(int)));

    connect(ui->massSpinBox, SIGNAL(valueChanged(double)), this, SLOT(massValueChanged(double)));
    connect(ui->elasticitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(elasticityValueChanged(double)));
    connect(ui->kSpringSpinBox, SIGNAL(valueChanged(double)), this, SLOT(kSpringValueChanged(double)));
    connect(ui->kDampSpinBox, SIGNAL(valueChanged(double)), this, SLOT(kDampValueChanged(double)));
    connect(ui->forceSpinBox, SIGNAL(valueChanged(double)), this, SLOT(forceValueChanged(double)));
    connect(ui->miscSpinBox, SIGNAL(valueChanged(double)), this, SLOT(miscValueChanged(double)));
    connect(ui->viscositySpinBox, SIGNAL(valueChanged(double)), this, SLOT(viscosityValueChanged(double)));
    connect(ui->stickinessSpinBox, SIGNAL(valueChanged(double)), this, SLOT(stickinessValueChanged(double)));
    connect(ui->timeStepSpinBox, SIGNAL(valueChanged(double)), this, SLOT(timeStepValueChanged(double)));
    connect(ui->precisionSpinBox, SIGNAL(valueChanged(double)), this, SLOT(precisionValueChanged(double)));
    connect(ui->gridSnapSpinBox, SIGNAL(valueChanged(int)), this, SLOT(gridSnapValueChanged(int)));

    connect(ui->forceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(forceComboChanged(int)));
    connect(ui->forceCheckBox, SIGNAL(stateChanged(int)), this, SLOT(forceCheckBoxChanged(int)));

    connect(ui->canvas, SIGNAL(updateControls()), this, SLOT(updateControls()));

    timer_ = new QTimer(this);
    timer_->setInterval(20);
    connect(timer_, &QTimer::timeout, this, &MainWindow::tick);
    readSettings();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::readSettings() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    currentDirectory_ = settings.value("directory", "").toString();
}

void MainWindow::writeSettings() {
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("directory", currentDirectory_);
}

void MainWindow::closeEvent(QCloseEvent* /*event*/) {
    writeSettings();
}

void MainWindow::about() {
   QMessageBox::about(this, tr("About QSpringies"),
        tr("<p>QSpringies Version %1</p>"
           "<p>Copyright &copy; %2 Simon J. Saunders</p>"
           "<p>QSpringies is a mass and spring simulation system implemented with Qt.</p>"
           "<p>It is based on XSpringies by Douglas M. DeCarlo.</p>"
           "<p>QSpringies is free software: you can redistribute it and/or modify "
           "it under the terms of the GNU General Public License as published by "
           "the Free Software Foundation, either version 3 of the License, or "
           "(at your option) any later version.</p>"
           "<p>QSpringies is distributed in the hope that it will be useful, "
           "but WITHOUT ANY WARRANTY; without even the implied warranty of "
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
           "GNU General Public License for more details.</p>"
           "<p>You should have received a copy of the GNU General Public License "
           "along with QSpringies. If not, see "
           "<a href='https://www.gnu.org/licenses/'>https://www.gnu.org/licenses/</a>.</p>"
           ).arg(QStringLiteral(VERSION_NUMBER), QStringLiteral(COPYRIGHT_YEAR)));
}

void MainWindow::aboutQt() {
   QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::fileOpen() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), currentDirectory_, tr("XSpringies (*.xsp)"));
    if (fileName.isEmpty())
        return;
    if (!fileCommand(fileName, FileLoad, system_)) {
        QMessageBox::critical(this, tr("Error"), tr("Open file failed."));
        return;
    }
    updateControls();
    ui->canvas->redraw();
    setCurrentFile(fileName);
}

void MainWindow::setCurrentFile(const QString& fileName) {
    fileName_ = fileName;
    QFileInfo fi(fileName);
    currentDirectory_ = fi.absolutePath();
    statusBar()->showMessage(tr("Current file: ") + fileName_);
}

void MainWindow::fileInsert() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Insert File"), currentDirectory_, tr("XSpringies (*.xsp)"));
    if (fileName.isEmpty())
        return;
    if (!fileCommand(fileName, FileInsert, system_))
        QMessageBox::critical(this, tr("Error"), tr("Insert file failed."));
    ui->canvas->redraw();
}

void MainWindow::fileSave() {
    if (fileName_.isEmpty())
        fileSaveAs();
    else if (!fileCommand(fileName_, FileSave, system_))
        QMessageBox::critical(this, tr("Error"), tr("Save file failed."));
}

void MainWindow::fileSaveAs() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), currentDirectory_, tr("XSpringies (*.xsp)"));
    if (fileName.isEmpty())
        return;
    if (!fileCommand(fileName, FileSave, system_)) {
        QMessageBox::critical(this, tr("Error"), tr("Save file failed."));
        return;
    }
    setCurrentFile(fileName);
}

void MainWindow::setRestLen() {
    system_.setRestLenth();
}

void MainWindow::setCenter() {
    system_.setCenter();
    ui->canvas->redraw();
}

void MainWindow::editDelete() {
    system_.deleteSelected();
    ui->canvas->redraw();
}

void MainWindow::editDuplicate() {
    system_.duplicateSelected();
    ui->canvas->redraw();
}

void MainWindow::editSelectAll() {
    system_.selectAll();
    if (system_.evalSelection())
        updateControls();
    ui->canvas->redraw();
}

void MainWindow::stateRestore() {
    system_ = savedSystem_;
    updateControls();
    ui->canvas->redraw();
}

void MainWindow::stateReset() {
    system_.reset();
    updateControls();
    ui->canvas->redraw();
}

void MainWindow::stateSave() {
    savedSystem_ = system_;
}

void MainWindow::start() {
    if (timer_->isActive()) {
        timer_->stop();
        ui->canvas->setAction(false);
        ui->startButton->setText(tr("Start"));
    } else {
        timer_->start();
        ui->canvas->setAction(true);
        ui->startButton->setText(tr("Stop"));
    }
}

void MainWindow::tick() {
    int width = ui->canvas->width();
    int height = ui->canvas->height();
    Physics phys(system_, width, height);
    if (phys.advance()) {
        ui->canvas->redraw();
        State& st = system_.getState();
        if (st.adaptive_step)
            setValue(ui->timeStepSpinBox, st.cur_dt);
    }
}

void MainWindow::forceComboChanged(int index) {
    static const char *forceNames[BF_NUM] = { "Gravity:", "Magnitude:", "Magnitude:", "Magnitude:" };
    static const char *miscNames[BF_NUM] = { "Direction:", "Damping:", "Exponent:", "Exponent:" };
    static double maxForceValue[BF_NUM] = { 10000000.0, 10000000.0, 10000000.0, 10000000.0 };
    static double minForceValue[BF_NUM] = { 0.0, -10000000.0, -10000000.0, -10000000.0 };
    static double maxMiscValue[BF_NUM] = { 360.0, 10000000.0, 1000.0, 1000.0 };
    static double minMiscValue[BF_NUM] = { -360.0, 0.0, 0.0, -0.0 };

    force_ = index;
    ui->forceLabel->setText(tr(forceNames[index]));
    ui->miscLabel->setText(tr(miscNames[index]));
    ui->forceSpinBox->setRange(minForceValue[index], maxForceValue[index]);
    ui->miscSpinBox->setRange(minMiscValue[index], maxMiscValue[index]);
    updateForceControls();
}

void MainWindow::forceCheckBoxChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.force_enabled[force_], state);
}

void MainWindow::updateForceControls() {
    State& state = system_.getState();
    setChecked(ui->forceCheckBox, state.force_enabled[force_]);
    setValue(ui->forceSpinBox, state.cur_grav_val[force_]);
    setValue(ui->miscSpinBox, state.cur_misc_val[force_]);
}

void MainWindow::updateControls() {
    State& state = system_.getState();
    setValue(ui->massSpinBox, state.cur_mass);
    setValue(ui->elasticitySpinBox, state.cur_rest);
    setValue(ui->kSpringSpinBox, state.cur_ks);
    setValue(ui->kDampSpinBox, state.cur_kd);
    setChecked(ui->fixedMassCheckBox, state.fix_mass);
    setChecked(ui->showSpringsCheckBox, state.show_spring);
    setValue(ui->viscositySpinBox, state.cur_visc);
    setValue(ui->stickinessSpinBox, state.cur_stick);
    setValue(ui->timeStepSpinBox, state.cur_dt);
    setValue(ui->precisionSpinBox, state.cur_prec);
    setChecked(ui->adaptiveCheckBox, state.adaptive_step);
    setChecked(ui->gridSnapCheckBox, state.grid_snap);
    setValue(ui->gridSnapSpinBox, state.cur_gsnap);
    setChecked(ui->northCheckBox, state.w_top);
    setChecked(ui->southCheckBox, state.w_bottom);
    setChecked(ui->eastCheckBox, state.w_left);
    setChecked(ui->westCheckBox, state.w_right);
    setChecked(ui->collideCheckBox, state.collide);
    updateForceControls();
}

void MainWindow::editButtonClicked() {
    ui->canvas->setMouseMode(ModeEdit);
}

void MainWindow::massButtonClicked() {
    ui->canvas->setMouseMode(ModeMass);
}

void MainWindow::springButtonClicked() {
    ui->canvas->setMouseMode(ModeSpring);
}

void MainWindow::fixedMassChanged(int state) {
    State& st = system_.getState();
    if (!updateBoolValue(st.fix_mass, state))
        return;
    for (int i = 0; i < system_.massCount(); i++) {
        Mass& mass = system_.getMass(i);
        if (mass.isSelected()) {
            mass.setFixed(st.fix_mass);
            mass.setTempFixed(false);
        }
    }
    ui->canvas->redraw();
}

void MainWindow::showSpringsChanged(int state) {
    State& st = system_.getState();
    if (updateBoolValue(st.show_spring, state))
        ui->canvas->redraw();
}

void MainWindow::adaptiveTimeStepChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.adaptive_step, state);
}

void MainWindow::gridSnapChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.grid_snap, state);
}

void MainWindow::northChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.w_top, state);
}

void MainWindow::southChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.w_bottom, state);
}

void MainWindow::eastChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.w_left, state);
}

void MainWindow::westChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.w_right, state);
}

void MainWindow::collideChanged(int state) {
    State& st = system_.getState();
    updateBoolValue(st.collide, state);
}

void MainWindow::massValueChanged(double value) {
    State& st = system_.getState();
    if (st.cur_mass != value) {
        st.cur_mass = value;
        for (int i = 0; i < system_.massCount(); ++i) {
            Mass& mass = system_.getMass(i);
            if (mass.isSelected()) {
                mass.mass = st.cur_mass;
                mass.radius = massRadius(st.cur_mass);
            }
        }
        ui->canvas->redraw();
    }
}

void MainWindow::elasticityValueChanged(double value) {
    State& st = system_.getState();
    if (st.cur_rest != value) {
        st.cur_rest = value;
        for (int i = 0; i < system_.massCount(); ++i) {
            Mass& mass = system_.getMass(i);
            if (mass.isSelected())
                mass.elastic = st.cur_rest;
        }
    }
}

void MainWindow::kSpringValueChanged(double value) {
    State& st = system_.getState();
    if (st.cur_ks != value) {
        st.cur_ks = value;
        for (int i = 0; i < system_.springCount(); ++i) {
            Spring& spring = system_.getSpring(i);
            if (spring.isSelected())
                spring.ks = st.cur_ks;
        }
    }
}

void MainWindow::kDampValueChanged(double value) {
    State& st = system_.getState();
    if (st.cur_kd != value) {
        st.cur_kd = value;
        for (int i = 0; i < system_.springCount(); ++i) {
            Spring& spring = system_.getSpring(i);
            if (spring.isSelected())
                spring.kd = st.cur_kd;
        }
    }
}

void MainWindow::forceValueChanged(double value) {
    State& st = system_.getState();
    st.cur_grav_val[force_] = value;
}

void MainWindow::miscValueChanged(double value) {
    State& st = system_.getState();
    st.cur_misc_val[force_] = value;
}

void MainWindow::viscosityValueChanged(double value) {
    State& st = system_.getState();
    st.cur_visc = value;
}

void MainWindow::stickinessValueChanged(double value) {
    State& st = system_.getState();
    st.cur_stick = value;
}

void MainWindow::timeStepValueChanged(double value) {
    State& st = system_.getState();
    st.cur_dt = value;
}

void MainWindow::precisionValueChanged(double value) {
    State& st = system_.getState();
    st.cur_prec = value;
}

void MainWindow::gridSnapValueChanged(int value) {
    State& st = system_.getState();
    st.cur_gsnap = value;
}
