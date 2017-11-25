empty =
space = ${empty} ${empty}
comma = ,

# Debug a variable, using unexpanded value.
define vdebug
$(info DEBUG $1 = $(value $1))
endef
# Debug a variable, using expanded value.
define xdebug
$(info DEBUG $1 := ${$1})
endef
# Both.
define debug
$(call vdebug,$1)\
$(call xdebug,$1)
endef

# Produce a variable, but also tell how.
# Note: this does not work properly for $1 etc.
define info_eval
$(info EVAL $1)\
$(eval $1)
endef
define info_val
$(info VAL $1)\
$($1)
endef

define info_set_var
$(info noquote $1 = $2)\
$(eval $1 := $$(value 2))
endef
define set_var
$(eval $1 := $$(value 2))
endef

# Safely call the shell, calling $(error) with the exit status if it fails.
# (assumes a sh-like shell)
define do_safe_shell
$(call set_var,shell_cmd,{ $1; }; echo $$?)\
$(call set_var,shell_tmp,$(shell ${shell_cmd}))\
$(call set_var,shell_status,$(lastword ${shell_tmp}))\
$(if $(patsubst 0,,${shell_status}),$(error ${SHELL} returned status ${shell_status} from: ${1}))\
$(call set_var,shell_output,$(wordlist 2,$(words ${shell_tmp}),bogus ${shell_tmp}))\
${shell_output}
endef
define safe_shell
$(strip $(call do_safe_shell,$1))
endef
