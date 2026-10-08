# RoseRefactor (edg2sage/xref.C): a hook called for each record of the cross-reference listing,
# while the symbol it describes is in memory
s|^BEGIN_EDG_NAMESPACE$|&\n/* edg2sage: called for each record of the cross-reference listing (see edg2sage/xref.C) */\nvoid (*edg2sage_xref_hook)(a_symbol_ptr, char, a_const_char *, a_line_number, int) = NULL;|
s|^    fprintf(f_xref_info, "%p\\t", (void \*)sym_ptr);$|    if (edg2sage_xref_hook != NULL) {  /* edg2sage */\n      edg2sage_xref_hook(sym_ptr, code, file_name, line_number, source_position->column);\n    }  /* if */\n&|
