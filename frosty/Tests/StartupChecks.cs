using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using DriPlus;

internal sealed class ScriptHost : ILaunchHost
{
    internal static readonly DateTime Epoch = new DateTime(2026, 11, 13, 20, 0, 0, DateTimeKind.Utc);
    internal static readonly GameWindow Real = new GameWindow(3042, Epoch.AddMilliseconds(300));
    internal static readonly GameWindow Splash = new GameWindow(1180, Epoch);
    internal int Millis, Logs, Queued, Reads, Starts;
    internal List<int> Activations = new List<int>();
    internal List<string> Events = new List<string>();
    internal List<string> Errors = new List<string>();
    internal List<DateTime> Filters = new List<DateTime>();
    internal Action Tick, RetryRace;
    internal Func<DateTime, WindowScan> OnScan;
    internal Func<GameWindow, string, Func<bool>, Action<string>, ActivationResult> OnActivate;
    private readonly Queue<Action> work = new Queue<Action>();
    internal readonly LaunchWatcher Watcher;
    internal ScriptHost() { Watcher = new LaunchWatcher(this); }
    public DateTime UtcNow { get { return Epoch.AddMilliseconds(Millis); } }
    public TimeSpan Elapsed { get { return TimeSpan.FromMilliseconds(Millis); } }
    public string OpenLog(string message) { Logs++; Events.Add(message); return "launch-" + Logs; }
    public void Note(string path, string message)
    {
        Events.Add(path + " " + message);
        if (RetryRace != null && message.Contains("can retry")) RetryRace();
    }
    public void Queue(Action task) { Queued++; work.Enqueue(task); }
    public void Pause() { Millis += 500; if (Tick != null) Tick(); if (Millis > 4000000) throw new Exception("test stalled"); }
    public void Error(string message) { Errors.Add(message); }
    public WindowScan Scan(DateTime earliest)
    {
        Filters.Add(earliest); Reads++;
        return OnScan == null ? State(WindowState.Ready, Real) : OnScan(earliest);
    }
    public ActivationResult Activate(GameWindow game, string log, Func<bool> current, Action<string> remember)
    {
        Activations.Add(game.Pid);
        if (OnActivate != null) return OnActivate(game, log, current, remember);
        if (!current()) return ActivationResult.Cancelled;
        remember(game.Key); Starts++; return ActivationResult.Handled;
    }
    internal void Run() { while (work.Count > 0) work.Dequeue()(); }
    internal static WindowScan State(WindowState state, GameWindow game = null) { return new WindowScan(state, game); }
    internal bool Has(string text) { return Events.Any(x => x.Contains(text)); }
}

internal class StartupChecks
{
    static int count;
    static void Pass(bool ok, string scenario) { if (!ok) throw new Exception("FAIL " + scenario); count++; Console.WriteLine("PASS " + scenario); }
    static WindowScan Waiting() { return ScriptHost.State(WindowState.Missing); }
    static WindowScan Ready(GameWindow game) { return ScriptHost.State(WindowState.Ready, game); }
    static CancellationToken NewToken() { return new CancellationTokenSource().Token; }
    static void Pre(ScriptHost host, CancellationToken token) { host.Watcher.OnCallback(token, true); }
    static void Post(ScriptHost host, CancellationToken token) { host.Watcher.OnCallback(token, false); }

