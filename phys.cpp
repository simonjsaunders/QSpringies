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

#include "phys.h"
#include "misc.h"
#include "system.h"
#include <math.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

const double DT_MIN = 0.0001;
const double DT_MAX = 0.5;

/* Stickiness calibration:  STICK_MAG = 1.0, means that a mass = 1.0 with gravity = 1.0 will remain
   stuck on a wall for all stickiness values > 1.0 */
const double STICK_MAG = 1.0;

Physics::Physics(System& s, int w, int h) : system_(s), width_(w), height_(h) {}

void Physics::accumulateAccel() {
    double gval, gmisc;
    double gx = 0, gy = 0, ogx = 0, ogy = 0;
    double center_x = width_/2.0, center_y = height_/2.0, center_rad = 1.0;
    int i;
    State& mst = system_.getState();
    
    /* ------------------ applied force effects ----------------------- */

    if (mst.center_id >= 0) {
        Mass& mass = system_.getMass(mst.center_id);
        if (mass.status & S_ALIVE) {
            center_x = mass.x;
            center_y = mass.y;
        } else {
            mst.center_id = -1;
        }
    }

    /* Do gravity */
    if (mst.force_enabled[FR_GRAV] > 0) {
        gval = mst.cur_grav_val[FR_GRAV];
        gmisc = mst.cur_misc_val[FR_GRAV];

        gx = COORD_DX(gval * sin(gmisc * M_PI / 180.0));
        gy = COORD_DY(gval * cos(gmisc * M_PI / 180.0));
    }
    
    /* Keep center of mass in the middle force */
    if (mst.force_enabled[FR_CMASS] > 0) {
        double mixix = 0.0, mixiy = 0.0, mivix = 0.0, miviy = 0.0, msum = 0.0;
        gval = mst.cur_grav_val[FR_CMASS];
        gmisc = mst.cur_misc_val[FR_CMASS];

        for (i = 0; i < system_.massCount(); i++) {
            Mass& m = system_.getMass(i);
            if (i != mst.center_id && (m.status & S_ALIVE) && !(m.status & S_FIXED)) {
                msum += m.mass;
                mixix += m.mass * m.x;
                mixiy += m.mass * m.y;
                mivix += m.mass * m.vx;
                miviy += m.mass * m.vy;
            }
        }

        if (msum) {
            mixix /= msum;
            mixiy /= msum;
            mivix /= msum;
            miviy /= msum;

            mixix -= center_x;
            mixiy -= center_y;

            ogx -= (gval * mixix + gmisc * mivix) / msum;
            ogy -= (gval * mixiy + gmisc * miviy) / msum;
        }
    }
    
    /* Apply Gravity, CM and air drag to all masses */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            /* Do viscous drag */
            if (i != mst.center_id) {
                m.ax = gx + ogx - mst.cur_visc * m.vx;
                m.ay = gy + ogy - mst.cur_visc * m.vy;
            } else {
                m.ax = gx - mst.cur_visc * m.vx;
                m.ay = gy - mst.cur_visc * m.vy;
            }
        }
    }
    
    /* Do point attraction force */
    if (mst.force_enabled[FR_PTATTRACT] > 0) {
        gval = mst.cur_grav_val[FR_PTATTRACT];
        gmisc = mst.cur_misc_val[FR_PTATTRACT];

        for (i = 0; i < system_.massCount(); i++) {
            Mass& m = system_.getMass(i);
            if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
                double dx, dy, mag, fmag;

                dx = (center_x - m.x);
                dy = (center_y - m.y);
                mag = sqrt(dx * dx + dy * dy);

                if (mag < m.radius + center_rad) {
                    dx *= mag / (m.radius + center_rad);
                    dy *= mag / (m.radius + center_rad);
                    mag = m.radius + center_rad;
                }

                fmag = gval / pow(mag, gmisc);

                m.ax += fmag * dx / mag;
                m.ay += fmag * dy / mag;
            }
        }
    }
    
    /* Wall attract/repel force */
    if (mst.force_enabled[FR_WALL] > 0) {
        double dax, day, dist;

        gval = -mst.cur_grav_val[FR_WALL];
        gmisc = mst.cur_misc_val[FR_WALL];

        for (i = 0; i < system_.massCount(); i++) {
            Mass& m = system_.getMass(i);
            int rad = screenRadius(m.radius);
            if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
                dax = day = 0;

                if (mst.w_left && (dist = m.x - rad) >= 0) {
                    if (dist < 1)  dist = 1;
                    dist = pow(dist, gmisc);
                    dax -= gval / dist;
                }
                if (mst.w_right && (dist = width_ - rad - m.x) >= 0) {
                    if (dist < 1)  dist = 1;
                    dist = pow(dist, gmisc);
                    dax += gval / dist;
                }
                if (mst.w_top && (dist = height_ - rad - m.y) >= 0) {
                    if (dist < 1)  dist = 1;
                    dist = pow(dist, gmisc);
                    day += gval / dist;
                }
                if (mst.w_bottom && (dist = m.y - rad) >= 0) {
                    if (dist < 1)  dist = 1;
                    dist = pow(dist, gmisc);
                    day -= gval / dist;
                }

                m.ax += dax;
                m.ay += day;
            }
        }
    }
    
    /* ------------------ spring effects ----------------------- */
    
    /* Spring compression/damping effects on masses */
    for (i = 0; i < system_.springCount(); i++) {
        Spring& s = system_.getSpring(i);
        if (s.status & S_ALIVE) {
            double dx, dy, force, forcex, forcey, mag, damp, mass1, mass2;

            Mass& m1 = system_.getMass(s.m1);
            Mass& m2 = system_.getMass(s.m2);

            dx = m1.x - m2.x;
            dy = m1.y - m2.y;

            if (dx || dy) {
                mag = sqrt(dx * dx + dy * dy);

                force = s.ks * (s.restlen - mag);
                if (s.kd)  {
                    damp = ((m1.vx - m2.vx) * dx + (m1.vy - m2.vy) * dy) / mag;
                    force -= s.kd * damp;
                }

                force /= mag;
                forcex = force * dx;
                forcey = force * dy;

                mass1 = m1.mass;
                mass2 = m2.mass;

                m1.ax += forcex / mass1;
                m1.ay += forcey / mass1;
                m2.ax -= forcex / mass2;
                m2.ay -= forcey / mass2;
            }
        }
    }
}

