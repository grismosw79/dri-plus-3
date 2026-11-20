using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Threading;

namespace DriPlus
{
    internal enum WindowState { Missing, Starting, Ready, Multiple, Inaccessible, EarlierGame }
    internal enum ActivationResult { GameExited, Owned, Cancelled, Handled }

    internal sealed class GameWindow
    {
        internal readonly int Pid;
        internal readonly DateTime Started;
        internal GameWindow(int pid, DateTime started) { Pid = pid; Started = started; }
        internal string Key
        {
            get { return Pid.ToString(CultureInfo.InvariantCulture) + "-" + Started.Ticks.ToString(CultureInfo.InvariantCulture); }
        }
    }

    internal sealed class WindowScan
    {
        internal readonly WindowState State;
        internal readonly GameWindow Game;
        internal WindowScan(WindowState state, GameWindow game) { State = state; Game = game; }
    }

    internal interface ILaunchHost
    {
        DateTime UtcNow { get; }
        TimeSpan Elapsed { get; }
        string OpenLog(string firstEvent);
        void Note(string path, string message);
        void Queue(Action work);
        void Pause();
        WindowScan Scan(DateTime earliest);
        ActivationResult Activate(GameWindow game, string log, Func<bool> current, Action<string> remember);
        void Error(string message);
    }

    internal sealed class LaunchWatcher
    {
        private sealed class Attempt
        {
            internal readonly CancellationToken Token;
            internal readonly DateTime Earliest;
            internal readonly string Log;
            internal readonly bool HasPre;
            internal bool SawPost, Running, Done;
            internal TimeSpan PostStart;
            internal Attempt(CancellationToken token, DateTime earliest, string log, bool hasPre)
            {
                Token = token; Earliest = earliest; Log = log; HasPre = hasPre;
            }
        }

        private readonly object gate = new object();
        private readonly ILaunchHost host;
        private readonly Queue<CancellationToken> retired = new Queue<CancellationToken>();
        private Attempt latest;
        private string lastHandled;
        private static readonly TimeSpan WindowTimeout = TimeSpan.FromMinutes(10);
        private static readonly TimeSpan StableWindow = TimeSpan.FromSeconds(1);

        internal LaunchWatcher(ILaunchHost host) { this.host = host; }

        private void Remember(string key) { lock (gate) { lastHandled = key; } }

        internal void OnCallback(CancellationToken token, bool before)
        {
            if (token.IsCancellationRequested) return;
            Attempt attempt;
            lock (gate)
            {
                if (!before && latest != null && latest.Token != token && retired.Contains(token)) return;
                // A new Pre opens a launch. Hosts without a usable token can still
                // pair Pre/Post and start a later launch with the same token value.
                if (latest == null || latest.Token != token || (before && (latest.SawPost || latest.Done)))
                {
                    DateTime earliest = before ? host.UtcNow : DateTime.MinValue;
                    string first = before ? "MMC pre-launch received. Waiting for CFB27."
                        : "MMC post-launch received without pre-launch. Automatic activation requires a fresh launch.";
                    var next = new Attempt(token, earliest, host.OpenLog(first), before);
                    if (latest != null && latest.Token.CanBeCanceled && latest.Token != token)
                    {
                        // A late Post from a superseded launch must not displace
                        // the current watcher. Retain only recent token identities.
                        if (retired.Count == 8) retired.Dequeue();
                        retired.Enqueue(latest.Token);
                    }
                    latest = next;
                }
                attempt = latest;
                if (!before && !attempt.SawPost)
                {
                    attempt.SawPost = true;
                    attempt.PostStart = host.Elapsed;
                    host.Note(attempt.Log, "MMC post-launch received. Starting the game-window timeout.");
                }
                if (attempt.Running || attempt.Done) return;
                attempt.Running = true;
            }
            try { host.Queue(() => Run(attempt)); }
            catch
            {
                lock (gate) { attempt.Running = false; }
                host.Note(attempt.Log, "Could not start the watcher thread.");
                throw;
            }
        }

        private bool Current(Attempt attempt)
        {
            lock (gate) { return latest == attempt && !attempt.Token.IsCancellationRequested; }
        }

