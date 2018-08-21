#include "msutil.h"
#include <errno.h> 
#include <fcntl.h> // open(2)
#include "tap.h"
#include "acism.h"

#ifdef ACISM_STATS
typedef struct { long long val; const char *name; } PSSTAT;
extern PSSTAT psstat[];
extern int pscand[];
#endif//ACISM_STATS

static FILE *matchfp;
static int actual = 0;

static int
on_match(int strnum, int textpos, MEMREF const *pattv)
{
    (void)strnum, (void)textpos, (void)pattv;
    ++actual;
    if (matchfp) fprintf(matchfp, "%9d %7d '%.*s'\n", textpos, strnum, (int)pattv[strnum].len, pattv[strnum].ptr);
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2 || argc > 5)
        usage("pattern_file [c] [nmatches [match.log]]\n" "If [c] is provided, search will be case-insensitive\n" "Creates acism.tmp");

    MEMBUF patt = chomp(slurp(argv[1]));
    if (!patt.ptr)
        die("cannot read %s", argv[1]);

    int npatts;
    MEMREF *pattv = refsplit(patt.ptr, '\n', &npatts);

    unsigned case_sensitive = 1;
    int offset = 2;
    if (argc >= 3 && argv[2][0] == 'c')
    {
        fprintf(stderr, "Case sensitive mode disabled\n");
        ++offset;
        case_sensitive = 0;
    }

    double t = tick();
    ACISM *psp = acism_create_full(pattv, npatts, case_sensitive);
    t = tick() - t;

    plan_tests(argc < (offset+1) ? 1 : 3);

    ok(psp, "acism_create(pattv[%d]) compiled, in %.3f secs", npatts, t);
    acism_dump(psp, PS_STATS, stderr, pattv);
#ifdef ACISM_STATS
    {
    int i;
    for (i = 1; i < (int)psstat[0].val; ++i)
        if (psstat[i].val)
            fprintf(stderr, "%11llu %s\n", psstat[i].val, psstat[i].name);
    }
#endif//ACISM_STATS

    diag("state machine saved as acism.tmp");
    FILE *fp = fopen("acism.tmp", "w");
    acism_save(fp, psp);
    fclose(fp);

    if (argc > offset) {

        int fd = open(argv[1], O_RDONLY);
        if (fd < 0) return fprintf(stderr, "acism_x: %s: cannot open: %s\n", argv[offset], strerror(errno));
        
        static char buf[1024*1024];
        MEMREF text = {buf, 0};
        int state = 0;
        double elapsed = 0, start = tick();
        if (argc > (offset + 1)) matchfp = fopen(argv[offset + 1], "w");
        while (0 < (text.len = read(fd, buf, sizeof buf))) {
            t = tick();
            (void)acism_more(psp, text, (ACISM_ACTION*)on_match, pattv, &state);
            elapsed += tick() - t;
            putc('.', stderr);
        }
        putc('\n', stderr);
        close(fd);
        if (matchfp) fclose(matchfp);
        ok(text.len == 0, "text_file scanned in 1M blocks; read(s) took %.3f secs", tick() - start - elapsed);

        int expected = atoi(argv[offset]);
        if (!ok(actual == expected, "%d matches found, in %.3f secs", expected, elapsed))
            diag("actual: %d\n", actual);
    }

    buffree(patt);
    free(pattv);
    acism_destroy(psp);

    return exit_status();
}
