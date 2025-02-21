(readers.spz)=

# writers.spz

The **SPZ reader** reads points in the [spz] format, designed for
storing compressed [gaussian splat] data. 

```{eval-rst}
.. embed::
```

## Example

```json
[
    {
        "type":"readers.spz",
        "filename":"inputfile.spz"
    },
    {
        "type":"writers.ply",
        "filename":"outputfile.ply"
    }
]
```

## Options

filename

: File to write. \[Required\]

```{include} reader_opts.md
```

[spz]: https://github.com/nianticlabs/spz
[gaussian splat]:
