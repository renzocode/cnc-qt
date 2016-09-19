/****************************************************************************
 * Main developer, C# developing:                                           *
 * Copyright (C) 2014-2016 by Sergey Zheigurov                              *
 * Russia, Novy Urengoy                                                     *
 * zheigurov@gmail.com                                                      *
 *                                                                          *
 * C# to Qt portation, Linux developing                                     *
 * Copyright (C) 2015-2016 by Eduard Kalinowski                             *
 * Germany, Lower Saxony, Hanover                                           *
 * eduard_kalinowski@yahoo.de                                               *
 *                                                                          *
 * C# project CNC-controller-for-mk1                                        *
 * https://github.com/selenur/CNC-controller-for-mk1                        *
 *                                                                          *
 * The Qt project                                                           *
 * https://github.com/eduard-x/cnc-qt                                       *
 *                                                                          *
 * CNC-Qt is free software; may be distributed and/or modified under the    *
 * terms of the GNU General Public License version 3 as published by the    *
 * Free Software Foundation and appearing in the file LICENSE_GPLv3         *
 * included in the packaging of this file.                                  *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU Lesser General Public         *
 * License along with CNC-Qt. If not, see  http://www.gnu.org/licenses      *
 ****************************************************************************/


#include <QObject>
#include <QRegExp>
#include <QDebug>
#include <QTime>
#include <QString>
// #include <QStringData>

#include <cmath>
#include <limits>

#include "includes/Settings.h"
#include "includes/GCode.h"
#include "includes/MainWindow.h"


#define DEBUG_ARC 0

/**
 * @brief consructor
 *
 */
GCodeData::GCodeData()
{
    changeInstrument = false;
    numberInstrument = 0;
    numberInstruction = 0;
    numberLine = 0;
    /** @var pauseMSeconds
     * no pause: -1
     * waiting: 0
     * pause > 0 in milliseconds
     */
    pauseMSeconds = -1;

    X = 0.0;
    Y = 0.0;
    Z = 0.0;
    A = 0.0;

    // arc parameters
    I = 0.0;
    J = 0.0;
    K = 0.0;

    plane = None;

    Radius = 0.0;
    vectorCoeff = 0.0;
    // end of arc

    typeMoving = NoType;

    movingCode =  NO_CODE;

    vectSpeed = 0.0;

    stepsCounter = 0;

    angle = 0.0;
    deltaAngle = 0.0;

    spindelON = false;
    splits = 0; // init

    diametr = 0.0;
};


/**
 * @brief constructor based on command
 *
 */
GCodeData::GCodeData(GCodeData *d)
{
    X = d->X;
    Y = d->Y;
    Z = d->Z;
    A = d->A;

    I = d->I;
    J = d->J;
    K = d->K;

    Radius = d->Radius;

    plane = d->plane;

    vectorCoeff = 0.0;

    typeMoving = d->typeMoving;

    spindelON = d->spindelON;
    vectSpeed = d->vectSpeed;

    splits = 0; // if arc, will be splitted, debug information only
    stepsCounter = 0; // should be calculated

    movingCode = NO_CODE;

    numberLine = d->numberLine;
    numberInstruction = 0;

    angle = 0.0;//d->angleVectors;

    deltaAngle = 0.0;

    changeInstrument = d->changeInstrument;
    numberInstrument = d->numberInstrument;
    pauseMSeconds = d->pauseMSeconds;
    diametr = d->diametr;
};


/**
 * @brief constructor
 *
 */
GCodeParser::GCodeParser()
{

}


/**
 * @brief
 *
 */
bool GCodeParser::addLine(GCodeData *c)
{
}


/**
 * @brief
 *
 */
bool GCodeParser::addArc(GCodeData *c)
{
}


/**
 * @brief read and parse into GCodeData list and OpenGL list
 * @see for the optimizations see https://blog.qt.io/blog/2014/06/13/qt-weekly-13-qstringliteral/
 * TODO convert QString to QStringLiteral
 *
 */
