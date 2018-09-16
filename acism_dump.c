/*
** Copyright (C) 2009-2014 Mischa Sandberg <mischasan@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License Version 3 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU Lesser General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <ctype.h>
#include <stdio.h>
#include "_acism.h"

#if ACISM_SIZE == 4
#   define FX   ""
#elif __LONG_MAX__ < 9223372036854775807
#   define FX   "ll"
#else
#   define FX   "l"
#endif

static void putb(FILE*out, int ch)   
{
    fprintf(out, isprint(ch) ? "'%c" : "%02.2X", ch);
}

static void putsym(FILE*out, int sym, char const*charv)
{
    if (sym) putb(out, charv[sym]); else fputs("--", out);
}

static void printrans(ACISM const*,TRAN,const char*,FILE*,MEMREF const*);
static void printree(ACISM const* psp,
                        int state,
                        int depth,
                        char* str,
                        const char* charv,
                        FILE* out,
                        MEMREF const* pattv);

#define PSTR(_psp,_i,_pattv) _i, \
        _pattv ? (int)_pattv[_i].len : 0, \
        _pattv ? _pattv[_i].ptr : ""

//--------------|---------------------------------------------
void
acism_dump(ACISM const* psp, PS_DUMP_TYPE pdt, FILE *out, MEMREF const*pattv)
{
    int         i, empty, sym, symdist[257] = { 0 };
    char        charv[256];
 
    for (i = 256; --i >=0;) charv[psp->symv[i]] = i;

    if (pdt & PS_STATS) {
        for (i = psp->tran_size, empty = 0; --i >= 0;) {
            if (psp->tranv[i]) {
                ++symdist[t_sym(psp, psp->tranv[i])];
            } else ++empty;
        }
        fprintf(out, "strs:%d syms:%d chars:%d "
                     "trans:%d empty:%d mod:%d hash:%d size:%lu\n",
                psp->nstrs, psp->nsyms, psp->nchars,
                psp->tran_size, empty, psp->hash_mod, psp->hash_size,
                (long)sizeof(ACISM) + p_size(psp));
    }

    // For TRAN/HASH/TREE, print a symbol map (sym -> char/byte).
    if (pdt & ~PS_STATS) {
         for (i = 1; i < psp->nsyms; i++) {
             fprintf(out, "  %u:", i); putsym(out, i, charv);
         }
         putc('\n', out);
     }

    if (pdt & PS_TRAN) {
        fprintf(out, "==== TRAN:\n%8s %8s %3s Ch MS Ref\n", "Cell", "State", "Sym");
        for (i = 1; i < (int)psp->tran_size; ++i) {
            printrans(psp, i, charv, out, pattv);
        }
    }

    if (pdt & PS_HASH && psp->hash_size) {
        fprintf(out, "==== HASH:\n.....:   state strno\n");
        for (i = 0; i < (int)psp->hash_size; ++i) {
            STATE state = psp->hashv[i].state;
            if (state)
                fprintf(out, "%5d: %7"FX"u %3d %8"FX"u %.*s\n",
                        i, state, i - p_hash(psp, state), 
                        PSTR(psp, psp->hashv[i].strno, pattv));
            else
                fprintf(out, "%5d: %7"FX"d --- %8"FX"d\n", i, state,
                        psp->hashv[i].strno);
        }
    }

    if (pdt & PS_TREE) {
        fprintf(out, "==== TREE:\n");
        char *str = (char*)malloc(psp->maxlen + 1);
        printree(psp, 0, 0, str, charv, out, pattv);
        free(str);
    }
    //TODO: calculate stats: backref chain lengths ...
}

static void
printrans(ACISM const*psp, STATE s, char const *charv,
            FILE *out, MEMREF const *pattv)
{
    (void)pattv;
    TRAN x = psp->tranv[s];
    if (!x) {
        fprintf(out, "%8d (empty)\n", s);
        return;
    }

    SYMBOL sym = t_sym(psp, x);
    fprintf(out, "%8d %8d %3u ", s, s - sym, sym);
    putsym(out, sym, charv);
    putc(' ', out);
    putc("M-"[!(x & IS_MATCH)], out);
    putc("S-"[!(x & IS_SUFFIX)], out);

    STATE next = t_next(psp, x);
    if (t_isleaf(psp, x)) {
        fprintf(out, " str= %d\n", t_strno(psp, x));
    } else {
        fprintf(out, " %s %d", sym == 0 ? "back" : "next", next);
        if (x & IS_MATCH) {
            int i;
            for (i = p_hash(psp, s); psp->hashv[i].state != s; ++i);
            fprintf(out, " str# %"FX"d", psp->hashv[i].strno);
        }
        putc('\n', out);
    }
}
static void
printree(ACISM const*psp, int state, int depth, char *str,
            char const *charv, FILE*out, MEMREF const*pattv)
{
    SYMBOL sym;
    TRAN x;

    if (depth > (int)psp->maxlen) {
        fputs("oops\n", out);
        return;
    }

    x = psp->tranv[state];
    fprintf(out, "%5d:%.*s", state, depth, str);
    if (t_valid(psp,x) && t_next(psp,x))
        fprintf(out, " b=%"FX"d%s", t_next(psp,x), x & T_FLAGS ? " BAD" : "");
    fprintf(out, "\n");

    for (sym = 1; sym < psp->nsyms; ++sym) {
        x = p_tran(psp, state, sym);
        if (t_valid(psp, x)) {
            str[depth] = charv[sym];
            fprintf(out, "%*s%c %c%c",
                    depth+6, "", charv[sym],
                    x & IS_MATCH ? 'M' : '-',
                    x & IS_SUFFIX ? 'S' : '-');
            if (x & IS_MATCH && pattv && t_isleaf(psp, x))
                fprintf(out, " %.0d -> %.*s", PSTR(psp, t_strno(psp,x), pattv));
            if (x & IS_SUFFIX)
                fprintf(out, " ->S %"FX"d", t_next(psp, psp->tranv[state]));
            fprintf(out, "\n");
            if (!t_isleaf(psp, x))
                printree(psp, t_next(psp, x), depth+1, str, charv, out, pattv);
        }
    }
}

//EOF
