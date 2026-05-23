"""Physically lit product, geometry and global-illumination examples."""
from .common import area, material, object_, plane, scene, specimen


def studio_materials():
    return {
        'floor': material([0.38, 0.4, 0.43], roughness=0.8),
        'plinth': material([0.66, 0.64, 0.58], roughness=0.55),
        'gold': material([0.83, 0.55, 0.2], metallic=1, roughness=0.18),
        'silver': material([0.8, 0.84, 0.9], metallic=1, roughness=0.16),
        'ceramic': material([0.025, 0.26, 0.22], roughness=0.25),
        'coral': material([0.65, 0.035, 0.02], roughness=0.32),
        'glass': material([0.98, 0.99, 1], roughness=0.02, transmission=1,
                           ior=1.5, absorption=[0.18, 0.035, 0.015]),
    }


def softboxes():
    return [area([-3, 7, 4], u=[4, 0, 0], v=[0, 0, 4], intensity=5),
            area([-10, 10, 2], u=[0, 0, -5], v=[0, 6, 0], intensity=8, color=[0.72, 0.85, 1]),
            area([-1, 10, -4], u=[4, 0, 0], v=[0, 4, 0], intensity=6, color=[1, 0.83, 0.65])]


def museum():
    s = scene([11, 12, 16], [0, 0.9, 0], samples=256, width=1200, height=800)
    s['camera']['fov'] = 32
    s['materials'] = studio_materials()
    s['objects'] = [plane('floor')]
    kinds = ['sphere', 'limited_cylinder', 'limited_cone', 'box', 'triangle', 'torus',
             'tanglecube', 'mobius', 'fractal', 'obj', 'group', 'sphere']
    colors = ['silver', 'ceramic', 'coral', 'silver', 'coral', 'silver',
              'coral', 'gold', 'ceramic', 'silver', 'ceramic', 'glass']
    for i, (kind, name) in enumerate(zip(kinds, colors)):
        x, z = (i % 4 - 1.5) * 3.1, (i // 4 - 1) * 3.4
        s['objects'].append(object_('box', 'plinth', [x, 0.28, z], size=[2.65, 0.56, 2.65]))
        item = specimen(kind, name)
        # Nest each unchanged local specimen under a shared display transform.
        s['objects'].append({'type': 'group', 'translate': [x, 1.55, z], 'rotate': [0, 0, 0],
                             'scale': [0.8] * 3, 'shear': [0, 0, 0], 'children': [item]})
    s['lights'] = [area([-4, 12, 5], u=[9, 0, 0], v=[0, 0, 7], intensity=5),
                    area([6, 15, -5], u=[7, 0, 0], v=[0, 4, 0], intensity=5)]
    return s


def product():
    s = scene([5, 3.2, 8], [-0.3, 1.05, 0], samples=768)
    s['camera']['fov'] = 28
    s['materials'] = studio_materials()
    s['materials']['floor'] = material([0.24, 0.27, 0.3], roughness=0.65)
    s['objects'] = [plane('floor'),
                    object_('sphere', 'glass', [1.25, 1, 0.2], radius=1),
                    object_('torus', 'gold', [-1.4, 1.05, 0], [80, 20, 0], major=0.8, minor=0.24),
                    object_('box', 'ceramic', [-0.4, 0.35, -1.4], [0, 15, 0], size=[1.2, 0.7, 1.2]),
                    object_('sphere', 'coral', [-2.8, 0.45, 0.1], scale=[1, 0.8, 1], radius=0.56)]
    s['lights'] = softboxes()
    s['render']['depth'] = 12
    return s


def cornell():
    s = scene([0, 2.5, 9.8], [0, 2.3, 0], samples=768, width=960, height=800)
    s['camera']['fov'] = 38
    s['background'] = [0, 0, 0]
    s['materials'] = {
        'white': material([0.72] * 3, roughness=0.8),
        'red': material([0.62, 0.025, 0.025], roughness=0.6),
        'green': material([0.035, 0.42, 0.08], roughness=0.6),
        'glass': material([1, 1, 1], roughness=0.025, transmission=1, ior=1.5,
                           absorption=[0.045, 0.01, 0.007]),
    }
    s['objects'] = [plane('white'), plane('white', [0, -1, 0], [0, 5, 0]),
                    plane('white', [0, 0, 1], [0, 0, -2.5]),
                    plane('red', [1, 0, 0], [-2.5, 0, 0]),
                    plane('green', [-1, 0, 0], [2.5, 0, 0]),
                    object_('box', 'white', [-1.1, 1.05, -0.5], [0, 22, 0],
                            size=[1.35, 2.1, 1.4]),
                    object_('sphere', 'glass', [1.05, 1.0, 0.55], radius=1)]
    s['lights'] = [area([0, 4.97, 0], [1.7, 0, 0], [0, 0, 1.7], 12)]
    s['render']['depth'] = 14
    return s


def sculpture():
    s = scene([.65, 2.55, 8.2], [.4, 1.85, -1.3], samples=2048, width=1100, height=640)
    s['camera']['fov'] = 37
    s['background'] = [0, 0, 0]
    s['materials'] = {'plaster': material([.55, .5335, .517], roughness=.9),
                      'red': material([.80, .008, .012], roughness=.23)}
    s['objects'] = [plane('plaster'), plane('plaster', [0, -1, 0], [0, 4.7, 0]),
                    plane('plaster', [0, 0, 1], [0, 0, -2.8]),
                    plane('plaster', [1, 0, 0], [-1.6, 0, 0]),
                    plane('plaster', [-1, 0, 0], [3.4, 0, 0]),
                    object_('obj', 'red', [-.15, .002, -1.75], [0, 25, 0], scale=[.9] * 3,
                            file='07_cornell_sculpture.obj')]
    # The upper panel emits toward the wall: its opaque black back faces the camera.
    s['lights'] = [area([-1.05, .08, -.15], [.95, 0, 0], [0, -.1, -.65], 70),
                    area([2.45, 3.65, -.65], [.55, 0, .85], [0, -1.05, 0], 10)]
    s['render'].update(depth=14, exposure=1.2)
    return s


def textures():
    s = scene([7, 7.5, 11], [0, 0.9, 0], samples=256)
    s['camera']['fov'] = 40
    s['materials'] = studio_materials()
    s['materials'].update({
        'checker': material([0.78] * 3, second=[0.04] * 3, pattern='checker', scale=3, roughness=0.4),
        'marble': material([0.7, 0.76, 0.72], second=[0.025, 0.08, 0.1],
                            pattern='marble', scale=3, roughness=0.35),
        'perlin': material([0.7, 0.23, 0.07], second=[0.05, 0.015, 0.008],
                            pattern='perlin', scale=6, roughness=0.55),
        'image': material([1, 1, 1], pattern='image', file='../bonus/assets/mosaic.ppm', roughness=0.4),
        'normal': material([0.12, 0.3, 0.55], metallic=0.6, roughness=0.3,
                            normalFile='../bonus/assets/normal.ppm', normalStrength=0.7, scale=5),
    })
    s['objects'] = [plane('floor')]
    for i, name in enumerate(('checker', 'marble', 'perlin', 'image', 'normal', 'gold')):
        x, z = (i % 3 - 1) * 2.3, (i // 3 - 0.5) * 2.7
        s['objects'].append(object_('sphere', name, [x, 1, z], radius=1))
    s['lights'] = softboxes()
    return s


PHYSICAL = [('05_primitive_museum', 'Finite primitive museum', museum),
            ('06_product_studio', 'Glass, gold and ceramic product study', product),
            ('07_cornell_sculpture', 'Faceted red head and indirect illumination', sculpture),
            ('08_cornell_glass', 'Cornell box, glass and color bleeding', cornell),
            ('09_texture_materials', 'Textures, normal maps and rough metals', textures)]