bool GCodeParser::readGCode(const QByteArray &gcode)
{
    //  QMutexLocker mLock(&mutex);
    gCodeList.clear();

    //  lock();

    goodList.clear();
//     badList.clear();

    //  unlock();

    QTextStream stream(gcode);
    stream.setLocale(QLocale("C"));
    // or this ? QString.split(QRegExp("\n|\r\n|\r"));
    // if we switch the input methode from QTextStream to QStringList, performance is about 15% higher

    QVector3D origin(0, 0, 0);
    QVector3D current_pos(0, 0, 0);
    bool b_absolute = true;
    float coef = 1.0; // 1 or 24.5

    QTime t;
    t.start();

    bool decoded;

    QVector<QString> gCodeLines;
    QString lastCmd;
    int lineNr = 0;
    QString lastCommand;
    QString paramX, paramY, paramZ, paramA, paramF;

    while(!stream.atEnd()) { // restruct lines
        QString lineStream = stream.readLine().toUpper();
        lineNr++;

        // ignore commentars
        if (lineStream.isEmpty()/* || lineStream.at(0) == ';' || lineStream.at(0) == '(' */ || lineStream.at(0) == '%') {
            continue;
        }

        int posComment = lineStream.indexOf(";");

        if (posComment >= 0) {
            lineStream = lineStream.mid(posComment);

            if (lineStream.isEmpty()) {
                continue;
            }
        }

        // this is commentar too : ( ... )
        int commentBeg = lineStream.indexOf('(');
        int commentEnd = -1;

        if (commentBeg >= 0) {
            commentEnd = lineStream.lastIndexOf(')');

            if (commentEnd > commentBeg) {
                lineStream = lineStream.remove(commentBeg, commentEnd - commentBeg + 1);
            }
        }

        if (lineStream.length() == 0) {
            continue;
        }

        QRegExp rx("([A-Z])((\\-)?(\\+)?\\d+(\\.\\d+)?)");
        int pos = 0;
        QString tmpStr;

        if (Settings::filterRepeat == true) {
            while ((pos = rx.indexIn(lineStream, pos)) != -1) {
                QString currentText = rx.cap(0);
                QChar c = currentText.at(0);

                if (c == 'N') { // ignore line number
                    pos += rx.matchedLength();
                    continue;
                }

                if (pos == 0) {
                    if (currentText == "G1" || currentText == "G01") {
                        lastCommand = currentText;
                    } else {
                        lastCommand = "";
                        paramX = "";
                        paramY = "";
                        paramZ = "";
                        paramA = "";
                        paramF = "";
                    }

                    pos += rx.matchedLength();
                } else {
                    // when last command exists
                    pos += rx.matchedLength();

                    if (lastCommand.length() > 0) {
                        if (c == 'X') {
                            if (currentText == paramX) {
                                continue;
                            } else {
                                paramX = currentText;
                            }
                        }

                        if (c == 'Y') {
                            if (currentText == paramY) {
                                continue;
                            } else {
                                paramY = currentText;
                            }
                        }

                        if (c == 'Z') {
                            if (currentText == paramZ) {
                                continue;
                            } else {
                                paramZ = currentText;
                            }
                        }

                        if (c == 'A') {
                            if (currentText == paramA) {
                                continue;
                            } else {
                                paramA = currentText;
                            }
                        }

                        if (c == 'F') {
                            if (currentText == paramF) {
                                continue;
                            } else {
                                paramF = currentText;
                            }
                        }
                    }
                }

                tmpStr += currentText;
                tmpStr += " ";
            }
        } else {
            while ((pos = rx.indexIn(lineStream, pos)) != -1) {
                QChar c = rx.cap(0).at(0);

                if (c == 'N') { // ignore line number
                    continue;
                }

                tmpStr += rx.cap(0);
                tmpStr += " ";
                pos += rx.matchedLength();
            }
        }

        if (tmpStr.length() == 0) {
            emit logMessage(QString("gcode parsing error: " + lineStream));
//             badList << lineStream;
            continue;
        }

        QChar c = tmpStr.at(0);

        if (!(c == 'G' || c == 'M' || c == 'F')) {
            if (lastCmd.length() > 0) {
                tmpStr = QString(lastCmd + " " + tmpStr);
            } else {
                emit logMessage(QString("gcode parsing error: " + lineStream));
//                 badList << QString::number(lineNr - 1) + ": " + lineStream;
            }
        } else {
            int posSpace = tmpStr.indexOf(" ");

            if (posSpace > 0) { // command with parameter
                if (posSpace == 2) { // insert '0' if two characters
                    tmpStr.insert(1, "0");
                    posSpace++;
                }

                lastCmd = tmpStr.left(posSpace);

            } else { // command without parameter
                if (tmpStr.length() == 2) { // insert '0' if two characters
                    tmpStr.insert(1, "0");
                    posSpace++;
                }

                lastCmd = tmpStr;
            }
        }

        gCodeLines << tmpStr;
    }

    emit logMessage(QString().sprintf("Read gcode, loaded. Time elapsed: %d ms", t.elapsed()));

    t.restart();

    int index = 0;
    PlaneEnum currentPlane;
    currentPlane = XY;

    Settings::coord[X].softLimitMax = 0;
    Settings::coord[X].softLimitMin = 0;
    Settings::coord[Y].softLimitMax = 0;
    Settings::coord[Y].softLimitMin = 0;
    Settings::coord[Z].softLimitMax = 0;
    Settings::coord[Z].softLimitMin = 0;

    GCodeData *tmpCommand = new GCodeData();

    foreach(QString line, gCodeLines) {
        decoded = true;
        QVector<QStringRef> vctRO = line.simplified().splitRef(" ", QString::SkipEmptyParts);
        QString cmd = vctRO.at(0).toString();

        if (cmd.isEmpty()) {
            continue;
        }

        switch(cmd.at(0).toLatin1()) {
            case 'G': {
                if (cmd == "G00") { // eilgang
                    QVector3D next_pos(b_absolute ? current_pos - origin : QVector3D(0, 0, 0));
                    float E;

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->X = next_pos.x();
                    tmpCommand->Y = next_pos.y();
                    tmpCommand->Z = next_pos.z();

                    detectMinMax(tmpCommand);

                    tmpCommand->splits = 0;

                    tmpCommand->typeMoving = GCodeData::Line;

                    tmpCommand->movingCode = RAPID_LINE_CODE;
                    tmpCommand->plane = currentPlane;

                    if (b_absolute) {
                        current_pos = next_pos + origin;
                    } else {
                        current_pos += next_pos;
                    }

                    gCodeList << *tmpCommand;
                    // init of next instuction
                    tmpCommand = new GCodeData(tmpCommand);

                    tmpCommand->numberLine = index;


                    tmpCommand->changeInstrument = false;
                    tmpCommand->pauseMSeconds = -1; // no pause

                    break;
                }

                if (cmd == "G01") { // feed
                    QVector3D next_pos(b_absolute ? current_pos - origin : QVector3D(0, 0, 0));
                    float E(-1.0);

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->X = next_pos.x();
                    tmpCommand->Y = next_pos.y();
                    tmpCommand->Z = next_pos.z();
                    tmpCommand->splits = 0;

                    detectMinMax(tmpCommand);

                    tmpCommand->typeMoving = GCodeData::Line;

                    tmpCommand->movingCode = FEED_LINE_CODE;

                    if (b_absolute) {
                        current_pos = next_pos + origin;
                    } else {
                        current_pos += next_pos;
                    }

                    tmpCommand->plane = currentPlane;

                    gCodeList << *tmpCommand;

                    calcAngleOfLines(gCodeList.count() - 1);

                    // init of next instuction
                    tmpCommand = new GCodeData(tmpCommand);

                    tmpCommand->numberLine = index;

                    tmpCommand->changeInstrument = false;
                    tmpCommand->pauseMSeconds = -1; // no pause

                    break;
                }

                // http://www.manufacturinget.org/2011/12/cnc-g-code-g02-and-g03/
                if (cmd == "G02" || cmd == "G03") { // arc
                    QVector3D next_pos(b_absolute ? current_pos - origin : QVector3D(0, 0, 0));
                    float E(-1.0);

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->X = next_pos.x();
                    tmpCommand->Y = next_pos.y();
                    tmpCommand->Z = next_pos.z();

                    detectMinMax(tmpCommand);

                    QVector3D arc_center(current_pos);
                    // float E_arc(-1.0);
                    float radius = 0.0;

                     if (parseArc(line, arc_center, radius, coef ) == false) {
                        decoded = false;
                        break;
                    }

                    tmpCommand->plane = currentPlane;

                    if (radius == 0.0) {
                        // the arc center coordinateds
                        tmpCommand->I = arc_center.x();
                        tmpCommand->J = arc_center.y();
                        tmpCommand->K = arc_center.z();
                    }

                    tmpCommand->Radius = radius;

                    if (cmd == "G02") {
                        tmpCommand->typeMoving = GCodeData::ArcCW;
                    } else {
                        tmpCommand->typeMoving = GCodeData::ArcCCW;
                    }

                    tmpCommand->movingCode = FEED_LINE_CODE;

                    if (b_absolute) {
                        current_pos = next_pos + origin;
                    } else {
                        current_pos += next_pos;
                    }

                    // qDebug() << "line " << tmpCommand->numberLine << "before convertArcToLines()" << gCodeList.count() << "splits" << tmpCommand->splits;
                    convertArcToLines(tmpCommand); // tmpCommand has data of last point

                    tmpCommand->numberLine = index;

                    tmpCommand->changeInstrument = false;
                    tmpCommand->pauseMSeconds = -1; // no pause

                    // qDebug() << "after " << gCodeList.count() << "splits" << tmpCommand->splits;
                    break;
                }

                if (cmd == "G04") {
                    // need next parameter
                    QStringRef property1 = vctRO.at(1).mid(0, 1);
                    QStringRef value1 = vctRO.at(1).mid(1);

                    if (property1 == "P") {
                        bool res;
                        tmpCommand->pauseMSeconds = value1.toInt(&res);

                        if (res == false) {
                            decoded = false;
                            break;
                        }
                    }

                    if (property1 == "X") {
                        bool res;
                        tmpCommand->pauseMSeconds = value1.toFloat(&res) * 1000;

                        if (res == false) {
                            decoded = false;
                            break;
                        }
                    }

                    break;
                }

                if (cmd == "G17") {
                    currentPlane = XY;
                    break;
                }

                if (cmd == "G18") {
                    currentPlane = YZ;
                    break;
                }

                if (cmd == "G19") {
                    currentPlane = ZX;
                    break;
                }

                if (cmd == "G20") {
                    coef = 25.4;
                    break;
                }

                if (cmd == "G21") {
                    coef = 1.0;
                    break;
                }

                if (cmd == "G28") {
                    QVector3D next_pos(std::numeric_limits<float>::infinity(),
                                       std::numeric_limits<float>::infinity(),
                                       std::numeric_limits<float>::infinity());
                    float E;

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    if (next_pos[0] == std::numeric_limits<float>::infinity()
                            && next_pos[1] == std::numeric_limits<float>::infinity()
                            && next_pos[2] == std::numeric_limits<float>::infinity()) {
                        current_pos = origin = QVector3D(0, 0, 0);
                    } else {
                        for(size_t i = 0 ; i < 3 ; ++i) {
                            if (next_pos[i] != std::numeric_limits<float>::infinity()) {
                                current_pos[i] = 0;
                                origin[i] = 0;
                            }
                        }
                    }

                    break;
                }

                if (cmd == "G90") {
                    b_absolute = true;
                    break;
                }

                if (cmd == "G91") {
                    b_absolute = false;
                    break;
                }

                if (cmd == "G92") {
                    QVector3D next_pos(current_pos);
                    float E;

                    if (parseCoord(line, next_pos, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    origin = current_pos - next_pos;
                    
                    break;
                }
                
                if (cmd == "G161") {
                    break;
                }
                
                if (cmd == "G162") {
                    break;
                }

                decoded = false;
                break;
            }

            case 'M': {
                if (cmd == "M00") {
                    tmpCommand->pauseMSeconds = 0; // waiting
                    break;
                }

                if (cmd == "M03") {
                    tmpCommand->spindelON = true;
                    break;
                }

                if (cmd == "M05") {
                    tmpCommand->spindelON = false;
                    break;
                }

                if (cmd == "M06") {
                    // need next parameter
                    QStringRef property1 = vctRO.at(1).mid(0, 1);
                    QStringRef value1 = vctRO.at(1).mid(1);

                    if (property1 == "T") {
                        tmpCommand->changeInstrument = true;
                        bool res;
                        tmpCommand->numberInstrument = value1.toInt(&res);

                        if (res == false) {
                            decoded = false;
                        }

                        tmpCommand->pauseMSeconds = value1.toInt(&res);

                        if (res == false) {
                            decoded = false;
                        }

                        if (vctRO.count() > 2) {
                            QStringRef property2 = vctRO.at(2).mid(0, 1);

                            if ( property2 == "D" ) {
                                QString value2 = vctRO.at(2).mid(1).toString().replace(Settings::fromDecimalPoint, Settings::toDecimalPoint);

                                tmpCommand->diametr = value2.toDouble(&res);

                                if (res == false) {
                                    decoded = false;
                                    break;
                                }
                            }
                        }
                    }

                    break;
                }

                if (cmd == "M18") {
                    break;
                }
                
                if (cmd == "M101") {
                    break;
                }
                
                if (cmd == "M102") {
                    break;
                }
                
                if (cmd == "M103") {
                    break;
                }
                
                if (cmd == "M104") {
                    break;
                }
                
                if (cmd == "M105") {
                    break;
                }
                
                if (cmd == "M108") {
                    break;
                }
                
                if (cmd == "M109") {
                    break;
                }
                
                if (cmd == "M113") {
                    break;
                }
                
                if (cmd == "M132") {
                    break;
                }
                
                if (cmd == "M206") {
                    float E;

                    if (parseCoord(line, origin, E, coef) == false) {
                        decoded = false;
                        break;
                    }

                    break;
                }

                decoded = false;
                break;
            }

            case 'F': {
                decoded = false;
                break;
            }

            default:
                decoded = false;
                break;
        }

        if (decoded == false) {
            emit logMessage(QString("gcode parsing error: " + line));
        } else {
            goodList << line;
        }

        index++;
    }

    //     QString log = "Read gcode, parsed. Time elapsed: " + QString::number(t.elapsed()) + " ms";
    emit logMessage(QString().sprintf("Read gcode, parsed. Time elapsed: %d ms", t.elapsed()));

    //     qDebug("read gcode, parsed. Time elapsed: %d ms", t.elapsed());

    gCodeLines.clear();
    //  unlock();

    return true;
}



/**
 * @brief detect the min and max ranges
 *
 * @param[in] pos actual index in GCode data list, if pos is 0: init of min/max
 *
 */
void GCodeParser::detectMinMax(const GCodeData &d)
{
    //     if (pos > 0 && pos < gCodeData.size()) {
    if (d.X > Settings::coord[X].softLimitMax) {
        Settings::coord[X].softLimitMax = d.X;
    }

    if (d.X < Settings::coord[X].softLimitMin) {
        Settings::coord[X].softLimitMin = d.X;
    }

    if (d.Y > Settings::coord[Y].softLimitMax) {
        Settings::coord[Y].softLimitMax = d.Y;
    }

    if (d.Y < Settings::coord[Y].softLimitMin) {
        Settings::coord[Y].softLimitMin = d.Y;
    }

    if (d.Z > Settings::coord[Z].softLimitMax) {
        Settings::coord[Z].softLimitMax = d.Z;
    }

    if (d.Z < Settings::coord[Z].softLimitMin) {
        Settings::coord[Z].softLimitMin = d.Z;
    }

    //         return;
    //     }

    //     if (pos == 0) {
    //         Settings::coord[X].softLimitMax = gCodeData.at(pos).X;
    //         Settings::coord[X].softLimitMin = gCodeData.at(pos).X;
    //         Settings::coord[Y].softLimitMax = gCodeData.at(pos).Y;
    //         Settings::coord[Y].softLimitMin = gCodeData.at(pos).Y;
    //         Settings::coord[Z].softLimitMax = gCodeData.at(pos).Z;
    //         Settings::coord[Z].softLimitMin = gCodeData.at(pos).Z;
    //     }
}


/**
 * @brief calculate angle between two points
 *
 * @param[in] pos1 first point
 * @param[in] pos2 second point
 *
 */
float GCodeParser::determineAngle(const QVector3D &pos1, const QVector3D &pos2, PlaneEnum pl)
{
    float radians = 0.0;

    switch (pl) {
        case XY: {
            if (pos1[X] == pos2[X] && pos1[Y] == pos2[Y]) { // if diff is 0
                return 0.0;
            }

            radians = atan2(pos1[Y] - pos2[Y], pos1[X] - pos2[X]);

            break;
        }

        case YZ: {
            if (pos1[Y] == pos2[Y] && pos1[Z] == pos2[Z]) {
                return 0.0;
            }

            radians = atan2(pos1[Z] - pos2[Z], pos1[Y] - pos2[Y]);

            break;
        }

        case ZX: {
            if (pos1[Z] == pos2[Z] && pos1[X] == pos2[X]) {
                return 0.0;
            }

            radians = atan2(pos1[X] - pos2[X], pos1[Z] - pos2[Z]);

            break;
        }

        default:
            qDebug() << "not defined plane of arc";
            break;
    }

    if (radians < 0.0) {
        radians += 2.0 * PI;
    }

    return radians;
}


/**
 * @brief calculates the angle diffenerce between two points
 *
 * @param[in] pos the actual position
 *
 */
void GCodeParser::calcAngleOfLines(int pos)
{
    if (pos < 1 || pos > gCodeList.count() - 1) {
        return;
    }

    switch (gCodeList.at(pos).plane) {
        case XY: {
            gCodeList[pos].angle = atan2(gCodeList.at(pos).Y - gCodeList.at(pos - 1).Y, gCodeList.at(pos).X - gCodeList.at(pos - 1).X);
            break;
        }

        case YZ: {
            gCodeList[pos].angle = atan2(gCodeList.at(pos).Z - gCodeList.at(pos - 1).Z, gCodeList.at(pos).Y - gCodeList.at(pos - 1).Y);
            break;
        }

        case ZX: {
            gCodeList[pos].angle = atan2(gCodeList.at(pos).X - gCodeList.at(pos - 1).X, gCodeList.at(pos).Z - gCodeList.at(pos - 1).Z);
            break;
        }

        default: {
            qDebug() << "calcAngleOfLines(): no plane information";
            break;
        }
    }

    if (gCodeList[pos].angle < 0.0) {
        gCodeList[pos].angle += 2.0 * PI;
    }
}


/**
 * @brief this function converts the arc to short lines: mk1 do not support the arc commands
 *
 * @param endData pointer to the list with decoded coordinates of endpoint
 *
 */
void GCodeParser::convertArcToLines(GCodeData *endData)
{
    if (gCodeList.count() == 0) {
        return;
    }

    if (endData == 0) {
        return;
    }

    if (!(endData->typeMoving == GCodeData::ArcCW || endData->typeMoving == GCodeData::ArcCCW) ) { // it's not arc
        return;
    }

    GCodeData &begData = gCodeList.last();
    // arcs
    // translate points to arc
    float r = 0.0; // length of sides
    float x2, x1, y2, y1, z2, z1;

    x1 = begData.X;;
    x2 = endData->X;

    y1 = begData.Y;
    y2 = endData->Y;

    z1 = begData.Z;
    z2 = endData->Z;

    float i, j, k;
    i = endData->I;
    j = endData->J;
    k = endData->K;

    QVector3D pos1(x1, y1, z1);
    QVector3D pos2(x2, y2, z2);

    float dPos = 0.0;
    float begPos = 0.0;

    switch (endData->plane) {
        case XY: {
            if (endData->Radius == 0.0) {
                r = sqrt(pow(x1 - i, 2) + pow(y1 - j, 2));
            } else {
                r = endData->Radius;
                // compute i, j
                //                 float a = determineAngle (pos1, pos2, endData->plane) + PI;
                //                 qDebug() << "radius " << r << "alpha" << a << "xy point 1" << x1 << y1 << "xy point 2" << x2 << y2;
            }

            dPos = z2 - z1;
            begPos = z1;
        }
        break;

        case YZ: {
            if (endData->Radius == 0.0) {
                r = sqrt(pow(y1 - j, 2) + pow(z1 - k, 2));
            } else {
                r = endData->Radius;
                // compute j, k
            }

            dPos = x2 - x1;
            begPos = x1;
        }
        break;

        case ZX: {
            if (endData->Radius == 0.0) {
                r = sqrt(pow(z1 - k, 2) + pow(x1 - i, 2));
            } else {
                r = endData->Radius;
                // compute k, i
            }

            dPos = y2 - y1;
            begPos = y1;
        }
        break;

        default:
            break;
    }

    float alpha = 0.0;
    float alpha_beg, alpha_end;

    if (r == 0.0) {
        qDebug() << "wrong, r = 0";
        return;
    }

    QVector3D posC(i, j, k);

    alpha_beg = determineAngle (pos1, posC, endData->plane);
    alpha_end = determineAngle (pos2, posC, endData->plane);

    if (endData->typeMoving == GCodeData::ArcCW) {
        if (alpha_beg == alpha_end) {
            alpha_beg += 2.0 * PI;
        }

        alpha = alpha_beg - alpha_end;

        if (alpha_beg < alpha_end) {
            alpha = fabs(alpha_beg + (2.0 * PI - alpha_end));
        }

    } else {
        if (alpha_beg == alpha_end) {
            alpha_end += 2.0 * PI;
        }

        alpha = alpha_end - alpha_beg;

        if (alpha_beg > alpha_end) {
            alpha = fabs(alpha_end + (2.0 * PI - alpha_beg));
        }
    }

    float bLength = r * alpha;

    int n = (int)(bLength * Settings::splitsPerMm) - 1; // num segments of arc per mm
    float splitLen = 1.0 / (float)Settings::splitsPerMm;

    if ( n == 0) {
        qDebug() << "wrong, n = 0" << alpha_beg << alpha_end;
        return;
    }

    float dAlpha = alpha / n;

    dPos = dPos / n;

    if (endData->typeMoving == GCodeData::ArcCW) {
        dAlpha = -dAlpha;
    }

#if DEBUG_ARC
    QString dbg;
#endif
    float angle = alpha_beg;
    float loopPos = begPos;

    // copy of parsed endpoint
    GCodeData *ncommand = new GCodeData(endData);

#if DEBUG_ARC
    qDebug() << "arc from " << begData.X << begData.Y << begData.Z  << "to" << endData->X << endData->Y << endData->Z << "splits: " << n;
#endif

    QVector<GCodeData> tmpList;

    ncommand->X = begData.X;
    ncommand->Y = begData.Y;
    ncommand->Z = begData.Z;
    ncommand->A = begData.A;

    detectMinMax(ncommand);

    ncommand->splits = n;
    ncommand->movingCode = ACCELERAT_CODE;

    // now split
    switch (endData->plane) {
        case XY: {
            for (int step = 0; step < n; ++step) {
                //coordinates of next arc point
                angle += dAlpha;
                loopPos += dPos;

                float c = cos(angle);
                float s = sin(angle);

                float x_new = i + r * c;
                float y_new = j + r * s;

                float angle = atan2(y_new - ncommand->Y, x_new - ncommand->X);

                if (angle < 0.0) {
                    angle += 2.0 * PI;
                }

                ncommand->angle = angle;
                ncommand->X = x_new;
                ncommand->Y = y_new;
                ncommand->Z = loopPos;

                detectMinMax(ncommand);
#if DEBUG_ARC
                dbg += QString().sprintf("n=%d x=%f y=%f angle=%f sin=%f cos=%f\n", step, x_new, y_new, angle, s, c);
#endif

                /** detection of end because of rounding */
                if (sqrt((x_new - endData->X) * (x_new - endData->X) + (y_new - endData->Y) * (y_new - endData->Y)) <= splitLen) {
                    float t_angle = atan2(y_new - endData->Y, x_new - endData->X);

                    if (t_angle < 0.0) {
                        t_angle += 2.0 * PI;
                    }

                    ncommand->angle = t_angle;

                    ncommand->X = endData->X;
                    ncommand->Y = endData->Y;
                    ncommand->Z = endData->Z;

                    detectMinMax(ncommand);

                    n = step;

                    tmpList << *ncommand;

                    break;
                }

                tmpList << *ncommand;
                ncommand = new GCodeData(*ncommand);

                ncommand->movingCode = CONSTSPEED_CODE;
                ncommand->splits = 0;
            }
        }
        break;

        case YZ: {
            for (int step = 0; step < n; ++step) {
                //coordinates of next arc point
                angle += dAlpha;
                loopPos += dPos;

                float c = cos(angle);
                float s = sin(angle);

                float y_new = j + r * c;
                float z_new = k + r * s;

                float angle = atan2(z_new - ncommand->Z, y_new - ncommand->Y);

                if (angle < 0.0) {
                    angle += 2.0 * PI;
                }

                ncommand->angle = angle;
                ncommand->Y = y_new;
                ncommand->Z = z_new;
                ncommand->X = loopPos;

                detectMinMax(ncommand);
#if DEBUG_ARC
                dbg += QString().sprintf("n=%d y=%f z=%f angle=%f sin=%f cos=%f\n", step, y_new, z_new, angle, s, c);
#endif

                /** detection of end because of rounding */
                if (sqrt((y_new - endData->Y) * (y_new - endData->Y) + (z_new - endData->Z) * (z_new - endData->Z)) <= splitLen) {
                    float t_angle = atan2(z_new - endData->Z, y_new - endData->Y);

                    if (t_angle < 0.0) {
                        t_angle += 2.0 * PI;
                    }

                    ncommand->angle = t_angle;

                    ncommand->X = endData->X;
                    ncommand->Y = endData->Y;
                    ncommand->Z = endData->Z;

                    detectMinMax(ncommand);

                    n = step;

                    tmpList << *ncommand;

                    break;
                }

                tmpList << *ncommand;
                ncommand = new GCodeData(*ncommand);

                ncommand->movingCode = CONSTSPEED_CODE;
                ncommand->splits = 0;
            }
        }
        break;

        case ZX: {
            for (int step = 0; step < n; ++step) {
                //coordinates of next arc point
                angle += dAlpha;
                loopPos += dPos;

                float c = cos(angle);
                float s = sin(angle);

                float z_new = k + r * c;
                float x_new = i + r * s;

                float angle = atan2(x_new - ncommand->X, z_new - ncommand->Z);

                if (angle < 0.0) {
                    angle += 2.0 * PI;
                }

                ncommand->angle = angle;
                ncommand->Z = z_new;
                ncommand->X = x_new;
                ncommand->Y = loopPos;

                detectMinMax(ncommand);
#if DEBUG_ARC
                dbg += QString().sprintf("n=%d z=%f x=%f angle=%f sin=%f cos=%f\n", step, z_new, x_new, angle, s, c);
#endif

                /** detection of end because of rounding */
                if (sqrt((x_new - endData->X) * (x_new - endData->X) + (z_new - endData->Z) * (z_new - endData->Z)) <= splitLen) {
                    float t_angle = atan2(x_new - endData->X, z_new - endData->Z);

                    if (t_angle < 0.0) {
                        t_angle += 2.0 * PI;
                    }

                    ncommand->angle = t_angle;

                    ncommand->X = endData->X;
                    ncommand->Y = endData->Y;
                    ncommand->Z = endData->Z;

                    detectMinMax(ncommand);

                    n = step;

                    tmpList << *ncommand;

                    break;
                }

                tmpList << *ncommand;

                ncommand = new GCodeData(*ncommand);
                ncommand->movingCode = CONSTSPEED_CODE;
                ncommand->splits = 0;
            }
        }
        break;

        default:
            qDebug() << "no plane info!";
            break;
    }


    if (tmpList.length() > 0) {
        tmpList[tmpList.length() - 1].movingCode = DECELERAT_CODE;
        tmpList[0].splits = n;
    }

    gCodeList += (tmpList);

    tmpList.clear();

#if DEBUG_ARC

    if ((fabs (x2 - gCodeList.last().X) > (bLength / splitsPerMm)) || (fabs (y2 - gCodeList.last().Y) > (bLength / splitsPerMm))) { // wenn zu weit vom ziel...
        qDebug() << "begin: " << x1 << y1 << "end" << x2 << y2 << "center" << i << j;
        qDebug() << "bogen " << bLength << "mm" << "r" << r << "a" << a << "triangle alpha" << alpha;
        qDebug() << "alpha:" << alpha_beg << "->" << alpha_end << "d alpha: " << dAlpha; // rad
        qDebug() << dbg;
    }

#endif
}


/**
 * @brief parsing of I, J, K, R parameters of G-Code
 *
 * @param
 * @return if anything is detected, return true
 */
bool GCodeParser::parseArc(const QString &line, QVector3D &pos, float &R, const float coef)
{
    if (line.isEmpty() == true) {
        return false;
    }

    QVector<QStringRef> chunksRO = line.splitRef(" ", QString::SkipEmptyParts);
    
    QVector3D arc(COORD_TOO_BIG, COORD_TOO_BIG, COORD_TOO_BIG); // too big coordinates

    if (chunksRO.count() <= 1) {
        return false;
    }

    bool res = false;

    R = 0.0;

    for (int i = 1; i < chunksRO.count(); i++) {
        QStringRef s = chunksRO.at(i);
        bool conv;

        switch(s.at(0).toLatin1()) {
            case 'I': {
                arc.setX(pos.x() + coef * (s.right(s.size() - 1).toDouble(&conv)));

                if (conv == true) {
                    res = true;
                }
                else{
                    qDebug() << "i" << s.right(s.size() - 1) << s.right(s.size() - 1).toDouble();
                }

                break;
            }

            case 'J': {
                arc.setY( pos.y() + coef * (s.right(s.size() - 1).toDouble(&conv)));

                if (conv == true) {
                    res = true;
                }
                else{
                    qDebug() << "j" << s.right(s.size() - 1) << s.right(s.size() - 1).toDouble();
                }

                break;
            }

            case 'K': {
                arc.setZ( pos.z() + coef * (s.right(s.size() - 1).toDouble(&conv)));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'R': {
                R = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            default:
                break;
        }
    }

    pos = arc;

    return res;
}


/**
 * @brief parsing X, Y, Z, E, F of G-Code parameters
 * rotation parameters A, B, C are not implemented
 *
 * @param
 * @return if anything is detected, return true
 */
bool GCodeParser::parseCoord(const QString &line, QVector3D &pos, float &E, const float coef, float *F)
{
    if (line.isEmpty() == true) {
        return false;
    }

    QVector<QStringRef> chunksRO = line.splitRef(" ", QString::SkipEmptyParts);

    if (chunksRO.count() <= 1) {
        return false;
    }

    bool res = false;

    for (int i=1; i< chunksRO.count(); i++) {
        QStringRef s = chunksRO.at(i);
        bool conv;

        switch(s.at(0).toLatin1()) {
            case 'X': {
                pos.setX( coef * (s.right(s.size() - 1).toDouble(&conv)));

                if (conv == true) {
                    res = true;
                }
                else{
                    qDebug() << "x" << s.right(s.size() - 1) << s.right(s.size() - 1).toDouble();
                }

                break;
            }

            case 'Y': {
                pos.setY( coef * (s.right(s.size() - 1).toDouble(&conv)));

                if (conv == true) {
                    res = true;
                }
                else{
                    qDebug() << "y" << s.right(s.size() - 1) << s.right(s.size() - 1).toDouble();
                }

                break;
            }

            case 'Z': {
                pos.setZ( coef * (s.right(s.size() - 1).toDouble(&conv)));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'A': // rotation X
            case 'B': // rotation Y
            case 'C': { // rotation Z are not supported
                break;
            }

            case 'E': {
                E = coef * (s.right(s.size() - 1).toDouble(&conv));

                if (conv == true) {
                    res = true;
                }

                break;
            }

            case 'F': {
                if (F) {
                    *F = s.right(s.size() - 1).toDouble(&conv);
                }

                if (conv == true) {
                    res = true;
                }

                break;
            }

            default:
                break;
        }
    }

    return res;
}


/**
 * @brief
 *
 */
QVector<QString> GCodeParser::getGoodList()
{
    return goodList;
}


/**
 * @brief
 *
 */
// QVector<QString> GCodeParser::getBadList()
// {
//     return badList;
// }


/**
 * @brief
 *
 */
QVector<GCodeData> GCodeParser::getGCodeData()
{
    //     qDebug() << "return gcode data" << gCodeList.count();
    return gCodeList;
}

