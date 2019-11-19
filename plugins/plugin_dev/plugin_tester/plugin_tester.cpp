/* pluginTest.cpp : Defines the entry point for the console application. */

/* */

#include <QtCore/QCoreApplication>

#include <stdio.h>

#include "plugin_api.h"
#include "plugin_base.h"

#include <QString>
#include <QLibrary>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

/* Assert codes */
#define LIBRARY_LOAD_ASSERT 1
#define PLUGIN_VERSION_ASSERT 2
#define PLUGIN_CREATION_ASSERT 3
#define PLUGIN_TEST_FAIL 4
#define PLUGIN_ATTACH_ASSERT 5

#define ARGC_COUNT 3
#define ARGV_INDEX_LIB_FILE 1
#define ARGV_INDEX_DATA_FILE 2

#define TEMP_STRING_LENGHT 256

char tempString[TEMP_STRING_LENGHT];

/*
 * ----------------------------------------------------------------------------------------------------------------------
 * -- Test helper functions --
 * ----------------------------------------------------------------------------------------------------------------------
 * */
bool test_plugin_decoders(QFile& dataFile, CPlugin_DLL_API *pluginAPI_p);
bool test_plugin_plotting(QFile& dataFile, CPlugin_DLL_API *pluginAPI_p);

QString getFileRowString(QTextStream& stream, int rowIndex);

/*----------------------------------------------------------------------------------------------------------------------
 * */