void Physics::rungeKutta(double h, bool testloc) {
    int i;

    accumulateAccel();

    /* k1 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);

        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            /* Initial storage */
            m.cur_x = m.x;
            m.cur_y = m.y;
            m.cur_vx = m.vx;
            m.cur_vy = m.vy;

            m.k1x = m.vx * h;
            m.k1y = m.vy * h;
            m.k1vx = m.ax * h;
            m.k1vy = m.ay * h;

            m.x = m.cur_x + m.k1x / 2;
            m.y = m.cur_y + m.k1y / 2;
            m.vx = m.cur_vx + m.k1vx / 2;
            m.vy = m.cur_vy + m.k1vy / 2;
        }
    }

    accumulateAccel();

    /* k2 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k2x = m.vx * h;
            m.k2y = m.vy * h;
            m.k2vx = m.ax * h;
            m.k2vy = m.ay * h;

            m.x = m.cur_x + m.k2x / 2;
            m.y = m.cur_y + m.k2y / 2;
            m.vx = m.cur_vx + m.k2vx / 2;
            m.vy = m.cur_vy + m.k2vy / 2;
        }
    }

    accumulateAccel();

    /* k3 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k3x = m.vx * h;
            m.k3y = m.vy * h;
            m.k3vx = m.ax * h;
            m.k3vy = m.ay * h;

            m.x = m.cur_x + m.k3x;
            m.y = m.cur_y + m.k3y;
            m.vx = m.cur_vx + m.k3vx;
            m.vy = m.cur_vy + m.k3vy;
        }
    }

    accumulateAccel();

    /* k4 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k4x = m.vx * h;
            m.k4y = m.vy * h;
            m.k4vx = m.ax * h;
            m.k4vy = m.ay * h;
        }
    }

    /* Find next position */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            if (testloc) {
                m.test_x = m.cur_x + (m.k1x/2.0 + m.k2x + m.k3x + m.k4x/2.0)/3.0;
                m.test_y = m.cur_y + (m.k1y/2.0 + m.k2y + m.k3y + m.k4y/2.0)/3.0;
                m.test_vx = m.cur_vx + (m.k1vx/2.0 + m.k2vx + m.k3vx + m.k4vx/2.0)/3.0;
                m.test_vy = m.cur_vy + (m.k1vy/2.0 + m.k2vy + m.k3vy + m.k4vy/2.0)/3.0;
            } else {
                m.x = m.cur_x + (m.k1x/2.0 + m.k2x + m.k3x + m.k4x/2.0)/3.0;
                m.y = m.cur_y + (m.k1y/2.0 + m.k2y + m.k3y + m.k4y/2.0)/3.0;
                m.vx = m.cur_vx + (m.k1vx/2.0 + m.k2vx + m.k3vx + m.k4vx/2.0)/3.0;
                m.vy = m.cur_vy + (m.k1vy/2.0 + m.k2vy + m.k3vy + m.k4vy/2.0)/3.0;
            }
        }
    }
}

