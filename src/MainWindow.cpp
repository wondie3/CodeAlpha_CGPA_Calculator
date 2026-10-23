#include "MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "CourseDialog.h"
#include "Storage.h"

namespace {

QString fmt(double v) { return QString::number(v, 'f', 2); }

void repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
}

QTableWidgetItem *makeItem(const QString &text, bool center)
{
    auto *item = new QTableWidgetItem(text);
    if (center) item->setTextAlignment(Qt::AlignCenter);
    return item;
}

Transcript emptyTranscript()
{
    Transcript t;
    t.semesters.append(Semester{QStringLiteral("Semester 1"), {}});
    return t;
}

QLabel *makeLabel(const QString &text, const char *objectName)
{
    auto *l = new QLabel(text);
    l->setObjectName(QLatin1String(objectName));
    return l;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(780, 580);
    resize(960, 680);
    buildUi();
    buildMenus();
    currentPath_ = Storage::defaultDataPath();
    loadInitialData();
}

// ---------------------------------------------------------------- UI construction

void MainWindow::buildUi()
{
    auto *central = new QWidget;
    central->setObjectName(QStringLiteral("central"));
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(10);

    // Student name row
    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(makeLabel(QStringLiteral("Student name"), "fieldLabel"));
    nameEdit_ = new QLineEdit;
    nameEdit_->setMaxLength(50);
    nameEdit_->setPlaceholderText(QStringLiteral("Enter the student's full name"));
    nameRow->addWidget(nameEdit_, 1);
    root->addLayout(nameRow);

    nameError_ = makeLabel(QString(), "errorLabel");
    nameError_->setVisible(false);
    root->addWidget(nameError_);

    connect(nameEdit_, &QLineEdit::textChanged, this, [this](const QString &text) {
        transcript_.studentName = text.trimmed();
        validateName(false);
        setDirty(true);
    });

    // Semester tabs with add/remove buttons in the corner
    tabs_ = new QTabWidget;
    tabs_->setDocumentMode(true);
    tabs_->setUsesScrollButtons(true);
    tabs_->setToolTip(QStringLiteral("Double-click a tab to rename the semester"));

    auto *corner = new QWidget;
    auto *cornerLayout = new QHBoxLayout(corner);
    cornerLayout->setContentsMargins(0, 0, 0, 4);
    auto *addSem = new QPushButton(QStringLiteral("+ Semester"));
    addSem->setObjectName(QStringLiteral("secondary"));
    auto *removeSem = new QPushButton(QStringLiteral("Remove Semester"));
    removeSem->setObjectName(QStringLiteral("danger"));
    cornerLayout->addWidget(addSem);
    cornerLayout->addWidget(removeSem);
    tabs_->setCornerWidget(corner, Qt::TopRightCorner);
    connect(addSem, &QPushButton::clicked, this, &MainWindow::addSemester);
    connect(removeSem, &QPushButton::clicked, this, &MainWindow::removeSemester);
    connect(tabs_->tabBar(), &QTabBar::tabBarDoubleClicked, this, &MainWindow::renameSemester);
    root->addWidget(tabs_, 1);

    // Summary card
    auto *summary = new QGroupBox;
    summary->setObjectName(QStringLiteral("summary"));
    auto *summaryLayout = new QHBoxLayout(summary);
    summaryLayout->setContentsMargins(24, 14, 24, 14);

    auto addStat = [&](const QString &caption, const char *valueName, QLabel *&valueLabel) {
        auto *box = new QVBoxLayout;
        box->setSpacing(2);
        box->addWidget(makeLabel(caption, "statCaption"));
        valueLabel = makeLabel(QStringLiteral("0.00"), valueName);
        box->addWidget(valueLabel);
        summaryLayout->addLayout(box);
    };
    addStat(QStringLiteral("Total credits"), "statValue", totalCreditsLabel_);
    summaryLayout->addStretch(1);
    addStat(QStringLiteral("Total grade points"), "statValue", totalPointsLabel_);
    summaryLayout->addStretch(1);
    addStat(QStringLiteral("Overall CGPA (out of 4.00)"), "cgpaValue", cgpaLabel_);
    root->addWidget(summary);

    statusBar()->showMessage(QStringLiteral("Ready"));
}

