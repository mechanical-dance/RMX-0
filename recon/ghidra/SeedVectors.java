// Pre-analysis: mark every Cortex-M vector-table entry as a Thumb function entry.
// @category lxr-0
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import java.math.BigInteger;

public class SeedVectors extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address base = currentProgram.getMinAddress();
        // Flash is read-only, so the decompiler folds literal-pool loads into constants.
        for (var b : currentProgram.getMemory().getBlocks()) b.setWrite(false);
        setAnalysisOption(currentProgram, "ARM Aggressive Instruction Finder", "true");
        Register tmode = currentProgram.getRegister("TMode");
        int n = 0;
        for (int i = 1; i < 128; i++) {
            long v = getInt(base.add(i * 4)) & 0xffffffffL;
            if (v == 0 || (v & 1) == 0) continue;
            Address a = toAddr(v & ~1L);
            if (!currentProgram.getMemory().contains(a)) continue;
            try {
                currentProgram.getProgramContext().setValue(tmode, a, a, BigInteger.ONE);
            } catch (Exception e) {
                // already decoded as Thumb by the Cortex language default
            }
            addEntryPoint(a);
            disassemble(a);
            createFunction(a, i == 1 ? "Reset_Handler" : "vec_" + i);
            n++;
        }
        println("seeded " + n + " vectors");
    }
}
