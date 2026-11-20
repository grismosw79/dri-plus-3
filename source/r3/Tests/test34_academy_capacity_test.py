"""Unchanged academy capacity/PT regression with Test34 extraction dependency."""
from pathlib import Path
from test34_snapshot_extract import adapt_pool_snapshot
parent=Path(__file__).parent/'test30_academy_capacity_test.py'
source=parent.read_text()
anchor="(Path(__file__).parent/'tu1029r5_pool_test.py').read_text().split('old=build(B,')[0]"
assert source.count(anchor)==1, 'Historical academy extractor changed'
replacement="adapt_pool_snapshot((Path(__file__).parent/'tu1029r5_pool_test.py').read_text()).split('old=build(B,')[0]"
exec(compile(source.replace(anchor,replacement),str(parent),'exec'))
