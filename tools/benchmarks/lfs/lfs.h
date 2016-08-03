#ifndef LFS_H
#define LFS_H

#include <sys/types.h>
#include <vector>

struct tup{
  size_t a;
  size_t b;
};

struct lfs_record_on_disk{
  size_t addr;
  size_t size;
};

struct lfs_record{
  size_t addr;
  size_t size;
  size_t pos;
};


void normal_write(size_t addr, char * data);
size_t normal_read(size_t addr, size_t size, char * res);



int lfs_set_blocksize(size_t blocksize);
int lfs_open(char * df, char * mf);
size_t lfs_read(size_t addr, size_t size, char * res);
void lfs_write(size_t addr, char * data);


// internal only?
lfs_record * read_record();
struct tup compare_tup(struct tup first, struct tup second);
int lfs_find_chunks(size_t a, size_t b, int index, lfs_record * my_recs, std::vector<lfs_record>& chunks_stack);

#endif /* !LFS_H */
