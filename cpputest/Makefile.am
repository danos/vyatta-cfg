
# error.c fails to find header for vasprintf hence the use of
# no-implicit-function-declaration
AM_CFLAGS = -std=gnu99 -pedantic -O2 -g -I src -Wall -Werror \
            -I$(top_srcdir)/src/client \
            -include /usr/include/CppUTest/MemoryLeakDetectorMallocMacros.h \
            -Wno-implicit-function-declaration

AM_CXXFLAGS = -std=c++0x -O2 -g -Isrc -Wall -Werror -Wno-deprecated \
              $(cpputest_CFLAGS) -I${top_srcdir}/src/client \
              -include /usr/include/CppUTest/MemoryLeakDetectorNewMacros.h

LDADD = $(cpputest_LIBS) -lstdc++ -lm -lc -lgcc_s \
        -lvyatta-util \
        -ljansson \
        -luriparser \
        -L/usr/lib/gcc/x86_64-linux-gnu/8

check_PROGRAMS =connect_tester error_tester

connect_tester_SOURCES = connectTester.cpp \
                        testMain.cpp \
                        ../src/client/connect.c \
                        ../src/client/error.c \
                        common_mocks.c

connect_tester_LDADD = $(LDADD)

# 'wrap' allows selective mocking without having to stub everything in the
# relevant libraries.
connect_tester_LDFLAGS = -Wl,-wrap,json_loadf \
                         -Wl,-wrap,json_dumps \
                         -Wl,-wrap,write

error_tester_SOURCES = errorTester.cpp \
                       testMain.cpp \
                       ../src/client/connect.c \
                       ../src/client/error.c \
                       ../src/client/transaction.c \
                       common_mocks.c

error_tester_LDADD = $(LDADD)

error_tester_LDFLAGS = -Wl,-wrap,json_loadf \
                       -Wl,-wrap,json_dumps \
                       -Wl,-wrap,write


TESTS = $(check_PROGRAMS)
