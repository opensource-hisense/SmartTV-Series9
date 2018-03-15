
TVTEE_DIR		:=	services/spd/tvtee
SPD_INCLUDES		:=	-Iinclude/bl32/payloads \
				-I${TVTEE_DIR}/include \
				-I${TVTEE_DIR}

SPD_SOURCES		:=	${TVTEE_DIR}/tvtee_common.c		\
				${TVTEE_DIR}/tvtee_helpers.S	\
				${TVTEE_DIR}/tvtee_main.c		\
				${TVTEE_DIR}/tvtee_pm.c

TVTEE32 := 1
$(eval $(call add_define,TVTEE32))

vpath %.c ${TVTEE_DIR}
vpath %.S ${TVTEE_DIR}

