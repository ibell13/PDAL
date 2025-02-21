(writers.spz)=

# writers.spz

The **SPZ writer** writes files in the [spz] format, designed for
storing compressed [gaussian splat] data.

```{note}

```

```{eval-rst}
.. embed::
```

## Example

```json
[
    {
        "type":"readers.ply",
        "filename":"inputfile.ply"
    },
    {
        "type":"writers.spz",
        "antialiased":true,
        "filename":"outputfile.spz"
    }
]
```

## Options

filename

: File to write. \[Required\]

antialiased

: Whether to mark the output file as containing antialiased data.
  \[Default: false\]

```{include} writer_opts.md
```

[spz]: https://github.com/nianticlabs/spz
[gaussian splat]:
