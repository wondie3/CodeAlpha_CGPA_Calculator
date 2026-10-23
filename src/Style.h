#pragma once

#include <QString>

inline QString appStyleSheet()
{
    return QStringLiteral(R"(
QWidget { color: #1b2559; font-size: 10pt; }
QMainWindow, QDialog, QWidget#central { background: #f4f6fb; }
QMenuBar { background: #ffffff; }
QMenuBar::item:selected { background: #dfe5ff; }
QMenu { background: #ffffff; border: 1px solid #d9deef; }
QMenu::item:selected { background: #dfe5ff; color: #1b2559; }
QStatusBar { background: #ffffff; color: #5a648f; }

QLabel#fieldLabel { font-weight: 600; }
QLabel#errorLabel { color: #d64545; }
QLabel#hint { color: #5a648f; }
QLabel#semesterGpa { font-weight: 600; color: #33407a; }

QLineEdit, QDoubleSpinBox {
       padding: 6px 8px; background: #ffffff; color: #1b2559;
    border: 1px solid #c5cbe0; border-radius: 6px;
    selection-background-color: #4f6bed; selection-color: #ffffff;
}
QLineEdit:focus, QDoubleSpinBox:focus { border: 1px solid #4f6bed; }
QLineEdit[invalid="true"] { border: 1px solid #d64545; background: #fff5f5; }

QPushButton {
    background: #4f6bed; color: #ffffff; border: none;
    border-radius: 6px; padding: 7px 16px; font-weight: 600;
}
QPushButton:hover { background: #3f58d6; }
QPushButton:pressed { background: #3248b8; }
QPushButton:disabled { background: #aab3d6; }
QPushButton#secondary { background: #ffffff; color: #33407a; border: 1px solid #c5cbe0; }
QPushButton#secondary:hover { background: #eef1fb; }
QPushButton#danger { background: #ffffff; color: #d64545; border: 1px solid #e8b4b4; }
QPushButton#danger:hover { background: #fff0f0; }

QTableWidget {
    background: #ffffff; alternate-background-color: #f7f8fd;
    gridline-color: #e3e6f2; border: 1px solid #d9deef; border-radius: 8px;
    selection-background-color: #dfe5ff; selection-color: #1b2559;
}
QHeaderView::section {
    background: #eef1fb; color: #33407a; padding: 8px;
    border: none; border-bottom: 1px solid #d9deef; font-weight: 600;
}

QTabWidget::pane { border: none; }
QTabBar::tab {
    padding: 8px 18px; margin-right: 3px; background: #e6e9f5; color: #5a648f;
    border-top-left-radius: 8px; border-top-right-radius: 8px;
}
QTabBar::tab:selected { background: #ffffff; color: #1b2559; font-weight: 600; }
QTabBar::tab:hover:!selected { background: #dfe3f3; }

QGroupBox#summary { background: #ffffff; border: 1px solid #d9deef; border-radius: 10px; }
QLabel#statCaption { color: #5a648f; }
QLabel#statValue { font-size: 16pt; font-weight: 700; }
QLabel#cgpaValue { font-size: 24pt; font-weight: 700; color: #4f6bed; }
)");
}
