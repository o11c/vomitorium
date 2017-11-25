test: test-dump
test-dump: test-dump.xml
test-dump.xml: lib/vomitorium.so
	# TODO test dumps for multiple languages
	${CC} -c -fplugin=lib/vomitorium.so -fplugin-arg-vomitorium-dump -fplugin-arg-vomitorium-output=$@ ${src}/test-data/hello-world.c -o /dev/null

