"""Reusable native-image setup without executing superseded vacancy policy assertions."""
from pathlib import Path
src=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(src.split('Q1,TQ1=recruit')[0],'runtime fixture setup','exec'))
src=(Path(__file__).parent/'hs15_runtime_test.py').read_text()
exec(compile('checks=[]'+src.split('checks=[]',1)[1].split('\nreset()\nkeep=')[0],'HS snapshot fixture setup','exec'))
