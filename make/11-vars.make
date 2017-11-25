WARNINGS = -Werror -Wall -Wextra -Wmissing-declarations -Wredundant-decls
WARNINGS += -Wshadow -Wundef
C_WARNINGS = -Wc++-compat -Wmissing-prototypes
CXX_WARNINGS =
CFLAGS += ${WARNINGS} ${C_WARNINGS}
CXXFLAGS += ${WARNINGS} ${CXX_WARNINGS}

override CPPFLAGS += -I ${include} -I gen/include -I ${src}

override CFLAGS += -fvisibility=hidden
override CXXFLAGS += -fvisibility=hidden

override CFLAGS += -std=c99
override CXXFLAGS += -std=c++0x
