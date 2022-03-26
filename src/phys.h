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

#ifndef PHYS_H
#define PHYS_H

class System;

class Physics {
public:
    Physics(System&, int, int);
    bool advance();

private:
    void accumulateAccel();
    void rungeKutta(double, bool);
    void adaptiveRungeKutta();
    System& system_;
    int width_;
    int height_;
};

#endif // PHYS_H
