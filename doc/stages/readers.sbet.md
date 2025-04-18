(readers.sbet)=

# readers.sbet

The **SBET reader** read from files in the SBET format, used for exchange data from inertial measurement units (IMUs).
SBET files store angles as radians, but by default this reader converts all angle-based measurements to degrees.
Set `angles_as_degrees` to `false` to disable this conversion.

```{eval-rst}
.. embed::
```

```{eval-rst}
.. streamable::
```

## Example

```json
[
    "sbetfile.sbet",
    "output.las"
]
```

## Options

filename

: File to read from \[Required\]
  Refer to {ref}`filespec` \[Required\]

```{include} reader_opts.md
```

angles_as_degrees

: Convert all angles to degrees. If false, angles are read as radians. \[Default: true\]
