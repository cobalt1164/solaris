#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Solaris - Game Boy Advance Emulator");
    mainWindow.resize(640, 480);

    // Apply basic dark styling matching the CHIP-8 app look
    mainWindow.setStyleSheet(
        "QMainWindow { background-color: #121214; }"
        "QLabel { color: #e0e0e6; font-family: 'Segoe UI', Arial, sans-serif; }"
    );

    QWidget* centralWidget = new QWidget(&mainWindow);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    
    QLabel* label = new QLabel("Solaris GBA Emulator Skeleton Ready", centralWidget);
    label->setAlignment(Qt::AlignCenter);
    
    QFont font = label->font();
    font.setPointSize(16);
    font.setBold(true);
    label->setFont(font);
    
    layout->addWidget(label);
    centralWidget->setLayout(layout);
    
    mainWindow.setCentralWidget(centralWidget);
    mainWindow.show();

    return app.exec();
}
