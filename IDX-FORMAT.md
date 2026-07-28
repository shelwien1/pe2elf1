# IDX Format Specification

IDX is a domain-specific language for defining context models and optimizable parameters in data compression algorithms. The `idx2inc.pl` preprocessor converts `.idx` files into C/C++ header files (`.inc`).

## Overview

The IDX system serves two main purposes:
1. **Context index generation** - Build multi-dimensional context indices by combining bit patterns
2. **Parameter optimization** - Define tunable parameters with binary-encoded values for automated optimization

## Processing Modes

### Debug Mode (`Debug 1`)
Generates runtime-adjustable parameters using the `mapping` class. Parameters can be modified during execution for profiling and optimization.

### Const Mode (`Const 1`)
Generates compile-time constants by evaluating binary patterns directly. Produces faster code but requires recompilation to change parameters.

## Global Directives

```
Prefix <name>      # Set prefix for all generated identifiers (e.g., "D0" -> "D0_")
Debug <0|1>        # Default debug mode for following declarations
Const <0|1>        # Default const mode for following declarations
```

## Line Modifiers

- `!` at line start - Toggle debug mode for this line
- `-` at line start - Toggle const mode for this line
- `#` - Comment (rest of line ignored)

## Context Index Definition

### Index Declaration
```
Index <name>       # Start a new context index (e.g., "Index Cont")
```

### Variable Mappings

#### Threshold Mapping (`tag: var, base!pattern`)
Maps a variable to quantized buckets based on threshold comparisons.

```
m0: i, 1!111111111111111
```

- `m0` - Tag name (generates `D0_m0` with prefix "D0")
- `i` - Source variable
- `1` - Base offset
- `111111111111111` - Binary pattern defining thresholds

Each `1` in the pattern creates a threshold at position `j + (base-1)`. For pattern `1!111` with 3 ones and base=1:
- Result = (var > 0) + (var > 1) + (var > 2)
- Creates 4 buckets: [0], [1], [2], [3+]

**Debug mode output:**
```c
pdesc( D0_m0, 1, "111111111111111" );
```

**Const mode output** (pattern "01001" = binary value 9):
```c
static const int D0_STEP = (9+1) * (1);
```

#### Bitmask Mapping (`tag: var, &pattern`)
Extracts specific bits from a variable.

```
m3: m1, &00000011
```

- Pattern `00000011` extracts lower 2 bits
- `1` positions indicate bits to keep
- Bits are packed contiguously in result

**Debug mode output:**
```c
pmask( D0_m3, "00000011" );
```

**Const mode output:**
```c
index = (index << 2) + ((var) & 3);  // 00000011
```

#### Incremental Context Mask (`tag: var, b&pattern`)
For incrementally updated contexts with a leading 1 bit, like:
```c
ctx = 1;
ctx += ctx + bit[0];  // ctx = 2 or 3
ctx += ctx + bit[1];  // ctx = 4,5,6,7
...
```

The mask pattern specifies which accumulated bits to extract, regardless of the current depth:
```
m3: m1, b&00000011
```

The leading 1 in `ctx` marks how many bits have been accumulated. The mask `00000011` extracts the last 2 accumulated bits at any stage. Uses `pmask2` / `masking_b` class.

### Direct Addition
```
ADD <multiplier>: <expression>
```

Adds a value directly to the context index with a multiplier:
```
ADD 2: OldLR
```

Generates:
```c
Cont = Cont * (2) + (OldLR);
```

## Parameter Definitions

### Number Declaration
```
Number <name>, <multiplier>, <base>!<pattern>
```

Defines an integer parameter:
```
Number STEP, 1, 1!01001
```

- `STEP` - Parameter name
- `1` - Multiplier
- `1` - Base value
- `01001` - Binary pattern (evaluates to 9, so result = (9+1)*1 = 10)

### Rate Declaration
```
Rate <name>, <base>!<pattern>
```

Defines a lookup table reference (for transition tables):
```
Rate w_wr, 0!00000
```

### Rate1 Declaration
```
Rate1 <name>, <base>!<pattern>
```

Defines an inverse rate (eSCALE / value).

## Template Files (.inc)

The `.inc` template file uses special markers. The `%M%` placeholder is replaced with the IDX prefix (e.g., `%M%_` becomes `D0_` when `Prefix D0` is set).

### MakeTables
```c
MakeTables
```
Inserts lookup tables (mappings, masks, constants) used by `MakeIndex`. Normally written to `_h.inc`, but can be placed elsewhere in the template.

