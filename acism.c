/*
** Copyright (C) 2009-2013 Mischa Sandberg <mischasan@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "_acism.h"

int
acism_more(ACISM const *psp, MEMREF const text,
           ACISM_ACTION *cb, void *context, int *statep)
{
    ACISM const ps = *psp;
    char const *cp = text.ptr, *endp = cp + text.len;
    STATE state = *statep;

    while (cp < endp) {
        _SYMBOL sym = ps.symv[(uint8_t)*cp++];

        if (!sym) {
            // Input byte is not in any pattern string.
            state = 0;
            continue;
        }

        // Search for a state with a valid transition for this
        // state and sym, following the backref chain.

        TRAN next, back;

        while (!t_valid(&ps, next = p_tran(&ps, state, sym))
               && t_valid(&ps, back = p_tran(&ps, state, 0)))
            state = t_next(&ps, back);

        if (!t_valid(&ps, next)) {
            // (sym) is not in any transition from this state.
            state = 0;
            continue;
        }

        if (!(next & (IS_MATCH | IS_SUFFIX))) {
            // No complete match yet; keep going.
            state = t_next(&ps, next);
            continue;
        }

        // At this point, one or more patterns have matched.
        // Find all matches by following the backref chain.
        // A valid node for (sym) with no SUFFIX marks the
        //  end of the suffix chain.
        // In the same backref traversal, find a new (state),
        //  if the original transition is to a leaf.

        STATE s = state;
        state = t_isleaf(&ps, next) ? 0 : t_next(&ps, next);

        while (1) {

            if (t_valid(&ps, next)) {

                if (next & IS_MATCH) {
                    unsigned strno, ss = s + sym, i;
                    if (t_isleaf(&ps, ps.tranv[ss])) {
                        strno = t_strno(&ps, ps.tranv[ss]);
                    } else {
                        for (i = p_hash(&ps, ss); ps.hashv[i].state != ss; ++i);
                        strno = ps.hashv[i].strno;
                    }

                    int ret = cb(strno, cp - text.ptr, context);
                    if (ret)
                        return *statep = state, ret;
                }

                if (!state && !t_isleaf(&ps, next))
                    state = t_next(&ps, next);
                if (state && !(next & IS_SUFFIX))
                    break;
            }

            if (!s)
                break;
            TRAN b = p_tran(&ps, s, 0);
            s = t_valid(&ps, b) ? t_next(&ps, b) : 0;
            next = p_tran(&ps, s, sym);
        }
    }

    return *statep = state, 0;
}
//EOF