/* ---------------------
   RKF45 method (upgraded from RK4) thanks to
   Nate Loofbourrow (loofbour@cis.ohio-state.edu)
   -------------------- */
/* Adapted from Numerical Recipes (2nd ed.) pp. 719-720 (RKF45 method). -njl */
void Physics::adaptiveRungeKutta() {
    int i;
    double err, errx, erry, errvx, errvy, maxerr, h;
    State& mst = system_.getState();

restart:
    if (mst.cur_dt > DT_MAX)
        mst.cur_dt = DT_MAX;
    if (mst.cur_dt < DT_MIN)
        mst.cur_dt = DT_MIN;

    h = mst.cur_dt;
    
    accumulateAccel();

    /* k1 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);

        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            /* Initial storage */
            m.cur_x = m.x;
            m.cur_y = m.y;
            m.cur_vx = m.vx;
            m.cur_vy = m.vy;

            m.k1x = m.vx * h;
            m.k1y = m.vy * h;
            m.k1vx = m.ax * h;
            m.k1vy = m.ay * h;

            m.x = m.cur_x + (0.2) * m.k1x;
            m.y = m.cur_y + (0.2) * m.k1y;
            m.vx = m.cur_vx + (0.2) * m.k1vx;
            m.vy = m.cur_vy + (0.2) * m.k1vy;
        }
    }

    accumulateAccel();

    /* k2 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k2x = m.vx * h;
            m.k2y = m.vy * h;
            m.k2vx = m.ax * h;
            m.k2vy = m.ay * h;

            m.x = m.cur_x + (3.0/40.0) * m.k1x + (9.0/40.0) * m.k2x;
            m.y = m.cur_y + (3.0/40.0) * m.k1y + (9.0/40.0) * m.k2y;
            m.vx = m.cur_vx + (3.0/40.0) * m.k1vx + (9.0/40.0) * m.k2vx;
            m.vy = m.cur_vy + (3.0/40.0) * m.k1vy + (9.0/40.0) * m.k2vy;
        }
    }

    accumulateAccel();

    /* k3 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k3x = m.vx * h;
            m.k3y = m.vy * h;
            m.k3vx = m.ax * h;
            m.k3vy = m.ay * h;

            m.x = m.cur_x + (0.3)*m.k1x + (-0.9)*m.k2x + (1.2)*m.k3x;
            m.y = m.cur_y + (0.3)*m.k1y + (-0.9)*m.k2y + (1.2)*m.k3y;
            m.vx = m.cur_vx + (0.3)*m.k1vx + (-0.9)*m.k2vx + (1.2)*m.k3vx;
            m.vy = m.cur_vy + (0.3)*m.k1vy + (-0.9)*m.k2vy + (1.2)*m.k3vy;
        }
    }

    accumulateAccel();

    /* k4 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k4x = m.vx * h;
            m.k4y = m.vy * h;
            m.k4vx = m.ax * h;
            m.k4vy = m.ay * h;

            m.x = m.cur_x + (-11.0/54.0)*m.k1x + (2.5)*m.k2x
                    + (-70.0/27.0)*m.k3x + (35.0/27.0)*m.k4x;
            m.y = m.cur_y + (-11.0/54.0)*m.k1y + (2.5)*m.k2y
                    + (-70.0/27.0)*m.k3y + (35.0/27.0)*m.k4y;
            m.vx = m.cur_vx + (-11.0/54.0)*m.k1vx + (2.5)*m.k2vx
                    + (-70.0/27.0)*m.k3vx + (35.0/27.0)*m.k4vx;
            m.vy = m.cur_vy + (-11.0/54.0)*m.k1vy + (2.5)*m.k2vy
                    + (-70.0/27.0)*m.k3vy + (35.0/27.0)*m.k4vy;
        }
    }

    accumulateAccel();

    /* k5 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k5x = m.vx * h;
            m.k5y = m.vy * h;
            m.k5vx = m.ax * h;
            m.k5vy = m.ay * h;

            m.x = m.cur_x + (1631.0/55926.0)*m.k1x + (175.0/512.0)*m.k2x
                    + (575.0/13824.0)*m.k3x + (44275.0/110592.0)*m.k4x
                    + (253.0/4096.0)*m.k5x;
            m.y = m.cur_y + (1631.0/55926.0)*m.k1y + (175.0/512.0)*m.k2y
                    + (575.0/13824.0)*m.k3y + (44275.0/110592.0)*m.k4y
                    + (253.0/4096.0)*m.k5y;
            m.vx= m.cur_vx + (1631.0/55926.0)*m.k1vx + (175.0/512.0)*m.k2vx
                    + (575.0/13824.0)*m.k3vx + (44275.0/110592.0)*m.k4vx
                    + (253.0/4096.0)*m.k5vx;
            m.vy= m.cur_vy + (1631.0/55926.0)*m.k1vy + (175.0/512.0)*m.k2vy
                    + (575.0/13824.0)*m.k3vy + (44275.0/110592.0)*m.k4vy
                    + (253.0/4096.0)*m.k5vy;
        }
    }

    accumulateAccel();

    /* k6 step */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.k6x = m.vx * h;
            m.k6y = m.vy * h;
            m.k6vx = m.ax * h;
            m.k6vy = m.ay * h;
        }
    }

    /* Find error */
    maxerr = 0.00001;
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            errx = ((37.0/378.0)-(2825.0/27648.0))*m.k1x +
                    ((250.0/621.0)-(18575.0/48384.0))*m.k3x +
                    ((125.0/594.0)-(13525.0/55296.0))*m.k4x +
                    (-277.0/14336.0)*m.k5x + ((512.0/1771.0)-(0.25))*m.k6x;
            erry = ((37.0/378.0)-(2825.0/27648.0))*m.k1y +
                    ((250.0/621.0)-(18575.0/48384.0))*m.k3y +
                    ((125.0/594.0)-(13525.0/55296.0))*m.k4y +
                    (-277.0/14336.0)*m.k5y + ((512.0/1771.0)-(0.25))*m.k6y;
            errvx = ((37.0/378.0)-(2825.0/27648.0))*m.k1vx +
                    ((250.0/621.0)-(18575.0/48384.0))*m.k3vx +
                    ((125.0/594.0)-(13525.0/55296.0))*m.k4vx +
                    (-277.0/14336.0)*m.k5vx + ((512.0/1771.0)-(0.25))*m.k6vx;
            errvy = ((37.0/378.0)-(2825.0/27648.0))*m.k1vy +
                    ((250.0/621.0)-(18575.0/48384.0))*m.k3vy +
                    ((125.0/594.0)-(13525.0/55296.0))*m.k4vy +
                    (-277.0/14336.0)*m.k5vy + ((512.0/1771.0)-(0.25))*m.k6vy;
            err = fabs(errx) + fabs(erry) + fabs(errvx) + fabs(errvy);

            if (err > maxerr)
                maxerr = err;
        }
    }

    /* Fudgy scale factor -- user controlled */
    maxerr /= mst.cur_prec;

    if (maxerr < 1.0) {
        mst.cur_dt *= 0.9 * exp(-log(maxerr)/8.0);
    } else {
        if (mst.cur_dt > DT_MIN) {
            for (i = 0; i < system_.massCount(); i++) {
                Mass& m = system_.getMass(i);
                if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
                    m.x = m.old_x;
                    m.y = m.old_y;
                    m.vx = m.old_vx;
                    m.vy = m.old_vy;
                }
            }

            mst.cur_dt *= 0.9 * exp(-log(maxerr)/4.0);

            goto restart;
        }
    }

    /* Find next position */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.x=m.cur_x + (37.0/378.0)*m.k1x + (250.0/621.0)*m.k3x
                    + (125.0/594.0)*m.k4x + (512.0/1771.0)*m.k6x;
            m.y=m.cur_y + (37.0/378.0)*m.k1y + (250.0/621.0)*m.k3y
                    + (125.0/594.0)*m.k4y + (512.0/1771.0)*m.k6y;
            m.vx=m.cur_vx + (37.0/378.0)*m.k1vx
                    + (250.0/621.0)*m.k3vx + (125.0/594.0)*m.k4vx
                    + (512.0/1771.0)*m.k6vx;
            m.vy=m.cur_vy + (37.0/378.0)*m.k1vy
                    + (250.0/621.0)*m.k3vy + (125.0/594.0)*m.k4vy
                    + (512.0/1771.0)*m.k6vy;
        }
    }
}

