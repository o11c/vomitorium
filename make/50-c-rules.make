# not worth clearing this for executables
override CFLAGS += -fPIC
override CXXFLAGS += -fPIC

obj/%.c.o: ${src}/%.c
	@mkdir -p ${@D}
	${CC} ${CFLAGS} ${CPPFLAGS} -c -o $@ $<
obj/%.cpp.o: ${src}/%.cpp
	@mkdir -p ${@D}
	${CXX} ${CXXFLAGS} ${CPPFLAGS} -c -o $@ $<

lib/%.so:
	@mkdir -p ${@D}
	$(if $(filter %.cpp.o,$^),${CXX},${CC}) -shared ${LDFLAGS} $^ ${LDLIBS} -o $@

bin/%.x:
	@mkdir -p ${@D}
	$(if $(filter %.cpp.o,$^),${CXX},${CC}) ${LDFLAGS} $^ ${LDLIBS} -o $@
