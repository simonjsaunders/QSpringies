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

#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QTime>

class System;

enum MouseMode {
    ModeEdit,
    ModeMass,
    ModeSpring
};

/* Number of previous mouse state saves */
#define MOUSE_PREV	4

class Canvas : public QWidget {
    Q_OBJECT
public:
    explicit Canvas(QWidget *parent = nullptr);
    MouseMode mouseMode() const;
    void setMouseMode(MouseMode);
    void setSystem(System*);
    System* getSystem();
    void redraw();
    void setAction(bool);
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void updateControls();

private:
    struct Circle {
        int x, y, radius, size;
    };
    struct MouseInfo {
        int x, y;
        unsigned long time;
    };
    void createPixmap(int, int);
    void drawMass(int, int, double);
    void drawMass(QPainter&, const Circle&, bool);
    void drawSpring(const QPoint&, const QPoint&);
    void drawSprings(QPainter&, const QVector<QLine>&, bool);
    void drawSystem();
    void eraseRect(const QRect&);
    void gridSnap(int&, int&);
    void mouseVelocity(int&, int&);
    void drawRubberBand();
    MouseMode mode_;
    std::unique_ptr<QPixmap> pixmap_;
    std::unique_ptr<QPixmap> spheres_[5];
    std::unique_ptr<QPixmap> selectedSpheres_[5];
    std::unique_ptr<QPixmap> systemPixmap_;
    System* system_;
    bool mouseDown_;
    QRect rect_;
    bool staticSpring_;
    bool action_;
    int selection_;
    MouseInfo mousePrev_[MOUSE_PREV];
    int mouseOffset_;
    QPoint startPoint_, endPoint_;
    bool shiftKeyDown_;
    Qt::MouseButton mouseButton_;
};

#endif // CANVAS_H
