"""Unchanged 137-school historical pool regression with Test34 extraction dependency."""
from pathlib import Path
from test34_snapshot_extract import adapt_pool_snapshot
parent=Path(__file__).parent/'tu1029r5_pool_test.py'
exec(compile(adapt_pool_snapshot(parent.read_text()),str(parent),'exec'))
