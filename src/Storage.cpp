#include "Storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace Storage
{

    namespace
    {

        constexpr int FormatVersion = 1;
        constexpr qint64 MaxFileBytes = 5 * 1024 * 1024;
        const char *const AppId = "CodeAlphaCGPA";

        bool writeAtomically(const QString &path, const QByteArray &bytes, bool keepBackup, QString *error)
        {
            auto fail = [error](const QString &message)
            {
                if (error)
                    *error = message;
                return false;
            };

            const QString dir = QFileInfo(path).absolutePath();
            if (!QDir().mkpath(dir))
                return fail(QStringLiteral("Cannot create folder:\n%1").arg(dir));

            if (keepBackup && QFile::exists(path))
            {
                const QString backup = path + QStringLiteral(".bak");
                QFile::remove(backup);
                QFile::copy(path, backup);
            }

            QSaveFile file(path);
            if (!file.open(QIODevice::WriteOnly))
                return fail(QStringLiteral("Cannot write to %1\n%2\n\nThe file may be open in another program "
                                           "or the location may be read-only.")
                                .arg(path, file.errorString()));
            if (file.write(bytes) != bytes.size())
            {
                file.cancelWriting();
                return fail(QStringLiteral("Write failed: %1").arg(file.errorString()));
            }
            if (!file.commit())
                return fail(QStringLiteral("Could not finish writing %1\n%2\n\nThe file may be open in "
                                           "another program (for example Excel).")
                                .arg(path, file.errorString()));
            return true;
        }

        QString csvText(QString s)
        {
            if (!s.isEmpty() && QStringLiteral("=+-@\t\r").contains(s.at(0)))
                s.prepend(QLatin1Char('\''));
            if (s.contains(QLatin1Char(',')) || s.contains(QLatin1Char('"')) || s.contains(QLatin1Char('\n')) || s.contains(QLatin1Char('\r')))
            {
                s.replace(QLatin1Char('"'), QStringLiteral("\"\""));
                s = QLatin1Char('"') + s + QLatin1Char('"');
            }
            return s;
        }

        QString num(double v) { return QString::number(v, 'f', 2); }

    } // namespace

    QString defaultDataPath()
    {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        return QDir(dir).filePath(QStringLiteral("transcript.json"));
    }

    bool saveTranscript(const Transcript &t, const QString &path, QString *error)
    {
        QJsonArray semesters;
        for (const Semester &s : t.semesters)
        {
            QJsonArray courses;
            for (const Course &c : s.courses)
            {
                courses.append(QJsonObject{{QStringLiteral("name"), c.name},
                                           {QStringLiteral("score"), c.score},
                                           {QStringLiteral("credits"), c.credits}});
            }
            semesters.append(QJsonObject{{QStringLiteral("title"), s.title},
                                         {QStringLiteral("courses"), courses}});
        }
        const QJsonObject root{{QStringLiteral("app"), QLatin1String(AppId)},
                               {QStringLiteral("version"), FormatVersion},
                               {QStringLiteral("student"), t.studentName},
                               {QStringLiteral("semesters"), semesters}};

        return writeAtomically(path, QJsonDocument(root).toJson(QJsonDocument::Indented), true, error);
    }

    LoadResult loadTranscript(const QString &path)
    {
        LoadResult result;
        auto fail = [&result](const QString &message)
        {
            result.ok = false;
            result.data = Transcript();
            result.error = message;
            return result;
        };

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return fail(QStringLiteral("Cannot open the file: %1").arg(file.errorString()));
        if (file.size() > MaxFileBytes)
            return fail(QStringLiteral("The file is too large to be a CGPA data file."));

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
            return fail(QStringLiteral("The file is not valid JSON (%1).").arg(parseError.errorString()));

        const QJsonObject root = doc.object();
        if (root.value(QStringLiteral("app")).toString() != QLatin1String(AppId))
            return fail(QStringLiteral("This is not a CGPA Calculator data file."));

        const int version = root.value(QStringLiteral("version")).toInt(-1);
        if (version != FormatVersion)
            return fail(QStringLiteral("Unsupported file version (%1).").arg(version));

        Transcript t;
        t.studentName = root.value(QStringLiteral("student")).toString();
        QString err;
        if (!t.studentName.isEmpty() && !Validation::isValidPersonName(t.studentName, &err))
            return fail(QStringLiteral("Invalid student name: %1").arg(err));

        const QJsonValue semValue = root.value(QStringLiteral("semesters"));
        if (!semValue.isArray())
            return fail(QStringLiteral("Missing \"semesters\" list."));
        const QJsonArray semesters = semValue.toArray();
        if (semesters.size() > Limits::MaxSemesters)
            return fail(QStringLiteral("Too many semesters (maximum %1).").arg(Limits::MaxSemesters));

        for (int si = 0; si < semesters.size(); ++si)
        {
            if (!semesters.at(si).isObject())
                return fail(QStringLiteral("Semester %1 is malformed.").arg(si + 1));
            const QJsonObject so = semesters.at(si).toObject();

            Semester sem;
            sem.title = so.value(QStringLiteral("title")).toString();
            if (!Validation::isValidSemesterTitle(sem.title, &err))
                return fail(QStringLiteral("Semester %1: %2").arg(si + 1).arg(err));

            const QJsonValue courseValue = so.value(QStringLiteral("courses"));
            if (!courseValue.isArray())
                return fail(QStringLiteral("Semester %1: missing course list.").arg(si + 1));
            const QJsonArray courses = courseValue.toArray();
            if (courses.size() > Limits::MaxCoursesPerSemester)
                return fail(QStringLiteral("Semester %1: too many courses (maximum %2).")
                                .arg(si + 1)
                                .arg(Limits::MaxCoursesPerSemester));

            for (int ci = 0; ci < courses.size(); ++ci)
            {
                const QString where = QStringLiteral("Semester %1, course %2: ").arg(si + 1).arg(ci + 1);
                if (!courses.at(ci).isObject())
                    return fail(where + QStringLiteral("malformed entry."));
                const QJsonObject co = courses.at(ci).toObject();

                Course c;
                c.name = co.value(QStringLiteral("name")).toString();
                if (!Validation::isValidCourseName(c.name, &err))
                    return fail(where + err);

                const QJsonValue score = co.value(QStringLiteral("score"));
                const QJsonValue credits = co.value(QStringLiteral("credits"));
                if (!score.isDouble() || score.toDouble() < 0.0 || score.toDouble() > 100.0)
                    return fail(where + QStringLiteral("score must be a number from 0 to 100."));
                if (!credits.isDouble() || credits.toDouble() < Limits::MinCredits || credits.toDouble() > Limits::MaxCredits)
                    return fail(where + QStringLiteral("credit hours out of range."));
                c.score = score.toDouble();
                c.credits = credits.toDouble();
                sem.courses.append(c);
            }
            t.semesters.append(sem);
        }

        result.ok = true;
        result.data = t;
        return result;
    }

    bool exportCsv(const Transcript &t, const QString &path, QString *error)
    {
        const QString eol = QStringLiteral("\r\n");
        QString out;

        out += QStringLiteral("Student,") + csvText(t.studentName) + eol;
        out += QStringLiteral("Generated,") + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")) + eol;
        out += eol;
        out += QStringLiteral("Semester,Course,Score,Letter Grade,Grade Points,Credit Hours,Quality Points") + eol;

        for (const Semester &s : t.semesters)
        {
            for (const Course &c : s.courses)
            {
                const GradeResult g = c.grade();
                out += csvText(s.title) + QLatin1Char(',') + csvText(c.name) + QLatin1Char(',') + num(c.score) + QLatin1Char(',') + g.letter + QLatin1Char(',') + num(g.points) + QLatin1Char(',') + num(c.credits) + QLatin1Char(',') + num(c.qualityPoints()) + eol;
            }
        }

        out += eol;
        out += QStringLiteral("Semester,Total Credits,Total Quality Points,GPA") + eol;
        for (const Semester &s : t.semesters)
        {
            out += csvText(s.title) + QLatin1Char(',') + num(s.totalCredits()) + QLatin1Char(',') + num(s.totalPoints()) + QLatin1Char(',') + num(s.gpa()) + eol;
        }
        out += QStringLiteral("Overall CGPA,") + num(t.totalCredits()) + QLatin1Char(',') + num(t.totalPoints()) + QLatin1Char(',') + num(t.cgpa()) + eol;

        const QByteArray bytes = QByteArray("\xEF\xBB\xBF") + out.toUtf8();
        return writeAtomically(path, bytes, false, error);
    }

} // namespace Storage
