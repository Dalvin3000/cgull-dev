#define GOOGLE_GLOG_DLL_DECL
#define GTEST_SHUFFLE 0

//#define _ENABLE_ATOMIC_ALIGNMENT_FIX
//#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS true
//#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING true

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
