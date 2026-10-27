// Post-analysis: write every function's decompiled C to the file given as the first script argument.
// @category lxr-0
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import java.io.PrintWriter;

public class DumpDecomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        int n = 0;
        try (PrintWriter out = new PrintWriter(getScriptArgs()[0])) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                DecompileResults r = d.decompileFunction(f, 60, monitor);
                out.println("// ==== " + f.getName() + " @ " + f.getEntryPoint());
                out.println(r.decompileCompleted() ? r.getDecompiledFunction().getC() : "// decompile failed");
                n++;
            }
        }
        println("decompiled " + n + " functions");
    }
}
