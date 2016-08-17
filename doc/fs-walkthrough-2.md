File system walkthrough Part 2
======

### Prelude: Terminal initialization

### Vnode operations (Part 3, `inactive` and `reclaim`)

### Ext2 file format

#### Regular files

#### Directories

#### Device files

### `namei` - Translate from path to vnode

### Vnode operations (Part 4, `lookup`)

### `bread` (Part 2, on files)

TODO:

* explain `bread` on files.

### Vnode operations (Part 5, `bmap`)

### End-to-end: loading inode of `/dev/tty`

### UIO - data transfers between I/O vectors

### Vnode operations (Part 6, `read` and `write`)

#### Hardware drivers (Part 3, `read` and `write`)

### End-to-end: reading and writing on terminal

### ~~Unfinished task~~ Exercise

* Implement device files `/dev/null` and `/dev/zero`.
