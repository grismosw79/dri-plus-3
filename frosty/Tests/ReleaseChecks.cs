using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Reflection.Emit;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;
using System.ComponentModel;

// Runs against the actual shipped plugin through reflection. This is an offline
// host fixture, not a claim of a Windows/Frosty or in-game test.
internal static class ReleaseChecks {
    static Assembly plugin;
    static int checks;
    static Type Type(string name) { return plugin.GetType("DriPlus." + name, true); }
    static object Call(string type, string method, params object[] args) {
        return Type(type).GetMethod(method, BindingFlags.NonPublic | BindingFlags.Static).Invoke(null, args);
    }
    static string Field(string type, string field) { return (string)Type(type).GetField(field, BindingFlags.NonPublic | BindingFlags.Static).GetValue(null); }
    static object Instance(string name) { return Activator.CreateInstance(Type(name), true); }
    static object Invoke(object instance, string method, params object[] args) {
        return instance.GetType().GetMethod(method, BindingFlags.NonPublic | BindingFlags.Instance).Invoke(instance, args);
    }
    static void Pass(bool ok, string message) {
        if(!ok) throw new Exception("FAIL " + message);
        checks++; Console.WriteLine("PASS " + message);
    }
    static string Hash(string path) {
        using(var f = File.OpenRead(path)) using(var h = SHA256.Create())
            return BitConverter.ToString(h.ComputeHash(f)).Replace("-", "").ToLowerInvariant();
    }
    static bool Receipt(string dir, int pid, DateTime since, bool ready) { return (bool)Call("Payload", "HasReceipt", dir, pid, since, ready); }
    static bool Ready(object o) { return (bool)Invoke(o, "WaitReady", 0); }
    static void Read(object o, string line, bool stdout = true) { Invoke(o, "Read", line, stdout); }
    // Decode the compiled method, so this checks the actual installed binary.
    static IEnumerable<MethodBase> Calls(MethodInfo method) {
        var ops = typeof(OpCodes).GetFields(BindingFlags.Public | BindingFlags.Static)
            .Where(f=>f.FieldType == typeof(OpCode)).Select(f=>(OpCode)f.GetValue(null))
            .ToDictionary(o=>unchecked((ushort)o.Value));
        var il = method.GetMethodBody().GetILAsByteArray();
        int p = 0;
        while(p < il.Length) {
            ushort code = il[p++];
            if(code == 0xfe) code = (ushort)(0xfe00 | il[p++]);
            var op = ops[code];
            if(op.OperandType == OperandType.InlineMethod)
                yield return method.Module.ResolveMethod(BitConverter.ToInt32(il,p));
            switch(op.OperandType) {
                case OperandType.InlineNone: break;
                case OperandType.ShortInlineBrTarget: case OperandType.ShortInlineI: case OperandType.ShortInlineVar: p++; break;
                case OperandType.InlineVar: p+=2; break;
                case OperandType.InlineI8: case OperandType.InlineR: p+=8; break;
                case OperandType.InlineSwitch: int n=BitConverter.ToInt32(il,p); p+=4+n*4; break;
                default: p+=4; break;
            }
        }
    }
    static void Main(string[] args) {
        plugin = Assembly.LoadFrom(Path.GetFullPath(args[0]));
        string dir = Path.GetFullPath(args[1]); Directory.CreateDirectory(dir);
        string native = Path.GetFullPath(args[2]);
        var attrs = plugin.GetCustomAttributes<Frosty.Core.Attributes.RegisterExecutionAction>().ToArray();
        var action = (Frosty.Core.ExecutionAction)Activator.CreateInstance(attrs.Single().ActionType);
        Pass(action.PreLaunchAction != null && action.PostLaunchAction != null, "shipped plugin registers one action with both Frosty callbacks");
        Pass(plugin.GetName().Version == new Version(3,0,22,3), "automatic-launch plugin has distinct T3 Steam Test assembly version");
        Pass(Type("Dialog").GetMethod("ConfirmMenu", BindingFlags.NonPublic | BindingFlags.Static) == null, "activation confirmation is absent from the shipped binary");
        var activationCalls = Calls(Type("GameHost").GetMethod("Activate", BindingFlags.Public | BindingFlags.Instance)).ToArray();
        Pass(activationCalls.Any(m=>m.DeclaringType == Type("FrostyLaunch") && m.Name == "StartSession") &&
             !activationCalls.Any(m=>m.DeclaringType == Type("Dialog")), "compiled automatic activation reaches StartSession without a dialog");
        var refs = plugin.GetReferencedAssemblies().Where(x=>x.Name.StartsWith("Frosty")).ToArray();
        Pass(refs.Length == 2 && refs.All(x=>x.Version == new Version(1,0,0,0) && x.GetPublicKeyToken().Length == 0), "Frosty reference assembly identities match RC3");
        var cancelled = new CancellationTokenSource(); cancelled.Cancel();
        action.PreLaunchAction(null, Frosty.Core.PluginManagerType.ModManager, cancelled.Token);
        action.PreLaunchAction(null, Frosty.Core.PluginManagerType.Editor, CancellationToken.None);
        FrostySdk.ProfilesLibrary.Enabled = false;
        action.PostLaunchAction(null, Frosty.Core.PluginManagerType.ModManager, CancellationToken.None);
        FrostySdk.ProfilesLibrary.Enabled = true;
        Pass(true, "cancelled, editor and non-CFB27 callbacks return without activation");
        Call("Payload", "Prepare", dir);
        foreach(string ext in new[]{"dll", "exe"}) {
            string name = "DRIPlus3-RM18R22." + ext;
            Pass(Hash(Path.Combine(dir, name)) == Hash(Path.Combine(native, name)), "embedded " + ext + " exactly matches release payload");
        }
        var stamp = File.GetLastWriteTimeUtc(Path.Combine(dir, Field("Payload", "Runtime")));
        Call("Payload", "Prepare", dir);
        Pass(File.GetLastWriteTimeUtc(Path.Combine(dir, Field("Payload", "Runtime"))) == stamp, "repeat extraction reuses identical payload");
        foreach(string name in new[]{"HC-Hire-Cache", "HS-Plan-Cache", "EPS1-Cache", "Portal-Cache"}) File.WriteAllText(Path.Combine(dir,name,"keep.fixture"), "preserve");
        File.WriteAllText(Path.Combine(dir, Field("Payload", "Launcher")), "truncated");
        Call("Payload", "Prepare", dir);
        Pass(Hash(Path.Combine(dir, Field("Payload", "Launcher"))) == Field("Payload", "LauncherHash"), "incomplete monitor is repaired");
        Pass(Directory.GetFiles(dir,"keep.fixture",SearchOption.AllDirectories).Length == 4, "all four dynasty cache folders survive extraction");
        string concurrent = Path.Combine(dir,"concurrent"); Directory.CreateDirectory(concurrent);
        Parallel.Invoke(()=>Call("Payload","Prepare",concurrent), ()=>Call("Payload","Prepare",concurrent));
        Pass(Hash(Path.Combine(concurrent,Field("Payload","Runtime"))) == Field("Payload","RuntimeHash") && Directory.GetFiles(concurrent,"*.tmp").Length == 0, "concurrent first extraction leaves exact payload and no staging files");

        var output = Instance("MonitorOutput");
        Read(output, "DRI_MONITOR_READY: all session logs are open; runtime monitoring is active.");
        Pass(!Ready(output), "monitor marker alone cannot announce READY");
        output = Instance("MonitorOutput");
        Read(output,"RM18R22 READY: active",false); Read(output,"DRI_MONITOR_READY: active",false);
        Pass(!Ready(output), "stderr cannot impersonate READY");
        output = Instance("MonitorOutput"); Read(output,"RM18R21 READY: active"); Read(output,"DRI_MONITOR_READY: active");
        Pass(!Ready(output), "older runtime cannot authorize R22 READY");
        output = Instance("MonitorOutput"); Read(output,"RM18R22 READY: active");
        Pass(!Ready(output), "native activation alone cannot announce READY");
        Read(output,"DRI_MONITOR_READY: all session logs are open; runtime monitoring is active.");
        Pass(Ready(output), "native R22 READY followed by release monitor marker is recognized");
        output = Instance("MonitorOutput"); Read(output,"RM18R22 refused activation (code 5015). Do not advance.");
        Pass((string)Invoke(output,"FailureCode",10) == "ACTIVATION-5015", "activation failure retains specific code instead of generic monitor exit");
        output = Instance("MonitorOutput"); Read(output, new string('x',10000));
        Pass(((string)Invoke(output,"Recent")).Length <= 4200 && (string)Invoke(output,"FailureCode",3) == "MONITOR-3", "diagnostic tail is bounded and missing-process exit has stable code");
        string message = (string)Call("FailureReport","Message","ACTIVATION-5015",Path.Combine(dir,"a-very-long.log"),1234);
        Pass(message.Length < 450 && message.Contains("ACTIVATION-5015") && message.Contains("COLLECT-DRI-DIAGNOSTICS.cmd") && message.Contains("last good dynasty save") && !message.Contains(dir), "short error requests collector ZIP, last good save and trigger without dumping paths or output");
        Pass((string)Call("FailureReport","ExceptionCode","LAUNCH",new Win32Exception(225)) == "LAUNCH/WIN32-225", "Windows file-block code is retained without claiming a false positive");

        int pid = 7781; DateTime since = DateTime.UtcNow.AddMinutes(-1);
        string receipt = Path.Combine(dir,"RM18R22_Receipt_20261115_230000_" + pid + ".txt");
        string good = "build=RM18R22_RELEASE\nmode=ACTIVE\ninitialize_result=0\nprocess_id="+pid+"\ncombined_dll_sha256="+Field("Payload","RuntimeHash")+"\n";
        Pass(!Receipt(dir,pid,since,true), "missing activation receipt cannot authorize READY");
        File.WriteAllText(receipt,good);
        Pass(Receipt(dir,pid,since,true), "release receipt with exact PID, runtime hash and successful activation grants READY");
        Pass(!Receipt(dir,pid+1,since,true), "receipt from another PID is rejected");
        Pass(!Receipt(dir,pid,DateTime.UtcNow.AddMinutes(1),true), "receipt from an earlier process lifetime is rejected");
        foreach(string bad in new[]{good.Replace("RELEASE","EXTENDED_TEST"),good.Replace("ACTIVE","DISABLE"),good.Replace("process_id=7781","process_id=1"),good.Replace(Field("Payload","RuntimeHash"),new string('0',64))}) {
            File.WriteAllText(receipt,bad); Pass(!Receipt(dir,pid,since,true), "receipt with mismatched build/mode/PID/hash is rejected");
        }
        File.WriteAllText(receipt,good.Replace("initialize_result=0","initialize_result=5015"));
        Pass(!Receipt(dir,pid,since,true) && Receipt(dir,pid,since,false), "failed activation blocks duplicate attempts without granting READY");
        string session = "release-check-" + Guid.NewGuid().ToString("N");
        var claim = (IDisposable)Call("Claim","TryCreate",session,since,9501);
        Pass(claim != null && Call("Claim","TryCreate",session,since,9501) == null, "two managers cannot claim the same game session");
        claim.Dispose();
        claim = (IDisposable)Call("Claim","TryCreate",session,since,9501);
        Pass(claim != null, "session claim can be reacquired after release"); claim.Dispose();
        Console.WriteLine(checks + " packaged release checks passed; no live game was started.");
    }
}
