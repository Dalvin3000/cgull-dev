#define GOOGLE_GLOG_DLL_DECL
#define GTEST_SHUFFLE 0

#include <gtest/gtest.h>

#include <QCoreApplication>



int main(int argc, char** argv)
{
#if defined(CGULL_QT)
    new QCoreApplication(argc, argv);
#endif
    ::testing::InitGoogleTest(&argc, argv);

    auto result = RUN_ALL_TESTS();

#if defined(CGULL_QT)
    delete qApp;
#endif

    return result;
}

#include "tests.h"

#if defined(CGULL_QT)
#include "tests_qt.h"
#endif

#if defined(CGULL_UV)
#include "tests_uv.h"
#endif
