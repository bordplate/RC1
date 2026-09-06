# Original matched func_00206B78; exercises a GP-relative relocation.
glabel gp_control
/* 107AF8 00206B78 6491828F */ lw $2, -0x6E9C($28)
/* 107AFC 00206B7C 0800E003 */ jr $31
/* 107B00 00206B80 0100422C */ sltiu $2, $2, 1
endlabel gp_control
