"""Original faceted head sculpture, authored from explicit anatomical landmarks (MIT)."""
import math


# Height, half-width and facial depth: dense rows follow the lips, nose and eye sockets.
PROFILES = (
    (0.00, 0.58, 0.62), (0.23, 0.79, 0.78), (0.48, 0.91, 0.82),
    (0.64, 0.95, 0.83), (0.78, 0.98, 0.85), (0.94, 1.01, 0.83),
    (1.13, 1.04, 0.84), (1.29, 1.08, 0.87), (1.49, 1.10, 0.89),
    (1.72, 1.09, 0.87), (1.93, 1.07, 0.84), (2.09, 1.06, 0.87),
    (2.27, 1.04, 0.88), (2.53, 0.98, 0.80), (2.78, 0.82, 0.66),
    (2.98, 0.51, 0.39), (3.06, 0.20, 0.18),
)
COLUMNS = (-1, -.78, -.56, -.38, -.20, 0, .20, .38, .56, .78, 1)


def bump(x, y, center_x, center_y, width, height):
    return math.exp(-((x - center_x) / width) ** 2 - ((y - center_y) / height) ** 2)


def facial_depth(x, y, fraction, base):
    z = base * math.sqrt(1 - .87 * fraction * fraction)
    # The bridge and nose tip stand in front of the cheek and upper-lip planes.
    z += .34 * bump(x, y, 0, 1.88, .17, .48)
    z += .63 * bump(x, y, 0, 1.43, .21, .23)
    for side in (-1, 1):
        z += .18 * bump(x, y, side * .22, 1.30, .13, .12)  # nostril wing
        z -= .35 * bump(x, y, side * .43, 1.96, .24, .15)  # eye socket
        z += .16 * bump(x, y, side * .43, 2.21, .31, .12)  # brow ridge
        z += .18 * bump(x, y, side * .64, 1.60, .25, .28)  # cheekbone
    z += .20 * bump(x, y, 0, .79, .43, .075)  # upper lip
    z -= .18 * bump(x, y, 0, .65, .44, .045)  # mouth opening / lip crease
    z += .17 * bump(x, y, 0, .49, .39, .09)   # lower lip
    z += .10 * bump(x, y, 0, .23, .40, .18)   # chin
    return z


def head_geometry():
    """Return an outward-wound, closed triangle mesh; no interpolated normals."""
    vertices, faces = [], []
    rows, columns = len(PROFILES), len(COLUMNS)
    sheet = rows * columns
    for back in (False, True):
        for y, half_width, base in PROFILES:
            for fraction in COLUMNS:
                x = half_width * fraction
                if back:
                    z = -.24 - .70 * math.sqrt(1 - fraction * fraction) * math.sin(
                        .10 + .87 * math.pi * y / PROFILES[-1][0])
                else:
                    z = facial_depth(x, y, fraction, base)
                vertices.append((x, y, z))

    def quad(a, b, c, d, reverse=False, alternate=False):
        triangles = ((a, b, d), (b, c, d)) if alternate else ((a, b, c), (a, c, d))
        faces.extend(tuple(reversed(t)) if reverse else t for t in triangles)

    for offset in (0, sheet):
        for row in range(rows - 1):
            for column in range(columns - 1):
                a = offset + row * columns + column
                quad(a, a + 1, a + columns + 1, a + columns, offset != 0,
                     (row + column) % 2 == 1)
    # Bridge the two sheets around all four boundaries without duplicated seam vertices.
    for row in range(rows - 1):
        left, right = row * columns, row * columns + columns - 1
        quad(left, left + columns, left + columns + sheet, left + sheet)
        quad(right, right + sheet, right + columns + sheet, right + columns)
    for column in range(columns - 1):
        a = column
        quad(a, a + sheet, a + sheet + 1, a + 1)
        a = (rows - 1) * columns + column
        quad(a, a + 1, a + sheet + 1, a + sheet)
    return vertices, faces


def head_mesh():
    vertices, faces = head_geometry()
    return ('# Original faceted head, tools/examples/head.py; MIT. Front faces +Z.\n'
            'o FacetedHead\ns off\n' +
            ''.join('v ' + ' '.join(f'{v:.8g}' for v in point) + '\n' for point in vertices) +
            ''.join('f ' + ' '.join(str(i + 1) for i in face) + '\n' for face in faces))
