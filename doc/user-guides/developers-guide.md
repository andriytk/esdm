Developers Guide
================

This document aims to give a quick conceptual overview of the API that is available to programs which link directly to ESDM.
It is not meant to be exhaustive, but rather to give enough background information so that reading the documentation of the ESDM API becomes easy.
The intended audience of this guide are developers of scientific applications who wish to benefit of the full potential of the ESDM API.

Readers who are interested in using an ESDM file system with a NetCDF based application should read the users guide instead.
Unfortunately, there is no guide for ESDM backend developers and ESDM core contributors yet.



Data Model
----------

The first thing to understand is the data model that is used by ESDM.
This data model is very similar to that employed by NetCDF, but it does add some abstractions, and is described in this section.


### Dataspace

A dataspace describes how data is stored in memory, it is basically a mapping of a logical dataspace to sequential bytes in memory.
All data handled by ESDM is associated with a dataspace, mostly the dataspaces are user provided.
Several copies of the same data may use distinct dataspaces,
and ESDM allows users to transform data from one layout to another by providing a source and a destination dataspace (`esdm_dataspace_copy_data()`).
User code needs to provide a dataspace when it defines a variable (here the data layout part is irrelevant), when it stores data, and when it reads data.

The logical dataspace is always a multidimensional, axis aligned box in ESDM.
As such, a dataspace consists of the following information:

  * the dimension count

  * start and end coordinates for each axis

  * a datatype describing a single value within the multidimensional array

  * (data serialization information)

The data serialization information is usually implicit, ESDM will simply assume C multidimensional array layout.
Fortran programs will need to set the data serialization information explicitly to match the inverse dimension order of Fortran.
The data serialization information (`stride`) can also achieve unorthodox effects like arrays with holes, or to replicate a single 2D slice along a third axis.

#### Creating and Destroying Dataspaces

ESDM provides two distinct mechanisms to create a dataspace:
A generic API which allocates the dataspace on the heap, and an API to quickly create a throw-away dataspace on the stack.

##### The Generic API

The functions used to create dataspaces are:

  * `esdm_dataspace_create()`

    Constructs a dataspace with a given dimension count, size and datatype.
    This assumes that the hypercube starts at (0, 0, ..., 0) and C data layout.

  * `esdm_dataspace_create_full()`

    Like `esdm_dataspace_create()`, but allows the user to specify start coordinates.

  * `esdm_dataspace_copy()`

    Copy constructor.

  * `esdm_dataspace_subspace()`

    Create a dataspace that contains a subset of the logical space of its parent dataspace.
    This copies the datatype and assumes C data layout for the subspace.
    If this is not desireable, follow up with a call to `esdm_dataspace_copyDatalayout()`.

  * `esdm_dataspace_makeContiguous()`

    Create a dataspace with the same logical space and datatype, but which uses the C data layout.

All these functions return a pointer to a new dataspace **which must be destroyed with a call to `esdm_dataspace_destroy()`**.

Data layout can only be set explicitly after a dataspace has been created.
This is done by a call to `esdm_dataspace_set_stride()` or to `esdm_dataspace_copyDatalayout()`.
The first allows the user to specify any regular data layout, including, but not limited to Fortran data layout.
The later assumes that the dataspace will be used to access the same data buffer as the dataspace from which the data layout is copied.
As such, `esdm_dataspace_copyDatalayout()` is very convenient for data subseting operations.

##### The Simple API

For convenience and performance reasons, ESDM provides a set of preprocessor macros that allocate a dataspace on the stack.
Macros are provided for 1D, 2D and 3D dataspaces, and come in variants that either assume start coordinates at the origin of the coordinate system, or allow the offset to be specified explicitly.
The macros without explicit start coordinates are:

    esdm_dataspace_1d()
    esdm_dataspace_2d()
    esdm_dataspace_3d()

The macros that take start coordinates simply add an "o" to the end of the name:

    esdm_dataspace_1do()
    esdm_dataspace_2do()
    esdm_dataspace_3do()

The result of these macros is a value of type `esdm_simple_dspace_t` which is a struct containing a single member `ptr` which contains the pointer value that can subsequently be used in all ESDM calls that require a dataspace.
I.e, a typical usage might be:

    esdm_simple_dspace_t region = esdm_dataspace_2do(x, width, y, height, type);
    esdm_read(dataset, buffer, region.ptr);

