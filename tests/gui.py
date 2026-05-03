#!/usr/bin/env python3
"""Exercise the actual SFML window, invalid reload recovery and photon cancellation."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import time

binary=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='raytracer-gui-') as temporary:
    directory=Path(temporary)
    scene=directory/'scène.json'
    output=directory/'render.ppm'
    document={'camera':{'position':[0,1,5],'target':[0,0,0]},
              'render':{'width':80,'height':48,'samples':2,'threads':1,'output':str(output)},
              'materials':{'m':{'color':[.8,.1,.1]}},
              'objects':[{'type':'sphere','material':'m'}],
              'lights':[{'type':'ambient','intensity':.3}]}
    scene.write_text(json.dumps(document))
    process=subprocess.Popen([str(binary),'--gui','--watch','--frames','90',str(scene)],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    try:
        deadline=time.monotonic()+5
        while not output.exists():
            assert process.poll() is None,'GUI exited before first render'
            assert time.monotonic()<deadline,'GUI did not render'
            time.sleep(.03)
        initial=output.read_bytes()
        scene.write_text('{invalid json')
        time.sleep(.25)
        assert process.poll() is None,'GUI died after invalid reload'
        document['materials']['m']['color']=[.1,.8,.1]
        scene.write_text(json.dumps(document))
        out,error=process.communicate(timeout=10)
        assert process.returncode==0,(process.returncode,error.decode(errors='replace'))
        assert output.read_bytes()!=initial,'valid reload did not update image'
    finally:
        if process.poll() is None:process.kill();process.wait()
    document['render'].update(integrator='photon',photons=2000000)
    document['lights']=[{'type':'point','position':[0,5,0]}]
    scene.write_text(json.dumps(document))
    start=time.monotonic()
    result=subprocess.run([str(binary),'--gui','--frames','2',str(scene)],capture_output=True,timeout=10)
    assert result.returncode==0,(result.returncode,result.stderr.decode(errors='replace'))
    assert time.monotonic()-start<8,'photon preparation did not honor cancellation'
    result=subprocess.run([str(binary),'--gui','--preview','--frames','2',str(scene)],capture_output=True,timeout=10)
    assert result.returncode==0,(result.returncode,result.stderr.decode(errors='replace'))
print('SFML progressive render, invalid->valid reload, photon cancellation and preview passed')
