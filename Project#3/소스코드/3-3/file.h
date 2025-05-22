struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};


// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // Protects everything below here
  int valid;          // Inode has been read from disk?

  short type;         // Copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDADDRS];

  // Red-black tree and cache memory
  void *cache_memory;
  struct rb_tree *cache_tree;

  // Counters for printing stats
  uint bmap_count;
  uint cache_hit_count;
  uint disk_access_count;
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
