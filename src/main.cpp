#include <QApplication>

#include "MainWindow.h"
#include "Style.h"
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("CodeAlpha"));
    QCoreApplication::setApplicationName(QStringLiteral("CGPA Calculator"));

    // Set the application stylesheet using your beautiful QSS rules
    app.setStyleSheet(appStyleSheet());

    // Fixed light palette fallback configuration
    QPalette light;
    light.setColor(QPalette::Window, QColor("#f4f6fb"));
    light.setColor(QPalette::WindowText, QColor("#1b2559"));
    light.setColor(QPalette::Base, QColor("#ffffff"));
    light.setColor(QPalette::AlternateBase, QColor("#f7f8fd"));
    light.setColor(QPalette::Text, QColor("#1b2559"));
    light.setColor(QPalette::PlaceholderText, QColor("#8a93b8"));
    light.setColor(QPalette::Button, QColor("#ffffff"));
    light.setColor(QPalette::ButtonText, QColor("#1b2559"));
    light.setColor(QPalette::ToolTipBase, QColor("#ffffff"));
    light.setColor(QPalette::ToolTipText, QColor("#1b2559"));
    light.setColor(QPalette::Highlight, QColor("#4f6bed"));
    light.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    app.setPalette(light);

    MainWindow window;
    window.show();
    return app.exec();
}
