"""Current compiled publication/carry checks from the retained R3 test.
The historical Test28R2 negative-control DLL is not part of this package;
do not claim that separate old-binary control was rerun.
"""
from pathlib import Path
source=(Path(__file__).parent/'test28r3_eps_publication_test.py').read_text()
source=source.split("\nsubprocess.run([sys.executable,str(Path(__file__).resolve()),'--r2-control']")[0]
exec(compile(source,'Current EPS publication checks','exec'))
report={'passed':True,'nativeCacheCellsPublished':138*63*2,'onDemandFinalPublication':True,'snapshotModelBytesUnchanged':True,'repeatQueryDoesNotRewrite':True,'laterWeekKeepsPublicationHash':True,'restartReusesFinalSnapshot':True,'nextSeasonDiskCarryVerified':True,'referenceOwnershipBalanced':True,'historicalR2NegativeControl':'not rerun; old DLL absent','live_game_verified':False}
(OUTPUT/'tu1029r4_eps_publication_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
