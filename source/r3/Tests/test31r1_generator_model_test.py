"""R1-only decoded generator and exact header-delta validation."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[4]
OUTPUT=Path(os.environ['DRI_TEST_OUTPUT_DIR']).resolve() if os.environ.get('DRI_TEST_OUTPUT_DIR') else Path(tempfile.mkdtemp(prefix='dri-test31r1-model-'))
subprocess.run(['node',str(ROOT/'Runtime/Portal/Source/verify_test31r1_generator.cjs'),str(ROOT),str(OUTPUT)],check=True)
