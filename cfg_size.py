#!/bin/env python2
import angr
import sys

p = angr.Project(sys.argv[1], load_options={'auto_load_libs': False})
if len(sys.argv) > 2:
    cfg = p.analyses.CFGAccurate()
else:
    cfg = p.analyses.CFG()
print len(list(cfg.nodes()))
