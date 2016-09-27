// This file is part of h5-memvol.
// 
// SCIL is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// SCIL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with h5-memvol.  If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "include/memvol.h"
#include "include/debug.h"
#include "include/memvol-internal.h"


static void* memvol_dataset_create(void* obj, H5VL_loc_params_t loc_params, const char* name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void** req)

{
	puts("------------ memvol_dataset_create() called -------------\n");

//Memory allocation for creating object 
	memvol_object_t* dset_object = (memvol_object_t*)malloc(sizeof(memvol_object_t));

        if( dset_object != NULL) {
            dset_object->type = (memvol_object_type)malloc(sizeof(memvol_object_type));

//Memory allocation for creating dataset structure
            dset_object->subclass = (memvol_dataset_t *)malloc(sizeof(memvol_dataset_t));
         
        }
        else {
	    printf("Can't allocate memory");
	    return 0;
	}
//-------------------------------------------------------------------------------------------

        memvol_group_t* parent_group;
        memvol_object_t* object = (memvol_object_t *)obj;

        if (object->type == GROUP_T) {
             
          parent_group = (memvol_group_t *)object->subclass;

            DEBUG_MESSAGE("parent_group %zu\n", parent_group);
            DEBUG_MESSAGE("parent_group name %d\n", parent_group->name);

        } 
        else if(object->type == DATASET_T) { 
          
           return 0; 
        }
    
        dset_object->type = DATASET_T;

        DEBUG_MESSAGE("dset_object->type %zu\n", dset_object->type);

        memvol_dataset_t*  dataset = dset_object->subclass;

        dataset->name = (char*)malloc(strlen(name) + 1);
	strcpy(dataset->name, name);   // name of the dataset

       // retrieving of dataset, dataspace and link creation property lists
	H5Pget(dcpl_id, H5VL_PROP_DSET_TYPE_ID, &dataset->datatype);    // datatype of the dataset
 
	H5Pget(dcpl_id, H5VL_PROP_DSET_SPACE_ID, &dataset->dataspace);  // dataspace

 hssize_t space_number = H5Sget_simple_extent_npoints(dataset->dataspace); 
DEBUG_MESSAGE("1. space_number = %d\n", space_number);

        H5Pget(dcpl_id, H5VL_PROP_DSET_LCPL_ID, &dataset->lcpl);        // link creation property list

	H5Pclose(dcpl_id);

        dataset->data = NULL;       // raw data 

        DEBUG_MESSAGE("datatype_name %s\n", dataset->name);
	DEBUG_MESSAGE("datatype_value %zu\n", dataset->datatype);
	DEBUG_MESSAGE("dataspace_value %zu\n", dataset->dataspace);
        DEBUG_MESSAGE("link_creation_property_list %zu\n", dataset->lcpl);
        DEBUG_MESSAGE("dataset data %zu \n", dataset->data);

		
        DEBUG_MESSAGE("dataset %zu\n", dataset);

       
        g_hash_table_insert(parent_group->children, strdup(name), dset_object);  // insertion in the table 
        DEBUG_MESSAGE("dataset_object %zu\n", dset_object);

	return (void *)dset_object;
}

static void* memvol_dataset_open(void *obj, H5VL_loc_params_t loc_params, const char *name, 
                  hid_t dapl_id, hid_t dxpl_id, void **req){

puts("------------ memvol_dataset_open() called -------------\n");

 
  
   memvol_object_t* loc_object = (memvol_object_t *)obj;

   memvol_group_t* parent = (memvol_group_t *)loc_object->subclass;

// opening
    memvol_object_t* dset_object = g_hash_table_lookup(parent->children, name);

     
    //debug Ausgaben
    if (dset_object == NULL) {
        puts("Dataset nicht im angegebenen Parent gefunden!\n");

    } else {

      memvol_dataset_t* dset = (memvol_dataset_t *)dset_object->subclass;

      strcpy(dset->name, name);
//bebug ausgabe
 
      DEBUG_MESSAGE("Opened dataset object %zu \n", dset_object);
      DEBUG_MESSAGE("dataset %zu \n", dset_object->subclass);
      DEBUG_MESSAGE("dataset name %s \n", dset->name);
      DEBUG_MESSAGE("dataset datatype %zu \n", dset->datatype);
      DEBUG_MESSAGE("dataset_dataspace %zu\n", dset->dataspace);

    DEBUG_MESSAGE("dataset data %zu \n", dset->data);

hssize_t space_number = H5Sget_simple_extent_npoints(dset->dataspace); 
DEBUG_MESSAGE("\n 2. space_number = %d\n", space_number);

    }

return (void *) dset_object;

}

static herr_t memvol_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                   hid_t xfer_plist_id, void * buf, void **req){

puts("------------ memvol_dataset_read() called -------------\n");

 htri_t ret;
 hssize_t write_number, n_points;


 memvol_object_t* object = (memvol_object_t*)dset;
 memvol_dataset_t*  dataset = (memvol_dataset_t* )object->subclass;

 if (dataset->data == NULL) {
  
    printf("Dataset is empty\n");
    return 0;
  }
  else {
   assert(H5Tget_class(mem_type_id) == H5Tget_class(dataset->datatype));

  
 //H5Tget_native_type ?

  if(file_space_id == H5S_ALL){

      if(mem_space_id == H5S_ALL){

      assert(write_number == n_points);
     
      //read data
      buf = dataset->data;
      
      }
      else { /*valid mem_space_id*/



      } 
   }
   else { /*valid file_space_id*/

      if(mem_space_id == H5S_ALL){


      }
      else {/*valid mem_space_id*/

      } 
   }

return 1;
}
}

