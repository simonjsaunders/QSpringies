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

#include "canvas.h"
#include "misc.h"
#include "system.h"
#include <QLine>
#include <QVector>
#include <QtWidgets>

const QColor backgroundColor(188, 247, 255);
const QColor springColor(6, 99, 154);
const QColor selectedColour(215, 180, 37);
const int springThickness = 2;

namespace {

double distance(const QPoint& p1, const QPoint& p2) {
    return hypot(p1.x() - p2.x(), p1.y() - p2.y());
}

QRect getBoundingBox(const QPoint& p1, const QPoint& p2, int margin) {
    int x1 = std::min(p1.x(), p2.x()) - margin;
    int y1 = std::min(p1.y(), p2.y()) - margin;
    int x2 = std::max(p1.x(), p2.x()) + margin;
    int y2 = std::max(p1.y(), p2.y()) + margin;
    QRect rect(x1, y1, x2 - x1, y2 - y1);
    return rect;
}

}

Canvas::Canvas(QWidget *parent) : QWidget(parent), mode_(ModeEdit),
    system_(nullptr), mouseDown_(false), staticSpring_(false), action_(false),
    selection_(-1), mouseOffset_(0), shiftKeyDown_(false),
    mouseButton_(Qt::NoButton) {
    spheres_[0].reset(new QPixmap(":/images/sphere30.png"));
    spheres_[1].reset(new QPixmap(":/images/sphere40.png"));
    spheres_[2].reset(new QPixmap(":/images/sphere50.png"));
    spheres_[3].reset(new QPixmap(":/images/sphere60.png"));
    spheres_[4].reset(new QPixmap(":/images/sphere70.png"));
    selectedSpheres_[0].reset(new QPixmap(":/images/sel_sphere30.png"));
    selectedSpheres_[1].reset(new QPixmap(":/images/sel_sphere40.png"));
    selectedSpheres_[2].reset(new QPixmap(":/images/sel_sphere50.png"));
    selectedSpheres_[3].reset(new QPixmap(":/images/sel_sphere60.png"));
    selectedSpheres_[4].reset(new QPixmap(":/images/sel_sphere70.png"));
}

MouseMode Canvas::mouseMode() const {
    return mode_;
}

void Canvas::setMouseMode(MouseMode mode) {
    mode_ = mode;
}

void Canvas::setSystem(System* system) {
    system_ = system;
}

System* Canvas::getSystem() {
    return system_;
}

void Canvas::setAction(bool action) {
    action_ = action;
}

void Canvas::paintEvent(QPaintEvent* event) {
    if (pixmap_.get() == 0)
        createPixmap(width(), height());
    QPainter painter(this);
    painter.drawPixmap(event->rect(), *pixmap_.get(), event->rect());
}

void Canvas::resizeEvent(QResizeEvent* event) {
    const QSize& size = event->size();
    createPixmap(size.width(), size.height());
}

void Canvas::createPixmap(int width, int height) {
    pixmap_.reset(new QPixmap(width, height));
    systemPixmap_.reset(new QPixmap(width, height));
    drawSystem();
}

void Canvas::drawMass(int mx, int my, double m) {
    int rad = massRadius(m);
    int size = sphereSize(rad);
    rad = sphereRadius(size);
    QRect target(mx - rad, my - rad, rad * 2, rad * 2);
    QRect source(0, 0, rad * 2, rad * 2);
    QPainter painter(pixmap_.get());
    painter.drawPixmap(target, *spheres_[size], source);
    update(target);
    rect_ = target;
}

void Canvas::drawMass(QPainter& painter, const Circle& circle, bool selected) {
    int rad = circle.radius;
    int size = circle.size;
    QRect target(circle.x, circle.y, rad * 2, rad * 2);
    QRect source(0, 0, rad * 2, rad * 2);
    painter.drawPixmap(target, selected ? *selectedSpheres_[size] : *spheres_[size], source);
}

void Canvas::drawSpring(const QPoint& p1, const QPoint& p2) {
    rect_ = getBoundingBox(p1, p2, springThickness);
    QPainter painter(pixmap_.get());
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(springColor);
    pen.setWidth(springThickness);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(p1, p2);
    update(rect_);
}

void Canvas::drawSprings(QPainter& painter, const QVector<QLine>& lines, bool selected) {
    QPen pen(selected ? selectedColour : springColor);
    pen.setWidth(springThickness);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLines(lines);
}

void Canvas::redraw() {
    drawSystem();
    update();
}

