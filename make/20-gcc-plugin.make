PLUGIN_CC ?= ${CC}

gcc_plugin_dir := $(call safe_shell,${PLUGIN_CC} -print-file-name=plugin)
# Huh, apparently this works even on old GCC ...
gcc_version := $(call safe_shell,${PLUGIN_CC} -dumpfullversion -dumpversion)
gcc_version_full = $(call safe_shell,${PLUGIN_CC} --version | head -n 1)
gcc_machine = $(call safe_shell,${PLUGIN_CC} -dumpmachine)

gcc_version_bits := $(wordlist 1,3,$(subst .,${space},${gcc_version}) 0 0)

$(call safe_shell,mkdir -p gen/include/vgcc)
$(call safe_shell,printf '#define VGCC_MAJOR %d\n#define VGCC_MINOR %d\n#define VGCC_PATCH %d\n' ${gcc_version_bits} > gen/include/vgcc/vgcc-version.h.tmp)
$(call safe_shell,${scripts}/move-if-changed gen/include/vgcc/vgcc-version.h.tmp gen/include/vgcc/vgcc-version.h)

override CPPFLAGS += -isystem ${gcc_plugin_dir}/include

info: info-gcc
info-gcc:
	@echo
	@echo 'Version (short): ${gcc_version}'
	@echo 'Version (bits): ${gcc_version_bits}'
	@echo 'Version (full): ${gcc_version_full}'
	@echo 'Target (w/o -m): ${gcc_machine}'
	@echo 'Plugins directory: ${gcc_plugin_dir}'
	@cd '${gcc_plugin_dir}' && ls -1 | sed -n 's/^/    /; s/\.so$$//p'
	@echo