bool Physics::advance() {
    int i;
    double stick_mag;
    static int num_since = 0;
    static double time_elapsed = 0.0;
    State& mst = system_.getState();

    /* Save initial values */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            m.old_x = m.x;
            m.old_y = m.y;
            m.old_vx = m.vx;
            m.old_vy = m.vy;
        }
    }

    if (mst.adaptive_step) {
        bool any_spring = false;

        for (i = 0; i < system_.springCount(); i++) {
            Spring& s = system_.getSpring(i);
            if (s.status & S_ALIVE) {
                any_spring = true;
                break;
            }
        }

        /* If no springs, then use dt=DEF_TSTEP */
        if (any_spring)
            /* check mst.stiff */
            adaptiveRungeKutta();
        else
            rungeKutta(mst.cur_dt = DEF_TSTEP, false);
    } else {
        rungeKutta(mst.cur_dt, false);
    }

    stick_mag = STICK_MAG * mst.cur_dt * mst.cur_stick;

    /* Crappy wall code */
    for (i = 0; i < system_.massCount(); i++) {
        Mass& m = system_.getMass(i);
        int rad = screenRadius(m.radius);

        if ((m.status & S_ALIVE) && !(m.status & S_FIXED)) {
            /* Delete "exploded" objects */
            if (m.ax - m.ax != 0.0 || m.ay - m.ay != 0.0
                    || m.x - m.x != 0.0 || m.y - m.y != 0.0) {
                system_.deleteMass(i);
                continue;
            }

            /* Check if stuck to a wall */
            if (m.old_vx == 0.0 && m.old_vy == 0.0) {
                /* Left or right wall */
                if ((mst.w_left && fabs(m.old_x - rad) < 0.5)
                        || (mst.w_right && fabs(m.old_x - width_ + rad) < 0.5)) {
                    if (fabs(m.vx) < stick_mag / m.mass) {
                        m.vx = m.vy = 0;
                        m.x = m.old_x;
                        m.y = m.old_y;

                        continue;
                    }
                } else if ((mst.w_bottom && fabs(m.old_y - rad) < 0.5)
                           || (mst.w_top && fabs(m.old_y - height_ + rad) < 0.5)) {
                    /* Top or bottom wall */
                    if (fabs(m.vy) < stick_mag / m.mass) {
                        m.vx = m.vy = 0;
                        m.x = m.old_x;
                        m.y = m.old_y;

                        continue;
                    }
                }
            }

            /* Bounce off left or right wall */
            if (mst.w_left && m.x < rad && m.old_x >= rad) {
                m.x = rad;

                if (m.vx < 0) {
                    m.vx = -m.vx * m.elastic;
                    m.vy *= m.elastic;

                    /* Get stuck if not going fast enough */
                    if (m.vx > 0) {
                        m.vx -= STICK_MAG * mst.cur_stick / m.mass;

                        if (m.vx < 0)
                            m.vx = m.vy = 0;
                    }
                }
            } else if (mst.w_right && m.x > width_ - rad && m.old_x <= width_ - rad) {
                m.x = width_ - rad;

                if (m.vx > 0) {
                    m.vx = -m.vx * m.elastic;
                    m.vy *= m.elastic;

                    /* Get stuck if not going fast enough */
                    if (m.vx < 0) {
                        m.vx += STICK_MAG * mst.cur_stick / m.mass;

                        if (m.vx > 0)
                            m.vx = m.vy = 0;
                    }
                }
            }
            /* Stick to top or bottom wall */
            if (mst.w_bottom && m.y < rad && m.old_y >= rad) {
                m.y = rad;

                if (m.vy < 0) {
                    m.vy = -m.vy * m.elastic;
                    m.vx *= m.elastic;

                    /* Get stuck if not going fast enough */
                    if (m.vy > 0) {
                        m.vy -= STICK_MAG * mst.cur_stick / m.mass;

                        if (m.vy < 0)
                            m.vx = m.vy = 0;
                    }
                }
            } else if (mst.w_top && m.y > (height_ - rad) && m.old_y <= (height_ - rad)) {
                m.y = height_ - rad;

                if (m.vy > 0) {
                    m.vy = -m.vy * m.elastic;
                    m.vx *= m.elastic;

                    /* Get stuck if not going fast enough */
                    if (m.vy < 0) {
                        m.vy += STICK_MAG * mst.cur_stick / m.mass;

                        if (m.vy > 0)
                            m.vx = m.vy = 0;
                    }
                }
            }
        }
    }

    /* Oblique impact */
    /* added by Martin Lukanowicz march 93 luky@ihssv.wsr.ac.at  */
    /* - changed a little by dmd */
    if (mst.collide > 0) {
        for (i = 0; i < system_.massCount(); i++) {
            int j;
            Mass& m1 = system_.getMass(i);

            if (m1.status & S_ALIVE) {
                double m1radius, m2radius, ratio;
                double dx, dy, dxq, dyq, sumxyq, m1x, m1y, mag;
                double m1vx, m1vy;
                double m2vx, m2vy;

                m1radius = (m1.status & S_FIXED) ? NAIL_SIZE : m1.radius;
                m1x = m1.x;
                m1y = m1.y;

                for (j = i+1; j < system_.massCount(); j++) {
                    Mass& m2 = system_.getMass(j);

                    if (m2.status & S_ALIVE) {
                        m2radius = (m2.status & S_FIXED) ? NAIL_SIZE : m2.radius;

                        dx = m2.x - m1x;
                        dy = m2.y - m1y;
                        dxq = dx*dx;
                        dyq = dy*dy;
                        sumxyq = dxq+dyq;
                        mag = sqrt(sumxyq);

                        if (mag < m1radius + m2radius) {
                            m1vx = m1.vx;
                            m1vy = m1.vy;
                            m2vx = m2.vx;
                            m2vy = m2.vy;

                            if ((m1vx-m2vx)*(dx)>0 || (m1vy-m2vy)*(dy)>0) {
                                if (dx == 0) dx = 1e-10;

                                if (!(m1.status & S_FIXED)) {
                                    if (m2.status & S_FIXED)
                                        ratio = 1+(m1.elastic+m2.elastic)/2;
                                    else
                                        ratio = (1+(m1.elastic+m2.elastic)/2)/
                                                (1+m1.mass/m2.mass);

                                    m1.vx = (m1vx - (m1vx - m2vx)*ratio)*(dxq/sumxyq) +
                                            m1vx*(dyq/sumxyq) -
                                            (m1vy - m2vy)*ratio*(dx*dy/sumxyq);
                                    m1.vy = (m1.vx - m1vx)*(dy/dx) + m1vy;
                                }


                                if (!(m2.status & S_FIXED)) {
                                    if (m1.status & S_FIXED)
                                        ratio = 1+(m1.elastic+m2.elastic)/2;
                                    else
                                        ratio = (1+(m1.elastic+m2.elastic)/2)/
                                                (1+m2.mass/m1.mass);

                                    m2.vx = (m2vx - (m2vx - m1vx)*ratio)*(dxq/sumxyq) +
                                            m2vx*(dyq/sumxyq) -
                                            (m2vy - m1vy)*ratio*(dx*dy/sumxyq);
                                    m2.vy = (m2.vx - m2vx)*(dy/dx) + m2vy;
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    time_elapsed += mst.cur_dt;

    if (time_elapsed > 0.05) {
        time_elapsed -= 0.05;
        num_since = 0;
        return true;
    }

    num_since++;

    if (num_since > 8) {
        num_since = 0;
        return true;
    }
    return false;
}
