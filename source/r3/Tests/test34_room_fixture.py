"""Adapt historical emulation buffers to Test34's appended annual-valid field.

Historical test files stay byte-identical. Only three exact fixture snippets
are adapted: the explicit projection sizeof assertion, two old output-buffer
allocations, and assertions for the new trailing field. Every prior assertion
is executed unchanged except the explicitly obsolete sizeof expectation.
"""
from pathlib import Path
import json


def run_inherited_room_test(wrapper_file, inherited_name, namespace):
    tests = Path(wrapper_file).resolve().parent
    recipes = {
        'test30_projection_batch_test.py': [
            ("assert size==(1584 if 'DriTest31ClassAnnual' in exports else 1580),size",
             "assert size==1588,size"),
            ("  assert bytes(uc.mem_read(batch+size*pos,size))==bytes(uc.mem_read(single,size)),(which,pos)",
             "  assert bytes(uc.mem_read(batch+size*pos,size))==bytes(uc.mem_read(single,size)),(which,pos)\n"
             "  assert ri(batch+size*pos+1584)==ri(single+1584)==1,(which,pos,'annual map validity')"),
        ],
        'quality_runtime_test.py': [
            ("qout=alloc(1584)",
             "assert call(exports['DriTest30QualitySize'])==1588\nqout=alloc(1588)"),
            ("assert call(exports['DriTestQualityRoom'],team,context,0,qout)==1",
             "assert call(exports['DriTestQualityRoom'],team,context,0,qout)==1\n"
             "assert ri(qout+1584)==1,'annual map validity'"),
        ],
        'signing_ownership_test.py': [
            (" out=alloc(1584);assert call(exports['DriTestQualityRoom'],destination,context,pos,out)==1",
             " assert call(exports['DriTest30QualitySize'])==1588\n"
             " out=alloc(1588);assert call(exports['DriTestQualityRoom'],destination,context,pos,out)==1\n"
             " assert ri(out+1584)==1,'annual map validity'"),
        ],
    }
    original = Path.read_text
    adapted = set()

    def read_text(path, *args, **kwargs):
        text = original(path, *args, **kwargs)
        if path.resolve().parent == tests and path.name in recipes:
            for old, new in recipes[path.name]:
                assert text.count(old) == 1, (path.name, 'fixture snippet changed', old)
                text = text.replace(old, new)
            adapted.add(path.name)
        return text

    Path.read_text = read_text
    try:
        source = (tests / inherited_name).read_text()
        exec(compile(source, inherited_name, 'exec'), namespace)
    finally:
        Path.read_text = original
    expected = ({'test30_projection_batch_test.py'} if inherited_name == 'test30_projection_batch_test.py'
                else {'quality_runtime_test.py', 'signing_ownership_test.py'})
    assert adapted == expected, (inherited_name, adapted, expected)
    result = {'passed': True, 'inherited_test': inherited_name,
              'adapted_fixture_files': sorted(adapted), 'portal_quality_bytes': 1588,
              'hs_annual_valid_offset': 1584, 'hs_annual_valid_asserted': True,
              'all_unaffected_assertions_retained': True, 'live_game_verified': False}
    output = namespace['OUTPUT']
    name = Path(wrapper_file).stem.removesuffix('_test') + '_results.json'
    (output / name).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
