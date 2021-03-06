Design patterns notes
------

Add whatever reasonable code style & programming practices here as reminders.

### Terms definition

1. **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT** and **MAY** are key words
  which follow definition in [RFC2119](https://www.ietf.org/rfc/rfc2119.txt).
  For those who are not familiar with these terms, here are definitions copied
  from that reference:
  * The word **MUST** (or **REQUIRED**, **SHALL**) means that the definition
    is an *absolute requirement*.
  * The phrase **MUST NOT** (or **SHALL NOT**) means that the definition
    is an *absolute prehibition*.
  * The word **SHOULD** (or **RECOMMENDED**) means that there may exist
    valid reasons in particular circumstances to ignore a particular item, but
    the full implications must be understood and carefully weighed before
    choosing a different course.
    - In case of choosing such different course, an explanation is needed.
  * The word **SHOULD NOT** (or **NOT RECOMMENDED**) means that
    there may exist valid reasons in particular circumstances when the
    particular behavior is acceptable or even useful, but the full
    implications should be understood and the case carefully weighed
    before implementing any behavior described with this label.
    - In case of implementing such behavior, an explanation is needed.
  * The word **MAY** (or **OPTIONAL**) means that an item is truly optional.
    An implementation which does not include that particular option **MUST** be
    prepared to interoperate with another implementation which does
    include the option, though perhaps with reduced functionality. In the
    same vein an implementation which does include a particular option
    **MUST** be prepared to interoperate with another implementation which
    does not include the option (except, of course, for the feature the
    option provides.)

### Code style notes

#### General

1. Pushing a commit with only code style changes is **NOT RECOMMENDED**.  Code
  style fix is really minor comparing to refactoring, bug fixes, new features,
  etc.

#### Functions

1. A function without arguments **MUST** be declared like

        void foo(void);

  The keyword `void` inside the parentheses **MUST NOT** be omitted, as nothing
  inside the parentheses means it can take *any* number of parameters, which
  is equivalent to

      void foo(...);

2. Internal functions **MUST** be prepended with double underscores `__`.
  * Code outside the source file **SHOULD NOT** call the internal functions,
    which are, functions whose names are prepended with double underscores `__`.
    - Some extreme cases include: architecture function `foo()` having to call
      machine function `bar()`, but the developer does not want to make `bar()`
      look like a regular function available everywhere.
  * The internal functions is **RECOMMENDED** to be declared as `static`.

#### Pointers

1. Pointer declaration **MUST** go like

        int *a;

  Leave one space between asterisk and type indicator, and do not leave
  a space between asterisk and pointer variable/function name.  There is no
  reason not to do so.

#### `typedef`

1. `typedef` *SHOULD* only be used in order to do either of the following:
  - provide clearer aliases (e.g. `uint8_t` against `unsigned char`).
  - hide what is inside (e.g. `pte_t`).  In this case, the data structure
    **MUST** be accessed via accessor functions to provide encapsulation.
    Doubly consider when doing so.
2. If necessary, data types *SHOULD* be `typedef`'d as `foo_t`, that is, ended
  with `_t`.
3. If necessary, function pointers *SHOULD* be `typedef`'d as `foo_fp`, that
  is, ended with `_fp`.

### Source organization notes

1. The file names in `include` directory, `include/arch/$ARCH` directory, and
  `include/arch/$ARCH/mach-$MACH` directory **MUST** be unique, as `gcc`
  looks for headers in all three directories, and will be confused if there
  are duplicates.

### Design notes

#### Driver framework

**Please review and update as needed.**

##### Principles

[TBD]

1. A device is defined as a set of hardware.
  * This set of hardware may either be concrete or virtual.
  * A piece of physical hardware may be viewed as more than one device.
2. Accessing devices:
  * Peripherals **MUST** be accessed as one or more devices.
  * Hardwares connected to southbridge **SHOULD** be accessed as one or more
    devices.
    - This include (but not limited to) interrupt controllers, real-time
      clocks, PCI bus, SMBus, and many more.
  * Hardwares connected to northbridge **MAY** be accessed as one or more
    devices.
  * Parts within the processor *MAY* also be accessed as devices.
3. A device could be a *character* device, a *block* device, a *network*
  device, or a *bus* device. Driver framework *SHOULD* reserve for future
  weird types of devices.
4. A device is always connected to a bus, with only a few exceptions.
  * The physical memory address space is viewed as a bus device. Since it is
    directly connected to some processor, it's `bus` pointer is set to `NULL`.
  * When directly accessible from the processor (possibly via `IN` and `OUT`)
    instructions, the IO port address space is viewed as a bus device, which
    is treated as above.
5. Kernel **MUST** communicate with all devices via this driver framework.
6. The kernel has routines for all directly-accessible buses, but should only
  be used inside corresponding drivers.
7. Kernel communicates with buses either directly or via other buses,
  depending on physical connection.
  * E.g., kernel communicates with memory bus directly.
  * E.g., kernel can communicate with PCI bus via a memory bus.
8. Kernel *SHOULD* communicate with devices other than buses via buses.

##### Design guidelines

Each driver **MUST** be designed so that it can be compiled and loaded as
kernel module(s).

1. Drivers **MUST** be compiled into PIC.

2. Internal functions **MUST** be prepended with double underscores `__`, and
   **MUST** be declared as `static`, so that drivers will not interfere with
   each other.

3. Each driver (or any other kernel module) **MUST** provide exactly one single
   routine to register its interface (structs or pointers) to the kernel. It
   **SHOULD** initialize the module, and **MAY** probe and/or initialize
   the device.

4. A bus device's driver **SHOULD** handle the case when it is directly
   accessible from the processor. This part can only be omitted when NO
   processor can access it directly, which is PCI's case.

5. Whenever possible, drivers **MUST** ask bus drivers for access functions.

#### `early_console_init()`

Each driver **MAY** provide a weak `early_console_init()` function for a default
console initialization.  Architecture-specific or machine-specific kernel code
**MAY** override the driver-provided default initialization routine with its
own, in which case `early_console_init()` **MUST** be a strong function.

Because of principle (4), and we are using a register-based interface, we are
essentially messing up with function pointers, which stores address of
routine entries.  And the virtual address of routine entries differ before and
after setting up memory mappings.  Therefore, in `kputs()` and `kputchar()` we
added additional logic to determine whether we are running at lower or upper
address space, and compute absolute addresses from stored function pointers
afterwards. [TBD]

#### Early-stage works

TODO: describe why we need `early_kva2pa()`s in early stage.  More precisely,
why we did not work at high address in the first place?

This design leads to an unfortunate consequence for GDB: you can no longer
happily debug your kernel with C code when it is running in low address.
However, as we finish only a little necessary work (mainly, for future
compatibility with DTB) in low address, we regard it as tolerable.
