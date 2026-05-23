"""Scene-building helpers for authored, reproducible Raytracer examples."""


def material(color, *, physical=True, **fields):
    return {'type': 'pbr' if physical else 'phong', 'color': color, **fields}


def object_(kind, name, position=(0, 0, 0), rotation=(0, 0, 0), scale=(1, 1, 1), **fields):
    return {'type': kind, 'material': name, 'translate': list(position),
            'rotate': list(rotation), 'scale': list(scale), 'shear': [0, 0, 0], **fields}


def plane(name, normal=(0, 1, 0), position=(0, 0, 0)):
    return object_('plane', name, position, normal=list(normal))


def area(position, u=(3, 0, 0), v=(0, 0, 3), intensity=5, color=(1, 1, 1)):
    return {'type': 'area', 'position': list(position), 'u': list(u), 'v': list(v),
            'intensity': intensity, 'color': list(color)}


def scene(camera=(7, 5, 11), target=(0, 1, 0), *, physical=True, samples=256, width=960, height=640):
    return {
        'camera': {'position': list(camera), 'target': list(target), 'up': [0, 1, 0], 'fov': 42},
        'render': {'width': width, 'height': height, 'samples': samples, 'threads': 0,
                   'depth': 10 if physical else 5, 'integrator': 'pbr' if physical else 'whitted',
                   'toneMap': 'neutral' if physical else 'none', 'colorSpace': 'srgb', 'exposure': 1},
        'background': [0.015, 0.02, 0.03], 'materials': {}, 'objects': [], 'lights': [],
    }


def palette(physical=False):
    colors = {'red': [0.75, 0.015, 0.025], 'green': [0.02, 0.65, 0.04],
              'blue': [0.035, 0.04, 0.8], 'yellow': [0.8, 0.7, 0.02],
              'cyan': [0.015, 0.65, 0.6], 'magenta': [0.65, 0.02, 0.4],
              'white': [0.72, 0.72, 0.72], 'floor': [0.48, 0.48, 0.48]}
    return {key: material(value, physical=physical, **({'roughness': 0.3} if physical else
                                                     {'specular': 0.6, 'shininess': 100}))
            for key, value in colors.items()}


def classic_lights():
    return [{'type': 'ambient', 'intensity': 0.12},
            {'type': 'point', 'position': [-4, 7, 6], 'intensity': 1.7, 'falloff': 0}]


def hourglass(objects, position, radius=1, height=2, name='green'):
    x, y, z = position
    objects.append(object_('limited_cone', name, [x, y, z], radius=radius, height=height))
    objects.append(object_('limited_cone', name, [x, y + 2 * height, z], [180, 0, 0],
                           radius=radius, height=height))


def beam(axis, a, b, name, radius=0.13, half_length=9):
    if axis == 'x':
        return object_('limited_cylinder', name, [half_length, a, b], [0, 0, 90],
                       radius=radius, height=2 * half_length)
    if axis == 'y':
        return object_('limited_cylinder', name, [a, -half_length, b],
                       radius=radius, height=2 * half_length)
    return object_('limited_cylinder', name, [a, b, -half_length], [90, 0, 0],
                   radius=radius, height=2 * half_length)


def specimen(kind, name='specimen'):
    definitions = {
        'sphere': object_('sphere', name, radius=1),
        'plane': plane(name, [0, 1, 0], [0, -0.8, 0]),
        'cylinder': object_('cylinder', name, radius=0.75),
        'cone': object_('cone', name, radius=0.65),
        'limited_cylinder': object_('limited_cylinder', name, [0, -1.1, 0], radius=0.8, height=2.2),
        'limited_cone': object_('limited_cone', name, [0, -1.1, 0], radius=1.1, height=2.2),
        'box': object_('box', name, rotation=[10, 25, 0], size=[1.7, 1.7, 1.7]),
        'triangle': object_('triangle', name, rotation=[0, 15, 0],
                            a=[-1.3, -1, 0], b=[1.3, -1, 0], c=[0, 1.4, 0]),
        'torus': object_('torus', name, rotation=[65, 20, 0], major=0.95, minor=0.3),
        'tanglecube': object_('tanglecube', name, rotation=[12, 22, 0], scale=[0.55] * 3),
        'fractal': object_('fractal', name, rotation=[10, -25, 0], iterations=10),
        'mobius': object_('mobius', name, rotation=[60, 15, 0], radius=1.1, width=0.38, segments=160),
        'obj': object_('obj', name, [0, -1.3, 0], scale=[0.85] * 3,
                       file='../bonus/assets/crystal.obj'),
        'group': {'type': 'group', 'translate': [0, -0.8, 0], 'rotate': [0, 20, 0],
                  'scale': [1, 1, 1], 'shear': [0, 0, 0], 'children': [
                      object_('box', name, size=[1.5, 0.4, 1.5]),
                      object_('torus', name, [0, 0.9, 0], [90, 0, 0], major=0.75, minor=0.23)]},
        'plugin_sphere': object_('plugin_sphere', name, radius=1),
    }
    return definitions[kind]
