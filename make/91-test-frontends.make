test: test-frontends
#test-frontends: stamp/test-frontend-ada.stamp XXX rejects outputting to /dev/null
#test-frontends: stamp/test-frontend-brig.stamp XXX takes a binary format, empty file not valid
test-frontends: stamp/test-frontend-c.stamp
test-frontends: stamp/test-frontend-c++.stamp
test-frontends: stamp/test-frontend-objective-c.stamp
test-frontends: stamp/test-frontend-objective-c++.stamp
test-frontends: stamp/test-frontend-d.stamp
test-frontends: stamp/test-frontend-f95.stamp
test-frontends: stamp/test-frontend-go.stamp
#test-frontends: stamp/test-frontend-java.stamp XXX can't find a classpath?

EMPTY_FILE = ${src}/test-data/empty
# Needs a real input file
#stamp/test-frontend-brig.stamp: EMPTY_FILE=${src}/test-data/empty.brig
stamp/test-frontend-d.stamp: EMPTY_FILE=${src}/test-data/empty.d
stamp/test-frontend-go.stamp: EMPTY_FILE=${src}/test-data/empty.go
# Avoid a crash.
stamp/test-frontend-java.stamp: EMPTY_FILE=${src}/test-data/empty.java

stamp/test-frontend-%.stamp: lib/vomitorium.so
# There are 2 errors we want to skip for:
# - frontend is not available for this compiler version
# - frontend is not installed
# If the frontend *is* installed, the first command's grep fails.
	${CC} -c -x $* ${EMPTY_FILE} -o /dev/null 2>&1 | grep -q -e 'language .* not recognized' -e 'error trying to exec .*: execvp: No such file or directory' || \
	    ${CC} -c -fplugin=lib/vomitorium.so -x $* ${EMPTY_FILE} -o /dev/null
	touch $@
