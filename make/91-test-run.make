test-run: stamp/test-xml.run

bin/test-xml.x: obj/test-run/test-xml.cpp.o obj/xml.cpp.o

stamp/%.run: bin/%.x
	@mkdir -p ${@D}
	$< > $@