As the esdm_simple_dspace_t lives on the stack, **it must not be destroyed with `esdm_dataspace_destroy()`**.
It simply ceases to exists when the surrounding block exits.


### Dataset

A dataset in ESDM is what a variable is in NetCDF or HDF5.
Each dataset is associated with a dataspace that describes the logical extends of the data,
and it acts as a container for data that is written into this logical space.

There is no requirement to fill the entire logical space with data.
Normally, reading nonexistent data results in an error.
However, a dataset can also be associated with a fill value to handle data with holes seamlessly.

There is also no requirement to write nonoverlapping data.
When writes overlap, ESDM will assume that both writes place the same data in the overlapping area.
If this condition does not hold, there is no guarantee which data will be returned on a read.

In addition to the data and the logical space, datasets can also contain a number of attributes.
Like attributes in NetCDF, these are meant to associate metadata with a dataset.

User code can either create a dataset with `esdm_dataset_create()` or look up an existing dataset from a container with `esdm_dataset_open()`.
In either case, the reference to the dataset must later be dropped by a call to `esdm_dataset_close()`.
A dataset can also be deleted with a call to `esdm_dataset_delete()` which will remove its data from the file system, as well as its name and link from its container.


### Container

Containers provide a means to organize and retrieve datasets.
When a dataset is created, it is added to a container and associated with a name for later retrieval.

Like datasets, containers are created with `esdm_container_create()` or looked up from the file system with `esdm_container_open()`.
Also like datasets, containers need to be closed with `esdm_container_close()` when their reference is not needed anymore.
Closing a container requires closing all datasets it contains first.
`esdm_container_delete()` removes a container from the file system.


### Grid

The grid abstraction exists for performance reasons only:
While it is possible to think of a dataset as a set of possibly overlapping chunks of data,
it is surprisingly hard to determine minimal sets of chunks to satisfy a read request.
User code, on the other hand, generally does not use overlapping chunks of data.
Instead, user code can be assumed to work on (semi-)regular nonoverlapping chunks of data.
Passing this chunking information to ESDM allows the library to make good decisions much faster.

Grids also allow user code to inquire how the data is available on disk,
allowing consumer code to iterate over complete and nonoverlapping sets of data chunks in the most efficient fashion.

To work with grids, the header `esdm-grid.h` must be included.

Like a dataspace, a grid covers an axis aligned hyperbox in the logical space.
This space is called the grid's domain, and it is defined when a grid is created with `esdm_grid_create()`,
`esdm_grid_createSimple()` allows omitting the start coordinates to use the origin as one corner of the hyperbox.

Once a grid has been created, its axes can be subdivided individually by calls to `esdm_grid_subdivide()`.
This allows the user to specify all the bounds for an axis explicitly.
In many contexts, however, it will be simpler to use `esdm_grid_subdivideFixed()` or `esdm_grid_subdivideFlexible()` which instruct ESDM to generate bounds in a regular way.
Fixed subdivision will produce intervals of a given size, flexible subdivision instructs ESDM to generate a specific number of intervals of similar size.

After the axis of a grid have been defined, individual grid cells may be turned into subgrids via `esdm_grid_createSubgrid()`.
After this, the axes of the parent grid are fixed and the subdivision calls cannot be used anymore.
The subgrid, on the other hand, is a newly created grid with the parent grids cell bounds as its domain.
Usually, user code will follow up with calls to `esdm_grid_subdivide*()` on the subgrid to define its axes.
Subgrids may be constructed recursively to any depth.

This subgrid feature is useful to define grids with semi-regular decompositions:
For instance, an image may be decomposed into stripes, which are themselves decomposed into rectangles,
but the rectangle bounds of one stripe do not match those of another stripe.
Such semi-regular decompositions are a common result of load balancing of earth system simulations.

Once a grids structure has been defined fully, it can be used to read/write data via `esdm_read_grid()` and `esdm_write_grid()`.
Parallel applications will want to distribute the grid to all involved processes first by calling `esdm_mpi_grid_bcast()` (include `esdm-mpi.h` for this).
Both, using a grid for input/output and communicating it over MPI will fix the grids structure, prohibiting future calls to subdivide axes or create subgrids.

