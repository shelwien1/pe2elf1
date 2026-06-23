/*
 * apxdis - decode hex byte lines as x86-64 (APX enabled) and print them in the
 *          mine32 canon-dump format: "<hex bytes>\t<intel disasm>".
 *
 * Reads whitespace/comma-separated hex bytes (one instruction per input line),
 * e.g. "62 f4 44 04 38 c3", and writes a line consumable by dump2asm.py.
 * A line may carry a trailing "# comment" which is ignored. Decode failures are
 * reported on stderr (and the line is skipped) so bad examples are caught.
 */
#include <Zydis/Zydis.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    ZydisDecoder dec;
    ZydisDecoderInit(&dec, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    ZydisDecoderEnableMode(&dec, ZYDIS_DECODER_MODE_APX, ZYAN_TRUE);
    ZydisFormatter fmt;
    ZydisFormatterInit(&fmt, ZYDIS_FORMATTER_STYLE_INTEL);

    char line[1024];
    int rc = 0;
    while (fgets(line, sizeof(line), stdin)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = 0;

        ZyanU8 buf[ZYDIS_MAX_INSTRUCTION_LENGTH];
        int n = 0;
        char *p = line;
        while (*p && n < (int)sizeof(buf)) {
            while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
            if (!*p) break;
            char *end;
            long v = strtol(p, &end, 16);
            if (end == p) break;
            buf[n++] = (ZyanU8)v;
            p = end;
        }
        if (n == 0)
            continue;

        ZydisDecodedInstruction insn;
        ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
        ZyanStatus st = ZydisDecoderDecodeFull(&dec, buf, n, &insn, ops);
        if (!ZYAN_SUCCESS(st)) {
            fprintf(stderr, "apxdis: decode failed for:");
            for (int i = 0; i < n; i++) fprintf(stderr, " %02x", buf[i]);
            fprintf(stderr, "  (status 0x%08x)\n", (unsigned)st);
            rc = 1;
            continue;
        }
        if (insn.length != n)
            fprintf(stderr, "apxdis: warning: consumed %u of %d bytes\n",
                    insn.length, n);

        char text[256];
        if (!ZYAN_SUCCESS(ZydisFormatterFormatInstruction(
                &fmt, &insn, ops, insn.operand_count_visible,
                text, sizeof(text), 0, ZYAN_NULL))) {
            fprintf(stderr, "apxdis: format failed\n");
            rc = 1;
            continue;
        }

        for (int i = 0; i < (int)insn.length; i++)
            printf("%s%02x", i ? " " : "", buf[i]);
        printf("\t%s\n", text);
    }
    return rc;
}
