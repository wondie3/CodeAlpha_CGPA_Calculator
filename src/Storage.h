#pragma once

#include "Grading.h"

namespace Storage
{

    struct LoadResult
    {
        bool ok = false;
        Transcript data;
        QString error;
    };

    QString defaultDataPath();
    LoadResult loadTranscript(const QString &path);
    bool saveTranscript(const Transcript &transcript, const QString &path, QString *error = nullptr);
    bool exportCsv(const Transcript &transcript, const QString &path, QString *error = nullptr);

} // namespace Storage
