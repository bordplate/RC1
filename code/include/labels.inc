# This file is used by the original compiler/assembler.
# Defines the expected assembly macros.
#
# Compatibility notes (verified with SN ps2eeas 1.8.19.316 in
# tools/cc/lib/gcc-lib/ee/2.95.2/ and GNU as 2.9-ee):
# - Labels are defined with `.equ label, .` instead of a `label:` line:
#   Ps2EeAs mangles a macro argument that is the entire label
#   (`\label:` assembles as the literal symbol `.label`).
# - `endlabel` carries no `.size`: Ps2EeAs page-faults when a symbol has
#   both `.ent` (from glabel) and `.size`. Objects are stripped before
#   linking, so the size entry is not needed.
# - `jlabel` omits `.local`: Ps2EeAs does not support the directive; a
#   symbol that is not global is local in both assemblers.

# A function symbol.
.macro glabel label, visibility=global
    .\visibility \label
    .type \label, @function
    .equ \label, .
    .ent \label
.endm

# The end of a function symbol.
.macro endlabel label
    .end \label
.endm

# An alternative entry to a function.
.macro alabel label, visibility=global
    .\visibility \label
    .type \label, @function
    .equ \label, .
.endm

# A label referenced by an error handler table.
.macro ehlabel label, visibility=global
    .\visibility \label
    .equ \label, .
.endm

# A label referenced by a jumptable.
.macro jlabel label, visibility=local
    .equ \label, .
.endm

# A data symbol.
.macro dlabel label, visibility=global
    .\visibility \label
    .type \label, @object
    .equ \label, .
.endm

# End of a data symbol.
.macro enddlabel label
    .size \label, . - \label
.endm

# Label to signal the symbol haven't been matched yet.
.macro nonmatching label, size=1
    .global \label.NON_MATCHING
    .type \label.NON_MATCHING, @object
    .size \label.NON_MATCHING, \size
    .equ \label.NON_MATCHING, .
.endm
