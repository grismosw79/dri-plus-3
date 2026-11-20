"""Supply the new exact dependency to the unchanged historical C extractor.

All inherited fixture inputs, comparisons and assertions remain byte-identical.
The legacy extractor deliberately evaluates annual_valid=0; the separate
Test34 vacancy tests exercise actual annual permission and shared accounting.
"""
def adapt_pool_snapshot(source):
    anchor=" pieces.append(function(hs,'hs15_snapshot_values'))"
    assert source.count(anchor)==1, 'Historical snapshot extractor changed'
    dependency=''' if 'static int hs34_vacancy_extra(' in hs:
  pieces.extend(['#include "class34_policy.h"',function(hs,'hs34_vacancy_extra')])
'''
    source=source.replace(anchor,dependency+anchor)
    # Native hs15_snapshot_block validates position before calling the pure
    # function. This public ctypes harness must enforce the same contract;
    # otherwise inlining the new bounded helper exposes the legacy harness's
    # unchecked external pos argument to GCC's array-bounds diagnostic.
    entry=' Hs15Snapshot s={0};s.prestige=prestige;s.projection_step=step;'
    assert source.count(entry)==1, 'Historical assess entry changed'
    source=source.replace(entry,' if(pos<0 || pos>=POSITIONS)return -1;\n'+entry)
    result=' return fn'
    assert source.count(result)==1, 'Historical build return changed'
    checks=" zeros=(C.c_int*22)()\n for bad in (-1,22):assert fn(zeros,zeros,zeros,0,0,0,bad,0,0)==-1\n"
    return source.replace(result,checks+result)
