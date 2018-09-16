#include "msutil.h"
#include <errno.h> 
#include <fcntl.h> // open(2)
#include "tap.h"
#include "acism.h"

// Move this to common plat.h
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS	1
static FILE* FOPEN(char const*path, char const*mode)
{
    FILE* fp;  return fopen_s(&fp, path, mode) ? NULL : fp;
}
#else//not _MSC_VER
#define FOPEN fopen
#endif//_MSC_VER

#ifdef ACISM_STATS
typedef struct { long long val; const char *name; } PSSTAT;
extern PSSTAT psstat[];
extern int pscand[];
#endif//ACISM_STATS

static int actual = 0, details = 1;

static int
on_match(int strnum, int textpos, MEMREF const *pattv)
{
    (void)strnum, (void)textpos, (void)pattv;
    ++actual;
    if (details) fprintf(stderr, "%9d %7d '%.*s'\n", textpos, strnum, (int)pattv[strnum].len, pattv[strnum].ptr);
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2 || argc > 4)
        usage("pattern_file target_file [[-]expected]\ne.g. acism_x patts act.txt -5");

    MEMBUF patt = chomp(slurp(argv[1]));
    if (!patt.ptr)
        die("cannot read %s", argv[1]);

    int npatts;
    MEMREF *pattv = refsplit(patt.ptr, '\n', &npatts);

    double t = tick();
    ACISM *psp = acism_create(pattv, npatts);
    t = tick() - t;

    plan_tests(argc < 3 ? 1 : 3);

    ok(psp, "acism_create(pattv[%d]) compiled, in %.3f secs", npatts, t);
    acism_dump(psp, PS_ALL, stderr, pattv);
#ifdef ACISM_STATS
    {
    int i;
    for (i = 1; i < (int)psstat[0].val; ++i)
        if (psstat[i].val)
            fprintf(stderr, "%11llu %s\n", psstat[i].val, psstat[i].name);
    }
#endif//ACISM_STATS

    diag("state machine saved as acism.tmp");
    FILE *fp = FOPEN("acism.tmp", "w");
    acism_save(fp, psp);
    fclose(fp);

    if (argc > 2) {
        // Negative count => print match details
        int expected = argc > 3 ? atoi(argv[3]) : 0;
        int details = expected < 0;
        if (details) expected = -expected;
        FILE*	textfp = FOPEN(argv[2], "r");		// REUSE PATTERN FILE AS A TARGET
        if (!fp) die("cannot open %s", argv[2]);
        static char buf[1024*1024];
        MEMREF		text = {buf, 0};
        int			state = 0;
        double		elapsed = 0, start = tick();
        while (0 < (text.len = fread(buf, sizeof*buf, sizeof buf, textfp))) {
            t = tick();
            (void)acism_more(psp, text, (ACISM_ACTION*)on_match, pattv, &state);
            elapsed += tick() - t;
            putc('.', stderr);
        }
        putc('\n', stderr);
        fclose(textfp);
        ok(text.len == 0, "text_file scanned in 1M blocks; read(s) took %.3f secs", tick() - start - elapsed);

        if (!ok(actual == expected || expected == 0, "%d matches found, in %.3f secs", actual, elapsed))
            diag("actual: %d\n", actual);
    }

    buffree(patt);
    free(pattv);
    acism_destroy(psp);

    return exit_status();
}
