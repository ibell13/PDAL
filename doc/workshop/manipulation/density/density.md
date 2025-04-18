(density)=

# Visualizing acquisition density

```{index} Density, OGR, hexagon tessellation
```

This exercise uses PDAL to generate a density surface. You can use this
surface to summarize acquisition quality.

## Exercise

PDAL provides an {ref}`application <density_command>` to compute a vector
field of hexagons computed with {ref}`filters.hexbin`. It is a kind of
simple interpolation, which we will use for visualization in {{ QGIS }}.

### Command

Invoke the following command, substituting accordingly, in your `Shell`:

```console
$ pdal density ./exercises/analysis/density/uncompahgre.laz  \
-o ./exercises/analysis/density/density.sqlite \
-f SQLite
```

```doscon
> pdal density ./exercises/analysis/density/uncompahgre.laz  ^
-o ./exercises/analysis/density/density.sqlite ^
-f SQLite
```

### Visualization

The command uses GDAL to output a [SQLite] file containing hexagon polygons.
We will now use {{ QGIS }} to visualize them:

1. Add a vector layer

   > ```{image} ../../images/density-add-layer.png
   > :target: ../../../_images/density-add-layer.png
   > ```

2. Navigate to the output directory

   > ```{image} ../../images/density-select-layer.png
   > :target: ../../../_images/density-select-layer.png
   > ```

3. Add the `density.sqlite` file to the view

   > ```{image} ../../images/density-file-open.png
   > :target: ../../../_images/density-file-open.png
   > ```

4. Right click on the `density.sqlite` layer in the `Layers` panel
   and then choose `Properties`

5. Within the `Symbology` tab, change `Single Symbol` to `Graduated` in the drop down

   > ```{image} ../../images/density-graduated-symbols-pick.png
   > :target: ../../../_images/density-graduated-symbols-pick.png
   > ```

6. Choose the `Count` column to visualize

   > ```{image} ../../images/density-count-attribute.png
   > :target: ../../../_images/density-count-attribute.png
   > ```

7. Choose the `Classify` button to add intervals

   > ```{image} ../../images/density-graduated-symbols.png
   > :target: ../../../_images/density-Graduated-symbols.png
   > ```

8. Adjust the visualization as desired

   > ```{image} ../../images/density-final-render.png
   > :target: ../../../_images/density-final-render.png
   > ```

## Notes

1. You can control how the density hexagon surface is created by
   using the options in {ref}`filters.hexbin`.

   The following settings will use a hexagon edge size of 24
   units.

   ```
   --filters.hexbin.edge_size=24
   ```

2. You can generate a contiguous boundary using {{ PDAL }}'s {ref}`tindex_command`.

[sqlite]: http://sqlite.org
[uncompahgre basin]: https://en.wikipedia.org/wiki/Uncompahgre_River
