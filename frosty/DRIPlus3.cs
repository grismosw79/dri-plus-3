using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using Frosty.Core;
using Frosty.Core.Attributes;
using FrostySdk;
using FrostySdk.Interfaces;

[assembly: AssemblyTitle("DRI+ 3.0")]
[assembly: AssemblyProduct("DRI+")]
[assembly: AssemblyDescription("Frosty Manager integration for the DRI+ 3.0 offline dynasty runtime")]
[assembly: AssemblyVersion("3.0.22.3")]
[assembly: AssemblyFileVersion("3.0.22.3")]
[assembly: AssemblyInformationalVersion("3.0.22.3-r22-tu1117-t3-steam-test")]
[assembly: PluginDisplayName("DRI+ 3.0")]
[assembly: PluginAuthor("DRI+")]
[assembly: PluginVersion("3.0 R22 TU1117 T3 Steam Test")]
[assembly: RegisterExecutionAction(typeof(DriPlus.FrostyLaunch))]

namespace DriPlus
{
    public sealed class FrostyLaunch : ExecutionAction
    {
        private static readonly LaunchWatcher Watcher = new LaunchWatcher(new GameHost());

        public override Action<ILogger, PluginManagerType, CancellationToken> PreLaunchAction
        {
            get { return BeforeLaunch; }
        }
        public override Action<ILogger, PluginManagerType, CancellationToken> PostLaunchAction
        {
            get { return AfterLaunch; }
        }
        private static void BeforeLaunch(ILogger logger, PluginManagerType manager, CancellationToken token)
        {
            Arm(manager, token, true);
        }
        private static void AfterLaunch(ILogger logger, PluginManagerType manager, CancellationToken token)
        {
            Arm(manager, token, false);
        }
        private static void Arm(PluginManagerType manager, CancellationToken token, bool before)
        {
            if (manager != PluginManagerType.ModManager || token.IsCancellationRequested ||
                !ProfilesLibrary.IsLoaded(ProfileVersion.CollegeFootball27)) return;
            try { Watcher.OnCallback(token, before); }
            catch (Exception ex) { FailureReport.EarlyFailure(ex); }
        }
        internal static ActivationResult StartSession(Process game, DateTime gameStarted, string log,
            Func<bool> current, Action monitorStarted)
        {
            if (game.HasExited) return ActivationResult.GameExited;
            int gameId = game.Id;
            string directory = Payload.RuntimeDirectory();
            Payload.Prepare(directory);
            var start = new ProcessStartInfo(Path.Combine(directory, Payload.Launcher), "active")
            {
                WorkingDirectory = directory, UseShellExecute = false, CreateNoWindow = true,
                RedirectStandardOutput = true, RedirectStandardError = true
            };
            // Monitor output is redirected so the plugin can wait until the CSV
            // and both component logs have actually opened before it reports READY.
            using (var child = new Process { StartInfo = start })
            {
                var output = new MonitorOutput();
                child.OutputDataReceived += (s, e) => output.Read(e.Data, true);
                child.ErrorDataReceived += (s, e) => output.Read(e.Data, false);
                if (!current()) return ActivationResult.Cancelled;
                if (game.HasExited) return ActivationResult.GameExited;
                if (!child.Start()) throw new IOException("The gameplay monitor could not start.");
                monitorStarted();
                child.BeginOutputReadLine();
                child.BeginErrorReadLine();
                Payload.Note(log, "RM18R22 monitor started.");
                bool shown = false;
                while (!child.HasExited)
                {
                    if (output.WaitReady(350) && !game.HasExited && !child.HasExited)
                    {
                        if (!Payload.HasReceipt(directory, gameId, gameStarted, true))
                            throw new IOException("The monitor started but its matching activation receipt could not be verified. Close the game before loading a save. Diagnostics are in " + directory);
                        shown = true;
                        Payload.Note(log, "READY. Monitor logs opened and current activation receipt verified.");
                        Dialog.Ready();
                    }
                    if (shown) { child.WaitForExit(); break; }
                }
                child.WaitForExit();
                var exit = child.ExitCode;
                string details = output.Recent();
                Payload.Note(log, "RM18R22 monitor exit " + exit + Environment.NewLine + details);
                // 16 and 17 come from the original monitor after it has displayed its own error.
                if ((exit != 0 && exit != 16 && exit != 17) || (!shown && exit == 0 && !game.HasExited))
                    Dialog.Error(FailureReport.Message(output.FailureCode(exit), log, gameId));
            }
            return ActivationResult.Handled;
        }
    }