void Canvas::drawSystem() {
    if (system_ == nullptr)
        return;

    int screenWidth = width();
    int screenHeight = height();
    QRect screenRect(0, 0, screenWidth, screenHeight);

    QPainter painter(systemPixmap_.get());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(screenRect, backgroundColor);

    const State& state = system_->getState();

    // Draw springs
    if (state.show_spring) {
        QVector<QLine> lines, selectedLines;
        for (int i = 0, n = system_->springCount(); i < n; ++i) {
            const Spring& spring = system_->getSpring(i);
            if (spring.isAlive()) {
                const Mass& m1 = system_->getMass(spring.m1);
                const Mass& m2 = system_->getMass(spring.m2);
                int x1 = coordX(m1.x);
                int y1 = coordY(m1.y);
                int x2 = coordX(m2.x);
                int y2 = coordY(m2.y);
                if (spring.isSelected())
                    selectedLines.push_back(QLine(x1, y1, x2, y2));
                else
                    lines.push_back(QLine(x1, y1, x2, y2));
            }
        }
        if (!lines.empty())
            drawSprings(painter, lines, false);
        if (!selectedLines.empty())
            drawSprings(painter, selectedLines, true);
    }

    // Draw masses
    QVector<Circle> circles, selectedCircles;

    for (int i = 0, n = system_->massCount(); i < n; ++i) {
        const Mass& mass = system_->getMass(i);
        if (mass.isAlive()) {
            bool fixed = mass.isFixed() && !mass.isTempFixed();
            int rad = fixed ? NAIL_SIZE : mass.radius;
            int size = sphereSize(rad);
            rad = sphereRadius(size);
            if (coordX(mass.x + rad) >= 0 && coordX(mass.x - rad) <= screenWidth &&
                    coordY(mass.y - rad) >= 0 && coordY(mass.y + rad) <= screenHeight) {
                Circle circle;
                circle.x = coordX(mass.x) - rad;
                circle.y = coordY(mass.y) - rad;
                circle.radius = rad;
                circle.size = size;
                if (mass.isSelected())
                    selectedCircles.push_back(circle);
                else
                    circles.push_back(circle);
            }
        }
    }
    for (const Circle& circle : circles) {
        drawMass(painter, circle, false);
    }
    for (const Circle& circle : selectedCircles) {
        drawMass(painter, circle, true);
    }
    QPainter painter2(pixmap_.get());
    painter2.drawPixmap(screenRect, *systemPixmap_.get(), screenRect);
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    if (system_ == nullptr || !mouseDown_)
        return;

    int mx = event->x();
    int my = event->y();

    mousePrev_[mouseOffset_].x = mx;
    mousePrev_[mouseOffset_].y = my;
    mousePrev_[mouseOffset_].time = event->timestamp();
    mouseOffset_ = (mouseOffset_ + 1) % MOUSE_PREV;

    gridSnap(mx, my);

    if (mode_ == ModeMass) {
        const State& state = system_->getState();
        eraseRect(rect_);
        drawMass(mx, my, state.cur_mass);
    } else if (mode_ == ModeSpring) {
        if (staticSpring_) {
            eraseRect(rect_);
            QPoint endPoint(mx, my);
            const Mass& m = system_->getMass(selection_);
            QPoint startPoint(coordX(m.x), coordY(m.y));
            drawSpring(startPoint, endPoint);
        } else {
            system_->moveFakeMass(coordX(mx), coordY(my));
        }
    } else if (mode_ == ModeEdit) {
        if (mouseButton_ == Qt::LeftButton) {
            if (selection_ < 0) {
                eraseRect(rect_);
                endPoint_ = event->pos();
                drawRubberBand();
            }
        } else {
            /* Move objects relative to mouse */
            system_->moveSelectedMasses(deltaX(mx - endPoint_.x()), deltaY(my - endPoint_.y()));
            redraw();
            endPoint_ = event->pos();
        }
    }
}