void MainWindow::buildMenus()
{
    auto add = [this](QMenu *menu, const QString &text, const QKeySequence &shortcut,
                      void (MainWindow::*slot)()) {
        QAction *action = menu->addAction(text);
        if (!shortcut.isEmpty()) action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, slot);
        return action;
    };

    QMenu *file = menuBar()->addMenu(QStringLiteral("&File"));
    add(file, QStringLiteral("&New"), QKeySequence::New, &MainWindow::newTranscript);
    add(file, QStringLiteral("&Open..."), QKeySequence::Open, &MainWindow::openFile);
    file->addSeparator();
    QAction *save = file->addAction(QStringLiteral("&Save"));
    save->setShortcut(QKeySequence::Save);
    connect(save, &QAction::triggered, this, [this] { saveFile(); });
    QAction *saveAs = file->addAction(QStringLiteral("Save &As..."));
    saveAs->setShortcut(QKeySequence::SaveAs);
    connect(saveAs, &QAction::triggered, this, [this] { saveFileAs(); });
    file->addSeparator();
    add(file, QStringLiteral("&Export to CSV (Excel)..."), QKeySequence(Qt::CTRL | Qt::Key_E),
        &MainWindow::exportCsv);
    file->addSeparator();
    QAction *quit = file->addAction(QStringLiteral("&Quit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(QStringLiteral("&Semester"));
    add(edit, QStringLiteral("&Add semester"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N),
        &MainWindow::addSemester);
    add(edit, QStringLiteral("&Remove current semester"), QKeySequence(), &MainWindow::removeSemester);
    QAction *addCourseAct = edit->addAction(QStringLiteral("Add &course"));
    addCourseAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(addCourseAct, &QAction::triggered, this, [this] { addCourse(tabs_->currentIndex()); });
}

QWidget *MainWindow::buildSemesterPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 12, 0, 0);
    layout->setSpacing(10);

    auto *table = new QTableWidget(0, 5);
    table->setObjectName(QStringLiteral("courseTable"));
    table->setHorizontalHeaderLabels({QStringLiteral("Course"), QStringLiteral("Score"),
                                      QStringLiteral("Grade"), QStringLiteral("Grade Points"),
                                      QStringLiteral("Credit Hours")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int i = 1; i < 5; ++i)
        table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);

    auto *addBtn = new QPushButton(QStringLiteral("Add Course"));
    auto *editBtn = new QPushButton(QStringLiteral("Edit Selected"));
    editBtn->setObjectName(QStringLiteral("secondary"));
    auto *delBtn = new QPushButton(QStringLiteral("Delete Selected"));
    delBtn->setObjectName(QStringLiteral("danger"));
    auto *gpaLabel = makeLabel(QString(), "semesterGpa");

    auto *bar = new QHBoxLayout;
    bar->addWidget(addBtn);
    bar->addWidget(editBtn);
    bar->addWidget(delBtn);
    bar->addStretch(1);
    bar->addWidget(gpaLabel);
    layout->addLayout(bar);
    layout->addWidget(table, 1);

    auto indexOfPage = [this, page] { return tabs_->indexOf(page); };
    connect(addBtn, &QPushButton::clicked, this, [=] { addCourse(indexOfPage()); });
    connect(editBtn, &QPushButton::clicked, this, [=] { editCourse(indexOfPage(), table->currentRow()); });
    connect(delBtn, &QPushButton::clicked, this, [=] { deleteCourse(indexOfPage(), table->currentRow()); });
    connect(table, &QTableWidget::cellDoubleClicked, this, [=](int row, int) { editCourse(indexOfPage(), row); });
    return page;
}

// ---------------------------------------------------------------- data and files