    internal sealed class MonitorOutput
    {
        private readonly StringBuilder tail = new StringBuilder();
        private bool activated, monitoring;
        private string activationFailure;
        internal void Read(string line, bool stdout)
        {
            if (line == null) return;
            lock (tail)
            {
                tail.AppendLine(line);
                const string refusal = "RM18R22 refused activation (code ";
                if (stdout && activationFailure == null && line.StartsWith(refusal, StringComparison.Ordinal))
                {
                    int end = line.IndexOf(')', refusal.Length);
                    uint code;
                    if (end > refusal.Length && uint.TryParse(line.Substring(refusal.Length, end - refusal.Length),
                        NumberStyles.None, CultureInfo.InvariantCulture, out code))
                        activationFailure = "ACTIVATION-" + code.ToString(CultureInfo.InvariantCulture);
                }
                if (tail.Length > 4200) tail.Remove(0, tail.Length - 3200);
                if (stdout && line.StartsWith("RM18R22 READY:", StringComparison.Ordinal)) activated = true;
                if (stdout && activated && line.StartsWith("DRI_MONITOR_READY:", StringComparison.Ordinal))
                {
                    monitoring = true;
                    Monitor.PulseAll(tail);
                }
            }
        }
        internal bool WaitReady(int milliseconds)
        {
            lock (tail) { if (!monitoring) Monitor.Wait(tail, milliseconds); return monitoring; }
        }
        internal string Recent() { lock (tail) { return tail.ToString(); } }
        internal string FailureCode(int exit)
        {
            lock (tail) { return activationFailure ?? ("MONITOR-" + exit.ToString(CultureInfo.InvariantCulture)); }
        }
    }

    internal static class FailureReport
    {
        internal static string ExceptionCode(string stage, Exception error)
        {
            var native = error as System.ComponentModel.Win32Exception;
            return stage + (native == null
                ? "/0x" + error.HResult.ToString("X8", CultureInfo.InvariantCulture)
                : "/WIN32-" + native.NativeErrorCode.ToString(CultureInfo.InvariantCulture));
        }
        internal static string Message(string code, string log, int? pid = null)
        {
            return "Error code: " + code + "\n\n" +
                "Stop advancing and close the game.\n" +
                "Send the diagnostics ZIP and your last good dynasty save.\n\n" +
                "Run COLLECT-DRI-DIAGNOSTICS.cmd in the release folder.\n" +
                "Include what you did just before the error.\n" +
                "If no logs were created, include a screenshot of this message.";
        }
        internal static void EarlyFailure(Exception error)
        {
            string log = null;
            try { log = Payload.OpenLaunchLog("Launch watcher failed: " + error); }
            catch (Exception) { /* The screenshot remains available if logging is blocked. */ }
            Dialog.Error(Message(ExceptionCode("LAUNCH-WATCHER", error), log));
        }
    }

    internal sealed class Claim : IDisposable
    {
        private readonly Mutex owner;
        private Claim(Mutex mutex) { owner = mutex; }
        internal static Claim TryCreate(string session, DateTime gameStarted, int gameId)
        {
            bool created;
            var mutex = new Mutex(false, @"Local\DRIPlus3.Frosty." + session, out created);
            try
            {
                // A restarted MMC can see a still-running game. A receipt written
                // in that game's lifetime proves a prior activation attempt.
                if (!created || Payload.HasReceipt(Payload.RuntimeDirectory(), gameId, gameStarted, false))
                {
                    mutex.Dispose();
                    return null;
                }
                return new Claim(mutex);
            }
            catch { mutex.Dispose(); throw; }
        }
        public void Dispose() { owner.Dispose(); }
    }

    internal static class Payload
    {
        internal const string Launcher = "DRIPlus3-RM18R22.exe";
        internal const string Runtime = "DRIPlus3-RM18R22.dll";
        internal const string RuntimeHash = "d3899e8cd372ca73ce53adcc6e6a8919c06d3082e91174f51284c8775ffebfce";
        internal const string LauncherHash = "a4cff9e12840b44d0833aca7645e8ab0fd0a9d604a4f2087ff5ab1fc45664144";
        private static readonly string[] CacheNames = { "HC-Hire-Cache", "HS-Plan-Cache", "EPS1-Cache", "Portal-Cache" };

