PHP_ARG_ENABLE([cmp7trace], [whether to enable cmp7trace], [AS_HELP_STRING([--enable-cmp7trace], [Enable cmp7trace])], [no])
if test "$PHP_CMP7TRACE" != "no"; then
  PHP_NEW_EXTENSION(cmp7trace, cmp7trace.c, $ext_shared)
fi
