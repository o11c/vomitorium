test: test-void
test-void: stamp/test-void.stamp
stamp/test-void.stamp: ${include}/vomitorium.h
	! grep -q '()' ${include}/vomitorium.h
	touch $@
