// Map the 0x2DF8-byte ITCM image (copied from flash 0x0803cb5c to 0x00000000 at boot) and decompile its functions to the file given as the first script argument.
// @category lxr-0
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.PrintWriter;

public class MapItcm extends GhidraScript {
    @Override
    public void run() throws Exception {
        long src = 0x0803cb5cL, len = 0x2df8;
        byte[] buf = new byte[(int) len];
        currentProgram.getMemory().getBytes(toAddr(src), buf);
        var blk = currentProgram.getMemory().createInitializedBlock("itcm", toAddr(0), len, (byte) 0, monitor, false);
        currentProgram.getMemory().setBytes(toAddr(0), buf);
        blk.setWrite(false);
        blk.setExecute(true);
        for (long e : new long[] {0x20c4, 0x2c8c}) {
            Address a = toAddr(e);
            disassemble(a);
            createFunction(a, "itcm_" + Long.toHexString(e));
        }
        analyzeChanges(currentProgram);
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(getScriptArgs()[0])) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(blk.getStart(), true)) {
                if (!blk.contains(f.getEntryPoint())) break;
                var r = d.decompileFunction(f, 60, monitor);
                out.println("// ==== " + f.getName() + " @ " + f.getEntryPoint());
                out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed");
            }
        }
    }
}
