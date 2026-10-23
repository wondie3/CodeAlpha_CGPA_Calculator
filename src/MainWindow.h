#pragma once

#include <QMainWindow>
#include "Grading.h"

class QCloseEvent;
class QLabel;
class QLineEdit;
class QTabWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void buildMenus();
    QWidget *buildSemesterPage();

    void loadInitialData();
    void resetTo(const Transcript &transcript);
    void newTranscript();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    bool saveTo(const QString &path);
    void exportCsv();
    bool maybeSave();

    void addSemester();
    void removeSemester();
    void renameSemester(int index);
    void addCourse(int semester);
    void editCourse(int semester, int row);
    void deleteCourse(int semester, int row);

    void refreshTab(int index);
    void refreshSummary();
    void dataChanged(int semester);
    bool validateName(bool requireNonEmpty);
    void setDirty(bool dirty);
    void updateTitle();

    Transcript transcript_;
    QString currentPath_;
    bool dirty_ = false;

    QLineEdit *nameEdit_ = nullptr;
    QLabel *nameError_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    QLabel *totalCreditsLabel_ = nullptr;
    QLabel *totalPointsLabel_ = nullptr;
    QLabel *cgpaLabel_ = nullptr;
};