void MainWindow::loadInitialData()
{
    if (QFile::exists(currentPath_)) {
        const Storage::LoadResult res = Storage::loadTranscript(currentPath_);
        if (res.ok) {
            resetTo(res.data);
            return;
        }
        // Keep the unreadable file instead of overwriting it on the next save.
        const QString bad = currentPath_ + QStringLiteral(".corrupt");
        QFile::remove(bad);
        QFile::rename(currentPath_, bad);
        QMessageBox::warning(this, QStringLiteral("Saved data could not be loaded"),
                             QStringLiteral("%1\n\nThe unreadable file was kept as:\n%2\n\nStarting with an empty record.")
                                 .arg(res.error, bad));
    }
    resetTo(emptyTranscript());
}

void MainWindow::resetTo(const Transcript &t)
{
    transcript_ = t;
    if (transcript_.semesters.isEmpty())
        transcript_.semesters.append(Semester{QStringLiteral("Semester 1"), {}});

    {
        const QSignalBlocker blocker(nameEdit_);
        nameEdit_->setText(transcript_.studentName);
    }
    validateName(false);

    while (tabs_->count() > 0) {
        QWidget *w = tabs_->widget(0);
        tabs_->removeTab(0);
        delete w;
    }
    for (const Semester &s : std::as_const(transcript_.semesters))
        tabs_->addTab(buildSemesterPage(), s.title);

    for (int i = 0; i < tabs_->count(); ++i) refreshTab(i);
    refreshSummary();
    setDirty(false);
}

void MainWindow::newTranscript()
{
    if (!maybeSave()) return;
    currentPath_.clear(); // first save asks where to put the new file
    resetTo(emptyTranscript());
    statusBar()->showMessage(QStringLiteral("New record created"), 3000);
}

void MainWindow::openFile()
{
    if (!maybeSave()) return;
    const QString start = currentPath_.isEmpty() ? QDir::homePath() : QFileInfo(currentPath_).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Open CGPA record"), start,
                                                      QStringLiteral("CGPA records (*.json);;All files (*)"));
    if (path.isEmpty()) return;

    const Storage::LoadResult res = Storage::loadTranscript(path);
    if (!res.ok) {
        // The current data stays untouched when a file cannot be loaded.
        QMessageBox::critical(this, QStringLiteral("Could not open file"), res.error);
        return;
    }
    currentPath_ = path;
    resetTo(res.data);
    statusBar()->showMessage(QStringLiteral("Opened %1").arg(path), 4000);
}

bool MainWindow::saveFile()
{
    if (currentPath_.isEmpty()) return saveFileAs();
    return saveTo(currentPath_);
}

bool MainWindow::saveFileAs()
{
    if (!validateName(true)) {
        nameEdit_->setFocus();
        QMessageBox::warning(this, QStringLiteral("Student name needed"),
                             nameError_->text().isEmpty() ? QStringLiteral("Please enter a valid student name.")
                                                          : nameError_->text());
        return false;
    }
    const QString start = currentPath_.isEmpty()
        ? QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath(QStringLiteral("cgpa_record.json"))
        : currentPath_;
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save CGPA record"), start,
                                                QStringLiteral("CGPA records (*.json)"));
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".json");
    return saveTo(path);
}

bool MainWindow::saveTo(const QString &path)
{
    if (!validateName(true)) {
        nameEdit_->setFocus();
        QMessageBox::warning(this, QStringLiteral("Student name needed"),
                             nameError_->text().isEmpty() ? QStringLiteral("Please enter a valid student name.")
                                                          : nameError_->text());
        return false;
    }
    transcript_.studentName = nameEdit_->text().trimmed();

    QString error;
    if (!Storage::saveTranscript(transcript_, path, &error)) {
        QMessageBox::critical(this, QStringLiteral("Save failed"), error);
        return false;
    }
    currentPath_ = path;
    setDirty(false);
    statusBar()->showMessage(QStringLiteral("Saved to %1").arg(path), 4000);
    return true;
}

