
TRUSTY_DIR		:=	services/spd/trusty
SPD_INCLUDES		:=	-Iinclude/bl32/payloads \
				-I${TRUSTY_DIR}/include \
				-I${TRUSTY_DIR}

SPD_SOURCES		:= ${TRUSTY_DIR}/trusty_helpers.S	\
				${TRUSTY_DIR}/trusty.c \

TVTEE32 := 1
$(eval $(call add_define,TVTEE32))

vpath %.c ${TRUSTY_DIR}
vpath %.S ${TRUSTY_DIR}

