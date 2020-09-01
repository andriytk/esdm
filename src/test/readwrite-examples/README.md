Example instructions for the read-write benchmark
=================================================

This directory contains instruction files for use with the readwrite-benchmark.
**Be warned, some of these write/read quite a lot of data!**

These are organized into a number of different scenarios.
Some scenarios consist only of a single file that is supposed to be used both for writing and reading.
More interesting scenarios consist of separate read/write instruction files,
these end either in the suffix "-write" or "-read" after a common prefix.
Some more complex scenarios may contain several "-write" or "-read" file,
in which case a sequence number is attached at the end of the file name.

The different instruction sets are described below:

  * climate-analysis-40

      * -write

        Simulates output of a single variable (8bytes per value) from a climate model using a 500x1000 lat/lon grid.
        Output is performed on a daily basis over the course of one year, with an ensemble of five runs.
        Since this simulates the output of an ensemble simulation run,
        each write request touches only a single time point / ensemble member.
        Total written data size is 8B * 500*1000 * 365 * 5 = 7.3GB

      * -read1, -read2, -read3

        Simulates read operation for different regional analysis.
        The analysed areas overlap strongly.
        Since this simulates a statistical analysis, each read requests data from all timesteps and ensemble members at once.
        The data is partitioned along the surface coordinates, only.

         1. Regional analysis of 125x250 ground cells over the entire year.
            Reads a total of 456MB of data.

         2. Regional, seasonal analysis of 150x200 ground cells over half a year.
            Reads a total of 216MB of data.

         3. Zonal, seasonal analysis of 100x1000 ground cells over three months.
            Reads a total of 360MB of data.

  * satellite-path

    Thanks to Armin Corbin (Institute of Geodesy and Geoinformation - University of Bonn) for supplying us with this usecase.

      * -generator.c

        This use case uses rather complex read instructions, so they are generated by a small program.
        This generator is found in satellite-path-generator.c, and the read instructions can be generated with

            gcc -o satellite-path-generator -lm satellite-path-generator.c
            ./satellite-path-generator > satellite-path-read

      * -write

        Produces two datasets that are written as time slices of a 3D grid.
        The dimensions are 57 height levels, 72 latitude and 144 longitude values.
        There are 144 time slices, modelling output every 10 minutes over the course of a day.

      * -read

        This file is generated by the program in satellite-path-generator.c, please refer to the instructions above.

        Analysis of data along the path of a satellite.
        The satellite path is simplified to follow a sine wave, wrapping the earth every 90 minutes.
        The data is sampled below the satellite position every 10 seconds,
        producing a total of 360 samples per orbit and 8640 samples total.

        For each sample, two read requests are made:

          * From the first variable, eight full height columns are read which are next to the satellite position in time, lat, and lon dimensions.
            I.e., the shape of the request is (2, 57, 2, 2).

          * From the second variable, only a hypercube with the shape (2, 2, 2, 2) is accessed below the satellite.
            The height (second dimension) of the request is chosen at random in this benchmark,
            in the original usecase, the hight was determined from the previous full-column request.