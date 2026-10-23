#pragma once

#include <QDialog>
#include "Grading.h"

class QLabel;
class QLineEdit;

class CourseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CourseDialog(QWidget *parent = nullptr, const Course *existing = nullptr);
    Course course() const;

protected:
    void accept() override;

private:
    void updatePreview();
    void setFieldInvalid(QLineEdit *edit, bool invalid);
    bool readNumber(const QLineEdit *edit, double &value) const;

    QLineEdit *nameEdit_;
    QLineEdit *scoreEdit_;
    QLineEdit *creditsEdit_;
    QLabel *previewLabel_;
    QLabel *errorLabel_;
};