void MainWindow::exportCsv()
{
    if (!validateName(true)) {
        nameEdit_->setFocus();
        QMessageBox::warning(this, QStringLiteral("Student name needed"),
                             nameError_->text().isEmpty() ? QStringLiteral("Please enter a valid student name.")
                                                          : nameError_->text());
        return;
    }
    QString base = nameEdit_->text().trimmed();
    base.replace(QRegularExpression(QStringLiteral("[^\\w-]+")), QStringLiteral("_"));
    const QString start = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                              .filePath(base + QStringLiteral("_CGPA.csv"));
    QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export to CSV"), start,
                                                QStringLiteral("CSV files (*.csv)"));
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += QStringLiteral(".csv");

    QString error;
    if (!Storage::exportCsv(transcript_, path, &error)) {
        QMessageBox::critical(this, QStringLiteral("Export failed"), error);
        return;
    }
    statusBar()->showMessage(QStringLiteral("Exported to %1").arg(path), 5000);
}

bool MainWindow::maybeSave()
{
    if (!dirty_) return true;
    const auto answer = QMessageBox::question(
        this, QStringLiteral("Unsaved changes"),
        QStringLiteral("Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) return saveFile();
    return answer == QMessageBox::Discard;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) event->accept();
    else event->ignore();
}

// ---------------------------------------------------------------- semesters and courses

void MainWindow::addSemester()
{
    if (transcript_.semesters.size() >= Limits::MaxSemesters) {
        QMessageBox::information(this, QStringLiteral("Limit reached"),
                                 QStringLiteral("You can have at most %1 semesters.").arg(Limits::MaxSemesters));
        return;
    }
    const QString title = QStringLiteral("Semester %1").arg(transcript_.semesters.size() + 1);
    transcript_.semesters.append(Semester{title, {}});
    const int index = tabs_->addTab(buildSemesterPage(), title);
    tabs_->setCurrentIndex(index);
    refreshTab(index);
    refreshSummary();
    setDirty(true);
}

void MainWindow::removeSemester()
{
    const int index = tabs_->currentIndex();
    if (index < 0) return;
    if (transcript_.semesters.size() <= 1) {
        QMessageBox::information(this, QStringLiteral("Cannot remove"),
                                 QStringLiteral("At least one semester is required."));
        return;
    }
    const Semester &s = transcript_.semesters.at(index);
    if (!s.courses.isEmpty()) {
        const auto answer = QMessageBox::question(
            this, QStringLiteral("Remove semester"),
            QStringLiteral("Remove \"%1\" and its %2 course(s)?").arg(s.title).arg(s.courses.size()));
        if (answer != QMessageBox::Yes) return;
    }
    transcript_.semesters.remove(index);
    QWidget *page = tabs_->widget(index);
    tabs_->removeTab(index);
    page->deleteLater();
    refreshSummary();
    setDirty(true);
}

void MainWindow::renameSemester(int index)
{
    if (index < 0 || index >= transcript_.semesters.size()) return;
    QString current = transcript_.semesters.at(index).title;
    while (true) {
        bool ok = false;
        const QString title = QInputDialog::getText(this, QStringLiteral("Rename semester"),
                                                    QStringLiteral("Semester name:"), QLineEdit::Normal,
                                                    current, &ok).trimmed();
        if (!ok) return;
        QString error;
        if (Validation::isValidSemesterTitle(title, &error)) {
            transcript_.semesters[index].title = title;
            tabs_->setTabText(index, title);
            setDirty(true);
            return;
        }
        QMessageBox::warning(this, QStringLiteral("Invalid name"), error);
        current = title;
    }
}

void MainWindow::addCourse(int si)
{
    if (si < 0 || si >= transcript_.semesters.size()) return;
    Semester &sem = transcript_.semesters[si];
    if (sem.courses.size() >= Limits::MaxCoursesPerSemester) {
        QMessageBox::information(this, QStringLiteral("Limit reached"),
                                 QStringLiteral("A semester can have at most %1 courses.")
                                     .arg(Limits::MaxCoursesPerSemester));
        return;
    }
    CourseDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        sem.courses.append(dialog.course());
        dataChanged(si);
    }
}