static herr_t memvol_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id,
                    hid_t xfer_plist_id, const void * buf, void **req){

puts("------------ memvol_dataset_write() called -------------\n");

 int dims;
 htri_t ret;
 hsize_t *dim;
 hsize_t *maxdim;
 hssize_t write_number, n_points;
 herr_t status;
 size_t type_size, size; 
 H5T_class_t class;


 memvol_object_t* object = (memvol_object_t*) dset;
 memvol_dataset_t*  dataset = (memvol_dataset_t* )object->subclass;


/* class of datatype */
 class = H5Tget_class(dataset->datatype); 

/*size of datatype in dataset*/

 size = H5Tget_size(dataset->datatype);
  
/*number of points in dataset*/
 n_points = H5Sget_simple_extent_npoints(dataset->dataspace);

if(dataset->data == NULL) {

    /*memory allocation for data*/  //to do
     dataset->data = calloc(n_points,size);

}

/*rank of the dataset dataspace*/
dims = H5Sget_simple_extent_ndims(dataset->dataspace); 

/* dimention size and maximal size*/
status = H5Sget_simple_extent_dims(dataset->dataspace, dim, maxdim);

/* number of elements to write */
write_number = H5Sget_select_npoints(mem_space_id); 

/*size of the mem_type in Bytes*/
type_size = H5Tget_size(mem_type_id); 

//assert, that datatype class is equal 
assert(H5Tget_class(mem_type_id) == H5Tget_class(dataset->datatype));

//assert, that data passt in dataset container
assert(write_number <= n_points);

if(file_space_id == H5S_ALL){

   if(mem_space_id == H5S_ALL){

     assert(write_number == n_points);
     //complete write
     // writes from buffer
       for(int index = 0; index <= write_number; index++){
          *(dataset->data + index) = buf[index];
       }
    }
   else { /*valid mem_space_id*/



   } 
}
else { /*valid file_space_id*/

    if(mem_space_id == H5S_ALL){


    }
    else {/*valid mem_space_id*/

    } 
}

H5S_sel_type sel_type = H5Sget_select_type(file_space_id); 
 
switch(sel_type){

	case H5S_SEL_NONE:
	{
          break;
	}
	case H5S_SEL_POINTS:
	{
         break;
	}
	case H5S_SEL_HYPERSLABS: 
	{
         break;
	}
	case H5S_SEL_ALL:
	{
          break;
	}
	default:
	{
             DEBUG_MESSAGE("unknown selection type\n");
	     ret = 0;
	}
}


return 1;
}

static  herr_t memvol_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments){

puts("------------ memvol_dataset_get() called -------------\n");

   memvol_object_t* object = (memvol_object_t*)dset;
   memvol_dataset_t*  dataset = (memvol_dataset_t* )object->subclass;

   herr_t ret_value = 1;

 //  va_start(arguments, 7);
   switch(get_type) {

           case H5VL_DATASET_GET_DAPL: 
           {	
		hid_t *ret_id = va_arg (arguments, hid_t *);
                printf("Access property list %p\n", *ret_id);
                              
		break;
           }
           case H5VL_DATASET_GET_DCPL:
           {
		hid_t *ret_id = va_arg (arguments, hid_t *);
		printf("Creation property list %p\n", *ret_id);
                        
		break;

           }

	   case H5VL_DATASET_GET_OFFSET:
           {
	         haddr_t *ret = va_arg (arguments, haddr_t *);
		printf("The offset of the dataset %p \n", *ret);
		 
			   /* Set return value */
			   //*ret = H5D__get_offset(dset);
			   //if(!H5F_addr_defined(*ret))
			//	   *ret = HADDR_UNDEF;
		break;
           }
           case H5VL_DATASET_GET_SPACE:
           {
		hid_t *ret_id = va_arg (arguments, hid_t *);
		printf("Dataspace %p\n", *ret_id);
                
  		break;

           }
	   case H5VL_DATASET_GET_SPACE_STATUS:
	   {
		H5D_space_status_t *allocation = va_arg (arguments, H5D_space_status_t *);
		printf("Space status %p\n", *allocation);
                
		break;
           }
           case H5VL_DATASET_GET_STORAGE_SIZE:
           {
		hsize_t *ret = va_arg (arguments, hsize_t *);
		printf("Storage size %p\n", *ret);
                
		break;
           }
           case H5VL_DATASET_GET_TYPE:
           {
      		hid_t *ret_id = va_arg (arguments, hid_t *);
		printf("Datatype %d\n", *ret_id);
                
		break;
           }
	   default:
	   {	DEBUG_MESSAGE("unknown type found\n");
		ret_value = 0;
	   }
   }
   va_end(arguments);
   return ret_value;       
}
 
static herr_t memvol_dataset_close(void* dset, hid_t dxpl_id, void** req) {

    puts("------------ memvol_dataset_close() called -------------\n");
  
    memvol_object_t *object = (memvol_object_t*) dset;
        
    memvol_dataset_t *dataset = object->subclass;
/*
    allocated memory will be free by closing of the location group
       
**/
    return 1;
}
