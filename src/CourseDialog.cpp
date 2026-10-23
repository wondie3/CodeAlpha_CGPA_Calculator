#include "CourseDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStyle>
#include <QVBoxLayout>

CourseDialog::CourseDialog(QWidget *parent, const Course *existing)
    : QDialog(parent)
{
    setWindowTitle(existing ? QStringLiteral("Edit Course")
                            : QStringLiteral("Add Course"));
    setModal(true);
    setMinimumWidth(380);

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setMaxLength(40);
    nameEdit_->setPlaceholderText(QStringLiteral("e.g. Calculus I"));

    scoreEdit_ = new QLineEdit(this);
    scoreEdit_->setPlaceholderText(QStringLiteral("0 - 100"));
    scoreEdit_->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("^\\d{0,3}([.,]\\d{0,2})?$")),
            scoreEdit_));

    creditsEdit_ = new QLineEdit(this);
    creditsEdit_->setText(QStringLiteral("3"));
    creditsEdit_->setPlaceholderText(QStringLiteral("e.g. 3"));
    creditsEdit_->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("^\\d{0,2}([.,]\\d{0,2})?$")),
            creditsEdit_));

    previewLabel_ = new QLabel(this);
    previewLabel_->setObjectName(QStringLiteral("hint"));

    errorLabel_ = new QLabel(this);
    errorLabel_->setObjectName(QStringLiteral("errorLabel"));
    errorLabel_->setWordWrap(true);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Course name"), nameEdit_);
    form->addRow(QStringLiteral("Score (0-100)"), scoreEdit_);
    form->addRow(QStringLiteral("Credit hours"), creditsEdit_);
    form->addRow(QStringLiteral("Result"), previewLabel_);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    buttons->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(buttons, &QDialogButtonBox::accepted, this, &CourseDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &CourseDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);
    layout->addLayout(form);
    layout->addWidget(errorLabel_);
    layout->addWidget(buttons);

    if (existing)
    {
        nameEdit_->setText(existing->name);
        scoreEdit_->setText(QString::number(existing->score, 'g', 6));
        creditsEdit_->setText(QString::number(existing->credits, 'g', 6));
    }

    connect(scoreEdit_, &QLineEdit::textChanged, this, [this]()
            {
        errorLabel_->clear();
        setFieldInvalid(scoreEdit_, false);
        updatePreview(); });

    connect(creditsEdit_, &QLineEdit::textChanged, this, [this]()
            {
        errorLabel_->clear();
        setFieldInvalid(creditsEdit_, false); });

    connect(nameEdit_, &QLineEdit::textChanged, this, [this]()
            {
        errorLabel_->clear();
        setFieldInvalid(nameEdit_, false); });

    updatePreview();
}

void CourseDialog::setFieldInvalid(QLineEdit *edit, bool invalid)
{
    edit->setProperty("invalid", invalid);
    edit->style()->unpolish(edit);
    edit->style()->polish(edit);
    edit->update();
}

bool CourseDialog::readNumber(const QLineEdit *edit, double &value) const
{
    QString text = edit->text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    bool ok = false;
    value = text.toDouble(&ok);
    return ok;
}

void CourseDialog::updatePreview()
{
    double score = 0.0;

    if (readNumber(scoreEdit_, score) && score >= 0.0 && score <= 100.0)
    {
        const GradeResult result = gradeFromScore(score);

        previewLabel_->setText(
            QStringLiteral("Grade %1  |  %2 grade points")
                .arg(result.letter, QString::number(result.points, 'f', 2)));
    }
    else
    {
        previewLabel_->setText(QStringLiteral("-"));
    }
}

Course CourseDialog::course() const
{
    double score = 0.0;
    double credits = 0.0;

    readNumber(scoreEdit_, score);
    readNumber(creditsEdit_, credits);

    return Course{nameEdit_->text().trimmed(), score, credits};
}

void CourseDialog::accept()
{
    const QString name = nameEdit_->text().trimmed();

    QString error;
    QLineEdit *badField = nullptr;

    double score = 0.0;
    double credits = 0.0;

    if (!Validation::isValidCourseName(name, &error))
    {
        badField = nameEdit_;
    }
    else if (!readNumber(scoreEdit_, score) || score < 0.0 || score > 100.0)
    {
        error = QStringLiteral("Score must be a number from 0 to 100.");
        badField = scoreEdit_;
    }
    else if (!readNumber(creditsEdit_, credits) || credits < Limits::MinCredits || credits > Limits::MaxCredits)
    {
        error = QStringLiteral("Credit hours must be a number from %1 to %2.")
                    .arg(Limits::MinCredits)
                    .arg(Limits::MaxCredits);
        badField = creditsEdit_;
    }

    if (badField)
    {
        errorLabel_->setText(error);
        setFieldInvalid(badField, true);
        badField->setFocus();
        badField->selectAll();
        return;
    }

    nameEdit_->setText(name);
    QDialog::accept();
}
