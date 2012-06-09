TEMPLATE = app
CONFIG += qt \
    thread
SOURCES += main.cpp \
    mainwindow.cpp \
    mesh.cpp \
    Exception.cpp \
    util.cpp \
    image.cpp \
    veclib/vecprint.cpp \
    meshlistview.cpp \
    vectorinputdialog.cpp \
    RenderedMesh.cpp \
    MeshFilesModel.cpp \
    MeshPacker.cpp
HEADERS += mainwindow.h \
    mesh.h \
    Exception.h \
    util.h \
    image.h \
    veclib/vecprint.h \
    veclib/veclib.h \
    veclib/sse-vec.h \
    veclib/sse-quat.h \
    veclib/sse-mat.h \
    veclib/soft-vec.h \
    veclib/soft-quat.h \
    veclib/soft-mat.h \
    veclib/neon-vec.h \
    veclib/common-vec.h \
    veclib/common-quat.h \
    veclib/common-mat.h \
    meshlistview.h \
    vectorinputdialog.h \
    RenderedMesh.h \
    MeshFilesModel.h \
    MeshPacker.h
QMAKE_CXXFLAGS += -std=c++11 -march=native -fopenmp
QMAKE_LFLAGS += -fopenmp
RESOURCES += \
    icons.qrc

OTHER_FILES += \
    README.txt
