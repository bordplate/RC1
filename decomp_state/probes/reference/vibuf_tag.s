# Original Splat instruction rows, retained because split omits matched C.
glabel scTag2
/* 13CBA0 0023BC20 3C300600 */ dsll32 $6, $6, 0
/* 13CBA4 0023BC24 3C280500 */ dsll32 $5, $5, 0
/* 13CBA8 0023BC28 3A310600 */ dsrl $6, $6, 4
/* 13CBAC 0023BC2C 3C380700 */ dsll32 $7, $7, 0
/* 13CBB0 0023BC30 2528A600 */ or $5, $5, $6
/* 13CBB4 0023BC34 3E380700 */ dsrl32 $7, $7, 0
/* 13CBB8 0023BC38 2528A700 */ or $5, $5, $7
/* 13CBBC 0023BC3C 0800E003 */ jr $31
/* 13CBC0 0023BC40 000085FC */ sd $5, 0($4)
endlabel scTag2
