all_sources := $(sort $(call safe_shell, find -L ${src}/ -name '*.c' -o -name '*.cpp'))
#all_headers := $(sort $(call safe_shell, find -L ${src}/ ${include}/ -name '*.h' -o -name '*.hpp'))

test: test-objects
test-objects: $(patsubst ${src}/%,obj/%.o,${all_sources})
