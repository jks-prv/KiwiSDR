// Copyright (c) 2013-2025 John Seamons, ZL4VO/KF6VO

#ifndef _ASM_H_
#define _ASM_H_

#include "../types.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

extern char ASCII[256][8];
void init_ASCII();

#define N_FN 64
#define NIFILES_LIST 32
extern char ifiles_list[NIFILES_LIST][N_FN];

#define NIFILES_NEST 3
extern char ifiles[NIFILES_NEST][N_FN];

extern int ifn, ifl, curline, debug;
extern char *fn, *bfs, *cfs, *hfs, *vfs, *efs;

#define	assert(cond) _assert(cond, # cond, __FILE__, __LINE__);


// string pool

typedef struct {
	char *str;
	int flags, val;
} strs_t;

#define	SF_LABEL	0x01
#define	SF_DEFINED	0x02

void string_enter(char *string, char **pointer, int flags);
strs_t *string_find(char *string);
void string_dump();
int num_strings();


// tokens

typedef enum {
	TT_EOL=0, TT_LABEL, TT_SYM, TT_NUM, TT_OPC, TT_PRE, TT_OPR, TT_DATA, TT_STRUCT, TT_ITER,
	TT_DEF, TT_FILE, TT_STATS, TT_ALIGN
} token_type_e;

const char * const ttype_s[] = {
    "EOL", "LABEL", "SYM", "NUM", "OPC", "PRE", "OPR", "DATA", "STRUCT", "ITER",
    "DEF", "FILE", "STATS", "ALIGN"
};

#define	TF_HEX		0x0001
#define	TF_RET		0x0002
#define	TF_CIN		0x0004
#define	TF_LOOP		0x0008

#define	TF_CFG_H	0x0100
#define	TF_DOT_H	0x0200
#define	TF_DOT_VP	0x0400
#define	TF_DOT_VB	0x0800

#define	TF_FIELD	0x1000
#define	TF_2OPR		0x2000
#define	TF_1OPR		0x4000

typedef struct {
	token_type_e ttype;
	char *str;
	int num;
	union {
	    int width;
	    int ifl;
	};
	int d_flags, flags;
} tokens_t;

extern tokens_t *tp_start, *tp_end;

const char *ttype(token_type_e ttype_e);
char *token(tokens_t *tp);
void token_dump(tokens_t *tp);
void dump_tokens(const char *pass, tokens_t *f, tokens_t *l);
void dump_tokens_until_eol(const char *id, tokens_t *f);
void dump_tokens_incl_eol(const char *id, tokens_t *f);
void insert(int n, tokens_t *tp, tokens_t **ep);
void pullup(tokens_t *dp, tokens_t *sp, tokens_t **ep);
void sym_undefined(tokens_t *tp);


// pre-processor

// pre-processor directives
#define	PP_DEF		0
#define	PP_MACRO	1
#define	PP_ENDM		2
#define	PP_REPEAT	3
#define	PP_ENDR		4
#define	PP_STRUCT	5
#define	PP_ENDS		6
#define	PP_FACTOR	7
#define	PP_ENDF		8
#define	PP_FCALL	9
#define	PP_IF		10
#define	PP_ELIF		11
#define	PP_ELSE		12
#define	PP_ENDIF	13
#define	PP_ERROR	14
#define	PP_DISPLAY	15

// pre-processor operators
#define	OPR_INC		0
#define	OPR_ADD		1
#define	OPR_DEC		2
#define	OPR_SUB		3
#define	OPR_MUL		4
#define	OPR_DIV		5
#define	OPR_SHL		6
#define	OPR_SHR		7
#define	OPR_LAND    8
#define	OPR_AND		9
#define	OPR_LOR		10
#define	OPR_OR		11
#define	OPR_EQ		12
#define	OPR_NEQ		13
#define	OPR_NOT		14
#define	OPR_SIZEOF	15
#define	OPR_CONCAT	16
#define	OPR_LABEL	17
#define	OPR_OPEN	18
#define	OPR_CLOSE	19
#define	OPR_MAX 	20
#define	OPR_MIN 	21

typedef enum {
	PT_NONE, PT_DEF, PT_STRUCT, PT_MEMBER, PT_MACRO
} preproc_type_e;

typedef struct {
	char *str;
	preproc_type_e ptype;
	
	// DEF
	int val, width;
	int flags;
	char config_prefix[16];
	
	// STRUCT
	int size;
	
	// MEMBER
	char *dtype;
	int ninst, offset;
	
	// MACRO
	tokens_t *args, *body;
	int nargs, nbody;
} preproc_t;

extern preproc_t preproc[1024];

preproc_t *pre(char *str, preproc_type_e ptype);


// expressions

int def(tokens_t *tp, tokens_t **ep);
tokens_t *cond(tokens_t *t, tokens_t **ep, int *val);
tokens_t *expr(tokens_t *tp, tokens_t **ep, int *val, int multi=0, tokens_t *ltp=NULL);
tokens_t *expr_collapse(tokens_t *t, tokens_t **ep, int *val);
tokens_t *expr_parens(tokens_t *t, tokens_t **ep, int *val);
int arg_match(tokens_t *body, tokens_t *args, int nargs);
int exp_macro(tokens_t **dp, tokens_t **to);


void sys_panic(const char *str);
void panic(const char *str, tokens_t *t = NULL);
void _syntax(int cond, tokens_t *tp, const char *fmt, ...);
void _note(const char *color, tokens_t *tp, const char *fmt, ...);
void _assert(int cond, const char *str, const char *file, int line);
void errmsg(tokens_t *t, const char *fmt, ...);

#define syntax(cond, tp, fmt, ...) _syntax(cond, tp, fmt, ## __VA_ARGS__)
#define note(color, tp, fmt, ...) _note(color, tp, fmt, ## __VA_ARGS__)

char *str_ends_with(char *s, const char *cs);
int count_ones(u4_t v, int *bit_no = NULL);

#endif
