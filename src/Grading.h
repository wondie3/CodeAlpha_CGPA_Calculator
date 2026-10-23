#pragma once

#include <QString>
#include <QVector>

namespace Limits
{
    constexpr int MaxSemesters = 20;
    constexpr int MaxCoursesPerSemester = 30;
    constexpr double MinCredits = 0.25;
    constexpr double MaxCredits = 10.0;
}

struct GradeResult
{
    QString letter;
    double points;
};

// Score (0-100) -> letter grade and grade points (4.00 scale).
GradeResult gradeFromScore(double score);

struct Course
{
    QString name;
    double score = 0.0;
    double credits = 0.0;

    GradeResult grade() const { return gradeFromScore(score); }
    double qualityPoints() const { return grade().points * credits; }
};

struct Semester
{
    QString title;
    QVector<Course> courses;

    double totalCredits() const;
    double totalPoints() const;
    double gpa() const;
};

struct Transcript
{
    QString studentName;
    QVector<Semester> semesters;

    double totalCredits() const;
    double totalPoints() const;
    double cgpa() const;
};

namespace Validation
{
    bool isValidPersonName(const QString &name, QString *error = nullptr);
    bool isValidCourseName(const QString &name, QString *error = nullptr);
    bool isValidSemesterTitle(const QString &title, QString *error = nullptr);
}
