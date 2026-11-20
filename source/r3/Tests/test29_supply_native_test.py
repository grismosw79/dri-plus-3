#!/usr/bin/env python3
"""Release-runner wrapper for the captured generation consumer checks."""
import os, pathlib, subprocess, sys
capture=os.environ.get('DRI_CAPTURE_DIR')
assert capture, 'DRI_CAPTURE_DIR is required; captured-native validation must not silently skip'
out=pathlib.Path(os.environ.get('DRI_TEST_OUTPUT_DIR','.') ) / 'supply-native.json'
out.parent.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(pathlib.Path(__file__).with_name('test29_supply_native.py')),capture,str(out)],check=True)
