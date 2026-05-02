#!/usr/bin/env python3
"""Real CLI tests for format codecs, advanced scenes, plugins and two TCP workers."""
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import time

binary=Path(sys.argv[1]).resolve()
root=Path(sys.argv[2]).resolve()
bad_abi=Path(sys.argv[3]).resolve()
count=0

def run(args,code=0,cwd=None):
    global count
    result=subprocess.run([str(binary),*map(str,args)],cwd=cwd or root,capture_output=True,timeout=90)
    assert result.returncode==code,(args,result.returncode,result.stderr.decode(errors='replace'))
    if code: assert result.stderr and not result.stdout
    count+=1
    return result

def free_port():
    with socket.socket() as server:
        server.bind(('127.0.0.1',0))
        return server.getsockname()[1]

with tempfile.TemporaryDirectory(prefix='raytracer-systems-') as folder:
    folder=Path(folder)
    output=folder/'image.ppm'
    source=root/'bonus/scenes/gallery.json'
    template=json.loads(source.read_text())
    template['render'].update(width=48,height=32,samples=4,ao=0,output=str(output))
    config=folder/'scene.json'
    def save(scene): config.write_text(json.dumps(scene))
    save(template)
    run([config])
    run(['--gui','--frames','2junk',config],84)
    run(['--preview',config])
    for name in ('torus','tanglecube','fractal','mobius','scenegraph','global_illumination','caustics','nested_media','adaptive','normal_perlin','plugin'):
        path=root/'bonus/scenes'/f'{name}.json'
        scene=json.loads(path.read_text())
        scene['render'].update(width=40,height=24,samples=3,ao=0,output=str(output))
        if 'photons' in scene['render']:scene['render']['photons']=5000
        for mat in scene.get('materials',{}).values():
            for key in ('file','normalFile'):
                if key in mat:mat[key]=str((path.parent/mat[key]).resolve())
        if 'plugins' in scene:scene['plugins']=[str(Path(sys.argv[4]).resolve())]
        save(scene)
        run([config])
        assert output.read_bytes().startswith(b'P6\n40 24\n255\n')
    pixels=bytes([255,0,0,0,255,0,0,0,255,255,255,255])
    (folder/'color.ppm').write_bytes(b'P6\n# binary fixture\n2 2\n255\n'+pixels)
    (folder/'sixteen.ppm').write_bytes(b'P6\n1 1\n65535\n'+struct.pack('>HHH',65535,32768,0))
    bmp_row0=bytes([255,0,0,255,255,255])+b'\0\0'
    bmp_row1=bytes([0,0,255,0,255,0])+b'\0\0'
    bmp_data=bmp_row0+bmp_row1
    (folder/'color.bmp').write_bytes(b'BM'+struct.pack('<IHHI',54+len(bmp_data),0,0,54)+struct.pack('<IiiHHIIIIII',40,2,2,1,24,0,len(bmp_data),0,0,0,0)+bmp_data)
    header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,2,2,24,32)
    bgr=b''.join(bytes([pixels[i+2],pixels[i+1],pixels[i]]) for i in range(0,len(pixels),3))
    (folder/'color.tga').write_bytes(header+bgr)
    (folder/'rle.tga').write_bytes(header[:2]+bytes([10])+header[3:]+bytes([3])+bgr)
    scene=json.loads(json.dumps(template))
    scene['materials']['image']={'type':'phong','pattern':'image','file':''}
    scene['objects']=[{'type':'plane','material':'image'},{'type':'sphere','material':'coral','translate':[0,1,0]}]
    for image in ('color.ppm','sixteen.ppm','color.bmp','color.tga','rle.tga'):
        scene['materials']['image']['file']=str(folder/image)
        save(scene)
        run([config])
        if image=='color.ppm':reference_codec=output.read_bytes()
        elif image!='sixteen.ppm':assert output.read_bytes()==reference_codec,f'{image} decoded differently'
    (folder/'broken.ppm').write_bytes(b'P6\n4 4\n255\n\0')
    scene['materials']['image']['file']=str(folder/'broken.ppm')
    save(scene)
    run([config],84)
    # Concave L-shaped polygon, normals, UV, MTL and texture referenced through MTL.
    (folder/'mesh.mtl').write_text('newmtl Painted\nKd 1 1 1\nNs 32\nmap_Kd color.ppm\n')
    (folder/'mesh.obj').write_text('mtllib mesh.mtl\nv -1 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nv 0 2 0\nv -1 2 0\nvt 0 0\nvt 1 0\nvt 1 0.5\nvt 0.5 0.5\nvt 0.5 1\nvt 0 1\nvn 0 0 1\nusemtl Painted\nf 1/1/1 2/2/1 3/3/1 4/4/1 5/5/1 6/6/1\n')
    scene=json.loads(json.dumps(template))
    scene['objects']=[{'type':'obj','material':'coral','file':str(folder/'mesh.obj')}]
    save(scene)
    run([config])
    scene=json.loads(json.dumps(template))
    scene['plugins']=[str(bad_abi)]
    save(scene)
    run([config],84)
    scene['plugins']=[str(folder/'missing.so')]
    save(scene)
    run([config],84)
    # Both real workers receive different row tiles from the coordinator.
    scene=json.loads(json.dumps(template))
    scene['render'].update(width=48,height=48,samples=7,threads=1)
    save(scene)
    run([config])
    expected=output.read_bytes()
    ports=[free_port(),free_port()]
    workers=[subprocess.Popen([str(binary),'--worker',str(port),str(folder)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL) for port in ports]
    try:
        for port in ports:
            deadline=time.monotonic()+10
            while True:
                try:
                    with socket.create_connection(('127.0.0.1',port),timeout=.1):break
                except OSError:
                    assert time.monotonic()<deadline,'worker did not start'
                    time.sleep(.03)
        endpoints=','.join(f'127.0.0.1:{p}' for p in ports)
        run(['--cluster',endpoints,config],cwd=folder)
        assert output.read_bytes()==expected,'distributed render differs from local render'
        # Scene/mesh suffixes cannot bypass resource confinement.
        (folder/'unsafe.scene').write_text(json.dumps({'include':[str(source)]}))
        (folder/'unsafe.mesh').write_text('mtllib '+str(root/'tests/legacy/subject.cfg')+'\n')
        unsafe=json.loads(json.dumps(scene))
        unsafe['objects']=[{'type':'obj','material':'coral','file':'unsafe.mesh'}]
        (folder/'mesh-scene.data').write_text(json.dumps(unsafe))
        for name in ('unsafe.scene','mesh-scene.data'):
            with socket.create_connection(('127.0.0.1',ports[0])) as client:
                request={'version':1,'scene':name,'begin':0,'end':1,'width':48,'height':48,'samples':1}
                data=json.dumps(request).encode();client.sendall(struct.pack('>I',len(data))+data)
                length=struct.unpack('>I',client.recv(4))[0]
                response=client.recv(length)
                assert response.startswith(b'RTR1 ERROR') and b'escapes root' in response,response
        # Protocol version mismatch and traversal rejection are real server responses.
        for request in ({'version':99,'scene':'scene.json','begin':0,'end':1,'width':48,'height':48,'samples':1},
                        {'version':1,'scene':'../scene.json','begin':0,'end':1,'width':48,'height':48,'samples':1}):
            with socket.create_connection(('127.0.0.1',ports[0])) as client:
                data=json.dumps(request).encode()
                client.sendall(struct.pack('>I',len(data))+data)
                length=struct.unpack('>I',client.recv(4))[0]
                response=client.recv(length)
                assert response.startswith(b'RTR1 ERROR'),response
        run(['--cluster',f'127.0.0.1:{free_port()}',config],84,cwd=folder)
    finally:
        for worker in workers:worker.terminate()
        for worker in workers:
            try:worker.wait(timeout=5)
            except subprocess.TimeoutExpired:worker.kill();worker.wait()
    if os.environ.get('RAYTRACER_TEST_GUI')=='1':
        save(template)
        run(['--gui','--watch','--frames','8',config])
print(f'{count} extended CLI scenarios plus two-worker protocol checks passed')
