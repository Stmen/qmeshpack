TEMPLATE = app
VERSION = 1.0.0xB3
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
QT += opengl
CONFIG += gui qt thread exceptions
CONFIG(release, debug|release): DEFINES += NDEBUG
QMAKE_CXXFLAGS = -std=c++11 -march=core2 -fopenmp
QMAKE_LFLAGS += -fopenmp
SOURCES += main.cpp \
    mainwindow.cpp \
    Exception.cpp \
    util.cpp \
    vectorinputdialog.cpp \
    GLView.cpp \
    Node.cpp \
    ModelView.cpp \
    WorkerThread.cpp \
    Mesh.cpp \
    Image.cpp \
    NodeModel.cpp
HEADERS += mainwindow.h \
    Exception.h \
    util.h \
    vectorinputdialog.h \
    GLView.h \
    Node.h \
    ModelView.h \
    config.h \
    WorkerThread.h \
    Mesh.h \
    Image.h \
    NodeModel.h
RESOURCES += \
    icons.qrc

OTHER_FILES += \
    README.txt
