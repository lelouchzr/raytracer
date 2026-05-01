# Scene compatibility fixtures

`original/*.txt` are unchanged test fixtures copied from the original repository's
`data/` directory. Six valid examples are accepted. `config.txt` and `final.txt`
contain negative colors or other invalid geometry; the adapter diagnoses them
instead of silently changing their meaning. `subject.cfg` exercises the libconfig
syntax shown by the project specification without requiring libconfig itself.

The adapter preserves the original camera target, repeated primitive blocks,
finite parallelograms (two triangles), material categories, HDR emission and
translations. All-zero historical scales mean identity because the original
renderer ignored those placeholders. Historical alpha was unused by the original
renderer and does not override the selected material category. Directional lights
without a direction use `[-1,-2,-1]` because the historical format only stored color.

Libconfig camera rotation uses Euler X/Y/Z in degrees, forward +Y and up +Z; the
camera in the supplied example therefore looks towards the scene from negative Y.
Colors with components above one use the specification's 0..255 convention.

Modern JSON goes to the existing strict JSON parser unchanged. The compatibility
layer is available with the bonus build. It parses input and produces the normal
scene model; all geometric, optical and output limits still pass through the usual
scene builder. Acceptance of a legacy file does not promise pixel identity with
the old renderer's optical bugs or sampling algorithm.
