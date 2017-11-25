test: test-use
test-use: stamp/test-use.stamp
stamp/test-use.stamp: lib/vomitorium.so
	${CC} -c -fplugin=lib/vomitorium.so -fplugin-arg-vomitorium-hello -fplugin-arg-vomitorium-info ${src}/test-data/hello-world.c -c -o /dev/null
	${CXX} -x c++ -c -fplugin=lib/vomitorium.so -fplugin-arg-vomitorium-hello -fplugin-arg-vomitorium-info ${src}/test-data/hello-world.c -c -o /dev/null
	touch $@
test: test-demo
test-demo: stamp/test-demo.stamp
stamp/test-demo.stamp: all
	${CC} -c -fplugin=lib/vomitorium.so -fplugin=lib/demo.so ${src}/test-data/hello-world.c -o /dev/null
	${CC} -c -fplugin=lib/demo.so -fplugin=lib/vomitorium.so ${src}/test-data/hello-world.c -o /dev/null >/dev/null 2>&1
	# Improper initialization should fail.
	! ${CC} -c -fplugin=lib/demo.so ${src}/test-data/hello-world.c -o /dev/null >/dev/null 2>&1
	touch $@
