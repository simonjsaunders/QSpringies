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

#include "system.h"
#include "misc.h"
#include <float.h>
#include <math.h>
#include <algorithm>

const double MPROXIMITY	= 8.0;
const double SPROXIMITY	= 8.0;

System::System() {
    initObjects();
}

void System::initObjects() {
    fakeMass_ = createMass();
    masses_[fakeMass_].status = S_FIXED;
    fakeSpring_ = createSpring();
    springs_[fakeSpring_].status = 0;

    addMassParent(fakeMass_, fakeSpring_);
    springs_[fakeSpring_].m1 = fakeMass_;
}

void System::attachFakeSpring(int tomass) {
    addMassParent(fakeMass_, fakeSpring_);
    springs_[fakeSpring_].m2 = tomass;
    springs_[fakeSpring_].setAlive(true);
    springs_[fakeSpring_].ks = state_.cur_ks;
    springs_[fakeSpring_].kd = state_.cur_kd;
}

void System::killFakeSpring() {
    springs_[fakeSpring_].setAlive(false);
}

void System::moveFakeMass(int mx, int my) {
    masses_[fakeMass_].x = mx;
    masses_[fakeMass_].y = my;
}

/* create_mass: return the index for a new mass,
   possibly allocating more space if necesary
   */
int System::createMass() {
    int num = massCount();
    masses_.resize(num + 1);
    return num;
}

/* create_spring: return the index for a new spring,
   possibly allocating more space if necesary
   */
int System::createSpring() {
    int num = springCount();
    springs_.resize(num + 1);
    return num;
}

void System::addMassParent(int which, int parent) {
    masses_[which].parents.push_back(parent);
}

void System::deleteMassParent(int which, int parent) {
    Mass& mass = masses_[which];
    if (mass.isAlive()) {
        auto it = std::find(mass.parents.begin(), mass.parents.end(), parent);
        if (it != mass.parents.end())
            mass.parents.erase(it);
    }
}

/* delete_spring: delete a particular spring
 */
void System::deleteSpring(int which) {
    Spring& spring = springs_[which];
    if (spring.isAlive()) {
        spring.status = 0;
        deleteMassParent(spring.m1, which);
        deleteMassParent(spring.m2, which);
    }
}

/* delete_mass: delete a particular mass, and all springs
   directly attached to it
   */
void System::deleteMass(int which) {
    Mass& mass = masses_[which];
    if (mass.isAlive()) {
        mass.status = 0;

        /* Delete all springs connected to it */
        for (int par : masses_[which].parents)
            deleteSpring(par);
    }

    if (which == state_.center_id)
        state_.center_id = -1;
}

/* delete_selected: delete all objects which
   are currently selected
   */
void System::deleteSelected() {
    int i, n = massCount();
    for (i = 0; i < n; i++) {
        if (masses_[i].isSelected())
            deleteMass(i);
    }
    n = springCount();
    for (i = 0; i < n; i++) {
        if (springs_[i].isSelected())
            deleteSpring(i);
    }
}

void System::deleteAll() {
    masses_.clear();
    springs_.clear();
    state_.center_id = -1;
}

void System::reconnectMasses() {
    int i, n = massCount();

    for (i = 0; i < n; i++)
        masses_[i].parents.clear();

    n = springCount();
    for (i = 0; i < n; i++) {
        addMassParent(springs_[i].m1, i);
        addMassParent(springs_[i].m2, i);
    }
}

/* nearest_object:  Find the nearest spring or mass to the position
   (x,y), or return -1 if none are close.  Set is_mass accordingly
   */