int main(int argc, char *argv[])
{
    DLL_API_PluginVersion_t DLL_API_Version;
    CPlugin_DLL_API *pluginAPI_p = nullptr;
    QCoreApplication a(argc, argv);

#ifdef WIN32
    if (argc != 3) {
        qInfo() << "Input parameter error\n"
                   "The testapp requires 2 parameters, library dll-file and data text file\n"
                   "Example\n./testapp plugin.dll test_data.txt\n";
        return -1;
    }
#else
    if (argc != ARGC_COUNT) {
        qInfo() << "Input parameter error\n"
                   "The testapp requires 2 parameters, library so-file and data text file\n"
                   "Example\n./testapp plugin.dll test_data.txt\n";
        return -1;
    }
#endif

    /*-- Check input parameters (plugin path and data file path) -- */

    QString plugin_filePath(argv[ARGV_INDEX_LIB_FILE]);
    QString data_filePath(argv[ARGV_INDEX_DATA_FILE]);

    if (!QLibrary::isLibrary(plugin_filePath)) {
        qInfo() << "Input parameter error\n" <<
            QString("The file %1 was not a library").arg(plugin_filePath);

        return -1;
    }

    QFileInfo dataFileInfo(data_filePath);
    if (!dataFileInfo.exists()) {
        qInfo() << "Input parameter error\n" <<
            QString("The file %1 doesn't exist").arg(data_filePath);
        return -1;
    }

    if ((dataFileInfo.suffix().compare(QString("txt"), Qt::CaseInsensitive) != 0)) {
        qInfo() << "Input parameter error\n" <<
            QString("The file %1 isn't valid").arg(data_filePath);
        return -1;
    }

    QFile dataFile(data_filePath);
    if (!dataFile.open(QIODevice::ReadOnly)) {
        qInfo() << "Input parameter error\n" <<
            QString("The file %1 couldn't be opened").arg(data_filePath);
        return -1;
    }

    /*-- Load and check library API -- */

    QLibrary *library_p = new QLibrary(plugin_filePath);

    try {
        library_p->load();

        if (!library_p->isLoaded()) {
            qInfo() << QString("Error: loading so/dll-file failed {%1} dl_error:%2")
                .arg(plugin_filePath).arg(library_p->errorString());
            throw(LIBRARY_LOAD_ASSERT); /*goto library_delete; */
        }

        DLL_API_GetPluginAPIVersion_t DLL_API_GetPluginAPIVersion =
            reinterpret_cast<DLL_API_GetPluginAPIVersion_t>(library_p->resolve(__DLL_API_GetPluginAPIVersion__));

        if (DLL_API_GetPluginAPIVersion != nullptr) {
            DLL_API_GetPluginAPIVersion(&DLL_API_Version);
        } else {
            qInfo() << QString("Error: Could not find {%1} in library {%2}, "
                               "the library is probarbly not a valid LogScrutinizer plugin")
                .arg(__DLL_API_GetPluginAPIVersion__).arg(plugin_filePath);
            throw(PLUGIN_VERSION_ASSERT); /* goto library_unload */
        }

        if (DLL_API_Version.version == DLL_API_VERSION) {
            DLL_API_SetAttachConfiguration_t DLL_API_SetAttachConfiguration =
                reinterpret_cast<DLL_API_SetAttachConfiguration_t>
                (library_p->resolve(__DLL_API_SetAttachConfiguration__));

            if (DLL_API_SetAttachConfiguration != nullptr) {
                DLL_API_AttachConfiguration_t attachConfiguration;
                memset(&attachConfiguration, 0, sizeof(attachConfiguration));
                DLL_API_SetAttachConfiguration(&attachConfiguration);
            } else {
                qInfo() << QString("Error: Could not find {%1} in library {%2}, the library is probarbly not a valid"
                                   "LogScrutinizer plugin")
                    .arg(__DLL_API_SetAttachConfiguration__).arg(plugin_filePath);
                throw(PLUGIN_ATTACH_ASSERT); /*goto library_unload; */
            }

            DLL_API_CreatePlugin_t DLL_API_CreatePlugin =
                reinterpret_cast<DLL_API_CreatePlugin_t>(library_p->resolve(__DLL_API_CreatePlugin__));
            if (DLL_API_CreatePlugin != nullptr) {
                pluginAPI_p = DLL_API_CreatePlugin();
            } else {
                qInfo() << QString("Error: Could not find {%1} in library {%2}, the library is probarbly not a valid"
                                   "LogScrutinizer plugin")
                    .arg(__DLL_API_CreatePlugin__).arg(plugin_filePath);
                throw(PLUGIN_CREATION_ASSERT); /*goto library_unload; */
            }
        } else {
            qInfo() << QString("Error: The versions between LogScrutinizer and the plugin {%1} are not matching\n"
                               "Plugin version=%2, LogScrutinzer version=%3\n Please either get matching versions")
                .arg(plugin_filePath).arg(DLL_API_Version.version).arg(DLL_API_VERSION);
            throw(PLUGIN_VERSION_ASSERT); /* goto library_unload; */
        }

        /*-- Run general tests on the plugin with the supplied data file -- */

        if (!test_plugin_decoders(dataFile, pluginAPI_p)) {
            throw PLUGIN_TEST_FAIL;
        }
        if (!test_plugin_plotting(dataFile, pluginAPI_p)) {
            throw PLUGIN_TEST_FAIL;
        }
    } catch (int error_code) {
        switch (error_code)
        {
            case PLUGIN_TEST_FAIL: /*fall-through */
            {
                DLL_API_DeletePlugin_t DLL_API_DeletePlugin =
                    reinterpret_cast<DLL_API_DeletePlugin_t>(library_p->resolve(__DLL_API_DeletePlugin__));
                if ((nullptr != DLL_API_DeletePlugin) && (nullptr != pluginAPI_p)) {
                    DLL_API_DeletePlugin(pluginAPI_p);
                    pluginAPI_p = nullptr;
                }
            }

            case PLUGIN_ATTACH_ASSERT: /*fall-through */
            case PLUGIN_CREATION_ASSERT: /*fall-through */
            case PLUGIN_VERSION_ASSERT: /*fall-through */
                if (library_p != nullptr) {
                    library_p->unload();
                }

            case LIBRARY_LOAD_ASSERT: /*fall-through */
                if (library_p != nullptr) {
                    delete library_p;
                }

            default:
                break;
        } /* switch */

        return -1;
    }

    /*-- Time to exit, unload -- */

    DLL_API_DeletePlugin_t DLL_API_DeletePlugin =
        reinterpret_cast<DLL_API_DeletePlugin_t>(library_p->resolve(__DLL_API_DeletePlugin__));

    if (DLL_API_DeletePlugin != nullptr) {
        DLL_API_DeletePlugin(pluginAPI_p);
        pluginAPI_p = nullptr;
    }

    library_p->unload();
    delete library_p;

    return 0;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool test_plugin_decoders(QFile& dataFile, CPlugin_DLL_API *pluginAPI_p)
{
    CList_LSZ *decoderList_p;
    if (pluginAPI_p->GetDecoders(&decoderList_p) && !decoderList_p->isEmpty()) {
        QTextStream stream(&dataFile);
        QString data;
        int index = 0;
        while (!stream.atEnd()) {
            auto data = stream.readLine();
            auto length = static_cast<const int>(data.length());
            if (length >= TEMP_STRING_LENGHT - 1) {
                qInfo() << "Error: data file contains too long lines";
                return false;
            }

            auto decoder_p = static_cast<CDecoder *>(decoderList_p->first());
            int decoderIndex = 0;
            strcpy(tempString, data.toLatin1().data());
            while (nullptr != decoder_p) {
                if (decoder_p->Decode(tempString, &length, TEMP_STRING_LENGHT)) {
                    qInfo() << "Row_" << index << " Decoder_" << decoderIndex <<
                        data.toLatin1().data() << " -> " << tempString;

                    /* Possibly the decoder has trashed the tempString, write the original back. */
                    strcpy(tempString, data.toLatin1().data());
                }
                decoder_p = static_cast<CDecoder *>(decoderList_p->GetNext(decoder_p));
            }
            ++index;
        }
    } else {
        qInfo() << "No Decoder support";
    }

    return true;
}

/*----------------------------------------------------------------------------------------------------------------------
 * */
bool test_plugin_plotting(QFile& dataFile, CPlugin_DLL_API *pluginAPI_p)
{
    CList_LSZ *plotList_p;
    if (!pluginAPI_p->GetPlots(&plotList_p) || plotList_p->isEmpty()) {
        qInfo() << QString("No plot support");
        return true;
    }

    /* Reset all plots in the plugin */
    CPlot *plot_p = (CPlot *)plotList_p->first();
    while (plot_p != nullptr) {
        plot_p->pvPlotClean();
        plot_p->PlotBegin();
        plot_p = (CPlot *)plotList_p->GetNext(plot_p);
    }

    /* Feed each plot with the test data */

    QTextStream stream(&dataFile);
    QString data;
    int index = 0;
    while (!stream.atEnd()) {
        data = stream.readLine();

        auto length = static_cast<const int>(data.length());
        auto const_data = data.toLatin1().constData();
        plot_p = (CPlot *)plotList_p->first();
        while (plot_p != nullptr) {
            plot_p->PlotRow(const_data, &length, index);
            plot_p = (CPlot *)plotList_p->GetNext(plot_p);
        }
        ++index;
    }

    /* Indicate that plotting is ended */
    plot_p = (CPlot *)plotList_p->first();
    while (plot_p != nullptr) {
        plot_p->PlotEnd();
        plot_p = (CPlot *)plotList_p->GetNext(plot_p);
    }

    /* Now each plot has generated data, go through the sub-plots and print the contents */
    plot_p = (CPlot *)plotList_p->first();
    while (plot_p != nullptr) {
        CList_LSZ *subPlotList_p;
        CList_LSZ *graphList_p;
        char *plotTitle_p;
        char *X_AxisLabel_p;

        plot_p->GetTitle(&plotTitle_p, &X_AxisLabel_p);
        plot_p->GetSubPlots(&subPlotList_p);

        qInfo() << QString("\nPlot:%1 X-Label:%2").arg(plotTitle_p).arg(X_AxisLabel_p);

        if (!subPlotList_p->isEmpty()) {
            char *subPlotTitle_p;
            char *Y_AxisLabel_p;
            GraphicalObject_Extents_t extents;
            CSubPlot *subPlot_p = (CSubPlot *)subPlotList_p->first();
            while (subPlot_p != nullptr) {
                subPlot_p->GetTitle(&subPlotTitle_p, &Y_AxisLabel_p);
                subPlot_p->GetExtents(&extents);

                qInfo() << QString("  SubPlot:%1 Y-Label:%2  X_min:%3 X_max:%4 Y_min:%5 Y_max:%6")
                    .arg(subPlotTitle_p).arg(Y_AxisLabel_p).arg(extents.x_min).arg(extents.x_max)
                    .arg(extents.y_min).arg(extents.y_max);

                subPlot_p->GetGraphs(&graphList_p);
                if (!graphList_p->isEmpty()) {
                    CGraph_Internal *graph_p = reinterpret_cast<CGraph *>(graphList_p->first());

                    while (graph_p != nullptr) {
                        graph_p->GetExtents(&extents);

                        qInfo() << QString("    Graph:%1 X_min:%2 X_max:%3 Y_min:%4 Y_max:%5")
                            .arg(graph_p->GetName()).arg(extents.x_min).arg(extents.x_max)
                            .arg(extents.y_min).arg(extents.y_max);

                        GraphicalObject_t *go_p = graph_p->GetFirstGraphicalObject();
                        while (go_p != nullptr) {
                            qInfo() << QString("      Graphical Object    x1:%1 y1:%2 ->  x2:%3 y2:%4")
                                .arg(go_p->x1).arg(go_p->y1).arg(go_p->x2).arg(go_p->y2);

                            char feedbackString[512];
                            QString rowString = getFileRowString(stream, go_p->row);
                            bool feedback = plot_p->vPlotGraphicalObjectFeedback(
                                rowString.toLatin1().constData(),
                                rowString.size(),
                                go_p->x2,
                                go_p->row,
                                graph_p,
                                feedbackString,
                                sizeof(feedbackString));

                            if (feedback) {
                                printf("    feedback: %s\n", feedbackString);
                            } else {
                                printf("\n");
                            }

                            go_p = graph_p->GetNextGraphicalObject();
                        }

                        graph_p = (CGraph *)graphList_p->GetNext((CListObject *)graph_p);
                    } /* while graph_p */
                } /* if graphList_p->isEmpty() */

                subPlot_p = (CSubPlot *)subPlotList_p->GetNext((CListObject *)subPlot_p);
            } /* while (subPlot_p != nullptr) */
        }

        plot_p = (CPlot *)plotList_p->GetNext(plot_p);
    } /* while */
    return true;
}

/***********************************************************************************************************************
*   getFileRowString
***********************************************************************************************************************/
QString getFileRowString(QTextStream& stream, int rowIndex)
{
    stream.seek(0);

    int index = 0;
    while (!stream.atEnd()) {
        QString data = stream.readLine();
        if (index == rowIndex) {
            return data;
        }
        ++index;
    }
    return QString();
}
