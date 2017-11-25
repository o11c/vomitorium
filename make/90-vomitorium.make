sources = \
    dump.cpp \
    dump-v1.cpp \
    events.cpp \
    hello.c \
    init.cpp \
    intern.cpp \
    iter.cpp \
    names.cpp \
    visit.cpp \
    weak.cpp \
    weak-check.cpp \
    xml.cpp \
    vomitorium.cpp
goals = lib/demo.so lib/vomitorium.so

default: all
all: ${goals}
clean:
	rm -rf bin/ lib/ obj/ stamp/
distclean: clean
	rm -rf gen/

override CFLAGS += ${CFLAGS_$@}
override CPPFLAGS += ${CPPFLAGS_$@}
override CXXFLAGS += ${CXXFLAGS_$@}
override LDFLAGS += ${LDFLAGS_$@}
override LDLIBS += ${LDLIBS_$@}
lib/demo.so: obj/demo.c.o| lib/vomitorium.so
LDFLAGS_lib/demo.so = '-Wl,-rpath=$${ORIGIN}'
LDLIBS_lib/demo.so = lib/vomitorium.so

lib/vomitorium.so: $(patsubst %,obj/%.o,${sources})