int System::nearestObject(int x, int y, bool* is_mass) const {
    int i, closest = -1;
    double dist, min_dist = MPROXIMITY * MPROXIMITY, rating, min_rating = DBL_MAX;
    bool masses_only = *is_mass;
    int n = massCount();

    *is_mass = true;

    if (masses_only)
        min_dist = min_dist * 36;

    /* Find closest mass */
    for (i = 0; i < n; i++) {
        const Mass& m = masses_[i];
        if (m.isAlive()) {
            int radius = screenRadius(m.radius);
            if ((dist = square(m.x - x) + square(m.y - y)
                 - square(radius)) < min_dist) {
                rating = square(m.x - x) + square(m.y - y);
                if (rating < min_rating) {
                    min_dist = dist;
                    min_rating = rating;
                    closest = i;
                }
            }
        }
    }

    if (closest != -1)
        return closest;

    if (masses_only)
        return -1;

    *is_mass = true;

    min_dist = SPROXIMITY;
    n = springCount();

    /* Find closest spring */
    for (i = 0; i < n; i++) {
        double x1, x2, y1, y2;

        if (springs_[i].isAlive()) {
            x1 = masses_[springs_[i].m1].x;
            y1 = masses_[springs_[i].m1].y;
            x2 = masses_[springs_[i].m2].x;
            y2 = masses_[springs_[i].m2].y;

            if (x > std::min(x1, x2) - SPROXIMITY && x < std::max(x1, x2) + SPROXIMITY &&
                    y > std::min(y1, y2) - SPROXIMITY && y < std::max(y1, y2) + SPROXIMITY) {
                double a1, b1, c1, dAB, d;

                a1 = y2 - y1;
                b1 = x1 - x2;
                c1 = y1 * x2 - y2 * x1;
                dAB = hypot(a1, b1);
                d = (x * a1 + y * b1 + c1) / dAB;

                dist = fabs(d);

                if (dist < min_dist) {
                    min_dist = dist;
                    closest = i;
                    *is_mass = false;
                }
            }
        }
    }

    return closest;
}

bool System::evalSelection() {
    int i;
    double sel_mass, sel_elas, sel_ks, sel_kd;
    bool sel_fix;
    bool found = false, changed = false;
    bool mass_same, elas_same, ks_same, kd_same, fix_same;

    for (i = 0; i < massCount(); i++) {
        if (masses_[i].isSelected()) {
            if (found) {
                if (mass_same && masses_[i].mass != sel_mass)
                    mass_same = false;
                if (elas_same && masses_[i].elastic != sel_elas)
                    elas_same = false;
                if (fix_same && masses_[i].isFixed())
                    fix_same = false;
            } else {
                found = true;
                sel_mass = masses_[i].mass;
                mass_same = true;
                sel_elas = masses_[i].elastic;
                elas_same = true;
                sel_fix = masses_[i].isFixed();
                fix_same = true;
            }
        }
    }

    if (found) {
        if (mass_same && sel_mass != state_.cur_mass) {
            state_.cur_mass = sel_mass;
            changed = true;
        }
        if (elas_same && sel_elas != state_.cur_rest) {
            state_.cur_rest = sel_elas;
            changed = true;
        }
        if (fix_same && sel_fix != state_.fix_mass) {
            state_.fix_mass = sel_fix;
            changed = true;
        }
    }

    found = false;
    for (i = 0; i < springCount(); i++) {
        if (springs_[i].isSelected()) {
            if (found) {
                if (ks_same && springs_[i].ks != sel_ks)
                    ks_same = false;
                if (ks_same && springs_[i].ks != sel_ks)
                    ks_same = false;
            } else {
                found = true;
                sel_ks = springs_[i].ks;
                ks_same = true;
                sel_kd = springs_[i].kd;
                kd_same = true;
            }
        }
    }

    if (found) {
        if (ks_same && sel_ks != state_.cur_ks) {
            state_.cur_ks = sel_ks;
            changed = true;
        }
        if (kd_same && sel_kd != state_.cur_kd) {
            state_.cur_kd = sel_kd;
            changed = true;
        }
    }

    return changed;
}

bool System::anythingSelected() const {
    for (const Mass& mass : masses_) {
        if (mass.isSelected())
            return true;
    }
    for (const Spring& spring : springs_) {
        if (spring.isSelected())
            return true;
    }
    return false;
}

void System::selectObject(int selection, bool is_mass, bool shifted) {
    if (is_mass) {
        if (shifted)
            masses_[selection].toggleSelected();
        else
            masses_[selection].setSelected(true);
    } else {
        if (shifted)
            springs_[selection].toggleSelected();
        else
            springs_[selection].setSelected(true);
    }
}

