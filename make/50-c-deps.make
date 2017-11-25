override CPPFLAGS += -MMD

-include $(if $(wildcard obj),$(call safe_shell,find obj -name '*.d'))