It is possible to iterate over all (sub-)cells of a grid.
This is done using an iterator, the three relevant functions are `esdm_gridIterator_create()`, `esdm_gridIterator_next()` and `esdm_gridIterator_destroy()`.
This is meant to be used by readers which inquire an available grid from a dataset using `esdm_dataset_grids()`.
This method of reading avoids any cropping or stitching together of data chunks within ESDM, delivering the best possible performance.

Grids always remain in possession of their dataspace.
Consequently, it is not necessary to dispose of them explicitly.
However, closing a dataspace invalidates all associated (sub-)grids.



Usage Examples
--------------

Learning usage of an API is easiest by seeing it in action in concrete examples.
As such, this section provides four relatively basic examples of how the ESDM API is supposed to be used,
which nevertheless cover all the required core functionality.


### Basic Writing

The simplest way to write a grey scale image to ESDM is as follows:

    //assume image data stored in either
    uint16_t imageBuffer[height][width];
    uint16_t (*imageBuffer)[width] = malloc(height*sizeof*imageBuffer);


    //initialize ESDM
    esdm_status result = esdm_init();
    assert(result == ESDM_SUCCESS);

    //create the container, dataspace, and dataset
    esdm_container_t* container;
    result = esdm_container_create("path/to/container", false, &container); //pass true to allow overwriting of an existing container
    assert(result == ESDM_SUCCESS);
    esdm_simple_dspace_t space = esdm_dataspace_2d(height, width, SMD_TYPE_UINT16);  //the data is 2D and consists of uint64_t values
    esdm_dataset_t* dataset;
    result = esdm_dataset_create(container, "myImage", space.ptr, &dataset);
    assert(result == ESDM_SUCCESS);

    //write the data
    result = esdm_write(dataset, imageBuffer, space.ptr);
    assert(result == ESDM_SUCCESS);

    //cleanup
    result = esdm_dataset_close(dataset);
    assert(result == ESDM_SUCCESS);
    result = esdm_container_close(container);
    assert(result == ESDM_SUCCESS);

    //bring down ESDM
    result = esdm_finalize();
    assert(result == ESDM_SUCCESS);

In this example, the same dataspace is used to create the dataset and to write the data, writing all the data in one large piece.
This is not necessary, the dataspaces that are passed to `esdm_write()` may be smaller than the dataset,
calling `esdm_write()` as many times as required to write the entire data.


### Grid Based Writing

When using grid based writing, the creation of the container and the dataset is exactly the same.
The creation of the grid, however is added explicitly.
In this case, we are going slice a 3D dataspace into 10 slices of similar size along the z axis,
and into rows of 256 lines along the y axis:

    //define the grid
    esdm_grid_t* grid;
    result = esdm_grid_createSimple(dataset, 3, (int64_t[3]){depth, height, width}, &grid);
    assert(result == ESDM_SUCCESS);
    result = esdm_grid_subdivideFixed(grid, 1, 256, true);  //the last parameter allows the last interval to be smaller than 256 lines
    assert(result == ESDM_SUCCESS);
    result = esdm_grid_subdivideFlexible(grid, 0, 10);
    assert(result == ESDM_SUCCESS);

    for(int64_t z0 = 0, slice = 0, curDepth; z0 < depth; z0 += curDepth, slice++) {
        //inquire the depth of the current slice
        //use of esdm_grid_subdivideFlexible() generally requires use of a grid bounds inquiry function
        int64_t z1;
        result = esdm_grid_getBound(grid, 0, slice + 1, &z1);
        assert(result == ESDM_SUCCESS);
        curDepth = z1 - z0;

        for(int64_t row = 0; row*256 < height; row++) {
            //compute the height of the current row
            //we can calculate this ourselves as we have used esdm_grid_subdivideFixed()
            int64_t height = (row < height/256 ? 256 : height - row*256);

            //set contents of dataBuffer

            //use the grid to write one chunk of data, the grid knows to which dataset it belongs
            result = esdm_write_grid(grid, esdm_dataspace_3do(z0, curDepth, row*256, height, 0, width, SMD_TYPE_DOUBLE).ptr, dataBuffer);
            assert(result == ESDM_SUCCESS);
        }
    }

    //no need to cleanup the grid, it belongs to the dataset and will be disposed off when the dataset is closed