void System::selectObjects(int ulx, int uly, int lrx, int lry) {
    for (int i = 0; i < massCount(); i++) {
        const Mass& m = masses_[i];
        if (m.isAlive()) {
            if (ulx <= m.x && m.x <= lrx && uly <= m.y && m.y <= lry)
                selectObject(i, true, false);
        }
    }

    for (int i = 0; i < springCount(); i++) {
        if (springs_[i].isAlive()) {
            const Mass& m1 = masses_[springs_[i].m1];
            const Mass& m2 = masses_[springs_[i].m2];

            if (ulx <= m1.x && m1.x <= lrx && uly <= m1.y && m1.y <= lry &&
                    ulx <= m2.x && m2.x <= lrx && uly <= m2.y && m2.y <= lry)
                selectObject(i, false, false);
        }
    }
}

void System::unselectAll() {
    for (Mass& mass : masses_) {
        if (mass.isSelected())
            mass.setSelected(false);
    }
    for (Spring& spring : springs_) {
        if (spring.isSelected())
            spring.setSelected(false);
    }
}

void System::selectAll() {
    for (Mass& mass : masses_) {
        if (mass.isAlive())
            mass.setSelected(true);
    }
    for (Spring& spring : springs_) {
        if (spring.isAlive())
            spring.setSelected(true);
    }
}

void System::duplicateSelected() {
    int i, j, num_map, spring_start;
    int which;

    spring_start = springCount();

    std::vector<int> mapfrom;
    std::vector<int> mapto;

    for (i = 0; i < massCount(); i++) {
        if (masses_[i].isSelected()) {
            which = createMass();
            mapto.push_back(which);
            mapfrom.push_back(i);
            masses_[which] = masses_[i];
            masses_[which].setSelected(false);
            masses_[which].parents.clear();
        }
    }
    num_map = mapto.size();

    for (i = 0; i < spring_start; i++) {
        if (springs_[i].isSelected()) {
            bool m1done = false, m2done = false;

            which = createSpring();
            springs_[which] = springs_[i];
            springs_[which].setSelected(false);

            for (j = 0; (!m1done || !m2done) && j < num_map; j++) {
                if (!m1done && springs_[which].m1 == mapfrom[j]) {
                    springs_[which].m1 = mapto[j];
                    addMassParent(mapto[j], which);
                    m1done = true;
                }
                if (!m2done && springs_[which].m2 == mapfrom[j]) {
                    springs_[which].m2 = mapto[j];
                    addMassParent(mapto[j], which);
                    m2done = true;
                }
            }
            if (!m1done && !m2done) {
                /* delete spring that isn't connected to anyone */
                deleteSpring(which);
            }
        }
    }
}

void System::moveSelectedMasses(int dx, int dy) {
    for (Mass& mass : masses_) {
        if (mass.isSelected()) {
            mass.x += dx;
            mass.y += dy;
        }
    }
}

void System::setMassVelocity(int vx, int vy, bool relative) {
    for (Mass& mass : masses_) {
        if (mass.isSelected()) {
            if (relative) {
                mass.vx += vx;
                mass.vy += vy;
            } else {
                mass.vx = vx;
                mass.vy = vy;
            }
        }
    }
}

void System::setTempFixed(bool store) {
    for (Mass& mass : masses_) {
        if (mass.isSelected()) {
            if (store) {
                mass.setTempFixed(false);
                if (!mass.isFixed()) {
                    mass.setTempFixed(true);
                    mass.setFixed(true);
                }
            } else {
                if (mass.isTempFixed())
                    mass.setFixed(false);
            }
        }
    }
}

void System::setRestLenth() {
    for (Spring& spring : springs_) {
        if (spring.isSelected()) {
            double dx = masses_[spring.m1].x - masses_[spring.m2].x;
            double dy = masses_[spring.m1].y - masses_[spring.m2].y;
            spring.restlen = hypot(dx, dy);
        }
    }
}

void System::setCenter() {
    int i, cent = -1;

    for (i = 0; i < massCount(); i++) {
        if (masses_[i].isSelected()) {
            if (cent != -1)
                return;
            cent = i;
        }
    }
    state_.center_id = cent;
}

void System::reset() {
    deleteAll();
    initObjects();
    state_.reset();
}
