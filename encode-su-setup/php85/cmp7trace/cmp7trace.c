/*
 * cmp7trace: log loose comparisons whose result differs between PHP 7 and PHP 8 rules.
 *
 * PHP 8 ("Saner string to number comparisons") compares a number with a non-numeric string
 * ('' or 'abc' or '12abc') as strings; PHP 7 converted the string to its leading number (or 0).
 * PHP 8 ("Saner numeric strings") also treats "12 " (trailing whitespace) as numeric, which
 * changes string == string comparisons. Hooks ==, !=, <, <=, >, >=, <=>, switch/case,
 * in_array() and array_search(); writes one line per distinct code location to the log.
 *
 * Diagnostic tool only; run with opcache disabled.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "php_ini.h"
#include "SAPI.h"
#include "zend_execute.h"
#include "zend_operators.h"
#include "zend_smart_str.h"
#include "zend_compile.h"
#include <ctype.h>
#include <stdio.h>

static HashTable seen;
static zif_handler orig_in_array = NULL;
static zif_handler orig_array_search = NULL;

PHP_INI_BEGIN()
	PHP_INI_ENTRY("cmp7trace.log", "/var/log/php/cmp7trace.log", PHP_INI_SYSTEM, NULL)
PHP_INI_END()

static bool is_num(zval *z)
{
	return Z_TYPE_P(z) == IS_LONG || Z_TYPE_P(z) == IS_DOUBLE;
}

/* the number PHP 7 used for a string in a comparison with a number: its leading number, or 0 */
static void php7_str_number(zend_string *s, zval *out)
{
	zend_long l;
	double d;
	uint8_t type = is_numeric_string_ex(ZSTR_VAL(s), ZSTR_LEN(s), &l, &d, true, NULL, NULL);
	if (type == IS_LONG) {
		ZVAL_LONG(out, l);
	} else if (type == IS_DOUBLE) {
		ZVAL_DOUBLE(out, d);
	} else {
		ZVAL_LONG(out, 0);
	}
}

static bool php8_numeric(zend_string *s)
{
	return is_numeric_string(ZSTR_VAL(s), ZSTR_LEN(s), NULL, NULL, false) != 0;
}

