
/* WARNING: Removing unreachable block (ram,0x001f0f58) */
/* WARNING: Removing unreachable block (ram,0x001f0edc) */
/* WARNING: Removing unreachable block (ram,0x001f1f24) */

void FUN_001f0ce0(void)

{
  ulong *puVar1;
  bool bVar2;
  uint uVar3;
  uint uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  long lVar8;
  long lVar9;
  int iVar10;
  uint *puVar11;
  float *pfVar12;
  char cVar13;
  int iVar14;
  uint uVar15;
  undefined4 uVar16;
  float fVar17;
  float fVar18;
  undefined4 in_stack_000006b0;
  undefined4 in_stack_000006b4;
  
  FUN_0022ca50();
  iVar14 = iRam0018c354;
  if ((DAT_0013cae4 & 0x500) != 0) {
    uGpffff8a04 = uRam0018c3bc;
    FUN_001fb280(DAT_0018cf3c,DAT_0018cf40,DAT_0018cf44);
    iRam0018c3ac = (int)DAT_00187080 >> 2;
    iRam0018c3b0 = (int)DAT_00187084 >> 2;
    iRam0018c3b4 = (int)DAT_00187088 >> 2;
    uRam0018c410 = 0x6e;
    uRam0018c411 = 0x6e;
    return;
  }
  uVar15 = (uint)&stack0x00000697 & 7;
  puVar1 = (ulong *)(&stack0x00000697 + -uVar15);
  *puVar1 = *puVar1 & -1L << (uVar15 + 1) * 8 | uRam001e77c0 >> (7 - uVar15) * 8;
  uVar15 = (uint)&stack0x0000069f & 7;
  puVar1 = (ulong *)(&stack0x0000069f + -uVar15);
  *puVar1 = *puVar1 & -1L << (uVar15 + 1) * 8 | uRam001e77c8 >> (7 - uVar15) * 8;
  uVar15 = (uint)&stack0x000006a7 & 7;
  puVar1 = (ulong *)(&stack0x000006a7 + -uVar15);
  *puVar1 = *puVar1 & -1L << (uVar15 + 1) * 8 | uRam001e77d0 >> (7 - uVar15) * 8;
  if ((DAT_0013cae4 & 0x2000) == 0) {
    if ((DAT_0013cae4 & 0x8000) != 0) {
      iVar10 = iRam0018c324 + -1;
      iRam0018c324 = 2;
      if ((-1 < iVar10) && (iRam0018c324 = iVar10, iVar10 == 2)) {
        iRam0018c324 = 5;
      }
    }
  }
  else {
    iRam0018c324 = iRam0018c324 + 1;
    if (iRam0018c324 == 3) {
      iRam0018c324 = 0;
    }
    else if (iRam0018c324 == 6) {
      iRam0018c324 = 3;
    }
  }
  if (((DAT_0013cae4 & 0x1000) == 0) || (uRam0018c328 = uRam0018c328 - 1, -1 < (int)uRam0018c328)) {
    if (((DAT_0013cae4 & 0x4000) != 0) &&
       (uRam0018c328 = uRam0018c328 + 1,
       *(int *)(&stack0x00000690 + iRam0018c324 * 4) <= (int)uRam0018c328)) {
      if (iRam0018c324 < 3) {
        iRam0018c324 = (iRam0018c324 + 9) % 6;
      }
      uRam0018c328 = 0;
    }
  }
  else {
    if (2 < iRam0018c324) {
      iRam0018c324 = (iRam0018c324 + 3) % 6;
    }
    uRam0018c328 = *(int *)(&stack0x00000690 + iRam0018c324 * 4) - 1;
  }
  if (*(int *)(&stack0x00000690 + iRam0018c324 * 4) <= (int)uRam0018c328) {
    uRam0018c328 = *(int *)(&stack0x00000690 + iRam0018c324 * 4) - 1;
  }
  switch(iRam0018c324) {
  case 0:
    if (((DAT_0013cae4 & 0x40) != 0) &&
       (uVar15 = 1 << (uRam0018c328 & 0x1f), uRam0018c320 = uRam0018c320 ^ uVar15, uVar15 == 0x10))
    {
      if ((uRam0018c320 & 0x10) == 0) {
        uRam0018c320 = 0xf;
      }
      else {
        uRam0018c320 = 0x10;
      }
    }
    break;
  case 1:
    switch(uRam0018c328) {
    case 0:
      if ((DAT_0013cae4 & 0x50) != 0) {
        if (((DAT_0013cae4 & 0x40) != 0) && (DAT_0018c32c = DAT_0018c32c + 1, 3 < DAT_0018c32c)) {
          DAT_0018c32c = 0;
        }
        if (((DAT_0013cae4 & 0x10) != 0) && (DAT_0018c32c = DAT_0018c32c + -1, DAT_0018c32c < 0)) {
          DAT_0018c32c = 3;
        }
        if (DAT_0018c32c == 1) {
          uRam0018c320 = 0;
        }
        else if (DAT_0018c32c < 2) {
          if (DAT_0018c32c == 0) {
            uRam0018c320 = 0xf;
          }
        }
        else if (DAT_0018c32c == 2) {
          uRam0018c320 = 0;
          FUN_001f9a28(&stack0x000006b0,0x13f3d0,0x187080);
          uRam0018c3a4 = FUN_001f9b20(&stack0x000006b0);
          uVar16 = FUN_001f9e90(in_stack_000006b0,in_stack_000006b4);
          uRam0018c3a0 = FUN_001fa5c8(uVar16,DAT_00187098);
          fRam0018c3a8 = DAT_0013f3d8 - DAT_00187088;
          uRam0018c39c = FUN_001fa5c8(DAT_0013f3e8,DAT_00187098);
        }
        else if (DAT_0018c32c == 3) {
          uRam0018c320 = 6;
        }
      }
      break;
    case 1:
      if (((DAT_0013cae4 & 0x40) != 0) && (iRam0018c330 = iRam0018c330 + 1, 7 < iRam0018c330)) {
        iRam0018c330 = 0;
      }
      if (((DAT_0013cae4 & 0x10) != 0) && (iRam0018c330 = iRam0018c330 + -1, iRam0018c330 < 0)) {
        iRam0018c330 = 7;
      }
      break;
    case 2:
      if (DAT_0015f640 == 0) {
        DAT_0018c334 = 0;
      }
      else {
        if (((DAT_0013cae4 & 0x40) != 0) && (DAT_0018c334 = DAT_0018c334 + 1, 2 < DAT_0018c334)) {
          DAT_0018c334 = 0;
        }
        if (((DAT_0013cae4 & 0x10) != 0) && (DAT_0018c334 = DAT_0018c334 + -1, DAT_0018c334 < 0)) {
          DAT_0018c334 = 2;
        }
      }
      break;
    case 3:
      if ((DAT_0013cae4 & 0x50) != 0) {
        uRam0018c338 = (uint)(uRam0018c338 == 0);
      }
      break;
    case 4:
      if ((DAT_0013cae4 & 0x50) != 0) {
        uRam0018c33c = (uint)(uRam0018c33c == 0);
      }
      break;
    case 5:
      if ((DAT_0013cae4 & 0x50) != 0) {
        bVar2 = uRam0018c340 == 0;
        uRam0018c340 = (uint)bVar2;
        if (bVar2) {
          DAT_00160ec0 = 0x47800000;
          DAT_0015ff30 = 0x40;
          DAT_00160f70 = 0x42800000;
          DAT_001603e4 = 0x42200000;
          uGpffff94bc = 0x40000;
        }
        else {
          DAT_00160ec0 = 0x48fa0000;
          DAT_00160f70 = 0x44340000;
          DAT_0015ff30 = 500;
          DAT_001603e4 = 0x43fa0000;
          uGpffff94bc = 0x1f4000;
        }
      }
      break;
    case 6:
      if (((DAT_0013cae4 & 0x40) != 0) && (iRam0018c344 = iRam0018c344 + 1, 2 < iRam0018c344)) {
        iRam0018c344 = 0;
      }
      if (((DAT_0013cae4 & 0x10) != 0) && (iRam0018c344 = iRam0018c344 + -1, iRam0018c344 < 0)) {
        iRam0018c344 = 2;
      }
      break;
    case 7:
      if ((DAT_0013cae4 & 0x50) != 0) {
        uRam0018c348 = (uint)(uRam0018c348 == 0);
      }
      break;
    case 8:
      if ((DAT_0013cae4 & 0x50) != 0) {
        bVar2 = DAT_0018c34c == 0;
        DAT_0018c34c = (uint)bVar2;
        if (bVar2) {
          fVar18 = 0.125;
        }
        else {
          fVar18 = 0.5;
        }
        fVar17 = (float)FUN_001fa6c0(DAT_001518d0);
        DAT_0018cf00 = fVar17 * fVar18;
        fVar17 = (float)FUN_001fa6c0(DAT_001518d2);
        DAT_0018cf04 = fVar17 * fVar18;
        DAT_0018cf08 = DAT_0018cf00 * 4.0;
        DAT_0018cf0c = DAT_0018cf04 * 4.0;
        FUN_001f2d98();
      }
      break;
    case 9:
      if ((DAT_0013cae4 & 0x50) != 0) {
        if ((DAT_0013cae4 & 0x40) == 0) {
          iRam0018c350 = iRam0018c350 + -1;
          if (iRam0018c350 < -1) {
            iRam0018c350 = 0x23;
          }
        }
        else {
          iRam0018c350 = iRam0018c350 + 1;
          if (iRam0018c350 == 0x24) {
            iRam0018c350 = -1;
          }
        }
      }
      if (iRam0018c350 < 0) {
        uRam0018c3bc = 0;
      }
      else {
        uRam0018c3bc = 2;
      }
      break;
    case 10:
      if ((DAT_0013cae4 & 0x50) == 0) {
        if ((DAT_0013cae4 & 0x20) != 0) {
          cVar13 = '\0';
          if ((&DAT_0014c050)[iRam0018c354 + DAT_0015ed84 * 0x10] == '\0') {
            cVar13 = -1;
          }
          (&DAT_0014c050)[iRam0018c354 + DAT_0015ed84 * 0x10] = cVar13;
          (&DAT_0015fc08)[iVar14] = cVar13;
        }
      }
      else if ((DAT_0013cae4 & 0x40) == 0) {
        iRam0018c354 = iRam0018c354 + -1;
        if (iRam0018c354 < 0) {
          iRam0018c354 = 0xf;
        }
      }
      else {
        iRam0018c354 = iRam0018c354 + 1;
        if (iRam0018c354 == 0x10) {
          iRam0018c354 = 0;
        }
      }
    }
    break;
  case 2:
    if ((DAT_0013cae4 & 0x40) != 0) {
      uRam0018c31c = uRam0018c31c ^ 1 << (uRam0018c328 & 0x1f);
    }
    break;
  case 3:
    uVar15 = 999999;
    if ((int)uRam0018c328 < 0xd) {
      puVar11 = (uint *)(&DAT_0013d428 + (uint)(byte)(&gp0xffff8410)[uRam0018c328] * 4);
      uVar15 = (uint)*(ushort *)(&DAT_001dffbe + (uint)(byte)(&gp0xffff8410)[uRam0018c328] * 0x18);
    }
    else {
      puVar11 = &DAT_0015ed98;
    }
    if ((DAT_0013cae0 & 8) == 0) {
      if ((DAT_0013cae0 & 2) == 0) {
        if ((DAT_0013cae4 & 0x40) != 0) {
          *puVar11 = *puVar11 + 1;
        }
        uVar4 = *puVar11;
        if ((DAT_0013cae4 & 0x20) != 0) {
          uVar3 = uVar4 - 1;
          if (0 < (int)uVar4) goto LAB_001f16c8;
          goto LAB_001f16cc;
        }
      }
      else {
        if ((DAT_0013cae4 & 0x40) != 0) {
          *puVar11 = *puVar11 + 600;
        }
        if ((DAT_0013cae4 & 0x20) == 0) {
          uVar4 = *puVar11;
        }
        else {
          if ((int)*puVar11 < 0x259) {
            *puVar11 = 0;
          }
          else {
            uVar3 = *puVar11 - 600;
LAB_001f16c8:
            *puVar11 = uVar3;
          }
LAB_001f16cc:
          uVar4 = *puVar11;
        }
      }
    }
    else {
      if ((DAT_0013cae4 & 0x40) != 0) {
        *puVar11 = *puVar11 + 0x3c;
      }
      if ((DAT_0013cae4 & 0x20) != 0) {
        if (0x3c < (int)*puVar11) {
          uVar3 = *puVar11 - 0x3c;
          goto LAB_001f16c8;
        }
        *puVar11 = 0;
        goto LAB_001f16cc;
      }
      uVar4 = *puVar11;
    }
    if ((int)uVar15 < (int)uVar4) {
      *puVar11 = uVar15;
    }
    if ((DAT_0013cae4 & 0x10) != 0) {
      (&DAT_0013e520)[(byte)(&gp0xffff8410)[uRam0018c328]] =
           (&DAT_0013e520)[(byte)(&gp0xffff8410)[uRam0018c328]] + '\x01';
    }
    if ((DAT_0013cae4 & 0x80) != 0) {
      (&DAT_0013e520)[(byte)(&gp0xffff8410)[uRam0018c328]] =
           (&DAT_0013e520)[(byte)(&gp0xffff8410)[uRam0018c328]] + -1;
    }
    iVar14 = (uint)(byte)(&gp0xffff8410)[uRam0018c328] * 0x4c;
    if (*(short *)(&DAT_0018640e + iVar14) <
        (short)(ushort)(byte)(&DAT_0013e520)[(byte)(&gp0xffff8410)[uRam0018c328]]) {
      (&DAT_0013e520)[(byte)(&gp0xffff8410)[uRam0018c328]] = (&DAT_0018640e)[iVar14];
    }
    break;
  case 4:
    if (uRam0018c328 == 0) {
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0) {
        DAT_0015f488 = fGpffff8888 + 1024.0;
      }
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0) {
        DAT_0015f488 = fGpffff8888 - 1024.0;
      }
      if (DAT_0015f488 <= DAT_0015f48c) {
        if (DAT_0015f488 < 0.0) {
          DAT_0015f488 = 0.0;
        }
      }
      else {
        fGpffff8888 = DAT_0015f48c;
      }
    }
    if (uRam0018c328 == 2) {
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0) {
        DAT_0015f48c = fGpffff888c + 1024.0;
      }
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0) {
        DAT_0015f48c = fGpffff888c - 1024.0;
      }
      if (DAT_0015f48c <= 524288.0) {
        if (DAT_0015f48c < DAT_0015f488) {
          DAT_0015f48c = DAT_0015f488;
        }
      }
      else {
        fGpffff888c = 524288.0;
      }
    }
    if (uRam0018c328 == 1) {
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0) {
        DAT_0015f490 = fGpffff8890 - 2.55;
      }
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0) {
        DAT_0015f490 = fGpffff8890 + 2.55;
      }
      if (DAT_0015f490 <= 255.0) {
        if (DAT_0015f490 < 0.0) {
          DAT_0015f490 = 0.0;
        }
      }
      else {
        fGpffff8890 = 255.0;
      }
    }
    if (uRam0018c328 == 3) {
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0) {
        DAT_0015f494 = fGpffff8894 - 2.55;
      }
      if ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0) {
        DAT_0015f494 = fGpffff8894 + 2.55;
      }
      if (DAT_0015f494 <= 255.0) {
        if (DAT_0015f494 < 0.0) {
          DAT_0015f494 = 0.0;
        }
      }
      else {
        fGpffff8894 = 255.0;
      }
    }
    if (uRam0018c328 == 4) {
      if ((DAT_0015f484 != -1) && ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0)) {
        DAT_0015f484 = DAT_0015f484 + '\x01';
      }
      if ((DAT_0015f484 != '\0') && ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0)) {
        DAT_0015f484 = DAT_0015f484 + -1;
      }
    }
    if (uRam0018c328 == 5) {
      if ((DAT_0015f485 != -1) && ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0)) {
        DAT_0015f485 = DAT_0015f485 + '\x01';
      }
      if ((DAT_0015f485 != '\0') && ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0)) {
        DAT_0015f485 = DAT_0015f485 + -1;
      }
    }
    if (uRam0018c328 == 6) {
      cVar13 = cGpffff8886;
      if ((cGpffff8886 != -1) && ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x800000040) != 0)) {
        DAT_0015f486 = cGpffff8886 + '\x01';
        cVar13 = cGpffff8886 + '\x01';
      }
      if ((cVar13 != '\0') && ((CONCAT44(DAT_0013cae4,DAT_0013cae0) & 0x400000010) != 0)) {
        DAT_0015f486 = cVar13 + -1;
      }
    }
    FUN_001f2588();
    break;
  case 5:
    if ((uRam0018c328 == 0) && ((DAT_0013cae4 & 0x50) != 0)) {
      uRam0018c408 = (uint)(uRam0018c408 == 0);
    }
    if ((uRam0018c328 == 1) && ((DAT_0013cae4 & 0x50) != 0)) {
      uRam0018c40c = (uint)(uRam0018c40c == 0);
    }
    if ((uRam0018c328 == 2) && (iVar14 = 0, (DAT_0013cae4 & 0xf0) != 0)) {
      bVar2 = false;
      if (0 < DAT_0015f654) {
        pfVar12 = DAT_001940e0;
        do {
          if (((((*pfVar12 < DAT_00187080 + 0.5) && (DAT_00187080 - 0.5 < *pfVar12)) &&
               (pfVar12[1] < DAT_00187084 + 0.5)) &&
              ((DAT_00187084 - 0.5 < pfVar12[1] && (pfVar12[2] < DAT_00187088 + 0.5)))) &&
             (DAT_00187088 - 0.5 < pfVar12[2])) {
            bVar2 = true;
            break;
          }
          iVar14 = iVar14 + 1;
          pfVar12 = pfVar12 + 1;
        } while (iVar14 < DAT_0015f654);
      }
      if (bVar2) {
        return;
      }
      FUN_00116248(&stack0x000006c0,&gp0xffff8420,0x1e77d8,DAT_0015ed84,0x1e77f8,0x15f030);
      uVar5 = FUN_00120478(DAT_00187080);
      uVar6 = FUN_00120478(DAT_00187084);
      uVar7 = FUN_00120478(DAT_00187088);
      FUN_00116248(&stack0x00000700,0x1e7810,uVar5,uVar6,uVar7);
      lVar8 = FUN_0011bc80(&stack0x000006c0,0x303);
      if (-1 < lVar8) {
        FUN_0011c088(lVar8,0,2);
        uVar5 = FUN_001166cc(&stack0x00000700);
        FUN_0011c520(lVar8,&stack0x00000700,uVar5);
        FUN_0011bf08(lVar8);
        uRam0018c411 = 0x79;
      }
      pfVar12 = DAT_001940e0;
      FUN_00116248(&stack0x000006c0,&gp0xffff8420,0x1e77d8,DAT_0015ed84,0x15f038,0x15f030);
      lVar8 = FUN_0011bc80(&stack0x000006c0,0x303);
      if (-1 < lVar8) {
        FUN_0011c088(lVar8,0,2);
        FUN_0011c520(lVar8,0x187080,4);
        FUN_0011c520(lVar8,0x187084,4);
        FUN_0011c520(lVar8,0x187088,4);
        FUN_0011c088(lVar8,0,0);
        lVar9 = FUN_0011c088(lVar8,0,2);
        if (lVar9 < 0x100001) {
          FUN_0011c088(lVar8,0,0);
          FUN_0011c2c0(lVar8,pfVar12,lVar9);
          DAT_0015f654 = (int)lVar9 / 0xc;
        }
        else {
          uGpffff8a54 = 0;
        }
        FUN_0011bf08(lVar8);
      }
    }
    if ((uRam0018c328 == 3) && ((DAT_0013cae4 & 0xf0) != 0)) {
      FUN_00116248(&stack0x000006b0,0x15f020,0x1e77d8,DAT_0015ed84,0x1e77f8,0x15f030);
      uVar5 = FUN_00120478(DAT_00187080);
      uVar6 = FUN_00120478(DAT_00187084);
      uVar7 = FUN_00120478(DAT_00187088);
      FUN_00116248(&stack0x000006f0,0x1e7830,uVar5,uVar6,uVar7);
      lVar8 = FUN_0011bc80(&stack0x000006b0,0x303);
      if (-1 < lVar8) {
        FUN_0011c088(lVar8,0,2);
        uVar5 = FUN_001166cc(&stack0x000006f0);
        FUN_0011c520(lVar8,&stack0x000006f0,uVar5);
        FUN_0011bf08(lVar8);
        uRam0018c410 = 0x79;
      }
    }
  }
  return;
}

