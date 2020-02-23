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

#ifndef SYSTEM_H
#define SYSTEM_H

#include <vector>
#include "state.h"

#define S_ALIVE		0x01
#define S_SELECTED	0x02
#define S_FIXED		0x04
#define S_TEMPFIXED	0x08

struct Mass
{
    Mass() : x(0), y(0), vx(0), vy(0), ax(0), ay(0), mass(0),
        elastic(0), radius(0), status(S_ALIVE)
    {
    }
    /* Current position, velocity, acceleration */
    double x, y;
    double vx, vy;
    double ax, ay;

    /* Mass and radius of mass */
    double mass;
    double elastic;
    int radius;

    /* Connections to springs */
    std::vector<int> parents;

    int status;

    /* RK temporary space */
    double cur_x, cur_y, cur_vx, cur_vy;
    double old_x, old_y, old_vx, old_vy;
    double test_x, test_y, test_vx, test_vy;
    double k1x, k1y, k1vx, k1vy;
    double k2x, k2y, k2vx, k2vy;
    double k3x, k3y, k3vx, k3vy;
    double k4x, k4y, k4vx, k4vy;
    double k5x, k5y, k5vx, k5vy;
    double k6x, k6y, k6vx, k6vy;
};

struct Spring
{
    Spring() : ks(0), kd(0), restlen(0), m1(0), m2(0), status(S_ALIVE)
    {
    }
    /* Ks, Kd and rest length of spring */
    double ks, kd;
    double restlen;

    /* Connected to masses m1 and m2 */
    int m1, m2;

    int status;
};

class System
{
public:
    System();
    void deleteMass(int);
    void deleteSpring(int);
    void deleteSelected();
    void addMassParent(int, int);
    void deleteMassParent(int, int);
    void selectObject(int, bool, bool);
    void selectObjects(int, int, int, int);
    void unselectAll();
    void selectAll();
    void moveSelectedMasses(int, int);
    void setCenter();
    bool anythingSelected() const;
    int createMass();
    int createSpring();
    int nearestObject(int, int, bool*);
    void deleteAll();
    void reconnectMasses();
    void setRestLenth();
    bool evalSelection();
    void duplicateSelected();
    void restoreState();
    void saveState();
    void attachFakeSpring(int);
    void killFakeSpring();
    void moveFakeMass(int, int);
    void setMassVelocity(int, int, bool);
    void setTempFixed(bool);
    int massCount() const
    {
        return masses_.size();
    }
    Mass& getMass(int mass)
    {
        return masses_[mass];
    }
    const Mass& getMass(int mass) const
    {
        return masses_[mass];
    }
    int springCount() const
    {
        return springs_.size();
    }
    Spring& getSpring(int spring)
    {
        return springs_[spring];
    }
    const Spring& getSpring(int spring) const
    {
        return springs_[spring];
    }
    State& getState()
    {
        return state_;
    }
    const State& getState() const
    {
        return state_;
    }
    void reset();
    bool isFakeMass(int mass) const
    {
        return mass == fakeMass_;
    }
    bool isFakeSpring(int spring) const
    {
        return spring == fakeSpring_;
    }
private:
    void initObjects();
    State state_;
    std::vector<Mass> masses_;
    std::vector<Spring> springs_;
    int fakeMass_;
    int fakeSpring_;
};

#endif