    static void Main()
    {
        var t = NewToken();
        var h = new ScriptHost();
        Pre(h,t); Pre(h,t); Post(h,t); Post(h,t);
        Pass(h.Logs == 1 && h.Queued == 1 && h.Reads == 0 && h.Has("pre-launch received") && h.Has("post-launch received"), "pre/post log before first scan, duplicate callbacks schedule one watcher");
        h.Run(); Post(h,t); h.Run();
        Pass(h.Activations.SequenceEqual(new[]{3042}) && h.Starts == 1 && h.Errors.Count == 0, "one automatic activation and one start even with a late post");

        h = new ScriptHost();
        Pre(h,t);
        h.OnScan = _ => h.Millis < 1000 ? ScriptHost.State(WindowState.Starting) : h.Millis < 3000 ? Waiting() : Ready(ScriptHost.Real);
        h.Tick = () => { if (h.Millis == 2000) Post(h,t); };
        h.Run();
        Pass(h.Has("Starting") && h.Has("Missing") && h.Has("post-launch received") && h.Activations.SequenceEqual(new[]{3042}) && h.Starts == 1, "temporary headless process exits, gap, same-token post and final window");

        h = new ScriptHost(); Pre(h,t); Post(h,t);
        h.OnScan = _ => h.Millis < 1000 ? Ready(ScriptHost.Splash) : h.Millis < 1500 ? Waiting() : Ready(ScriptHost.Real);
        h.Run();
        Pass(h.Activations.SequenceEqual(new[]{3042}), "short-lived splash window never activates");

        h = new ScriptHost(); Pre(h,t); Post(h,t);
        h.OnScan = _ => h.Millis == 0 ? ScriptHost.State(WindowState.Multiple) : h.Millis == 500 ? ScriptHost.State(WindowState.Inaccessible) : Ready(ScriptHost.Real);
        h.Run();
        Pass(h.Has("Multiple") && h.Has("Inaccessible") && h.Starts == 1 && h.Errors.Count == 0, "overlap or temporary lack of access clears before selecting a game");

        h = new ScriptHost(); Pre(h,t); Post(h,t);
        h.OnScan = _ => Ready(h.Activations.Count == 0 ? ScriptHost.Splash : ScriptHost.Real);
        h.OnActivate = (g,log,current,remember) => {
            if (g.Pid == ScriptHost.Splash.Pid) return ActivationResult.GameExited;
            remember(g.Key); h.Starts++; return ActivationResult.Handled;
        };
        h.Run();
        Pass(h.Activations.SequenceEqual(new[]{1180,3042}) && h.Starts == 1 && h.Has("GameExited"), "process exits before activation, replacement requires its own stable window");

        h = new ScriptHost();
        Pre(h,t);
        h.OnScan = _ => h.Millis < 20*60000 + 7000 ? Waiting() : Ready(ScriptHost.Real);
        h.Tick = () => { if(h.Millis == 20*60000) Post(h,t); };
        h.Run();
        Pass(h.Millis == 20*60000+8000 && h.Starts == 1 && h.Errors.Count == 0, "20 minutes applying mods before post does not consume the window timeout");
        Pass(h.Events.Count < 12, "slow launches log state transitions rather than every poll");

        h = new ScriptHost(); Pre(h,t);
        h.OnScan = _ => Ready(ScriptHost.Real); h.Run(); Post(h,t); h.Run();
        Pass(h.Starts == 1 && h.Has("post-launch received") && h.Queued == 1, "host with late post can still activate from pre alone");

        h = new ScriptHost(); Post(h,t); h.Run();
        Pass(h.Logs == 1 && h.Reads == 0 && h.Starts == 0 && h.Errors.Count == 1 && h.Errors[0].Contains("LAUNCH-NO-PRE"), "post-only callback cannot attach automatically to an existing game");
        Post(h,t); h.Run();
        Pass(h.Errors.Count == 1 && h.Queued == 1, "duplicate post-only callbacks do not repeat the refusal");
        Pre(h,t); Post(h,t); h.Run();
        Pass(h.Starts == 1 && h.Logs == 2, "fresh pre-launch recovers from an unpaired post");

        h = new ScriptHost(); Pre(h,t); Post(h,t);
        h.Tick = () => { if (h.Millis < 1000 && h.Starts != 0) throw new Exception("activated before stable window"); };
        h.Run();
        Pass(h.Starts == 1 && h.Millis == 1000, "automatic activation waits for a full stable-window interval");

        h = new ScriptHost(); Pre(h,t);
        h.OnScan = _ => Waiting();
        h.Tick = () => { if(h.Millis == 15*60000) Post(h,t); };
        h.Run();
        Pass(h.Millis == 25*60000 && h.Errors.Count == 1 && h.Has("Timed out") && h.Activations.Count == 0, "ten-minute failure deadline begins at post and leaves a log");

        var cts = new CancellationTokenSource();
        h = new ScriptHost(); Pre(h,cts.Token); h.OnScan = _ => Waiting();
        h.Tick = () => { if(h.Millis == 1500) cts.Cancel(); }; h.Run(); Post(h,cts.Token);
        Pass(h.Queued == 1 && h.Has("cancelled by MMC") && h.Activations.Count == 0 && h.Errors.Count == 0, "host cancellation ends the attempt and suppresses post");

        h = new ScriptHost(); t=NewToken(); Pre(h,t); Post(h,t);
        h.OnActivate = (g,log,current,remember) => { remember(g.Key); return ActivationResult.Handled; };
        h.Run(); Post(h,t); h.Run(); Pre(h,NewToken()); h.Run();
        Pass(h.Activations.Count == 1 && h.Starts == 0 && h.Has("already handled"), "handled activation failure cannot retry in the same process lifetime");

        h = new ScriptHost(); t=NewToken(); Pre(h,t); Post(h,t);
        h.OnActivate = (g,log,current,remember) => ActivationResult.Owned;
        h.Run();
        Pass(h.Has("Owned") && h.Starts == 0, "existing receipt or another manager claim cannot activate twice");

        h = new ScriptHost(); t = CancellationToken.None;
        Pre(h,t); Post(h,t); h.Run(); var next = new GameWindow(3042,h.UtcNow.AddMilliseconds(1));
        h.OnScan = _ => Ready(next); Pre(h,t); Post(h,t); h.Run();
        Pass(h.Logs == 2 && h.Starts == 2 && h.Filters.Last() == ScriptHost.Epoch.AddMilliseconds(1000), "tokenless relaunch and recycled PID with different start time");

        h = new ScriptHost(); var first=NewToken(); var second=NewToken();
        Pre(h,first); Pre(h,second); Post(h,first); Post(h,second); h.Run();
        Pass(h.Logs == 2 && h.Has("replaced this watcher") && h.Starts == 1, "a newer launch supersedes a pending watcher");
        Post(h,first); h.Run();
        Pass(h.Logs == 2 && h.Queued == 2 && h.Starts == 1, "late post from a superseded launch cannot displace the current launch");

        h = new ScriptHost(); t=NewToken(); Pre(h,t); 
        h.OnScan = _ => { throw new IOException("fixture early failure"); }; h.Run();
        h.OnScan = _ => Ready(ScriptHost.Real); Post(h,t); h.Run();
        Pass(h.Logs == 1 && h.Queued == 2 && h.Starts == 1 && h.Errors.Count == 0, "same-token post rearms a pre worker that actually failed");

        h = new ScriptHost(); t=NewToken(); Pre(h,t);
        h.OnScan = _ => { if (h.Reads == 1) throw new IOException("fixture boundary"); return Ready(ScriptHost.Real); };
        h.RetryRace = () => Post(h,t);
        h.Run();
        Pass(h.Queued == 1 && h.Starts == 1 && h.Has("can retry"), "same-token post arriving during worker cleanup cannot lose the retry");

        h = new ScriptHost(); t=NewToken(); Pre(h,t); Post(h,t); h.OnScan = _ => { throw new IOException("fixture post failure"); };
        h.Run(); Post(h,t); h.Run();
        Pass(h.Has("fixture post failure") && h.Errors.Count == 1 && h.Activations.Count == 0 && h.Queued == 1, "post failure logs the cause and does not loop");

        h = new ScriptHost(); t=NewToken(); Pre(h,t); Post(h,t);
        h.OnScan = _ => h.Millis < 1500 ? ScriptHost.State(WindowState.EarlierGame) : Ready(ScriptHost.Real);
        h.Run();
        Pass(h.Filters.All(x => x == ScriptHost.Epoch) && h.Has("EarlierGame") && h.Starts == 1, "matching post retains the prelaunch start-time boundary");

        cts = new CancellationTokenSource(); h = new ScriptHost(); Pre(h,cts.Token);
        h.OnActivate = (g,log,current,remember) => { cts.Cancel(); if(current()) throw new Exception("stale permission"); return ActivationResult.Cancelled; };
        h.Run();
        Pass(h.Starts==0 && h.Has("Cancelled"), "cancellation immediately before activation suppresses startup");

        t=NewToken(); h = new ScriptHost(); Pre(h,t);
        h.OnActivate = (g,log,current,remember) => { Post(h,t); remember(g.Key); h.Starts++; return ActivationResult.Handled; };
        h.Run(); Post(h,t);
        Pass(h.Queued==1 && h.Starts==1, "post callback during activation does not create a competing watcher");

        string early = Payload.OpenLaunchLog("fixture pre-launch log");
        Pass(File.Exists(early) && Path.GetFileName(early).StartsWith("Frosty-R22_") &&
             File.ReadAllText(early).Contains("fixture pre-launch log"), "physical launch log exists before any game process or monitor");
        string other = Payload.OpenLaunchLog("fixture post-only log");
        Pass(other != early && File.ReadAllText(early).Contains("fixture pre-launch log"), "two rapid launches cannot overwrite the same log");
        File.Delete(early); File.Delete(other);
        Console.WriteLine(count + " launch regression checks passed.");
    }
}

