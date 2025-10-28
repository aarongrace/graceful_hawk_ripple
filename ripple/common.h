	;address of the canvas, which should be MALLOCed
	COMMON CANVAS, C_ROWS * BYTES_IN_W


	COMMON	OBJ_LIST,4
; configuration for the linked list
LL_EMPTY	=	0	; pointer to the empty method, see stack.h
LL_PUSH	=	4	; pointer to the push method, see stack.h
LL_PEEK	=	8	; pointer to the pop method, see stack.h
LL_REMOVE	=	12	; pointer to the pop method, see stack.h

; fields of the node
N_DATA	=	0	; data that the node holds
N_NEXT	=	4	; pointer to the next node
NODESIZE=	8	; each node holds data and the next pointer

	;move_object
	COMMON	FILL_S_FIRST_LETTER,4

	COMMON	PROGRAM_CYCLE_AD, 4
	COMMON	INIT_R2,4

BYTES_IN_W = 4
; canvas parameters
C_UPPER_ROWS = 8
C_UPPER_COLUMNS = 3
C_REG_COLUMNS = 2
C_LOWER_ROWS = 8
C_LOWER_DIGS = M_LEFT_W + M_MID_W + M_THIRD_W + M_RIGHT_W
C_ROWS = C_UPPER_ROWS + C_LOWER_ROWS

C_UPPER_DIGS = 24

C_REG_DIGS = 16

M_LEFT_S = 22
M_LEFT_W = 9
M_MID_S = 34
M_MID_W = 10
M_THIRD_S = 46
M_THIRD_W = 8
M_RIGHT_S = 55
M_RIGHT_W = 8

MAX_RIPPLE_NUM = 8	
	COMMON	RIPPLE_ARRAY, MAX_RIPPLE_NUM * BYTES_IN_W
;attributes of Ripple object
RIPPLE_X=0
RIPPLE_Y=4
RIPPLE_RAD=8
RIPPLE_WIDTH=12
RIPPLE_LIFE=16
RIPPLE_SIZE=20

	COMMON	CIRCLE, 20 * 32 * BYTES_IN_W ;(provide for max radius 20)
	COMMON	CIRCLE_WITH_DUPES, 20 * 32 * BYTES_IN_W ;(provide for max radius 20)
CIRCLE_SIZE_FIELD = 0
CIRCLE_ARRAY_START = 4

; the memory display
	COMMON	MEMD_EMPTY_SPACE, 16
	ALIGN	8
	COMMON	MEMD_START, 64 ;allocate 8 rows for the MEMD
MAIN_STOP = MEMD_START+4
MEMD_END = MEMD_START+20
MEMD_ROWS = 8

;macros
MACRO TOSUBR subr
	LIL	R1,subr
	JSRS	R1,R1
ENDMAC

MACRO TOSUBR_3 subr, =r3val
	LIL	R3, r3val
	TOSUBR	subr
ENDMAC

MACRO TOSUBR_34 subr, =r3val, =r4val
	LIL	R4, r4val
	TOSUBR_3	subr,r3val
ENDMAC

MACRO TOSUBR_345 subr, =r3val, =r4val, =r5val
	LIL	R5, r5val
	TOSUBR_34	subr, r3val, r4val
ENDMAC


MACRO LOADCOM reg, com
; loads the value of com into reg
	LIL	reg, com
	LOADS	reg,reg
ENDMAC

MACRO SETCOM reg, reg2, com
; stores the value of reg2 into com
	LIL	reg, com
	STORES	reg2,reg
ENDMAC

MACRO SETCOMI reg, reg2, com, =val
; stores the value of val into com
	LIL	reg, com
	LIL	reg2, val
	STORES	reg2,reg
ENDMAC

MACRO C_S
	STORES	R1,R2
	ADDI	R2,R2,ARSIZE
ENDMAC

MACRO R_S
	ADDI	R2,R2,-ARSIZE
	LOADS	PC,R2
ENDMAC

	ALIGN	8

BACKGROUND:
	W	#11137777
	W	#11113777
	W	#11111377
	W	#11111133
	W	#01111133
	W	#00111113
	W	#00011113
	W	#00000111

BALL_TEMPLATE:
	W	#00000304
	W	#00001BAE
	W	#00002BAE
	W	#00003BAE

YOU_TEMPLATE:
	W	#00000103
	W	#00000FAB
