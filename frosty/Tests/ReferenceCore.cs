// Build/test API contract only. Never install this assembly in Frosty.
using System;
using System.Reflection;
using System.Threading;
using FrostySdk.Interfaces;
[assembly: AssemblyVersion("1.0.0.0")]
namespace Frosty.Core {
    public enum PluginManagerType { Editor = 0, ModManager = 1 }
    public abstract class ExecutionAction {
        public virtual Action<ILogger, PluginManagerType, CancellationToken> PreLaunchAction { get { return null; } }
        public virtual Action<ILogger, PluginManagerType, CancellationToken> PostLaunchAction { get { return null; } }
    }
}
namespace Frosty.Core.Attributes {
    [AttributeUsage(AttributeTargets.Assembly)]
    public sealed class PluginDisplayNameAttribute : Attribute { public PluginDisplayNameAttribute(string value) {} }
    [AttributeUsage(AttributeTargets.Assembly)]
    public sealed class PluginAuthorAttribute : Attribute { public PluginAuthorAttribute(string value) {} }
    [AttributeUsage(AttributeTargets.Assembly)]
    public sealed class PluginVersionAttribute : Attribute { public PluginVersionAttribute(string value) {} }
    [AttributeUsage(AttributeTargets.Assembly)]
    public sealed class RegisterExecutionAction : Attribute { public Type ActionType; public RegisterExecutionAction(Type value) { ActionType = value; } }
}