### MakeIndex
```c
MakeIndex <index_name>
```
Inserts the context building code:
```c
  Cont = 0;
  D0_m0.inc( Cont, i );
  D0_m1.inc( Cont, k );
  D0_m2.inc( Cont, j );
  D0_m3.inc( Cont, m1 );
  D0_m4.inc( Cont, OldVal );
  Cont = Cont*(2) + (OldLR);
```

### Volume Variable
For each `Index` declaration, a `*_Volume` constant is generated:
```c
static const int D0_Cont_Volume = 1* D0_m0.Size* D0_m1.Size* D0_m2.Size* D0_m3.Size* D0_m4.Size* 2;
```

### Table Declaration
```c
Table( Type, %M%Name, %M%Name_Volume );
```

Behavior depends on `$UseNew` mode in idx2inc.pl:

**Static allocation (`$UseNew=0`):**
```c
Type D0_Name[D0_Name_Volume];
```

**Dynamic allocation (`$UseNew=1`):**
```c
Type* D0_Name;
// In Init(): D0_Name = new Type[D0_Name_Volume];
// In Quit(): delete D0_Name;
```

This allows the same `.idx`/`.inc` files to produce static or dynamic allocation without modification, which is important for auto-optimization tools.

### Custom Sections
```c
def_Data
  // Data declarations - stored in generated struct
end_Data

def_Init
  // Initialization code - inserted into Init() method
end_Init
```

## Generated Class Structure

The `_h.inc` output defines a struct that encapsulates all generated resources:

```c
struct D0_T {
#undef USE_NEW
#define USE_NEW 1  // or 0

  // Contents of def_Data section
  // Table declarations (static or pointer based on USE_NEW)

  void D0_Init( void ) {
    // Table allocations (if USE_NEW=1)
    // Contents of def_Init section
  }

  void D0_Quit( void ) {
    // Table deallocations (if USE_NEW=1)
  }
};
```

## Output Files

For input `sh_model-D0.idx`:

| File | Description |
|------|-------------|
| `sh_model-D0_h.inc` | Header: constants, mappings, struct with Init/Quit |
| `sh_model-D0_p.inc` | Processed template with MakeIndex expanded |

## Example

### Input: sh_model-D0.idx
```
Prefix D0
Debug 1
Const 0

Index Cont
 m0: i,       1!111111111111111
 m1: k,       1!1
 m2: j,       1!1
 m3: m1,      &00000011
 m4: OldVal,  1!1
 ADD 2: OldLR
```

### Output: sh_model-D0_h.inc
```c
pdesc( D0_m0, 1, "111111111111111" );
pdesc( D0_m1, 1, "1" );
pdesc( D0_m2, 1, "1" );
pmask( D0_m3, "00000011" );
pdesc( D0_m4, 1, "1" );

static const int D0_Cont_Volume = 1* D0_m0.Size* D0_m1.Size* D0_m2.Size* D0_m3.Size* D0_m4.Size* 2;
```

### Output: sh_model-D0_p.inc
```c
uint MakeCont( uint OldLR, uint OldVal, uint m1, uint j, uint k, uint i ) {
  int Cont = 0;

  Cont = 0;
  D0_m0.inc( Cont, i );
  D0_m1.inc( Cont, k );
  D0_m2.inc( Cont, j );
  D0_m3.inc( Cont, m1 );
  D0_m4.inc( Cont, OldVal );
  Cont = Cont*(2) + (OldLR);

  return Cont;
}
```

## Binary Pattern Encoding

Binary patterns encode integer values with additional semantics:

| Pattern | Binary Value | With base=1 | Result |
|---------|-------------|-------------|--------|
| `01001` | 9 | 9+1 | 10 |
| `111` | 7 | 7+1 | 8 |
| `0` | 0 | 0+1 | 1 |

In **debug mode**, each bit position becomes a tunable parameter that automated optimization tools can toggle. In **const mode**, the pattern is evaluated once at preprocessing time.

## Runtime Support Classes

The generated code requires these runtime classes from `sh_mapping.inc`:

- `mapping` - Threshold-based quantization with profiling support
- `masking` - Bitmask extraction
- `masking_b` - Bit-grouped mask mapping

Macros:
- `pdesc(name, base, pattern)` - Create profiling-enabled mapping
- `pmask(name, pattern)` - Create bitmask
- `pmask2(name, pattern)` - Create bit-grouped mask
