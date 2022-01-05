/***********************************************************************************************************************
** Copyright (C) 2019 Robert Klang
** Contact: https://www.logscrutinizer.com
***********************************************************************************************************************/

#include <QRandomGenerator>
#include <QtCore/QCoreApplication>
#include <chrono>
#include <stdio.h>

#include "plugin_api.h"
#include "plugin_base.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QLibrary>
#include <QString>
#include <QThread>

#define ARGC_COUNT 2
#define ARGV_INDEX_DATA_FILE 1

#define TEMP_STRING_LENGHT 256

/*char tempString[TEMP_STRING_LENGHT]; */

/* ----------------------------------------------------------------------------------------------------------------------
 * -- Test helper functions --
 * ----------------------------------------------------------------------------------------------------------------------
 * */

/* "Line:0 Time:0 Value:1",
 * "Line:0 Time:1 Value:4", */

/*----------------------------------------------------------------------------------------------------------------------
 */
int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    bool doReopen = false;
    QString data_filePath;
    if (argc != ARGC_COUNT) {
        data_filePath = QString("data.txt");
    } else {
        data_filePath = QString((argv[ARGV_INDEX_DATA_FILE]));
    }

    QFile data_file(data_filePath);
    auto start = std::chrono::steady_clock::now();
    bool doWrap = true;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto delta =
            std::chrono::duration_cast<std::chrono::seconds>(now - start).count();

        if (delta > 5) {
            now = std::chrono::steady_clock::now();
            doWrap = doReopen;
        }

        if (doWrap && data_file.isOpen()) {
            data_file.close();
            doWrap = false;
        }

        if (!data_file.isOpen()) {
            if (!data_file.open(QIODevice::WriteOnly)) {
                qInfo() << "Input parameter error\n"
                        << QString("The file %1 cannot be created").arg(data_filePath);
                return -1;
            }
            QThread::sleep(4);
        }

        auto fi = QFileInfo(data_file);
        qInfo() << "File streamed to : " << fi.absoluteFilePath();

        try {
            int time = 0;
            for (int loop = 0; loop < 10; loop++) {
                QString data;
                for (int index = 0; index < 100; index++) {
                    quint32 v = QRandomGenerator::global()->bounded(256);

                    data.append(QString("Line:0 Time:%1 Value:%2\n").arg(time).arg(v));
                    time++;
                }
                data_file.write(data.toLatin1().constData(), data.size());
                data_file.flush();
                QThread::msleep(100);
            }
        } catch (int error_code) {
            switch (error_code)
            {
                case 0: /*fall-through */
                default:
                    break;
            } /* switch */
            qInfo() << "Aiiieeee";
            return -1;
        }
    }
    qInfo() << "No more";
    return 0;
}