/* PHP 7 numeric strings: no trailing whitespace */
static bool php7_numeric(zend_string *s)
{
	if (!php8_numeric(s)) {
		return false;
	}
	unsigned char c = ZSTR_LEN(s) ? (unsigned char) ZSTR_VAL(s)[ZSTR_LEN(s) - 1] : 'x';
	return !(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

/*
 * For a pair whose comparison changed between PHP 7 and 8, set *r7 / *r8 to the results of
 * the three-way comparison under each version and return true. Otherwise return false.
 */
static bool changed_pair(zval *a, zval *b, int *r7, int *r8)
{
	zval tmp;
	ZVAL_DEREF(a);
	ZVAL_DEREF(b);
	if (is_num(a) && Z_TYPE_P(b) == IS_STRING) {
		if (php8_numeric(Z_STR_P(b))) {
			return false;
		}
		php7_str_number(Z_STR_P(b), &tmp);
		*r7 = zend_compare(a, &tmp);
		*r8 = zend_compare(a, b);
		return true;
	}
	if (Z_TYPE_P(a) == IS_STRING && is_num(b)) {
		if (php8_numeric(Z_STR_P(a))) {
			return false;
		}
		php7_str_number(Z_STR_P(a), &tmp);
		*r7 = zend_compare(&tmp, b);
		*r8 = zend_compare(a, b);
		return true;
	}
	if (Z_TYPE_P(a) == IS_STRING && Z_TYPE_P(b) == IS_STRING) {
		bool n7 = php7_numeric(Z_STR_P(a)) && php7_numeric(Z_STR_P(b));
		bool n8 = php8_numeric(Z_STR_P(a)) && php8_numeric(Z_STR_P(b));
		if (n7 == n8) {
			return false;
		}
		if (n7) {
			*r7 = zend_compare(a, b); /* both numeric in PHP 7: never happens (PHP 7 numeric implies PHP 8 numeric) */
		} else {
			int c = zend_binary_strcmp(Z_STRVAL_P(a), Z_STRLEN_P(a), Z_STRVAL_P(b), Z_STRLEN_P(b));
			*r7 = ZEND_NORMALIZE_BOOL(c);
		}
		*r8 = zend_compare(a, b);
		return true;
	}
	return false;
}

static void append_zval(smart_str *buf, zval *z)
{
	ZVAL_DEREF(z);
	switch (Z_TYPE_P(z)) {
		case IS_UNDEF: smart_str_appends(buf, "undef"); break;
		case IS_NULL: smart_str_appends(buf, "null"); break;
		case IS_FALSE: smart_str_appends(buf, "false"); break;
		case IS_TRUE: smart_str_appends(buf, "true"); break;
		case IS_LONG: smart_str_append_long(buf, Z_LVAL_P(z)); break;
		case IS_DOUBLE: smart_str_append_printf(buf, "%.6g", Z_DVAL_P(z)); break;
		case IS_STRING: {
			size_t n = Z_STRLEN_P(z) > 40 ? 40 : Z_STRLEN_P(z);
			smart_str_appendc(buf, '\'');
			for (size_t i = 0; i < n; i++) {
				unsigned char c = (unsigned char) Z_STRVAL_P(z)[i];
				if (c < 32 || c >= 127 || c == '\'' || c == '\\') {
					smart_str_append_printf(buf, "\\x%02x", c);
				} else {
					smart_str_appendc(buf, (char) c);
				}
			}
			if (Z_STRLEN_P(z) > 40) {
				smart_str_appends(buf, "...");
			}
			smart_str_appendc(buf, '\'');
			break;
		}
		default: smart_str_appends(buf, zend_zval_type_name(z)); break;
	}
}

static bool is_template_object(zval *This)
{
	if (Z_TYPE_P(This) != IS_OBJECT) {
		return false;
	}
	for (zend_class_entry *ce = Z_OBJCE_P(This); ce; ce = ce->parent) {
		if (zend_string_equals_literal_ci(ce->name, "vB_Template")) {
			return true;
		}
	}
	return false;
}

static void append_location(smart_str *buf, zend_execute_data *ex)
{
	while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
		ex = ex->prev_execute_data;
	}
	if (!ex) {
		smart_str_appends(buf, "?");
		return;
	}
	smart_str_append(buf, ex->func->op_array.filename);
	smart_str_append_printf(buf, ":%u", ex->opline ? ex->opline->lineno : 0);
	if (ex->func->op_array.function_name) {
		smart_str_appends(buf, " (");
		if (ex->func->common.scope) {
			smart_str_append(buf, ex->func->common.scope->name);
			smart_str_appends(buf, "::");
		}
		smart_str_append(buf, ex->func->op_array.function_name);
		smart_str_appendc(buf, ')');
	}
	if (ex->func->type == ZEND_EVAL_CODE) {
		/* eval()'d code (plugins, templates): show who evaluated it */
		int depth = 0;
		for (zend_execute_data *p = ex->prev_execute_data; p && depth < 2; p = p->prev_execute_data) {
			if (!p->func || !ZEND_USER_CODE(p->func->type)) {
				continue;
			}
			smart_str_appends(buf, " <- ");
			if (p->func->common.scope) {
				smart_str_append(buf, p->func->common.scope->name);
				smart_str_appends(buf, "::");
			}
			if (p->func->common.function_name) {
				smart_str_append(buf, p->func->common.function_name);
				smart_str_appendc(buf, '@');
			}
			smart_str_append_printf(buf, "%s:%u", ZSTR_VAL(p->func->op_array.filename), p->opline ? p->opline->lineno : 0);
			if (is_template_object(&p->This)) {
				zval rv;
				zval *tpl = zend_read_property(Z_OBJCE(p->This), Z_OBJ(p->This), "template", sizeof("template") - 1, true, &rv);
				if (tpl && Z_TYPE_P(tpl) == IS_STRING) {
					smart_str_appends(buf, " [template ");
					smart_str_append(buf, Z_STR_P(tpl));
					smart_str_appendc(buf, ']');
				}
			}
			depth++;
		}
	}
}

static void report(zend_execute_data *ex, const char *what, zval *a, zval *b, const char *r7, const char *r8)
{
	smart_str loc = {0};
	append_location(&loc, ex);
	smart_str_appendc(&loc, '|');
	smart_str_appends(&loc, what);
	smart_str_0(&loc);
	/* seen is persistent (outlives the request): the _str_ variants copy the key into persistent memory */
	if (zend_hash_str_exists(&seen, ZSTR_VAL(loc.s), ZSTR_LEN(loc.s))) {
		smart_str_free(&loc);
		return;
	}
	zend_hash_str_add_empty_element(&seen, ZSTR_VAL(loc.s), ZSTR_LEN(loc.s));

	smart_str line = {0};
	smart_str_append(&line, loc.s);
	smart_str_appends(&line, " | ");
	append_zval(&line, a);
	smart_str_appends(&line, " vs ");
	append_zval(&line, b);
	smart_str_append_printf(&line, " | php7=%s php8=%s | %s %s\n", r7, r8,
		SG(request_info).request_method ? SG(request_info).request_method : "CLI",
		SG(request_info).request_uri ? SG(request_info).request_uri : "");
	smart_str_0(&line);
	FILE *f = fopen(INI_STR("cmp7trace.log"), "a");
	if (f) {
		fwrite(ZSTR_VAL(line.s), 1, ZSTR_LEN(line.s), f);
		fclose(f);
	}
	smart_str_free(&line);
	smart_str_free(&loc);
}

static const char *bool_str(bool b)
{
	return b ? "true" : "false";
}

static int cmp_handler(zend_execute_data *execute_data)
{
	const zend_op *opline = EX(opline);
	zval *op1 = zend_get_zval_ptr(opline, opline->op1_type, &opline->op1, execute_data);
	zval *op2 = zend_get_zval_ptr(opline, opline->op2_type, &opline->op2, execute_data);
	int r7, r8;
	if (!op1 || !op2) {
		return ZEND_USER_OPCODE_DISPATCH;
	}
	if (opline->opcode == ZEND_IN_ARRAY) {
		/* in_array() with a literal array of non-numeric strings, compiled to an opcode */
		zval *needle = op1;
		ZVAL_DEREF(needle);
		if (!opline->extended_value && is_num(needle)) {
			bool f7 = false, f8 = false;
			zend_string *key;
			ZEND_HASH_FOREACH_STR_KEY(Z_ARRVAL_P(op2), key) {
				zval kz;
				if (!key) {
					continue;
				}
				ZVAL_STR(&kz, key);
				if (changed_pair(needle, &kz, &r7, &r8)) {
					f7 = f7 || r7 == 0;
					f8 = f8 || r8 == 0;
				} else {
					bool e = zend_compare(needle, &kz) == 0;
					f7 = f7 || e;
					f8 = f8 || e;
				}
			} ZEND_HASH_FOREACH_END();
			if (f7 != f8) {
				report(execute_data, "in_array(literal)", needle, op2, bool_str(f7), bool_str(f8));
			}
		}
		return ZEND_USER_OPCODE_DISPATCH;
	}
	if (!changed_pair(op1, op2, &r7, &r8)) {
		return ZEND_USER_OPCODE_DISPATCH;
	}
	switch (opline->opcode) {
		case ZEND_IS_EQUAL:
		case ZEND_CASE:
			if ((r7 == 0) != (r8 == 0)) {
				report(execute_data, opline->opcode == ZEND_CASE ? "case" : "==", op1, op2, bool_str(r7 == 0), bool_str(r8 == 0));
			}
			break;
		case ZEND_IS_NOT_EQUAL:
			if ((r7 == 0) != (r8 == 0)) {
				report(execute_data, "!=", op1, op2, bool_str(r7 != 0), bool_str(r8 != 0));
			}
			break;
		case ZEND_IS_SMALLER:
			if ((r7 < 0) != (r8 < 0)) {
				report(execute_data, "<", op1, op2, bool_str(r7 < 0), bool_str(r8 < 0));
			}
			break;
		case ZEND_IS_SMALLER_OR_EQUAL:
			if ((r7 <= 0) != (r8 <= 0)) {
				report(execute_data, "<=", op1, op2, bool_str(r7 <= 0), bool_str(r8 <= 0));
			}
			break;
		case ZEND_SPACESHIP:
			if (ZEND_NORMALIZE_BOOL(r7) != ZEND_NORMALIZE_BOOL(r8)) {
				char s7[4], s8[4];
				snprintf(s7, sizeof(s7), "%d", ZEND_NORMALIZE_BOOL(r7));
				snprintf(s8, sizeof(s8), "%d", ZEND_NORMALIZE_BOOL(r8));
				report(execute_data, "<=>", op1, op2, s7, s8);
			}
			break;
	}
	return ZEND_USER_OPCODE_DISPATCH;
}

/* in_array() / array_search() without $strict */
static void check_search(zend_execute_data *execute_data, bool is_search)
{
	uint32_t argc = ZEND_CALL_NUM_ARGS(execute_data);
	zval *needle, *haystack, *val;
	int r7, r8;
	if (argc < 2) {
		return;
	}
	needle = ZEND_CALL_ARG(execute_data, 1);
	haystack = ZEND_CALL_ARG(execute_data, 2);
	ZVAL_DEREF(needle);
	ZVAL_DEREF(haystack);
	if (Z_TYPE_P(haystack) != IS_ARRAY || (argc >= 3 && zend_is_true(ZEND_CALL_ARG(execute_data, 3)))) {
		return;
	}
	/* cheap pre-check: is there any pair whose comparison changed? */
	bool any = false;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(haystack), val) {
		if (changed_pair(needle, val, &r7, &r8) && ((r7 == 0) != (r8 == 0))) {
			any = true;
			break;
		}
	} ZEND_HASH_FOREACH_END();
	if (!any) {
		return;
	}
	zend_long pos = 0, p7 = -1, p8 = -1;
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(haystack), val) {
		bool e7, e8;
		if (changed_pair(needle, val, &r7, &r8)) {
			e7 = r7 == 0;
			e8 = r8 == 0;
		} else {
			e7 = e8 = (zend_compare(needle, val) == 0);
		}
		if (p7 < 0 && e7) p7 = pos;
		if (p8 < 0 && e8) p8 = pos;
		if (p7 >= 0 && p8 >= 0) break;
		pos++;
	} ZEND_HASH_FOREACH_END();
	if (is_search ? (p7 != p8) : ((p7 >= 0) != (p8 >= 0))) {
		char s7[32], s8[32];
		if (is_search) {
			snprintf(s7, sizeof(s7), "pos%ld", (long) p7);
			snprintf(s8, sizeof(s8), "pos%ld", (long) p8);
		} else {
			snprintf(s7, sizeof(s7), "%s", bool_str(p7 >= 0));
			snprintf(s8, sizeof(s8), "%s", bool_str(p8 >= 0));
		}
		report(EG(current_execute_data) ? EG(current_execute_data)->prev_execute_data : NULL,
			is_search ? "array_search()" : "in_array()", needle, haystack, s7, s8);
	}
}

