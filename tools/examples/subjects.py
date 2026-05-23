"""Independently modelled scenes inspired by selected subject illustrations."""
import math
from .common import beam, classic_lights, hourglass, material, object_, palette, plane, scene


def lattice():
    s = scene([1.2, 1.1, 5.5], [-2, -1.5, -7], physical=False, samples=64)
    s['camera']['fov'] = 75
    s['background'] = [0, 0, 0]
    s['materials'] = palette()
    for axis, name in (('x', 'red'), ('y', 'green'), ('z', 'blue')):
        for a in (-6, -3, 0, 3, 6):
            for b in (-6, -3, 0, 3, 6):
                s['objects'].append(beam(axis, a, b, name))
    s['lights'] = [*classic_lights(), {'type': 'point', 'position': [3, 2, -6], 'intensity': 1.3}]
    return s


def column():
    s = scene([7, 4.3, 10], [0, 2.5, 0], physical=False, samples=96)
    s['camera']['fov'] = 38
    s['materials'] = palette()
    s['materials']['metal'] = material([0.65] * 3, physical=False, reflection=0.25,
                                       specular=0.8, shininess=100)
    s['objects'] = [plane('red'), plane('red', [0, -1, 0], [0, 6, 0]),
                    plane('red', [0, 0, 1], [0, 0, -4.5]),
                    object_('limited_cylinder', 'metal', radius=1, height=5.5)]
    for x in (-3.4, 3.4):
        for z in (-3, 0):
            hourglass(s['objects'], [x, 0, z], radius=0.6, height=3)
    for offset, color in ((0, 'blue'), (2.1, 'yellow'), (4.2, 'magenta')):
        for i in range(44):
            t = i * 0.18 + offset
            s['objects'].append(object_('sphere', color,
                                        [1.18 * math.cos(t), 0.3 + i * 0.116, 1.18 * math.sin(t)],
                                        radius=0.17))
    s['lights'] = [{'type': 'ambient', 'intensity': 0.13},
                   {'type': 'point', 'position': [-1.5, 5.5, 4], 'intensity': 2.3}]
    return s


SUBJECTS = [('02_rgb_lattice', 'RGB cylinder lattice', lattice),
            ('04_helix_column', 'Helices around a reflective column', column)]