### Simple Reading

Reading data is very similar to writing it.
Nevertheless, a simple example is given to read an entire dataset in one piece:

    //open the container and dataset, and inquire the dataspace
    esdm_container_t* container;
    result = esdm_container_open("path/to/container", ESDM_MODE_FLAG_READ, &container);
    assert(result == ESDM_SUCCESS);
    esdm_dataset_t* dataset;
    result = esdm_dataset_open(container, "myDataset", ESDM_MODE_FLAG_READ, &dataset);
    assert(result == ESDM_SUCCESS);
    esdm_dataspace_t* dataspace;
    esdm_dataset_get_dataspace(dataset, &dataspace);  //this returns a reference to the internal dataspace, do not destroy or modify it

    //allocate a buffer large enough to hold the data and generate a dataspace for it
    result = esdm_dataspace_makeContiguous(dataspace, &dataspace);  //the buffer will be in contiguous C data layout
    assert(result == ESDM_SUCCESS);
    int64_t bufferSize = esdm_dataspace_total_bytes(dataspace);
    void* buffer = malloc(bufferSize);

    //read the data
    result = esdm_read(dataset, buffer, dataspace);
    assert(result == ESDM_SUCCESS);

    //do stuff with buffer and dataspace

    //cleanup
    result = esdm_dataspace_destroy(dataspace); //esdm_dataspace_makeContiguous() creates a new dataspace
    assert(result == ESDM_SUCCESS);
    result = esdm_dataset_close(dataset);
    assert(result == ESDM_SUCCESS);
    result = esdm_container_close(container);
    assert(result == ESDM_SUCCESS);


### Grid Based Reading

Reading an entire dataset as a single chunk is generally a really bad idea.
Datasets, especially those generated by earth system models, may be huge, many times larger than the available main memory.
Reading a dataset in the form of reader defined chunks is possible with `esdm_read()`, but not necessarily efficient.
The chunks on disk may not match those which are used to read, requiring `esdm_read()` to

  * read multiple chunks from disk and stitch them together,

  * and to read more data from disk than is actually required.

If the dataset has been written using a grid, this grid can be recovered to inform the reading process of the actual data layout on disk:

    //open the container and dataset, and inquire the available grids
    esdm_container_t* container;
    result = esdm_container_open("path/to/container", ESDM_MODE_FLAG_READ, &container);
    assert(result == ESDM_SUCCESS);
    esdm_dataset_t* dataset;
    result = esdm_dataset_open(container, "myDataset", ESDM_MODE_FLAG_READ, &dataset);
    assert(result == ESDM_SUCCESS);
    int64_t gridCount;
    esdm_grid_t** grids;
    result = esdm_dataset_grids(dataset, &gridCount, &grids);
    assert(result == ESDM_SUCCESS);

    //select a grid, here we just use the first one
    assert(gridCount >= 1);
    esdm_grid_t* grid = grids[0];
    free(grids);  //we are responsible to free this array

    //iterate over the data, reading the data one stored chunk at a time
    esdm_gridIterator_t* iterator;
    result = esdm_gridIterator_create(grid, &iterator);
    assert(result == ESDM_SUCCESS);
    while(true) {
        esdm_dataspace_t* cellSpace;
        result = esdm_gridIterator_next(&iterator, 1, &cellSpace);
        assert(result == ESDM_SUCCESS);

        if(!iterator) break;

        //allocate a buffer large enough to hold the data
        int64_t bufferSize = esdm_dataspace_total_bytes(cellSpace);
        void* buffer = malloc(bufferSize);

        //read the data
        result = esdm_read_grid(grid, cellSpace, buffer);
        assert(result == ESDM_SUCCESS);

        //do stuff with buffer and cellSpace

        //cleanup
        free(buffer);
        result = esdm_dataspace_destroy(cellSpace);
        assert(result == ESDM_SUCCESS);
    }

    //cleanup
    //no cleanup necessary for the iterator, it has already been destroyed by esdm_gridIterator_next()
    result = esdm_dataset_close(dataset);
    assert(result == ESDM_SUCCESS);
    result = esdm_container_close(container);
    assert(result == ESDM_SUCCESS);
