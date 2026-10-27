"""Replace Ghidra DAT_08xxxxxx literal-pool refs with the 32-bit word stored there: DAT_x -> 0xVALUE/*@x*/."""
import re, struct, sys
img, src, dst = sys.argv[1:4]
p = open(img, 'rb').read()
BASE = 0x08008000
def sub(m):
    a = int(m.group(1), 16)
    o = a - BASE
    if 0 <= o <= len(p) - 4 and a % 4 == 0:
        return '0x%08x/*@%x*/' % (struct.unpack_from('<I', p, o)[0], a)
    return m.group(0)
open(dst, 'w').write(re.sub(r'DAT_(08[0-9a-f]{6})', sub, open(src).read()))
