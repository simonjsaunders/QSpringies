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

#include "file.h"
#include "misc.h"
#include "system.h"
#include <QFile>
#include <QtGlobal>
#include <QTextStream>

#define MAGIC_CMD	"#1.0"
#define FILE_EXT	".xsp"

QString extendFile(const QString& file)
{
    QString result = file;
    if (!result.endsWith(FILE_EXT))
        result += FILE_EXT;
    return result;
}

bool fileLoad(const QString& fileName, FileCmd command, System& system)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);

    // First line must start with MAGIC_CMD
    QString line = in.readLine();
    if (line.isNull() || !line.startsWith(MAGIC_CMD))
        return false;

    State& state = system.getState();
    bool selectNew = false;
    if (command == FileLoad)
        system.reset();
    else if (!system.anythingSelected())
        selectNew = true;

    int spring_start = system.springCount();
    int num_map = 0;
    std::vector<int> mapfrom;
    std::vector<int> mapto;

    while (!in.atEnd())
    {
        QString cmd;
        in >> cmd;
        if (in.atEnd())
            break;
        if (cmd == "mass")
        {
            int which = system.createMass();
            mapto.push_back(which);
            Mass& mass = system.getMass(which);
            int mass_num;
            in >> mass_num >> mass.x >> mass.y >> mass.vx >> mass.vy >> mass.mass >> mass.elastic;
            mapfrom.push_back(mass_num);
            num_map++;
            if (mass.mass < 0)
            {
                mass.mass = -mass.mass;
                mass.status |= S_FIXED;
            }
            if (mass.mass == 0)
                mass.mass = 1.0;

            mass.radius = massRadius(mass.mass);
            if (selectNew)
                system.selectObject(which, true, false);
        }
        else if (cmd == "spng")
        {
            int bogus;
            int which = system.createSpring();
            Spring& spring = system.getSpring(which);
            in >> bogus >> spring.m1 >> spring.m2 >> spring.ks >> spring.kd >> spring.restlen;
            if (selectNew)
                system.selectObject(which, false, false);
        }
        else if (command == FileInsert)
        {
            /* skip non mass/spring commands if in insert mode */
        }
        else if (cmd == "cmas")
            in >> state.cur_mass;
        else if (cmd == "elas")
            in >> state.cur_rest;
        else if (cmd == "kspr")
            in >> state.cur_ks;
        else if (cmd == "kdmp")
            in >> state.cur_kd;
        else if (cmd == "fixm")
        {
            int temp;
            in >> temp;
            state.fix_mass = temp != 0;
        }
        else if (cmd == "shws")
        {
            int temp;
            in >> temp;
            state.show_spring = temp != 0;
        }
        else if (cmd == "cent")
            in >> state.center_id;
        else if (cmd == "frce")
        {
            int which, temp;
            in >> which;
            if (which >= 0 && which < BF_NUM)
            {
                in >> temp >> state.cur_grav_val[which] >> state.cur_misc_val[which];
                state.force_enabled[which] = temp != 0;
            }
            else if (which == BF_NUM)
            {
                in >> temp;
                state.collide = temp != 0;
                in.readLineInto(0);
            }
        }
        else if (cmd == "visc")
            in >> state.cur_visc;
        else if (cmd == "stck")
            in >> state.cur_stick;
        else if (cmd == "step")
            in >> state.cur_dt;
        else if (cmd == "prec")
            in >> state.cur_prec;
        else if (cmd == "adpt")
        {
            int temp;
            in >> temp;
            state.adaptive_step = temp != 0;
        }
        else if (cmd == "gsnp")
        {
            int temp;
            in >> state.cur_gsnap >> temp;
            state.grid_snap = temp  != 0;
        }
        else if (cmd == "wall")
        {
            int wt, wl, wr, wb;
            in >> wt >> wl >> wr >> wb;
            state.w_top = wt != 0;
            state.w_left = wl != 0;
            state.w_right = wr != 0;
            state.w_bottom = wb != 0;
        }
        else
        {
            qWarning("Unknown command: %s\n", qUtf8Printable(cmd));
        }
    }

    /* Connect springs to masses */
    for (int i = spring_start; i < system.springCount(); i++)
    {
        bool m1done = false, m2done = false;

        if (system.isFakeSpring(i))
            continue;

        Spring& spring = system.getSpring(i);
        for (int j = 0; (!m1done || !m2done) && j < num_map; j++)
        {
            if (!m1done && spring.m1 == mapfrom[j])
            {
                spring.m1 = mapto[j];
                m1done = true;
            }
            if (!m2done && spring.m2 == mapfrom[j])
            {
                spring.m2 = mapto[j];
                m2done = true;
            }
        }
        if (!m1done && !m2done)
        {
            /* delete spring */
            system.deleteSpring(i);
            fprintf(stderr, "Spring %d not connected to existing mass\n", i);
        }
    }
    system.reconnectMasses();
    return true;
}

bool fileSave(const QString& fileName, System& system)
{
    State& state = system.getState();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out.setRealNumberPrecision(12);

    out << MAGIC_CMD << " *** XSpringies data file\n";
    /* Settings */
    out << "cmas " << state.cur_mass << '\n';
    out << "elas " << state.cur_rest << '\n';
    out << "kspr " << state.cur_ks << '\n';
    out << "kdmp " << state.cur_kd << '\n';
    out << "fixm " << (state.fix_mass ? 1 : 0) << '\n';
    out << "shws " << (state.show_spring ? 1 : 0) << '\n';
    out << "cent " << state.center_id << '\n';

    for (int i = 0; i < BF_NUM; i++)
    {
        out << "frce " << i << ' ' << (state.force_enabled[i] ? 1 : 0)
            << ' ' << state.cur_grav_val[i] << ' ' << state.cur_misc_val[i] << '\n';
    }

    out << "frce " << BF_NUM << ' ' << (state.collide ? 1 : 0) << " 0 0\n";
    out << "visc " << state.cur_visc << '\n';
    out << "stck " << state.cur_stick << '\n';
    out << "step " << state.cur_dt << '\n';
    out << "prec " << state.cur_prec << '\n';
    out << "adpt " << state.adaptive_step << '\n';
    out << "gsnp " << state.cur_gsnap << ' ' << (state.grid_snap ? 1 : 0) << '\n';
    out << "wall " << (int)state.w_top << ' ' << (int)state.w_left << ' '
        << (int)state.w_right << ' ' << (int)state.w_bottom << '\n';

    /* Masses and springs */
    for (int i = 0; i < system.massCount(); i++)
    {
        Mass& mass = system.getMass(i);
        if (mass.status & S_ALIVE)
        {
            out << "mass " << i << ' ' << mass.x << ' ' << mass.y
                << ' ' << mass.vx << ' ' << mass.vy << ' '
                << (mass.status & S_FIXED ? -mass.mass : mass.mass)
                << ' ' << mass.elastic << '\n';
        }
    }
    for (int i = 0; i < system.springCount(); i++)
    {
        Spring& spring = system.getSpring(i);
        if (spring.status & S_ALIVE)
        {
            out << "spng " << i << ' ' << spring.m1 << ' ' << spring.m2 << ' '
                << spring.ks << ' ' << spring.kd << ' ' << spring.restlen << '\n';
        }
    }
    return true;
}

bool fileCommand(const QString& fileName, FileCmd command, System& system)
{
    if (command == FileLoad || command == FileInsert)
        return fileLoad(extendFile(fileName), command, system);
    if (command == FileSave)
        return fileSave(extendFile(fileName), system);
    return false;
}
