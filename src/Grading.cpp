#include "Grading.h"

GradeResult gradeFromScore(double score)
{
    if (score >= 90)
        return {QStringLiteral("A+"), 4.00};
    if (score >= 83)
        return {QStringLiteral("A"), 4.00};
    if (score >= 80)
        return {QStringLiteral("A-"), 3.75};
    if (score >= 75)
        return {QStringLiteral("B+"), 3.50};
    if (score >= 70)
        return {QStringLiteral("B"), 3.00};
    if (score >= 65)
        return {QStringLiteral("B-"), 2.75};
    if (score >= 60)
        return {QStringLiteral("C+"), 2.50};
    if (score >= 50)
        return {QStringLiteral("C"), 2.00};
    if (score >= 45)
        return {QStringLiteral("C-"), 1.75};
    if (score >= 40)
        return {QStringLiteral("D"), 1.00};
    return {QStringLiteral("F"), 0.00};
}

double Semester::totalCredits() const
{
    double total = 0.0;
    for (const Course &c : courses)
        total += c.credits;
    return total;
}

double Semester::totalPoints() const
{
    double total = 0.0;
    for (const Course &c : courses)
        total += c.qualityPoints();
    return total;
}

double Semester::gpa() const
{
    const double credits = totalCredits();
    return credits > 0.0 ? totalPoints() / credits : 0.0;
}

double Transcript::totalCredits() const
{
    double total = 0.0;
    for (const Semester &s : semesters)
        total += s.totalCredits();
    return total;
}

double Transcript::totalPoints() const
{
    double total = 0.0;
    for (const Semester &s : semesters)
        total += s.totalPoints();
    return total;
}

double Transcript::cgpa() const
{
    const double credits = totalCredits();
    return credits > 0.0 ? totalPoints() / credits : 0.0;
}

namespace Validation
{

    static bool fail(QString *error, const QString &message)
    {
        if (error)
            *error = message;
        return false;
    }

    bool isValidPersonName(const QString &name, QString *error)
    {
        if (name.size() < 2 || name.size() > 50)
            return fail(error, QStringLiteral("Name must be 2-50 characters long."));

        bool hasLetter = false;
        for (const QChar c : name)
        {
            if (c.isLetter())
            {
                hasLetter = true;
            }
            else if (c.isMark() || c == QLatin1Char(' ') || c == QLatin1Char('\'') || c == QLatin1Char('-') || c == QLatin1Char('.') || c == QChar(0x2019))
            {
                continue;
            }
            else
            {
                return fail(error, QStringLiteral(
                                       "Name may only contain letters, spaces, apostrophes, hyphens and periods."));
            }
        }
        if (!hasLetter)
            return fail(error, QStringLiteral("Name must contain letters."));
        if (name != name.trimmed())
            return fail(error, QStringLiteral("Name must not start or end with a space."));
        if (name.contains(QStringLiteral("  ")))
            return fail(error, QStringLiteral("Name must not contain consecutive spaces."));
        return true;
    }

    bool isValidCourseName(const QString &name, QString *error)
    {
        if (name.size() < 2 || name.size() > 40)
            return fail(error, QStringLiteral("Course name must be 2-40 characters long."));

        const QString allowed = QStringLiteral(" &-.()/");
        bool hasLetter = false;
        for (const QChar c : name)
        {
            if (c.isLetter())
                hasLetter = true;
            else if (!c.isDigit() && !c.isMark() && !allowed.contains(c))
                return fail(error, QStringLiteral(
                                       "Course name may only contain letters, digits, spaces and & - . ( ) /"));
        }
        if (!hasLetter)
            return fail(error, QStringLiteral("Course name must contain letters."));
        if (name != name.trimmed())
            return fail(error, QStringLiteral("Course name must not start or end with a space."));
        return true;
    }

    bool isValidSemesterTitle(const QString &title, QString *error)
    {
        if (title.trimmed().isEmpty() || title.size() > 30)
            return fail(error, QStringLiteral("Semester name must be 1-30 characters long."));
        for (const QChar c : title)
        {
            if (c.category() == QChar::Other_Control)
                return fail(error, QStringLiteral("Semester name contains invalid characters."));
        }
        return true;
    }

} // namespace Validation