        private void Run(Attempt attempt)
        {
            WindowState? previous = null;
            string candidate = null;
            TimeSpan firstSeen = TimeSpan.Zero;
            bool retryAfterPost = false;
            try
            {
                // Without Pre there is no trusted start-time boundary. Never
                // attach automatically to a game that was already running.
                if (!attempt.HasPre)
                {
                    host.Note(attempt.Log, "Automatic activation refused: no matching pre-launch callback. Close CFB27 and launch again through MMC.");
                    if (Current(attempt)) host.Error(FailureReport.Message("LAUNCH-NO-PRE", attempt.Log));
                    return;
                }
                while (Current(attempt))
                {
                    bool timedOut;
                    lock (gate) { timedOut = attempt.SawPost && host.Elapsed - attempt.PostStart >= WindowTimeout; }
                    if (timedOut)
                    {
                        host.Note(attempt.Log, "Timed out waiting for one stable CFB27 window after post-launch.");
                        if (Current(attempt)) host.Error(FailureReport.Message("LAUNCH-WINDOW-TIMEOUT", attempt.Log));
                        return;
                    }
                    WindowScan scan = host.Scan(attempt.Earliest);
                    string key = scan.Game == null ? null : scan.Game.Key;
                    if (scan.State != previous || key != candidate)
                    {
                        host.Note(attempt.Log, "CFB27 window: " + scan.State +
                            (scan.Game == null ? "" : " (PID " + scan.Game.Pid + ")"));
                        previous = scan.State;
                        candidate = key;
                        firstSeen = host.Elapsed;
                    }
                    if (scan.State == WindowState.Ready && host.Elapsed - firstSeen >= StableWindow && Current(attempt))
                    {
                        lock (gate)
                        {
                            if (lastHandled == key)
                            {
                                host.Note(attempt.Log, "This game session has already handled automatic activation.");
                                return;
                            }
                        }
                        ActivationResult result = host.Activate(scan.Game, attempt.Log, () => Current(attempt), Remember);
                        host.Note(attempt.Log, "Activation result: " + result);
                        if (result != ActivationResult.GameExited) return;
                        // A splash process can exit before activation. A
                        // replacement needs its own stable window and checks.
                        candidate = null;
                        previous = null;
                    }
                    host.Pause();
                }
                host.Note(attempt.Log, attempt.Token.IsCancellationRequested
                    ? "Launch cancelled by MMC." : "A newer MMC launch replaced this watcher.");
            }
            catch (Exception ex)
            {
                host.Note(attempt.Log, "Watcher failed: " + ex);
                lock (gate) { retryAfterPost = !attempt.SawPost; }
                if (retryAfterPost) host.Note(attempt.Log, "MMC post-launch can retry this watcher.");
                else if (Current(attempt)) host.Error(FailureReport.Message(FailureReport.ExceptionCode("LAUNCH", ex), attempt.Log));
            }
            finally
            {
                bool retry;
                lock (gate)
                {
                    attempt.Running = false;
                    attempt.Done = !retryAfterPost;
                    retry = retryAfterPost && attempt.SawPost && latest == attempt && !attempt.Token.IsCancellationRequested;
                    if (retry) attempt.Running = true;
                }
                // Post can arrive between recording the failure and releasing
                // the old worker. Transfer its retry to this same worker.
                if (retry) Run(attempt);
            }
        }
    }

    internal sealed class GameHost : ILaunchHost
    {
        private readonly Stopwatch clock = Stopwatch.StartNew();
        public DateTime UtcNow { get { return DateTime.UtcNow; } }
        public TimeSpan Elapsed { get { return clock.Elapsed; } }
        public string OpenLog(string firstEvent) { return Payload.OpenLaunchLog(firstEvent); }
        public void Note(string path, string message) { Payload.Note(path, message); }
        public void Queue(Action work)
        {
            new Thread(work.Invoke) { IsBackground = true, Name = "DRI+ launch" }.Start();
        }
        public void Pause() { Thread.Sleep(500); }
        public void Error(string message) { Dialog.Error(message); }

        public WindowScan Scan(DateTime earliest)
        {
            Process[] games = Process.GetProcessesByName("CollegeFB27");
            GameWindow candidate = null;
            int live = 0;
            bool inaccessible = false;
            bool earlier = false;
            try
            {
                foreach (Process game in games)
                {
                    try
                    {
                        if (game.HasExited) continue;
                        DateTime started = game.StartTime.ToUniversalTime();
                        bool eligible = started >= earliest && game.MainWindowHandle != IntPtr.Zero;
                        if (game.HasExited) continue;
                        live++;
                        if (started < earliest) earlier = true;
                        if (eligible) candidate = new GameWindow(game.Id, started);
                    }
                    catch (InvalidOperationException) { } // Ended during inspection.
                    catch (Win32Exception ex)
                    {
                        if (ex.NativeErrorCode != 87 && ex.NativeErrorCode != 1168) inaccessible = true;
                    }
                }
            }
            finally { foreach (Process game in games) game.Dispose(); }
            if (inaccessible) return new WindowScan(WindowState.Inaccessible, null);
            if (live > 1) return new WindowScan(WindowState.Multiple, null);
            if (earlier) return new WindowScan(WindowState.EarlierGame, null);
            if (candidate != null) return new WindowScan(WindowState.Ready, candidate);
            return new WindowScan(live == 0 ? WindowState.Missing : WindowState.Starting, null);
        }

        public ActivationResult Activate(GameWindow window, string log, Func<bool> current, Action<string> remember)
        {
            Process game;
            try { game = Process.GetProcessById(window.Pid); }
            catch (ArgumentException) { return ActivationResult.GameExited; }
            using (game)
            {
                try
                {
                    if (game.HasExited || game.StartTime.ToUniversalTime() != window.Started ||
                        game.MainWindowHandle == IntPtr.Zero) return ActivationResult.GameExited;
                }
                catch (InvalidOperationException) { return ActivationResult.GameExited; }
                catch (Win32Exception ex)
                {
                    if (game.HasExited || ex.NativeErrorCode == 87 || ex.NativeErrorCode == 1168)
                        return ActivationResult.GameExited;
                    throw;
                }
                using (var claim = Claim.TryCreate(window.Key, window.Started, window.Pid))
                {
                    if (claim == null) return ActivationResult.Owned;
                    if (!current()) return ActivationResult.Cancelled;
                    if (game.HasExited) return ActivationResult.GameExited;
                    WindowScan now = Scan(window.Started);
                    if (now.State != WindowState.Ready || now.Game == null || now.Game.Key != window.Key) return ActivationResult.GameExited;
                    if (!current()) return ActivationResult.Cancelled;
                    Note(log, "Activating DRI+ automatically for fresh game PID " + window.Pid + ". Stable window verified; runtime build checks still required.");
                    return FrostyLaunch.StartSession(game, window.Started, log, current, () => remember(window.Key));
                }
            }
        }
    }
}