void Canvas::mousePressEvent(QMouseEvent *event) {
    if (system_ == nullptr || mouseDown_)
        return;
    shiftKeyDown_ = ((event->modifiers() & Qt::ShiftModifier) != 0);
    mouseButton_ = event->button();
    memset(mousePrev_, 0, sizeof(mousePrev_));
    mouseDown_ = true;
    int mx = event->x();
    int my = event->y();
    gridSnap(mx, my);
    if (mode_ == ModeMass) {
        const State& state = system_->getState();
        drawMass(mx, my, state.cur_mass);
    } else if (mode_ == ModeSpring) {
        bool isMass = true;
        staticSpring_ = (!action_ || mouseButton_ == Qt::RightButton);
        QPoint startPoint(mx, my);
        QPoint endPoint(startPoint);
        selection_ = system_->nearestObject(coordX(mx), coordY(my), &isMass);
        if (selection_ >= 0 && isMass) {
            const Mass& m = system_->getMass(selection_);
            startPoint.setX(coordX(m.x));
            startPoint.setY(coordY(m.y));
            if (staticSpring_) {
                drawSpring(startPoint, endPoint);
            } else {
                system_->attachFakeSpring(selection_);
                system_->moveFakeMass(coordX(mx), coordY(my));
                redraw();
            }
        } else {
            mouseDown_ = false;
        }
    } else if (mode_ == ModeEdit) {
        startPoint_ = event->pos();
        endPoint_ = startPoint_;
        if (mouseButton_ == Qt::LeftButton) {
            bool isMass = false;
            selection_ = system_->nearestObject(coordX(mx), coordY(my), &isMass);

            /* If not shift clicking, unselect all currently selected items */
            if (!shiftKeyDown_) {
                system_->unselectAll();
                redraw();
            }
        } else {
            system_->setTempFixed(true);
        }
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    if (system_ == nullptr || !mouseDown_)
        return;
    mouseDown_ = false;
    int mx = event->x();
    int my = event->y();
    gridSnap(mx, my);
    if (mode_ == ModeMass) {
        const State& state = system_->getState();
        Mass& m = system_->getMass(system_->createMass());
        m.x = coordX((double)mx);
        m.y = coordY((double)my);
        m.mass = state.cur_mass;
        m.radius = massRadius(state.cur_mass);
        m.elastic = state.cur_rest;
        if (state.fix_mass)
            m.setFixed(true);
        /* Select newly added mass */
        if (!shiftKeyDown_)
            system_->unselectAll();
        m.setSelected(true);
        redraw();
    } else if (mode_ == ModeSpring) {
        const State& state = system_->getState();
        bool isMass = true;
        int startSel = selection_;

        if (!staticSpring_)
            system_->killFakeSpring();

        selection_ = system_->nearestObject(coordX(mx), coordY(my), &isMass);
        if ((staticSpring_ || !action_ || mouseButton_ == Qt::RightButton)
                && selection_ >= 0 && isMass && selection_ != startSel) {
            const Mass& startMass = system_->getMass(startSel);
            const Mass& endMass = system_->getMass(selection_);

            QPoint startPoint(coordX(startMass.x), coordX(startMass.y));
            QPoint endPoint(coordX(endMass.x), coordY(endMass.y));

            int newsel = system_->createSpring();
            Spring& spring = system_->getSpring(newsel);
            spring.m1 = startSel;
            spring.m2 = selection_;
            spring.ks = state.cur_ks;
            spring.kd = state.cur_kd;
            spring.restlen = distance(startPoint, endPoint);

            system_->addMassParent(startSel, newsel);
            system_->addMassParent(selection_, newsel);

            /* Select newly added spring */
            if (!shiftKeyDown_)
                system_->unselectAll();
            spring.setSelected(true);
        }
        redraw();
    } else if (mode_ == ModeEdit) {
        if (mouseButton_ == Qt::LeftButton) {
            if (selection_ < 0) {
                system_->selectObjects(std::min(coordX(startPoint_.x()), coordX(mx)),
                                        std::min(coordY(startPoint_.y()), coordY(my)),
                                        std::max(coordX(startPoint_.x()), coordX(mx)),
                                        std::max(coordY(startPoint_.y()), coordY(my)));
                if (system_->evalSelection())
                    emit updateControls();
                redraw();
            } else {
                bool isMass = false;
                selection_ = system_->nearestObject(coordX(mx), coordY(my), &isMass);
                if (selection_ >= 0) {
                    system_->selectObject(selection_, isMass, shiftKeyDown_);
                    if (system_->evalSelection())
                        emit updateControls();
                    redraw();
                }
            }
        } else if (mouseButton_ == Qt::MiddleButton) {
            system_->setTempFixed(false);
            redraw();
        } else if (mouseButton_ == Qt::RightButton) {
            int mvx, mvy;
            mouseVelocity(mvx, mvy);
            system_->setMassVelocity(deltaX(mvx), deltaY(mvy), false);
            system_->setTempFixed(false);
            redraw();
        }
    }
}

void Canvas::eraseRect(const QRect& rect) {
    QPainter painter(pixmap_.get());
    painter.drawPixmap(rect, *systemPixmap_.get(), rect);
    update(rect);
}

void Canvas::drawRubberBand() {
    QRect rect(getBoundingBox(startPoint_, endPoint_, 0));
    QRect outside(rect.adjusted(-1, -1, 1, 1));
    QPainter painter(pixmap_.get());
    painter.setPen(selectedColour);
    painter.drawRect(rect);
    update(outside);
    rect_ = outside;
}

void Canvas::gridSnap(int& x, int& y) {
    if (system_ == nullptr)
        return;
    const State& state = system_->getState();
    if (state.grid_snap && mode_ != ModeEdit) {
        int gsnap = state.cur_gsnap;
        x = ((x + gsnap / 2) / gsnap) * gsnap;
        y = ((y + gsnap / 2) / gsnap) * gsnap;
    }
}

void Canvas::mouseVelocity(int& mx, int& my) {
    int i, totalx = 0, totaly = 0, scale = 0, dx, dy, dt;
    int fudge = 256;

    for (i = 0; i < MOUSE_PREV - 1; i++) {
        const MouseInfo& m1 = mousePrev_[(mouseOffset_ + 2 + i) % MOUSE_PREV];
        const MouseInfo& m2 = mousePrev_[(mouseOffset_ + 1 + i) % MOUSE_PREV];
        dx = m1.x - m2.x;
        dy = m1.y - m2.y;
        dt = m1.time - m2.time;
        if (dt != 0) {
            scale += 64 * i * i;
            totalx += 64 * i * i * fudge * dx / dt;
            totaly += 64 * i * i * fudge * dy / dt;
        }
    }

    if (scale) {
        totalx /= scale;
        totaly /= scale;
    }

    mx = totalx;
    my = totaly;
}