static ZEND_NAMED_FUNCTION(cmp7_in_array)
{
	check_search(execute_data, false);
	orig_in_array(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

static ZEND_NAMED_FUNCTION(cmp7_array_search)
{
	check_search(execute_data, true);
	orig_array_search(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

PHP_MINIT_FUNCTION(cmp7trace)
{
	static const uint8_t ops[] = {ZEND_IS_EQUAL, ZEND_IS_NOT_EQUAL, ZEND_IS_SMALLER, ZEND_IS_SMALLER_OR_EQUAL,
		ZEND_CASE, ZEND_SPACESHIP, ZEND_IN_ARRAY};
	REGISTER_INI_ENTRIES();
	zend_hash_init(&seen, 256, NULL, NULL, 1);
	for (size_t i = 0; i < sizeof(ops); i++) {
		zend_set_user_opcode_handler(ops[i], cmp_handler);
	}
	zend_function *f = zend_hash_str_find_ptr(CG(function_table), "in_array", sizeof("in_array") - 1);
	if (f && f->type == ZEND_INTERNAL_FUNCTION) {
		orig_in_array = f->internal_function.handler;
		f->internal_function.handler = cmp7_in_array;
	}
	f = zend_hash_str_find_ptr(CG(function_table), "array_search", sizeof("array_search") - 1);
	if (f && f->type == ZEND_INTERNAL_FUNCTION) {
		orig_array_search = f->internal_function.handler;
		f->internal_function.handler = cmp7_array_search;
	}
	return SUCCESS;
}

PHP_RINIT_FUNCTION(cmp7trace)
{
	/* no frameless/compile-time-resolved internal calls, so in_array() goes through the handler above */
	CG(compiler_options) |= ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS | ZEND_COMPILE_NO_BUILTINS;
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(cmp7trace)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

zend_module_entry cmp7trace_module_entry = {
	STANDARD_MODULE_HEADER,
	"cmp7trace",
	NULL,
	PHP_MINIT(cmp7trace),
	PHP_MSHUTDOWN(cmp7trace),
	PHP_RINIT(cmp7trace),
	NULL,
	NULL,
	"0.1",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_CMP7TRACE
ZEND_GET_MODULE(cmp7trace)
#endif