        internal static string RuntimeDirectory()
        {
            string local = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            if (string.IsNullOrEmpty(local)) throw new IOException("Windows Local AppData is unavailable.");
            return Path.Combine(local, "DRIPlus", "CFB27", "Runtime");
        }
        private static readonly object LogLock = new object();
        internal static string OpenLaunchLog(string firstEvent)
        {
            string dir = RuntimeDirectory();
            Directory.CreateDirectory(dir);
            string name = "Frosty-R22_" + DateTime.UtcNow.ToString("yyyyMMdd_HHmmss_fff", CultureInfo.InvariantCulture)
                + "_" + Guid.NewGuid().ToString("N") + ".txt";
            string path = Path.Combine(dir, name);
            File.WriteAllText(path, DateTime.UtcNow.ToString("o") + " " + firstEvent + Environment.NewLine);
            return path;
        }
        internal static void Note(string log, string message)
        {
            lock (LogLock)
            {
                try { File.AppendAllText(log, DateTime.UtcNow.ToString("o") + " " + message + Environment.NewLine); }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
            }
        }
        internal static void Prepare(string directory)
        {
            Directory.CreateDirectory(directory);
            foreach (string name in CacheNames) Directory.CreateDirectory(Path.Combine(directory, name));
            Install(directory, Runtime, RuntimeHash);
            Install(directory, Launcher, LauncherHash);
        }
        private static string HashOf(string path)
        {
            using (var input = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (var sha = SHA256.Create())
                return BitConverter.ToString(sha.ComputeHash(input)).Replace("-", "").ToLowerInvariant();
        }
        private static void Install(string directory, string name, string hash)
        {
            string dest = Path.Combine(directory, name);
            if (File.Exists(dest) && HashOf(dest) == hash) return;
            string staging = dest + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                using (var payload = typeof(Payload).Assembly.GetManifestResourceStream("DRIPlus." + name))
                using (var copy = new FileStream(staging, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    if (payload == null) throw new IOException("Missing embedded component " + name);
                    payload.CopyTo(copy);
                    copy.Flush(true);
                }
                if (HashOf(staging) != hash) throw new IOException("The embedded " + name + " failed its checksum.");
                try
                {
                    if (File.Exists(dest)) File.Replace(staging, dest, null);
                    else File.Move(staging, dest);
                }
                catch (IOException) { if (!File.Exists(dest) || HashOf(dest) != hash) throw; }
            }
            finally { if (File.Exists(staging)) File.Delete(staging); }
        }
        internal static bool HasReceipt(string directory, int pid, DateTime sinceUtc, bool requireReady)
        {
            if (!Directory.Exists(directory)) return false;
            string expectedPid = "process_id=" + pid.ToString(CultureInfo.InvariantCulture);
            foreach (string path in Directory.GetFiles(directory, "RM18R22_Receipt_*_" + pid + ".txt"))
            {
                if (File.GetLastWriteTimeUtc(path) < sinceUtc) continue;
                var lines = File.ReadAllLines(path);
                if (Array.IndexOf(lines, "build=RM18R22_RELEASE") >= 0 &&
                    Array.IndexOf(lines, "mode=ACTIVE") >= 0 &&
                    (!requireReady || Array.IndexOf(lines, "initialize_result=0") >= 0) &&
                    Array.IndexOf(lines, expectedPid) >= 0 &&
                    Array.IndexOf(lines, "combined_dll_sha256=" + RuntimeHash) >= 0) return true;
            }
            return false;
        }
    }

    internal static class Dialog
    {
        [DllImport("user32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern int MessageBoxW(IntPtr owner, string message, string title, uint flags);
        private const uint Foreground = 0x00010000 | 0x00040000;
        internal static void Ready()
        {
            MessageBoxW(IntPtr.Zero, "DRI+ activated automatically and the gameplay monitor is running. You can load your prepared offline dynasty.",
                "DRI+ 3.0 R22 | READY", Foreground | 0x40);
        }
        internal static void Error(string message) { MessageBoxW(IntPtr.Zero, message, "DRI+ 3.0 | Error", Foreground | 0x10); }
    }
}