void MainWindow::editCourse(int si, int row)
{
    if (si < 0 || si >= transcript_.semesters.size()) return;
    Semester &sem = transcript_.semesters[si];
    if (row < 0 || row >= sem.courses.size()) {
        statusBar()->showMessage(QStringLiteral("Select a course first"), 3000);
        return;
    }
    const Course existing = sem.courses.at(row);
    CourseDialog dialog(this, &existing);
    if (dialog.exec() == QDialog::Accepted) {
        sem.courses[row] = dialog.course();
        dataChanged(si);
    }
}

void MainWindow::deleteCourse(int si, int row)
{
    if (si < 0 || si >= transcript_.semesters.size()) return;
    Semester &sem = transcript_.semesters[si];
    if (row < 0 || row >= sem.courses.size()) {
        statusBar()->showMessage(QStringLiteral("Select a course first"), 3000);
        return;
    }
    const auto answer = QMessageBox::question(
        this, QStringLiteral("Delete course"),
        QStringLiteral("Delete \"%1\"?").arg(sem.courses.at(row).name));
    if (answer != QMessageBox::Yes) return;
    sem.courses.remove(row);
    dataChanged(si);
}

// ---------------------------------------------------------------- view refresh

void MainWindow::dataChanged(int semester)
{
    refreshTab(semester);
    refreshSummary();
    setDirty(true);
}

void MainWindow::refreshTab(int index)
{
    if (index < 0 || index >= tabs_->count() || index >= transcript_.semesters.size()) return;
    QWidget *page = tabs_->widget(index);
    auto *table = page->findChild<QTableWidget *>(QStringLiteral("courseTable"));
    auto *gpaLabel = page->findChild<QLabel *>(QStringLiteral("semesterGpa"));
    const Semester &sem = transcript_.semesters.at(index);

    table->setRowCount(sem.courses.size());
    for (int r = 0; r < sem.courses.size(); ++r) {
        const Course &c = sem.courses.at(r);
        const GradeResult g = c.grade();
        table->setItem(r, 0, makeItem(c.name, false));
        table->setItem(r, 1, makeItem(fmt(c.score), true));
        QTableWidgetItem *letter = makeItem(g.letter, true);
        if (g.letter == QLatin1String("F")) letter->setForeground(QColor(0xd6, 0x45, 0x45));
        table->setItem(r, 2, letter);
        table->setItem(r, 3, makeItem(fmt(g.points), true));
        table->setItem(r, 4, makeItem(fmt(c.credits), true));
    }
    gpaLabel->setText(QStringLiteral("Semester GPA: %1   |   Credits: %2")
                          .arg(fmt(sem.gpa()), fmt(sem.totalCredits())));
}

void MainWindow::refreshSummary()
{
    totalCreditsLabel_->setText(fmt(transcript_.totalCredits()));
    totalPointsLabel_->setText(fmt(transcript_.totalPoints()));
    cgpaLabel_->setText(fmt(transcript_.cgpa()));
}

bool MainWindow::validateName(bool requireNonEmpty)
{
    const QString name = nameEdit_->text().trimmed();
    QString error;
    bool ok = true;
    if (name.isEmpty()) {
        ok = !requireNonEmpty;
        error = QStringLiteral("Student name is required.");
    } else {
        ok = Validation::isValidPersonName(name, &error);
    }
    nameEdit_->setProperty("invalid", !ok);
    repolish(nameEdit_);
    nameError_->setText(ok ? QString() : error);
    nameError_->setVisible(!ok);
    return ok;
}

void MainWindow::setDirty(bool dirty)
{
    dirty_ = dirty;
    updateTitle();
}

void MainWindow::updateTitle()
{
    const QString file = currentPath_.isEmpty() ? QStringLiteral("Untitled") : QFileInfo(currentPath_).fileName();
    setWindowTitle(QStringLiteral("CGPA Calculator - %1[*]").arg(file));
    setWindowModified(dirty_);
}
